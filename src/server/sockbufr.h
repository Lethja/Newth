#ifndef OPEN_WEB_SOCKET_BUFFER_H
#define OPEN_WEB_SOCKET_BUFFER_H

#include <stdio.h>

typedef struct SocketBuffer {
    int clientSocket;
    size_t idx;
    char buffer[BUFSIZ];
} SocketBuffer;

/**
 * Create a socket buffer for a socket
 * @param clientSocket The socket to create in a socketBuffer
 * @return The new socket buffer (stack allocated)
 * @note One socket buffer per socket please
 * @note if you've called socketBufferWrite() be sure to call socketBufferFlush() before leaving scope of declaration
 */
SocketBuffer socketBufferNew(int clientSocket);

/**
 * Flush any data left in the TCP socket buffer out of memory and onto the network
 * @param self The socket buffer to flush
 * @return 0 on success, else error
 */
size_t socketBufferFlush(SocketBuffer *self);

/**
 * Write to the TCP socket buffer
 * @param self The TCP socket to send data over
 * @param data The data to send into the TCP socket
 * @return 0 on success, else error
 * @note socketBufferFlush should always be called before a SocketBuffer goes out of scope or is freed from the heap
 */
size_t socketBufferWrite(SocketBuffer *self, const char *data);

#endif /* OPEN_WEB_SOCKET_BUFFER_H */
