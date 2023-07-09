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
        if (send(self->clientSocket, self->buffer, self->idx, 0) == -1)
            return 1;
    }

    self->idx = 0;
    return 0;
}

size_t socketBufferWriteText(SocketBuffer *self, const char *data) {
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

size_t socketBufferWriteData(SocketBuffer *self, const char *data, size_t len) {
    size_t i = 0;

    while (i < len) {

        if (self->idx == BUFSIZ) {
            if (send(self->clientSocket, self->buffer, self->idx, 0) == -1)
                return 1;
            self->idx = 0;
        }

        self->buffer[self->idx] = *data;
        ++self->idx;
        ++data;
        ++i;

    }
    return 0;
}
