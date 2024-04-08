#include "recvbufr.h"
#include "uri.h"

#pragma region Static Helper Functions

/**
 * Update the state of this transmission after data has been successfully appended to it.
 * Handles the details of different types of encoding
 * @param self In: The buffer to update the state of
 * @param added In: The amount of data added to the end of the buffer
 * @param error Out: NULL on success, user friendly error message otherwise
 * @return zero when the data transmission should keep going, other on transmission finished or error
 */
static inline char DataIncrement(RecvBuffer *self, PlatformFileOffset added) {
    if (self->options & RECV_BUFFER_DATA_LENGTH_KNOWN) {
        self->length.known.escape += added;
        if (self->length.known.escape >= self->length.known.total)
            return 1;
    } else if (self->options & RECV_BUFFER_DATA_LENGTH_TOKEN) {
        /* TODO: Check for token in the buffer */
    } else {
        self->length.unknown.escape += added;
        if (self->length.unknown.escape >= self->length.unknown.limit)
            return 1;
    }
    return 0;
}

/**
 * Decode and append a HTTP chunk encoded body to the socket buffer
 * @param self The socket buffer to append to
 * @param len The maximum length of data to append
 * @return NULL on success, user friendly error message otherwise
 * @remark The caller should check if the transmission has finished
 */
static inline const char *recvBufferAppendChunk(RecvBuffer *self, size_t len) {
    size_t i = 0;

    if (self->length.chunk.next == -1)
        goto recvBufferAppendChunk_parse;

    /* Iterate over the data until the next chunk */
    recvBufferAppendChunk_iterate:
    while (i < len && i < self->length.chunk.next) {
        SOCK_BUF_TYPE l;
        char buf[SB_DATA_SIZE];
        size_t s = self->length.chunk.next < len ? self->length.chunk.next : len;
        if (s > SB_DATA_SIZE)
            s = SB_DATA_SIZE;

        if ((l = recv(self->serverSocket, buf, s, MSG_PEEK)) == -1)
            return strerror(platformSocketGetLastError());

        platformMemoryStreamSeek(self->buffer, 0, SEEK_END);
        if (self->length.chunk.next >= l) {
            if ((fwrite(buf, 1, l, self->buffer)) != l)
                if (ferror(self->buffer))
                    return "Error writing to volatile buffer";

            self->length.chunk.next -= (PlatformFileOffset) l, len -= l;
            if (recv(self->serverSocket, buf, l, 0) == -1)
                return strerror(platformSocketGetLastError());
        } else {
            char *p = &buf[self->length.chunk.next];
            if (strcmp(p, HTTP_EOL) != 0)
                return "Malformed Chunk Encoding";

            if ((fwrite(buf, 1, l, self->buffer)) != l)
                if (ferror(self->buffer))
                    return "Error writing to volatile buffer";

            self->length.chunk.next -= self->length.chunk.next, len -= l;
            if (recv(self->serverSocket, buf, self->length.chunk.next, 0) == -1)
                return strerror(platformSocketGetLastError());
        }

        if (l < s)
            return NULL; /* Next chuck not in transmission buffer yet */
    }

    /* Parse the next chunk encoding */
    recvBufferAppendChunk_parse:
    {
        const char *e;
        char buf[20] = {0}, *finish, *hex;
        size_t l, j, k;

        if ((k = recv(self->serverSocket, buf, 19, MSG_PEEK)) == -1)
            return strerror(platformSocketGetLastError());

        else if(k < (self->length.chunk.next == -1 ? 3 : 5))
            return NULL; /* Chunk not available yet */

        if (self->length.chunk.next != -1) {
            if (!(hex = strstr(buf, HTTP_EOL)))
                return "Malformed Chunk Encoding";

            if ((finish = strstr(&hex[2], HTTP_EOL)))
                hex = &hex[2];
            else
                return "Malformed Chunk Encoding";
        } else
            hex = buf, finish = strstr(buf, HTTP_EOL);

        finish[0] = '\0';
        if ((e = ioHttpBodyChunkHexToSize(hex, &l)))
            return e;

        j = (&finish[2] - buf);

        if (recv(self->serverSocket, buf, j, 0) == -1)
            return strerror(platformSocketGetLastError());

        if (l > 0) {
            self->length.chunk.next = (PlatformFileOffset) l;
            goto recvBufferAppendChunk_iterate;
        } else
            self->length.chunk.next = -1;
    }

    return NULL;
}

