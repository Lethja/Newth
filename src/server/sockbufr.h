#ifndef OPEN_WEB_SOCKET_BUFFER_H
#define OPEN_WEB_SOCKET_BUFFER_H

#include <stdio.h>

typedef struct SocketBuffer {
    int clientSocket;
    size_t idx;
    char buffer[BUFSIZ];
} SocketBuffer;

SocketBuffer socketBufferNew(int clientSocket);

size_t socketBufferFlush(SocketBuffer *self);

size_t socketBufferWrite(SocketBuffer *self, const char *data);

#endif /* OPEN_WEB_SOCKET_BUFFER_H */
