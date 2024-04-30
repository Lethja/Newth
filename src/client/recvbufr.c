#include "recvbufr.h"
#include "uri.h"

#pragma region Static Helper Functions

/**
 * Append data from a buffer to the end of the RecvBuffer, increase heap allocation if necessary
 * @param self In: The RecvBuffer to append to
 * @param buf In: The buffer to append from
 * @param len In: The size of buffer to append from
 * @return NULL on success, user friendly error message otherwise
 */
static inline char *BufferAppend(RecvBuffer *self, char *buf, size_t len) {
    if (self->len + len > self->max) {
        size_t max = self->len + len;
        if (platformHeapResize((void **) &self->buffer, sizeof(char), max))
            return strerror(errno);

        self->max = max;
    }

    memcpy(&self->buffer[self->len], buf, len), self->len += len;
    return NULL;
}

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

        if (self->length.chunk.next >= l) {
            if (BufferAppend(self, buf, l))
                return strerror(errno);

            self->length.chunk.next -= (PlatformFileOffset) l, len -= l;
            if (recv(self->serverSocket, buf, l, 0) == -1)
                return strerror(platformSocketGetLastError());
        } else {
            char *p = &buf[self->length.chunk.next];
            if (strcmp(p, HTTP_EOL) != 0)
                return "Malformed Chunk Encoding";

            if (BufferAppend(self, buf, len))
                return strerror(errno);

            self->length.chunk.next -= self->length.chunk.next, len -= l;
            if (recv(self->serverSocket, buf, self->length.chunk.next, 0) == -1)
                return strerror(platformSocketGetLastError());
        }

        if (l < s)
            return NULL; /* Next chuck not in transmission buffer yet */
    }

    if (i == len)
        return NULL;

    /* Parse the next chunk encoding */
    recvBufferAppendChunk_parse:
    {
        const char *e;
        char b[20] = {0}, *finish, *hex;
        size_t l, j, k;

        if ((k = recv(self->serverSocket, b, 19, MSG_PEEK)) == -1)
            return strerror(platformSocketGetLastError());

        else if (k < (self->length.chunk.next == -1 ? 3 : 5))
            return NULL; /* Chunk not available yet */

        if (self->length.chunk.next != -1) {
            if (!(hex = strstr(b, HTTP_EOL)))
                return "Malformed Chunk Encoding";

            if ((finish = strstr(&hex[2], HTTP_EOL)))
                hex = &hex[2];
            else
                return "Malformed Chunk Encoding";
        } else
            hex = b, finish = strstr(b, HTTP_EOL);

        finish[0] = '\0';
        if ((e = ioHttpBodyChunkHexToSize(hex, &l)))
            return e;

        j = (&finish[2] - b);

        if (recv(self->serverSocket, b, j, 0) == -1)
            return strerror(platformSocketGetLastError());

        if (l > 0) {
            self->length.chunk.next = (PlatformFileOffset) l;
            goto recvBufferAppendChunk_iterate;
        } else {
            self->length.chunk.next = -1;
            recvBufferSetLengthComplete(self);
        }
    }

    return NULL;
}

#pragma endregion

const char *recvBufferAppend(RecvBuffer *self, size_t len) {
    size_t i = 0;
    const char *error = NULL;

    if(self->options & RECV_BUFFER_DATA_LENGTH_COMPLETE)
        return "Request completed";

    if (!self->buffer) {
        if (!(self->buffer = calloc(SB_DATA_SIZE, 1)))
            return strerror(errno);
        else
            self->max = SB_DATA_SIZE, self->len = self->idx = 0;
    }

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
                if (BufferAppend(self, buf, l))
                    return strerror(errno);

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
        free(self->buffer), self->buffer = NULL, self->idx = self->len = self->max = 0;
}

char *recvBufferCopyBetween(RecvBuffer *self, PlatformFileOffset start, PlatformFileOffset end) {
    char *newCopy;
    size_t len = end - start;

    if (start + len > end)
        return NULL;

    if (start + len > self->len) {
        len = self->len - start;

        if (!(newCopy = calloc(len + 1, 1)))
            return NULL;

        memcpy(newCopy, &self->buffer[start], len + 1);
    } else {
        if (!(newCopy = calloc(len, 1)))
            return NULL;

        memcpy(newCopy, &self->buffer[start], len - 1);
    }

    return newCopy;
}

