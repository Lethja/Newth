#include "err.h"
#include "recvbufr.h"
#include "uri.h"

#pragma region Static Helper Functions

/**
 * recv() wrapper that automatically blocks until either 1 byte or a non-blocking error is returned
 * @param fd In: The socket to read from
 * @param buf Out: The buffer write to
 * @param n In: The size in bytes to read
 * @param flags In: The recv() flags to pass
 * @return The number of bytes eventually (including 0 to indicate socket shutdown) or -1 on recv error
 */
static inline SOCK_BUF_TYPE AppendWait(int fd, void *buf, size_t n, int flags) {
    SOCK_BUF_TYPE s;
BlockAppend_keepGoing:
    switch ((s = recv(fd, buf, n, flags))) {
        case -1:
            switch (platformSocketGetLastError()) {
                    #pragma region Handle socket error
                case SOCKET_TRY_AGAIN:
#if SOCKET_TRY_AGAIN != SOCKET_WOULD_BLOCK
                case SOCKET_WOULD_BLOCK:
#endif
                    goto BlockAppend_keepGoing;
            }
        /* Fallthrough */
        default:
            return s;
    }
}

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
 * @return zero when the data transmission should keep going, other on transmission finished or error
 */
static inline char DataIncrement(RecvBuffer *self, size_t added) {
    if (self->options & RECV_BUFFER_DATA_LENGTH_KNOWN) {
        self->length.known.escape += (PlatformFileOffset) added;
        if (self->length.known.escape >= self->length.known.total)
            goto DataIncrement_complete;
    } else if (self->options & RECV_BUFFER_DATA_LENGTH_TOKEN) {
        self->length.token.length += (PlatformFileOffset) added;
        if (platformStringFindNeedleRaw(self->buffer, self->length.token.token, self->len,
                                        strlen(self->length.token.token)))
            goto DataIncrement_complete;
    } else { /* RECV_BUFFER_DATA_LENGTH_UNKNOWN */
        self->length.unknown.escape += (PlatformFileOffset) added;
        if (self->length.unknown.escape >= self->length.unknown.limit)
            return 1;
    }
    return 0;

DataIncrement_complete:
    recvBufferSetLengthComplete(self);
    return 1;
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
        SOCK_BUF_TYPE l, s = (SOCK_BUF_TYPE) (self->length.chunk.next < len ? self->length.chunk.next : len);
        char buf[SB_DATA_SIZE];

        if (s > SB_DATA_SIZE)
            s = SB_DATA_SIZE;

        if ((l = AppendWait(self->serverSocket, buf, s, MSG_PEEK)) == -1)
            goto recvBufferAppendChunk_socketError;

        if (self->length.chunk.next >= l) {
            if (BufferAppend(self, buf, l))
                return strerror(errno);

            self->length.chunk.next -= (PlatformFileOffset) l, len -= l;
            if (AppendWait(self->serverSocket, buf, l, 0) == -1)
                goto recvBufferAppendChunk_socketError;
        } else {
            char *p = &buf[self->length.chunk.next];
            if (strcmp(p, HTTP_EOL) != 0)
                return ErrMalformedChunkEncoding;

            if (BufferAppend(self, buf, len))
                return strerror(errno);

            self->length.chunk.next -= self->length.chunk.next, len -= l;
            if (AppendWait(self->serverSocket, buf, (SOCK_BUF_TYPE) self->length.chunk.next, 0) == -1)
                goto recvBufferAppendChunk_socketError;
        }

        if (l < s)
            return NULL; /* Next chuck not in transmission buffer yet */
    }

    if (i == len)
        return NULL;

    /* Parse the next chunk encoding */
