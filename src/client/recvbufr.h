#ifndef NEW_DL_RECEIVE_BUFFER_H
#define NEW_DL_RECEIVE_BUFFER_H

#include "../platform/platform.h"
#include "site.h"

typedef struct RecvBuffer {
    PlatformFileOffset escape, remain;
    FILE *buffer;
    SOCKET serverSocket;
    char options;
} RecvBuffer;

/**
 * Read data out of the socket and append to the buffer
 * @param self The buffer to start or append more network data into the buffer of
 * @param len The length of the network data to fetch
 * @return
 */
char *recvBufferAppend(RecvBuffer *self, size_t len);

/**
 * Remove the buffer from memory if there is one
 * @param self The buffer to clear of memory allocations
 */
void recvBufferClear(RecvBuffer *self);

/**
 * Disregard a certain amount of the start of the current buffer
 * @param self The buffer to disregard the start of
 * @param len The amount of data from the start to disregard
 */
void recvBufferDitch(RecvBuffer *self, PlatformFileOffset len);

/**
 * Remove all memory allocations. Usually used to clean up after a catastrophic failure with the underlying socket
 * @param self The buffer to free all memory allocations from
 * @remark This function is not suitable if the buffer is going to be reused.
 * If in doubt use recvBufferClear() instead
 */
void recvBufferFailFree(RecvBuffer *self);

/**
 * Fetch whatever is in the buffer into a string array
 * @param self The buffer to fetch out of
 * @param buf The character buffer to fetch into
 * @param pos The position in the buffer to fetch from
 * @param len The length of the buffer to write into including the null terminator
 * @return NULL on success, user friendly error message otherwise
 */
char *recvBufferFetch(RecvBuffer *self, char *buf, PlatformFileOffset pos, size_t len);

/**
 * Find a specific character
 * @param self In: The buffer to find data in
 * @param pos In: The position from the start of the temporary buffer to look
 * @param token In: The data to match
 * @param len In: The length of the data to match
 * @return The position relative to the start of the buffer or -1 when no match is found
 */
PlatformFileOffset recvBufferFind(RecvBuffer *self, PlatformFileOffset pos, const char *token, size_t len);

/**
 * Search for a byte match to data in the sockets data consuming everything in the stream before the match
 * @param self In: The buffer to search for the token in
 * @param token In: The data to match
 * @param len In: The length of the data to match
 * @return NULL on match, user friendly error message otherwise
 * @remark The buffer will be consumed up to the first matched instance,
 * if no match is found the entire buffer will be consumed! Use this function with caution.
 */
const char *recvBufferFindAndDitch(RecvBuffer *self, const char *token, size_t len);

/**
 * Search for a byte match to data in the sockets data appending everything in the stream before the match
 * @param self In: The buffer to search for the token in
 * @param token In: The data to match
 * @param len In: The length of the data to match
 * @param max In: Roughly the maximum length the buffer can grow to before giving up and returning
 * @return NULL on match, user friendly error message otherwise
 * @remark The buffer will be appended to to the first matched instance,
 * if no match is found or the buffer reaches the maximum size allowed
 * then the entire buffer up to that point will stored in memory for further parsing.
 */
const char *recvBufferFindAndFetch(RecvBuffer *self, const char *token, size_t len, size_t max);

/**
 * Create a socket buffer to receive data on
 * @param serverSocket In: The socket this buffer should manage
 * @param options In: The options to set on the new socketBuffer
 * @return The new socket buffer (stack allocated)
 * @note One socket buffer per socket
 */
RecvBuffer recvBufferNew(SOCKET serverSocket, char options);

#endif /* NEW_DL_RECEIVE_BUFFER_H */