void recvBufferDitch(RecvBuffer *self, PlatformFileOffset len) {
    if (len) {
        if (len < self->len)
            memmove(self->buffer, &self->buffer[len], self->len - len), self->len -= len;
        else
            free(self->buffer), self->buffer = NULL;
    }
}

void recvBufferDitchBetween(RecvBuffer *self, PlatformFileOffset start, PlatformFileOffset len) {
    size_t e = start + len, l = self->len - e;
    memmove(&self->buffer[start], &self->buffer[e], l);
    self->len -= len;
}

void recvBufferFailFree(RecvBuffer *self) {
    if (self->buffer)
        free(self->buffer);
}

char *recvBufferFetch(RecvBuffer *self, char *buf, PlatformFileOffset pos, size_t len) {
    if (self->buffer) {
        size_t l = (self->len - pos);
        if (l < len)
            memcpy(buf, &self->buffer[pos], l), buf[l] = '\0';
        else
            memcpy(buf, &self->buffer[pos], len - 1), buf[len - 1] = '\0';

        return NULL;
    }

    return "No buffered data";
}

PlatformFileOffset recvBufferFind(RecvBuffer *self, PlatformFileOffset pos, const char *token, size_t len) {
    PlatformFileOffset r;
    char *p;
    size_t i, j, l;

    if (!self->buffer || pos > self->len)
        return -1;

    p = &self->buffer[pos], l = self->len - pos - 1;

    for (i = 0; i < l; ++i) {
        if (toupper(p[i]) == toupper(token[0])) {
            size_t k;

            if (i + len > self->len)
                return -1;

            for (k = 0, j = i; k < len; ++k, ++j) {
                if (toupper(p[j]) != toupper(token[k]))
                    break;
            }

            if (k == len) {
                r = pos + (PlatformFileOffset) i;
                return r;
            }
        }
    }

    return -1;
}