#pragma endregion

const char *recvBufferAppend(RecvBuffer *self, size_t len) {
    size_t i = 0;
    const char *error = NULL;

    if (!self->buffer)
        self->buffer = platformMemoryStreamNew();
    else
        platformMemoryStreamSeek(self->buffer, SEEK_END, 0);

    if (self->options & RECV_BUFFER_DATA_LENGTH_CHUNK)
        return recvBufferAppendChunk(self, len);

    while (i < len) {
        char buf[SB_DATA_SIZE];
        PlatformFileOffset l;
        size_t s = len - i;

        if (s > SB_DATA_SIZE)
            s = SB_DATA_SIZE;

        switch ((l = recv(self->serverSocket, buf, s, 0))) {
            case -1:
                /* TODO: Handle when something is wrong with the socket */
                return strerror(platformSocketGetLastError());
            case 0:
                if (i)
                    return NULL;

                return "No data to be retrieved";
            default:
                if ((fwrite(buf, 1, l, self->buffer)) != l) {
                    if (ferror(self->buffer))
                        return "Error writing to volatile buffer";
                }

                /* TODO: Replace with a function pointer to a append function for each length mode */
                if (DataIncrement(self, l))
                    return error;

                i += l;
                break;
        }
    }

    return NULL;
}

void recvBufferClear(RecvBuffer *self) {
    if (self->buffer)
        fclose(self->buffer), self->buffer = NULL;
}

FILE *recvBufferCopyBetween(RecvBuffer *self, PlatformFileOffset start, PlatformFileOffset end) {
    FILE *newCopy = tmpfile();
    PlatformFileOffset i, tmp = platformMemoryStreamTell(self->buffer);

    platformMemoryStreamSeek(self->buffer, 0, SEEK_END);
    if ((i = platformMemoryStreamTell(self->buffer)) < end)
        end = i;

    i = start, platformMemoryStreamSeek(self->buffer, start, SEEK_SET);

    while (i < end) {
        char buf[SB_DATA_SIZE];
        SOCK_BUF_TYPE j = fread(buf, 1, SB_DATA_SIZE, self->buffer);

        if (fwrite(buf, 1, j, newCopy) != j) {
            fclose(newCopy);
            return NULL;
        }

        i += (PlatformFileOffset) j;
    }

    platformMemoryStreamSeek(self->buffer, tmp, SEEK_SET), rewind(newCopy);
    return newCopy;
}

void recvBufferDitch(RecvBuffer *self, PlatformFileOffset len) {
    FILE *tmp;
    SOCK_BUF_TYPE i;
    char buf[SB_DATA_SIZE];

    if (!self->buffer || !(tmp = tmpfile()))
        return;

    platformMemoryStreamSeek(self->buffer, len, SEEK_SET);
    while ((i = fread(buf, 1, SB_DATA_SIZE, self->buffer)))
        fwrite(buf, 1, i, tmp);

    fclose(self->buffer), self->buffer = tmp;
}

char *recvBufferDitchBetween(RecvBuffer *self, PlatformFileOffset start, PlatformFileOffset len) {
    FILE *new;
    PlatformFileOffset i = 0, k, m = platformMemoryStreamTell(self->buffer);

    platformMemoryStreamSeek(self->buffer, 0, SEEK_END);
    k = platformMemoryStreamTell(self->buffer);
    platformMemoryStreamSeek(self->buffer, 0, SEEK_SET);

    if (k <= start)
        return NULL;

    new = tmpfile();
    while (i < start) {
        SOCK_BUF_TYPE j;
        char buf[SB_DATA_SIZE];
        PlatformFileOffset l = start - i;
        if (l > SB_DATA_SIZE)
            l = SB_DATA_SIZE;

        if (!(j = fread(buf, 1, l, self->buffer))) {
            fclose(new);
            return "Volatile buffer read error";
        }

        if (fwrite(buf, 1, j, new) != j) {
            fclose(new);
            return "Volatile buffer write error";
        }

        i += (PlatformFileOffset) j;
    }

    platformMemoryStreamSeek(self->buffer, len, SEEK_CUR), i += len;

    while (i < k) {
        SOCK_BUF_TYPE j;
        char buf[SB_DATA_SIZE];
        PlatformFileOffset l = k - i;
        if (l > SB_DATA_SIZE)
            l = SB_DATA_SIZE;

        if (!(j = fread(buf, 1, l, self->buffer))) {
            fclose(new);
            return "Volatile buffer read error";
        }

        if (fwrite(buf, 1, j, new) != j) {
            fclose(new);
            return "Volatile buffer write error";
        }

        i += (PlatformFileOffset) j;
    }

    if (m > start)
        m = m > len ? m - len : 0;

    fclose(self->buffer), platformMemoryStreamSeek(new, m, SEEK_SET), self->buffer = new;
    return NULL;
}

