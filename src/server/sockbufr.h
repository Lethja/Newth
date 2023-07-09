#ifndef OPEN_WEB_SOCKET_BUFFER_H
#define OPEN_WEB_SOCKET_BUFFER_H

#include "../platform/platform.h"

#include <stdio.h>

enum SOC_BUF_OPT {
    SOC_BUF_OPT_EXTEND = 1, SOC_BUF_OPT_HTTP_CHUNK = 2
};

typedef struct SocketBuffer {
    SOCKET clientSocket;
    SOCK_BUF_TYPE idx, ext;
    char options, buffer[BUFSIZ], *extended;
} SocketBuffer;

/**
 * Create a socket buffer for a socket
 * @param clientSocket In: The socket to create in a socketBuffer
 * @param options In: The options to set on the new socketBuffer
 * @return The new socket buffer (stack allocated)
 * @note One socket buffer per socket
 */
SocketBuffer socketBufferNew(SOCKET clientSocket, char options);

/**
 * Flush any data left in the TCP socket buffer out of memory and onto the network
 * @param self In: The socket buffer to flush
 * @return 0 on success, else error
 */
size_t socketBufferFlush(SocketBuffer *self);

/**
 * Write text to the TCP socket buffer
 * @param self In: The TCP socket to send data over
 * @param data In: A null-terminated string array to send into the TCP socket
 * @return 0 on success, else error
 * @note socketBufferFlush should always be called before a SocketBuffer goes out of scope or is freed from the heap
 */
size_t socketBufferWriteText(SocketBuffer *self, const char *data);

/**
 * Write data to the TCP socket buffer
 * @param self In: The TCP socket to send data over
 * @param data In: The data to send into the TCP socket
 * @param len In: The length of the data in bytes
 * @return 0 on success, else error
 * @note socketBufferFlush should always be called before a SocketBuffer goes out of scope or is freed from the heap
 */
size_t socketBufferWriteData(SocketBuffer *self, const char *data, size_t len);

#endif /* OPEN_WEB_SOCKET_BUFFER_H */