const char *recvBufferFindAndDitch(RecvBuffer *self, const char *token, size_t len, SOCK_BUF_TYPE *ditched) {
    SOCK_BUF_TYPE d;
    PlatformFileOffset o;
    const char *e;

    if (!self->buffer)
        if ((e = recvBufferAppend(self, SB_DATA_SIZE)))
            return e;

    while (self->len < len)
        if ((e = recvBufferAppend(self, SB_DATA_SIZE)))
            return e;

    d = 0;
    while ((o = recvBufferFind(self, 0, token, len)) == -1) {
        if (!(e = recvBufferAppend(self, SB_DATA_SIZE))) {
            if ((o = recvBufferFind(self, 0, token, len)) != -1)
                break;

            /* The token wasn't found in the current buffer, ditch what's here to keep memory usage down */
            if (self->len > len) {
                /* Overlapping the old buffer with the new by length of token prevents missing a potential match */
                PlatformFileOffset ditch = (PlatformFileOffset) (self->len - len);
                recvBufferDitch(self, ditch);
                d += ditch;
            }
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
    PlatformFileOffset i;
    const char *e;

    if (max < len)
        return "Token bigger then buffer limits";

    if (!self->buffer)
        if ((e = recvBufferAppend(self, SB_DATA_SIZE)))
            return e;

    while (self->len < len)
        if ((e = recvBufferAppend(self, SB_DATA_SIZE)))
            return e;

    if (self->len <= len) {
        if ((e = recvBufferAppend(self, SB_DATA_SIZE)))
            return e;
    }

    i = 0;
    while (recvBufferFind(self, i, token, len) == -1) {
        if (!(e = recvBufferAppend(self, SB_DATA_SIZE))) {
            if (self->len > max)
                return "Exceeded maximum allowed buffer size";
        } else
            return e;
    }

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

const char *recvBufferNewFromUriDetails(RecvBuffer *self, void *details, int options) {
    const char *e;
    SocketAddress a;
    UriDetails *d = details;

    if ((e = uriDetailsCreateSocketAddress(d, &a, uriDetailsGetScheme(d))))
        return e;

    if ((e = recvBufferNewFromSocketAddress(self, a, options)))
        return e;

    return NULL;
}

const char *recvBufferNewFromUri(RecvBuffer *self, const char *uri, int options) {
    const char *e;
    UriDetails detail = uriDetailsNewFrom(uri);

    e = recvBufferNewFromUriDetails(self, &detail, options);
    uriDetailsFree(&detail);
    return e;
}

const char *recvBufferConnect(RecvBuffer *self) {
    if (connect(self->serverSocket, &self->serverAddress.address.sock, sizeof(self->serverAddress.address.sock)) == -1) {
        if (self->serverSocket == INVALID_SOCKET)
            CLOSE_SOCKET(self->serverSocket);

        return strerror(platformSocketGetLastError());
    }

    return NULL;
}

const char *recvBufferReconnect(RecvBuffer *self) {
    SOCKET sock;
    char *e;

    CLOSE_SOCKET(self->serverSocket);
    if ((e = ioCreateSocketFromSocketAddress(&self->serverAddress, &sock)))
        return e;

    self->serverSocket = sock;

    return recvBufferConnect(self);
}

const char *recvBufferSend(RecvBuffer *self, const void *data, size_t n, int flags) {
    const char *e = 0;
    SOCK_BUF_TYPE s;
    unsigned int attempt = 0;

    recvBufferSend_retry:
    if (attempt++ >= 3)
        return e;

    switch ((s = send(self->serverSocket, data, n, flags))) {
        case -1:
        case 0:
            /* TODO: attempt reconnect */
            e = recvBufferReconnect(self);
            if (e)
                goto recvBufferSend_retry;
            break;
        default:
            if (s == n)
                break;
            /* TODO: Do something about the size mismatch */
    }

    return NULL;
}

void recvBufferSetLengthChunk(RecvBuffer *self) {
    self->options &=
            ~(RECV_BUFFER_DATA_LENGTH_KNOWN) | ~(RECV_BUFFER_DATA_LENGTH_TOKEN) | ~(RECV_BUFFER_DATA_LENGTH_COMPLETE);
    self->options |= RECV_BUFFER_DATA_LENGTH_CHUNK;
    self->length.chunk.total = 0, self->length.chunk.next = -1;
}

void recvBufferSetLengthComplete(RecvBuffer *self) {
    self->options &=
            ~(RECV_BUFFER_DATA_LENGTH_KNOWN) | ~(RECV_BUFFER_DATA_LENGTH_TOKEN) | ~(RECV_BUFFER_DATA_LENGTH_CHUNK);
    self->options |= RECV_BUFFER_DATA_LENGTH_COMPLETE;
    self->length.chunk.total = 0, self->length.chunk.next = -1;
}

void recvBufferSetLengthKnown(RecvBuffer *self, PlatformFileOffset length) {
    self->options &=
            ~(RECV_BUFFER_DATA_LENGTH_CHUNK) | ~(RECV_BUFFER_DATA_LENGTH_TOKEN) | ~(RECV_BUFFER_DATA_LENGTH_COMPLETE);
    self->options |= RECV_BUFFER_DATA_LENGTH_KNOWN;
    self->length.known.total = length, self->length.known.escape = 0;
}

void recvBufferSetLengthUnknown(RecvBuffer *self, size_t limit) {
    self->options &=
            ~(RECV_BUFFER_DATA_LENGTH_CHUNK) | ~(RECV_BUFFER_DATA_LENGTH_KNOWN) | ~(RECV_BUFFER_DATA_LENGTH_TOKEN) |
            ~(RECV_BUFFER_DATA_LENGTH_COMPLETE);
    self->length.unknown.limit = limit, self->length.unknown.escape = 0;
}

void recvBufferSetLengthToken(RecvBuffer *self, const char *token, size_t length) {
    self->options &= ~(RECV_BUFFER_DATA_LENGTH_CHUNK) | ~(RECV_BUFFER_DATA_LENGTH_KNOWN);
    self->options |= RECV_BUFFER_DATA_LENGTH_TOKEN;
    self->length.token.token = token, self->length.token.length = length;
}
