#ifndef NEW_TH_SEND_BUFFER_H
#define NEW_TH_SEND_BUFFER_H

#include "../platform/platform.h"

#include <stdio.h>

enum SOC_BUF_OPT {
    SOC_BUF_OPT_EXTEND = 1, SOC_BUF_ERR_CLEAR = 2, SOC_BUF_ERR_FULL = 4, SOC_BUF_ERR_FAIL = 8
};

typedef struct SendBuffer {
    long idx;
    FILE *buffer;
    SOCKET clientSocket;
    char options;
} SendBuffer;

/**
 * Flush any data left in the TCP socket buffer out of memory and onto the network
 * @param self In: The socket buffer to flush
 * @return Amount of bytes flushed out of the socket buffer
 * @remark The function will set SOC_BUF_ERR_FULL or SOC_BUF_ERR_FAIL if applicable, caller should check state of these
 */
size_t sendBufferFlush(SendBuffer *self);

/**
 * Free any and all memory allocations held by this socket buffer in the case on an unrecoverable connection error
 * @param self In: the socket buffer to free resources from
 */
void sendBufferFailFree(SendBuffer *self);

/**
 * Create a socket buffer for a socket
 * @param clientSocket In: The socket to create in a socketBuffer
 * @param options In: The options to set on the new socketBuffer
 * @return The new socket buffer (stack allocated)
 * @note One socket buffer per socket
 */
SendBuffer sendBufferNew(SOCKET clientSocket, char options);

/**
 * Write data to the TCP socket buffer
 * @param self In: The TCP socket to send data over
 * @param data In: The data to send into the TCP socket
 * @param len In: The length of the data in bytes
 * @return Amount of bytes written into the socket buffer
 * @note sendBufferFlush should always be called before a SocketBuffer goes out of scope or is freed from the heap
 * @remark The function will set SOC_BUF_ERR_FULL or SOC_BUF_ERR_FAIL if applicable, caller should check state of these
 */
size_t sendBufferWriteData(SendBuffer *self, const char *data, size_t len);

/**
 * Write text to the TCP socket buffer
 * @param self In: The TCP socket to send data over
 * @param data In: A null-terminated string array to send into the TCP socket
 * @return Amount of bytes written into the socket buffer
 * @note sendBufferFlush should always be called before a SocketBuffer goes out of scope or is freed from the heap
 * @remark The function will set SOC_BUF_ERR_FULL or SOC_BUF_ERR_FAIL if applicable, caller should check state of these
 */
size_t sendBufferWriteText(SendBuffer *self, const char *data);

/**
 * Create a new buffer or set an existing one ready for appending
 * @param self The socket buffer to get the create or get the buffer of
 * @return A file pointer ready for fprintf() or fwrite() on success, NULL on failure
 */
FILE *sendBufferGetBuffer(SendBuffer *self);

/**
 * Print a message into a socket buffer
 * @param self The socket buffer to get the create or get the buffer of
 * @param max The maximum length of the message
 * @param string A printf-like string
 * @return The number of characters printed or a negative number on error
 */
int sendBufferPrintf(SendBuffer *self, size_t max, const char *format, ...);

#endif /* NEW_TH_SEND_BUFFER_H */
