#include "sockbufr.h"

#include <string.h>

SocketBuffer socketBufferNew(SOCKET clientSocket, char options) {
    SocketBuffer self;
    self.clientSocket = clientSocket, self.extension = NULL, self.idx = 0, self.options = options;
    memset(self.buffer, 0, SB_DATA_SIZE);
    return self;
}

static size_t socketBufferFlushExtension(SocketBuffer *self) {
    MemoryPool *extension = self->extension;
    size_t sent;

    if (extension->idx < self->idx) {
        sent = send(self->clientSocket, &self->buffer[extension->i], self->idx - extension->idx, 0);
        if (sent == -1) {
            int err = platformSocketGetLastError();
            switch (err) { /* NOLINT(hicpp-multiway-paths-covered) */
                case SOCKET_WOULD_BLOCK:
#if SOCKET_WOULD_BLOCK != SOCKET_TRY_AGAIN
                case SOCKET_TRY_AGAIN:
#endif
                    return 0;
                default:
                    socketBufferKill(self);
                    return 0;
            }
        } else {
            extension->idx += sent;
        }
        return sent;
    } else if (extension->i < extension->length) {
        sent = send(self->clientSocket, &extension->data[extension->i], extension->length - extension->i, 0);
        if (sent == -1) {
            int err = platformSocketGetLastError();
            switch (err) { /* NOLINT(hicpp-multiway-paths-covered) */
                case SOCKET_WOULD_BLOCK:
#if SOCKET_WOULD_BLOCK != SOCKET_TRY_AGAIN
                case SOCKET_TRY_AGAIN:
#endif
                    return 0;
                default:
                    socketBufferKill(self);
                    return 0;
            }
        } else {
            extension->i += sent;
        }
    } else
        sent = 0;

#pragma region Free extension on the run where it has been cleaned out
    if (extension->i == extension->length) {
        socketBufferMemoryPoolFree(self->extension), self->extension = NULL;
        self->options &= ~(SOC_BUF_ERR_FULL);
        self->idx = 0;
    }
#pragma endregion

    return sent;
}

size_t socketBufferFlush(SocketBuffer *self) {
    if (self->idx) {
        SOCK_BUF_TYPE i, sent;

        if (self->options & SOC_BUF_ERR_FULL && self->extension)
            return socketBufferFlushExtension(self);

        i = 0;
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

                    if (i && !(self->options & SOC_BUF_OPT_EXTEND))
                        memmove(self->buffer, &self->buffer[i], self->idx - i), self->idx -= i;

                    return i;
                default:
                    socketBufferKill(self);
                    return 0;
            }
        } else if (i < self->idx)
            i += sent;

        if (i == self->idx) {
            LINEDBG;
            if ((!(self->options & SOC_BUF_OPT_EXTEND)) || (self->options & SOC_BUF_ERR_FULL)) {
                LINEDBG;
                self->idx = 0;
            }
            return i;
        }

        /* TODO: Clean this mess up */
        goto socketBufferFlushTryAgain;
    }

    return 0;
}

void socketBufferKill(SocketBuffer *self) {
    if (self->extension)
        socketBufferMemoryPoolFree(self->extension);

    self->extension = NULL, self->idx = 0, self->options = SOC_BUF_ERR_FAIL;
}

size_t socketBufferWriteData(SocketBuffer *self, const char *data, size_t len) {
    size_t i;

    for (i = 0; i < len; ++i) {
        if (self->idx == SB_DATA_SIZE) {
            size_t sent = socketBufferFlush(self);

            if (self->options & SOC_BUF_ERR_FAIL)
                return 0;
            else if (self->options & SOC_BUF_ERR_FULL) {
                if (self->options & SOC_BUF_OPT_EXTEND) {
#pragma region Write directory onto the heap when set to extended mode and buffer is full
                    size_t start = SB_DATA_SIZE - (SB_DATA_SIZE - i);
                    self->extension = socketBufferMemoryPoolAppend(self->extension, data, len - start);
                    return len;
#pragma endregion
                } else {
                    return sent;
                }
            }
        }

        self->buffer[self->idx] = *data;
        ++self->idx, ++data;
    }

    return i;
}

size_t socketBufferWriteText(SocketBuffer *self, const char *data) {
    return socketBufferWriteData(self, data, strlen(data));
}

MemoryPool *socketBufferMemoryPoolNew(const char *data, size_t bytes) {
    MemoryPool *self = malloc(sizeof(MemoryPool));

    if (bytes)
        self->data = malloc(bytes), memcpy(self->data, data, bytes);

    self->length = bytes, self->idx = self->i = 0;
    return self;
}

MemoryPool *socketBufferMemoryPoolAppend(MemoryPool *self, const char *data, size_t bytes) {
    if (self && self->length) {
        char *new = realloc(self->data, self->length + bytes);

        if (new)
            self->data = new, memcpy(&self->data[self->length], data, bytes), self->length += bytes;

    } else {
        if (self) {
            if (self->data)
                free(self->data);
            free(self);
        }

        return socketBufferMemoryPoolNew(data, bytes);
    }

    return self;
}

void socketBufferMemoryPoolFree(MemoryPool *self) {
    if (self->data)
        free(self->data);

    free(self);
}
