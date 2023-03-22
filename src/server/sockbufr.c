#include <string.h>
#include <unistd.h>
#include "sockbufr.h"

SocketBuffer socketBufferNew(SOCKET clientSocket) {
    SocketBuffer self;
    self.clientSocket = clientSocket, self.idx = 0;
    memset(self.buffer, 0, BUFSIZ);
    return self;
}

size_t socketBufferFlush(SocketBuffer *self) {
    if (self->idx) {
        if (send(self->clientSocket, self->buffer, self->idx, 0) == -1)
            return 1;
    }

    self->idx = 0;
    return 0;
}

size_t socketBufferWrite(SocketBuffer *self, const char *data) {
    while (*data != '\0') {
        if (self->idx == BUFSIZ) {
            if (send(self->clientSocket, self->buffer, self->idx, 0) == -1)
                return 1;
            self->idx = 0;
        }

        self->buffer[self->idx] = *data;
        ++self->idx;
        ++data;
    }
    return 0;
}