recvBufferAppendChunk_parse: {
        const char *e;
        char b[20] = {0}, *finish, *hex;
        SOCK_BUF_TYPE l, j, k;

        if ((k = AppendWait(self->serverSocket, b, 19, MSG_PEEK)) == -1)
            goto recvBufferAppendChunk_socketError;

        else if (k < (self->length.chunk.next == -1 ? 3 : 5))
            return NULL; /* Chunk not available yet */

        if (self->length.chunk.next != -1) {
            if (!(hex = strstr(b, HTTP_EOL)))
                return ErrMalformedChunkEncoding;

            if ((finish = strstr(&hex[2], HTTP_EOL)))
                hex = &hex[2];
            else
                return ErrMalformedChunkEncoding;
        } else {
            hex = b;
            if (!(finish = strstr(b, HTTP_EOL)))
                return ErrMalformedChunkEncoding;
        }

        finish[0] = '\0';
        if ((e = ioHttpBodyChunkHexToSize(hex, (size_t *) &l)))
            return e;

        j = (&finish[2] - b);

        if (AppendWait(self->serverSocket, b, j, 0) == -1)
            goto recvBufferAppendChunk_socketError;

        if (l > 0) {
            self->length.chunk.next = (PlatformFileOffset) l;
            goto recvBufferAppendChunk_iterate;
        } else {
            self->length.chunk.next = -1;
            recvBufferSetLengthComplete(self);
        }
    }

    return NULL;

recvBufferAppendChunk_socketError:
    return strerror(platformSocketGetLastError());
}

#pragma endregion

const char *recvBufferAppend(RecvBuffer *self, size_t len) {
    size_t i = 0;

    if (self->options & RECV_BUFFER_DATA_LENGTH_COMPLETE)
        return ErrRequestCompleted;

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
        SOCK_BUF_TYPE l;
        size_t s = len - i;

        if (s > SB_DATA_SIZE)
            s = SB_DATA_SIZE;

recvBufferAppend_tryAgain:
        /* TODO: Add callback call here so progress can be reported and a stuck connection can be cancelled by a user */
        switch ((l = recv(self->serverSocket, buf, s, 0))) {
            case -1:
                switch (platformSocketGetLastError()) {
                        #pragma region Handle socket error
                    case SOCKET_TRY_AGAIN:
#if SOCKET_TRY_AGAIN != SOCKET_WOULD_BLOCK
                    case SOCKET_WOULD_BLOCK:
#endif
                        goto recvBufferAppend_tryAgain;
                    default:
                        return strerror(platformSocketGetLastError());
                }
            case 0:
                if (i)
                    return NULL;

                return ErrNoDataToBeRetrieved;
            default:
                if (BufferAppend(self, buf, l))
                    return strerror(errno);

                if (DataIncrement(self, l))
                    return NULL;

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
    PlatformFileOffset len = end - start;

    if (start + len > end)
        return NULL;

    if (start + len > self->len) {
        len = (PlatformFileOffset) self->len - start;

        if (!(newCopy = calloc((size_t) len + 1, 1)))
            return NULL;

        memcpy(newCopy, &self->buffer[start], (size_t) len + 1);
    } else {
        if (!(newCopy = calloc((size_t) len, 1)))
            return NULL;

        memcpy(newCopy, &self->buffer[start], (size_t) len - 1);
    }

    return newCopy;
}

void recvBufferDitch(RecvBuffer *self, PlatformFileOffset len) {
    if (len) {
        if (len < self->len) {
            size_t diff = self->len - (size_t) len;
            memmove(self->buffer, &self->buffer[len], diff), self->len -= (size_t) len;
        } else
            free(self->buffer), self->buffer = NULL, self->len = 0;
    }
}

void recvBufferFailFree(RecvBuffer *self) {
    if (self->buffer)
        free(self->buffer);
}

const char *recvBufferFetch(RecvBuffer *self, char *buf, PlatformFileOffset pos, size_t len) {
    if (self->buffer) {
        size_t l = (size_t) (self->len - pos);
        if (l < len)
            memcpy(buf, &self->buffer[pos], l), buf[l] = '\0';
        else
            memcpy(buf, &self->buffer[pos], len - 1), buf[len - 1] = '\0';

        return NULL;
    }

    return ErrNoBufferedData;
}

PlatformFileOffset recvBufferFind(RecvBuffer *self, PlatformFileOffset pos, const char *token, size_t len) {
    PlatformFileOffset r;
    char *p;
    size_t i, j, l;

    if (!self->buffer || pos > self->len)
        return -1;

    p = &self->buffer[pos], l = (size_t) (self->len - pos - 1);

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
                d += (SOCK_BUF_TYPE) ditch;
            }
        } else
            return e;
    }

    if (o > 1)
        recvBufferDitch(self, o), d += (SOCK_BUF_TYPE) o;

    if (ditched)
        *ditched = d;

    return NULL;
}