void recvBufferFailFree(RecvBuffer *self) {
    if (self->buffer)
        fclose(self->buffer);
}

char *recvBufferFetch(RecvBuffer *self, char *buf, PlatformFileOffset pos, size_t len) {
    if (self->buffer) {
        size_t l;
        if (fseek(self->buffer, pos, SEEK_SET))
            return strerror(errno);

        if ((l = fread(buf, 1, len - 1, self->buffer))) {
            buf[l] = '\0';
            return NULL;
        }
    }

    return "No buffered data";
}

PlatformFileOffset recvBufferFind(RecvBuffer *self, PlatformFileOffset pos, const char *token, size_t len) {
    PlatformFileOffset r;
    size_t i, j, l;
    char buf[SB_DATA_SIZE];

    if (!self->buffer)
        return -1;

    j = r = 0, platformMemoryStreamSeek(self->buffer, pos, SEEK_SET);
    while ((l = fread(buf, 1, SB_DATA_SIZE, self->buffer)) > 0) {
        for (i = 0; i < l; ++i) {
            if (buf[i] == token[j]) {
                ++j;
                if (j == len) {
                    r += pos + (PlatformFileOffset) (i - len + 1);
                    return r;
                }
            } else
                j = 0;
        }

        r += (PlatformFileOffset) l;
    }

    return -1;
}

const char *recvBufferFindAndDitch(RecvBuffer *self, const char *token, size_t len, SOCK_BUF_TYPE *ditched) {
    SOCK_BUF_TYPE d;
    PlatformFileOffset o;
    const char *e;

    if (!self->buffer) {
        if ((e = recvBufferAppend(self, SB_DATA_SIZE)))
            return e;
    }

    d = 0;
    while ((o = recvBufferFind(self, 0, token, len)) == -1) {
        if (!(e = recvBufferAppend(self, SB_DATA_SIZE))) {
            PlatformFileOffset p;

            platformMemoryStreamSeek(self->buffer, 0, SEEK_END), p = platformMemoryStreamTell(self->buffer);
            if (p > len)
                p -= (PlatformFileOffset) len, recvBufferDitch(self, p);
            d += p;
        } else
            return e;
    }

    if (o > 1)
        recvBufferDitch(self, o), d += o;

    if (ditched)
        *ditched = d;

    return NULL;
}

const char *recvBufferFindAndFetch(RecvBuffer *self, const char *token, size_t len, size_t max) {
    PlatformFileOffset i = 0, o;
    const char *e;

    if (!self->buffer) {
        if ((e = recvBufferAppend(self, SB_DATA_SIZE)))
            return e;
    }

    while ((o = recvBufferFind(self, i, token, len)) == -1) {
        if (!(e = recvBufferAppend(self, SB_DATA_SIZE))) {
            platformMemoryStreamSeek(self->buffer, 0, SEEK_END);

            if ((i = platformMemoryStreamTell(self->buffer)) > max)
                return "Exceeded maximum allowed buffer size";
            else
                i -= (PlatformFileOffset) len;
        } else
            return e;
    }

    platformMemoryStreamSeek(self->buffer, i + o, SEEK_CUR);
    return NULL;
}

void recvBufferUpdateSocket(RecvBuffer *self, const SOCKET *socket) {
    self->serverSocket = *socket;
}

