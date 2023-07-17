#include "sockbufr.h"

#include <string.h>

SocketBuffer socketBufferNew(SOCKET clientSocket, char options) {
    SocketBuffer self;
    self.clientSocket = clientSocket, self.extension = NULL, self.idx = 0, self.options = options;
    memset(self.buffer, 0, BUFSIZ);
    return self;
}

size_t socketBufferFlush(SocketBuffer *self) {
    if (self->idx) {
        size_t i = 0;
        SOCK_BUF_TYPE sent;
        socketBufferFlushTryAgain:
        sent = send(self->clientSocket, &self->buffer[i], self->idx - i, 0);
        if (sent == -1) {
            int err = platformSocketGetLastError();
            switch (err) { /* NOLINT(hicpp-multiway-paths-covered) */
                case SOCKET_WOULD_BLOCK:
#if SOCKET_WOULD_BLOCK != SOCKET_TRY_AGAIN
                    case SOCKET_TRY_AGAIN:
#endif
                    self->options |= SOC_BUF_ERR_FULL;

                    if (self->options & SOC_BUF_OPT_EXTEND) {
                        self->extension = socketBufferMemoryPoolAppend(self->extension, &self->buffer[i], self->idx - i);
                        /* Not yet implemented */
                        LINEDBG;
                        self->options |= SOC_BUF_ERR_FAIL;
                        return 0;
                    } else if (i) {
                        LINEDBG;
                        memmove(self->buffer, &self->buffer[i], self->idx - i), self->idx -= i;
                    }
                    return i;
                default:
                    self->options |= SOC_BUF_ERR_FAIL;
                    return 0;
            }
        } else if (i < self->idx)
            i += sent;

        if (i == self->idx) {
            self->idx = 0;
            return i;
        }

        /* TODO: Clean this mess up */
        goto socketBufferFlushTryAgain;
    }

    return 0;
}

size_t socketBufferWriteData(SocketBuffer *self, const char *data, size_t len) {
    size_t i;

    for (i = 0; i < len; ++i) {
        if (self->idx == BUFSIZ) {
            size_t sent = socketBufferFlush(self);

            if (self->options & SOC_BUF_ERR_FAIL)
                return 0;
            else if (!sent || self->options & SOC_BUF_ERR_FULL)
                return i;
        }

        self->buffer[self->idx] = *data;
        ++self->idx, ++data;
    }

    return i;
}

size_t socketBufferWriteText(SocketBuffer *self, const char *data) {
    return socketBufferWriteData(self, data, strlen(data));
}

MemoryPool *socketBufferMemoryPoolNew(char *data, size_t bytes) {
    MemoryPool *self = malloc(sizeof(MemoryPool));
    if (bytes)
        self->data = malloc(bytes), memcpy(self->data, data, bytes);

    self->length = bytes, self->i = 0;
    return self;
}

MemoryPool *socketBufferMemoryPoolAppend(MemoryPool *self, char *data, size_t bytes) {
    if (self && self->length) {
        char *new = realloc(self->data, self->length + bytes);
        if (new)
            self->data = new, memcpy(&self->data[self->length], data, bytes), self->length += bytes;
    } else {
        if (self)
            free(self);

        return socketBufferMemoryPoolNew(data, bytes);
    }

    return self;
}
