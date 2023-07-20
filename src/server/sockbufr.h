#ifndef OPEN_WEB_SOCKET_BUFFER_H
#define OPEN_WEB_SOCKET_BUFFER_H

#include "../platform/platform.h"

#include <stdio.h>

enum SOC_BUF_OPT {
    SOC_BUF_OPT_EXTEND = 1, SOC_BUF_OPT_HTTP_CHUNK = 2, SOC_BUF_ERR_FULL = 4, SOC_BUF_ERR_FAIL = 8
};

typedef struct MemoryPool {
    size_t length, i;
    char *data;
} MemoryPool;

typedef struct SocketBuffer {
    SOCKET clientSocket;
    SOCK_BUF_TYPE idx;
    char options, buffer[BUFSIZ];
    MemoryPool *extension;
} SocketBuffer;

/**
 * Flush any data left in the TCP socket buffer out of memory and onto the network
 * @param self In: The socket buffer to flush
 * @return Amount of bytes flushed out of the socket buffer
 * @remark The function will set SOC_BUF_ERR_FULL or SOC_BUF_ERR_FAIL if applicable, caller should check state of these
 */
size_t socketBufferFlush(SocketBuffer *self);

/**
 * Append to a memory pool. This should only be used when a socket would block but processing can not be deferred
 * @param self In: The memory pool to append to
 * @param data In: The data to append onto the end of the memory pool
 * @param bytes In: The length of the data
 * @return The memory pool with appended data
 * @remark The return point might be the same as self but it also might not, always update your pointer
 */
MemoryPool *socketBufferMemoryPoolAppend(MemoryPool *self, const char *data, size_t bytes);

/**
 * Create a new memory pool. This should only be used when a socket would block but processing cannot be deferred
 * @param data In: The data to write to into the memory pool
 * @param bytes In: The length of the data
 * @return A newly created memory pool
 */
MemoryPool *socketBufferMemoryPoolNew(const char *data, size_t bytes);

/**
 * Create a socket buffer for a socket
 * @param clientSocket In: The socket to create in a socketBuffer
 * @param options In: The options to set on the new socketBuffer
 * @return The new socket buffer (stack allocated)
 * @note One socket buffer per socket
 */
SocketBuffer socketBufferNew(SOCKET clientSocket, char options);

/**
 * Write data to the TCP socket buffer
 * @param self In: The TCP socket to send data over
 * @param data In: The data to send into the TCP socket
 * @param len In: The length of the data in bytes
 * @return Amount of bytes written into the socket buffer
 * @note socketBufferFlush should always be called before a SocketBuffer goes out of scope or is freed from the heap
 * @remark The function will set SOC_BUF_ERR_FULL or SOC_BUF_ERR_FAIL if applicable, caller should check state of these
 */
size_t socketBufferWriteData(SocketBuffer *self, const char *data, size_t len);

/**
 * Write text to the TCP socket buffer
 * @param self In: The TCP socket to send data over
 * @param data In: A null-terminated string array to send into the TCP socket
 * @return Amount of bytes written into the socket buffer
 * @note socketBufferFlush should always be called before a SocketBuffer goes out of scope or is freed from the heap
 * @remark The function will set SOC_BUF_ERR_FULL or SOC_BUF_ERR_FAIL if applicable, caller should check state of these
 */
size_t socketBufferWriteText(SocketBuffer *self, const char *data);

#endif /* OPEN_WEB_SOCKET_BUFFER_H */
