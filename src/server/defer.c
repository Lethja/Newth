#include "defer.h"

static SOCKET *sockets = NULL;
static SocketBuffer *socketBuffers = NULL;
static ExtendedBuffer **extendedBuffers = NULL;
static size_t len = 0;

static void allocate(size_t numberElements) {
    switch (numberElements) {
        default: {
            SocketBuffer *newSocketBuffers = realloc(socketBuffers, numberElements * sizeof(SocketBuffer));
            SOCKET *newSockets = realloc(sockets, numberElements * sizeof(SOCKET));
            ExtendedBuffer **newExtendedBuffers = realloc(socketBuffers, numberElements * sizeof(ExtendedBuffer *));

            if (newSockets && newSocketBuffers && newExtendedBuffers)
                sockets = newSockets, socketBuffers = newSocketBuffers, extendedBuffers = newExtendedBuffers;
            else
                LINEDBG;
        }
            break;
        case 0:
            if (sockets)
                free(sockets), sockets = NULL;
            if (socketBuffers)
                free(socketBuffers), socketBuffers = NULL;
            if (extendedBuffers) {
                size_t i;
                for (i = 0; i < len; ++i) {
                    if (extendedBuffers[i])
                        free(extendedBuffers[i]);
                }

                free(extendedBuffers);
            }
            break;
        case 1:
            if (!sockets && !socketBuffers && !extendedBuffers) {
                sockets = malloc(sizeof(SOCKET)), socketBuffers = malloc(
                        sizeof(SocketBuffer)), extendedBuffers = malloc(sizeof(char *));
            }
            break;
    }
}

static size_t deferFindIdx(SOCKET socket) {
    size_t i;
    for (i = 0; i < len; ++i) {
        if (socket == sockets[i])
            return i;
    }
    return -1;
}

SocketBuffer *deferFind(SOCKET socket, ExtendedBuffer **extendedBuffer) {
    size_t i = deferFindIdx(socket);
    if (i == -1)
        return NULL;

    *extendedBuffer = extendedBuffers[i];
    return &socketBuffers[i];
}

void deferAdd(SocketBuffer *socketBuffer, ExtendedBuffer *extendedBuffer) {
    if (deferFind(socketBuffer->clientSocket, NULL))
        return;

    ++len;
    allocate(len);

    memcpy(&socketBuffer[len - 1], socketBuffer, sizeof(SocketBuffer));
    sockets[len - 1] = socketBuffer->clientSocket, extendedBuffers[len - 1] = extendedBuffer;
}

void deferRemove(SOCKET socket) {
    if (len > 1) {
        size_t i = deferFindIdx(socket);
        if (i == -1)
            return;

        if (i + 1 != len)
            memmove(socketBuffers, &socketBuffers[i + 1], len - i - 1);
    }

    --len;
    allocate(len);
}

ExtendedBuffer *extendedBufferNew(char *data, size_t bytes) {
    ExtendedBuffer *self = malloc(sizeof(ExtendedBuffer));
    if (bytes)
        self->data = malloc(bytes), memcpy(self->data, data, bytes);

    self->length = bytes, self->i = 0;
    return self;
}

ExtendedBuffer *extendedBufferAppend(ExtendedBuffer *self, char *data, size_t bytes) {
    if (self && self->length) {
        char *new = realloc(self->data, self->length + bytes);
        if (new)
            self->data = new, memcpy(&self->data[self->length], data, bytes), self->length += bytes;
    } else {
        if (self)
            free(self);

        return extendedBufferNew(data, bytes);
    }

    return self;
}
