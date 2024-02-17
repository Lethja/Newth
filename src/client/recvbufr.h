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
 * Remove all memory allocations. Usually used to clean up after a catastrophic failure with the underlying socket
 * @param self The buffer to free all memory allocations from
 * @remark This function is not suitable if the buffer is going to be reused.
 * If in doubt use recvBufferClear() instead
 */
void recvBufferFailFree(RecvBuffer *self);

/**
 * Find a specific character
 * @param self In: The buffer to find data in
 * @param pos In: The position from the start of the temporary buffer to look
 * @param data In: The data to match
 * @param len In: The length of the data to match
 * @return The position relative to the start of the buffer or -1 when no match is found
 */
PlatformFileOffset recvBufferFind(RecvBuffer *self, PlatformFileOffset pos, const char *data, size_t len);

/**
 * Disregard a certain amount of the start of the current buffer
 * @param self The buffer to disregard the start of
 * @param len The amount of data from the start to disregard
 */
void recvBufferDitch(RecvBuffer *self, PlatformFileOffset len);

RecvBuffer recvBufferNew(SOCKET serverSocket, char options);

/**
 * Search for a byte match to data in the sockets data consuming everything in the stream before the match
 * @param self In: The buffer to search for the token in
 * @param token In: The data to match
 * @param len In: The length of the data to match
 * @return NULL on match, user friendly error message otherwise
 * @remark The buffer will be consumed up to the first matched instance,
 * if no match is found the entire buffer will be consumed! Use this function with caution.
 */
const char *recvBufferSearchFor(RecvBuffer *self, const char *token, size_t len);

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
const char *recvBufferSearchTo(RecvBuffer *self, const char *token, size_t len, size_t max);

#endif /* NEW_DL_RECEIVE_BUFFER_H */