const char *recvBufferFindAndFetch(RecvBuffer *self, const char *token, size_t len, size_t max) {
    PlatformFileOffset i;
    const char *e;

    if (max < len)
        return ErrTokenBiggerThenBufferLimits;

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
                return ErrExceededMaximumAllowedBufferSize;
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

    if (connect(sock, (struct sockaddr*) &serverAddress, sizeof(struct sockaddr_storage))) {
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

    memset(&a, 0, sizeof(SocketAddress));
    if ((e = uriDetailsCreateSocketAddress(d, &a, (unsigned short) uriDetailsGetScheme(d))))
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
    struct sockaddr *socketAddress = &self->serverAddress.address.sock;
    size_t socketLength = sizeof(self->serverAddress.address.sock);

    if (connect(self->serverSocket, socketAddress, socketLength) == -1) {
        if (self->serverSocket == INVALID_SOCKET)
            CLOSE_SOCKET(self->serverSocket);

        return strerror(platformSocketGetLastError());
    }

    if (platformSocketSetBlock(self->serverSocket, 0) == -1)
        return strerror(platformSocketGetLastError());

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
    const char *e = 0, *d = data;
    SOCK_BUF_TYPE s, sent = 0;
    unsigned int attempt = 0;
    char dump[SB_DATA_SIZE];

    #pragma region Check that the last request ended successfully and that the same TCP stream can be reused

    if (self->options && !(self->options & RECV_BUFFER_DATA_LENGTH_COMPLETE))
        recvBufferReconnect(self), recvBufferClear(self);

    recvBufferSetLengthUnknown(self, SB_DATA_SIZE);

    #pragma endregion

    #pragma region Limit the amount of reattempts at establishing a connection there can be

recvBufferSend_reattempt:
    ++attempt;

    #pragma endregion

    #pragma region Make sure the entirity of the data is sent on a non-blocking socket

recvBufferSend_keepSending:
    switch ((s = send(self->serverSocket, &d[sent], n - sent, flags))) {
        case -1:
            switch (platformSocketGetLastError()) {
                case SOCKET_TRY_AGAIN:
#if SOCKET_TRY_AGAIN != SOCKET_WOULD_BLOCK
                case SOCKET_WOULD_BLOCK:
#endif
                    goto recvBufferSend_keepSending;
                default:
recvBufferSend_reconnect:
                    if (attempt >= 3)
                        return e;

                    e = recvBufferReconnect(self);
                    goto recvBufferSend_reattempt;
            }
        case 0:
            goto recvBufferSend_keepSending;
        default:
            sent += s;
            if ((size_t) sent == n)
                break;

            goto recvBufferSend_keepSending;
    }

    #pragma endregion

    #pragma region Check the remote is alive and responding otherwise reattempt

recvBufferSend_reply:
    switch (recv(self->serverSocket, dump, SB_DATA_SIZE, MSG_PEEK)) {
        case -1:
            switch (platformSocketGetLastError()) {
                case SOCKET_TRY_AGAIN:
#if SOCKET_TRY_AGAIN != SOCKET_WOULD_BLOCK
                case SOCKET_WOULD_BLOCK:
#endif
                    goto recvBufferSend_reply;
            }
        case 0:
            sent = 0;
            goto recvBufferSend_reconnect;
        default:
            break;
    }

    #pragma endregion

    return NULL;
}

/**
 * Helper function for when a RecvBuffer is set into chunk mode.
 * It assumes the current start of the buffer is now at the beginning of the first chunk hex and pre-computes chunks
 * already in the buffer allowing the next call to recvBufferAppend to be synchronized with the state of the buffer
 * @param self The buffer to pre-compute chunks for
 * @remark In the case that a chunks metadata is between the end of one packet and the beginning of another
 * this function will pull the beginning of the next packet into recvBuffer so that the chunk metadata can be parsed.
 * If the remote is not immediately forthcoming with information the function assumes this connection to have a
 * malicious nature and will drop the connection, clear the buffer and set the connection length mode to unknown
 */
static inline void HttpChunkParseExistingBuffer(RecvBuffer *self) {
    char *finish, hex[20];

    self->length.chunk.total = self->length.chunk.next = 0;
    while (self->length.chunk.total < self->len) {
        memset(hex, 0, sizeof(hex));
        recvBufferFetch(self, hex, self->length.chunk.total ? self->length.chunk.total + 2 : self->length.chunk.total, 20);
        if ((finish = strstr(hex, HTTP_EOL))) {
            size_t s;

            finish[0] = '\0';
            if (!ioHttpBodyChunkHexToSize(hex, &s)) {
                size_t i = strlen(hex) + (self->length.chunk.total ? 4 : 2), l = self->length.chunk.total + i;

                if (self->len - l > 0)
                    memmove(&self->buffer[self->length.chunk.total], &self->buffer[l], self->len - l), self->len -= i;
                else
                    self->len -= i;

                self->length.chunk.total += (PlatformFileOffset) s, self->length.chunk.next = (PlatformFileOffset) s;
            }
        } else { /* Try and buffer in a overlapping chunk, when remote won't assume intent is malicious and close */
            if ((AppendWait(self->serverSocket, hex, 19, MSG_PEEK) != -1)) {
                char *p;
                if ((p = strstr(hex, HTTP_EOL))) {
                    size_t n = p - (char*)hex + 2;
                    if (self->max < self->len + n) {
                        if (platformHeapResize((void **) &self->buffer, sizeof(char), self->len + n))
                            goto HttpChunkParseExistingBuffer_abort;

                        self->max = self->len + n;
                    }

                    memcpy(&self->buffer[self->len], hex, n), AppendWait(self->serverSocket, hex, n, 0);
                    self->len += n;
                } else
                    goto HttpChunkParseExistingBuffer_abort;
            } else
                goto HttpChunkParseExistingBuffer_abort;
        }
    }

    self->length.chunk.next = self->length.chunk.total - (PlatformFileOffset) self->len;
    return;

HttpChunkParseExistingBuffer_abort:
    recvBufferClear(self);
    CLOSE_SOCKET(self->serverSocket), self->serverSocket = INVALID_SOCKET;
    recvBufferSetLengthUnknown(self, 0);
}

void recvBufferSetLengthChunk(RecvBuffer *self) {
    self->options &= ~(RECV_BUFFER_DATA_LENGTH_KNOWN | RECV_BUFFER_DATA_LENGTH_TOKEN |
                       RECV_BUFFER_DATA_LENGTH_COMPLETE);
    self->options |= RECV_BUFFER_DATA_LENGTH_CHUNK;
    if (self->buffer && self->len)
        HttpChunkParseExistingBuffer(self);
    else
        self->length.chunk.total = 0, self->length.chunk.next = -1;
}

void recvBufferSetLengthComplete(RecvBuffer *self) {
    self->options &= ~(RECV_BUFFER_DATA_LENGTH_KNOWN | RECV_BUFFER_DATA_LENGTH_TOKEN | RECV_BUFFER_DATA_LENGTH_CHUNK);
    self->options |= RECV_BUFFER_DATA_LENGTH_COMPLETE;
    /* Clear data */
    self->length.chunk.total = 0, self->length.chunk.next = -1;
}

void recvBufferSetLengthKnown(RecvBuffer *self, PlatformFileOffset length) {
    self->options &= ~(RECV_BUFFER_DATA_LENGTH_CHUNK | RECV_BUFFER_DATA_LENGTH_TOKEN |
                       RECV_BUFFER_DATA_LENGTH_COMPLETE);
    self->options |= RECV_BUFFER_DATA_LENGTH_KNOWN;
    self->length.known.total = length, self->length.known.escape = 0;
}

void recvBufferSetLengthUnknown(RecvBuffer *self, size_t limit) {
    self->options &= ~(RECV_BUFFER_DATA_LENGTH_CHUNK | RECV_BUFFER_DATA_LENGTH_KNOWN | RECV_BUFFER_DATA_LENGTH_TOKEN |
                       RECV_BUFFER_DATA_LENGTH_COMPLETE);
    self->length.unknown.limit = (PlatformFileOffset) limit, self->length.unknown.escape = 0;
}

void recvBufferSetLengthToken(RecvBuffer *self, const char *token, size_t length) {
    self->options &= ~(RECV_BUFFER_DATA_LENGTH_CHUNK | RECV_BUFFER_DATA_LENGTH_KNOWN |
                       RECV_BUFFER_DATA_LENGTH_COMPLETE);
    self->options |= RECV_BUFFER_DATA_LENGTH_TOKEN;
    self->length.token.token = token, self->length.token.length = (PlatformFileOffset) length;
}
