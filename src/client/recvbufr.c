#include "recvbufr.h"
#include "uri.h"

#pragma region Static Helper Functions

static inline PlatformFileOffset ExtractChunkSize(RecvBuffer *self, const char **e) {
    char *buf, *p;
    size_t len;
    PlatformFileOffset start, end, hex;

    /* TODO: test in case where buffer is ditched */
    platformMemoryStreamSeek(self->buffer, self->length.chunk.next, SEEK_CUR);
    hex = self->length.chunk.total ? self->length.chunk.total + 2 : 0;
    end = recvBufferFind(self, hex, HTTP_EOL, 2);

    if (end == -1) {
        *e = "Malformed Chunk Encoding";
        return -1;
    }

    if (hex > 1)
        start = hex - 2, len = end - start;
    else
        start = hex, len = end - hex;

    buf = malloc(len + 1);

    if (recvBufferFetch(self, buf, start, len + 1)) {
        free(buf);
        return -1;
    }

    if (self->length.chunk.total) {
        if (buf[0] != '\r' || buf[1] != '\n') {
            free(buf);
            *e = "Malformed Chunk Encoding";
            return 0;
        }

        p = &buf[2];
    } else
        p = buf;

    hex = (PlatformFileOffset) ioHttpBodyChunkHexToSize(p), free(buf);
    recvBufferDitchBetween(self, start, (PlatformFileOffset) len + 2);

    return hex;
}

static inline char DataIncrement(RecvBuffer *self, PlatformFileOffset added, const char **e) {
    if (self->options & RECV_BUFFER_DATA_LENGTH_CHUNK) {
        self->length.chunk.next -= added;

        while (self->length.chunk.next < 0) {
            PlatformFileOffset chunk;
            if ((chunk = ExtractChunkSize(self, e)) <= 0) {
                if (*e)
                    return 1;

                break;
            }

            self->length.chunk.next += chunk, self->length.chunk.total += chunk;
        }

        if (!self->length.chunk.next)
            return 1;

    } else if (self->options & RECV_BUFFER_DATA_LENGTH_KNOWN) {
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

#pragma endregion

const char *recvBufferAppend(RecvBuffer *self, size_t len) {
    size_t i = 0;
    const char *e = NULL;

    if (!self->buffer)
        self->buffer = platformMemoryStreamNew();
    else
        platformMemoryStreamSeek(self->buffer, SEEK_END, 0);

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

                if (DataIncrement(self, l, &e))
                    return e;

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
        m -= len;

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

const char *recvBufferFindAndDitch(RecvBuffer *self, const char *token, size_t len) {
    PlatformFileOffset o;
    const char *e;

    if (!self->buffer) {
        if ((e = recvBufferAppend(self, SB_DATA_SIZE)))
            return e;
    }

    while ((o = recvBufferFind(self, 0, token, len)) == -1) {
        if (!(e = recvBufferAppend(self, SB_DATA_SIZE))) {
            PlatformFileOffset p;

            platformMemoryStreamSeek(self->buffer, 0, SEEK_END), p = platformMemoryStreamTell(self->buffer);
            if (p > len)
                p -= (PlatformFileOffset) len, recvBufferDitch(self, p);
        } else
            return e;
    }

    if (o > 1)
        recvBufferDitch(self, o);

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

void recvBufferSetLengthChunk(RecvBuffer *self) {
    self->options &= ~(RECV_BUFFER_DATA_LENGTH_KNOWN) | ~(RECV_BUFFER_DATA_LENGTH_TOKEN);
    self->options |= RECV_BUFFER_DATA_LENGTH_CHUNK;
    self->length.chunk.total = self->length.chunk.next = 0;
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