RecvBuffer recvBufferNew(SOCKET serverSocket, SocketAddress serverAddress, int options) {
    RecvBuffer self;

    self.options = options, self.serverSocket = serverSocket, self.serverAddress = serverAddress, self.buffer = NULL;
    memset(&self.length, 0, sizeof(RecvBufferLength));
    return self;
}

const char *recvBufferNewFromSocketAddress(RecvBuffer *self, SocketAddress serverAddress, int options) {
    SOCKET sock;
    char *e;

    if ((e = ioCreateSocketFromSocketAddress(&serverAddress, &sock)))
        return e;

    if (connect(sock, &self->serverAddress.address.sock, sizeof(self->serverAddress.address.sock)) == -1) {
        if (sock == INVALID_SOCKET)
            CLOSE_SOCKET(sock);

        return strerror(platformSocketGetLastError());
    }

    *self = recvBufferNew(sock, serverAddress, options);
    return NULL;
}

const char *recvBufferNewFromUri(RecvBuffer *self, const char *uri, int options) {
    const char *e;
    UriDetails detail;
    SocketAddress address;

    detail = uriDetailsNewFrom(uri);
    e = uriDetailsCreateSocketAddress(&detail, &address, uriDetailsGetScheme(&detail));
    uriDetailsFree(&detail);

    if (e)
        return e;

    if ((e = recvBufferNewFromSocketAddress(self, address, options)))
        return e;

    return NULL;
}

const char *recvBufferReconnect(RecvBuffer *self) {
    SOCKET sock;
    char *e;

    CLOSE_SOCKET(self->serverSocket);
    if ((e = ioCreateSocketFromSocketAddress(&self->serverAddress, &sock)))
        return e;

    if (connect(sock, &self->serverAddress.address.sock, sizeof(self->serverAddress.address.sock)) == -1) {
        if (sock == INVALID_SOCKET)
            CLOSE_SOCKET(sock);

        return strerror(platformSocketGetLastError());
    }

    return NULL;
}

const char *recvBufferSend(RecvBuffer *self, const void *data, size_t n, int flags) {
    const char* e;
    SOCK_BUF_TYPE s;
    recvBufferSend_retry:
    s = send(self->serverSocket, data, n, flags);

    switch(s) {
        case -1:
        case 0:
            /* TODO: attempt reconnect */
            e = recvBufferReconnect(self);
            if (e)
                return e;

            goto recvBufferSend_retry;
        default:
            if(s == n)
                break;
            /* TODO: Do something about the size mismatch */
    }

    return NULL;
}

void recvBufferSetLengthChunk(RecvBuffer *self) {
    self->options &= ~(RECV_BUFFER_DATA_LENGTH_KNOWN) | ~(RECV_BUFFER_DATA_LENGTH_TOKEN);
    self->options |= RECV_BUFFER_DATA_LENGTH_CHUNK;
    self->length.chunk.total = 0, self->length.chunk.next = -1;
}

void recvBufferSetLengthKnown(RecvBuffer *self, PlatformFileOffset length) {
    self->options &= ~(RECV_BUFFER_DATA_LENGTH_CHUNK) | ~(RECV_BUFFER_DATA_LENGTH_TOKEN);
    self->options |= RECV_BUFFER_DATA_LENGTH_KNOWN;
    self->length.known.total = length, self->length.known.escape = 0;
}

void recvBufferSetLengthUnknown(RecvBuffer *self, size_t limit) {
    self->options &=
            ~(RECV_BUFFER_DATA_LENGTH_CHUNK) | ~(RECV_BUFFER_DATA_LENGTH_KNOWN) | ~(RECV_BUFFER_DATA_LENGTH_TOKEN);
    self->length.unknown.limit = limit, self->length.unknown.escape = 0;
}

void recvBufferSetLengthToken(RecvBuffer *self, const char *token, size_t length) {
    self->options &= ~(RECV_BUFFER_DATA_LENGTH_CHUNK) | ~(RECV_BUFFER_DATA_LENGTH_KNOWN);
    self->options |= RECV_BUFFER_DATA_LENGTH_TOKEN;
    self->length.token.token = token, self->length.token.length = length;
}
