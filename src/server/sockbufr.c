#include <string.h>
#include "sockbufr.h"

SocketBuffer socketBufferNew(SOCKET clientSocket, char options) {
    SocketBuffer self;
    self.clientSocket = clientSocket, self.idx = self.ext = 0, self.options = options;
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
            switch (err) {
                case 0:
                case SOCKET_WOULD_BLOCK:
#if SOCKET_WOULD_BLOCK != SOCKET_TRY_AGAIN
                case SOCKET_TRY_AGAIN:
#endif
                    /* TODO: defer job here */
                    goto socketBufferFlushTryAgain;
                default:
                    return 1;
            }
        } else if (i < self->idx)
            i += sent;

        if (i == self->idx) {
            self->idx = 0;
            return 0;
        }

        /* TODO: Clean this mess up */
        goto socketBufferFlushTryAgain;
    }

    return 0;
}

size_t socketBufferWriteText(SocketBuffer *self, const char *data) {
    while (*data != '\0') {
        if (self->idx == BUFSIZ)
            if (socketBufferFlush(self) == 1)
                return 1;

        self->buffer[self->idx] = *data;
        ++self->idx, ++data;
    }

    return 0;
}

size_t socketBufferWriteData(SocketBuffer *self, const char *data, size_t len) {
    size_t i;

    for (i = 0; i < len; ++i) {
        if (self->idx == BUFSIZ)
            if (socketBufferFlush(self) == 1)
                return 1;

        self->buffer[self->idx] = *data;
        ++self->idx, ++data;
    }

    return 0;
}
