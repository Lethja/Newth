#ifndef NEW_DL_RECEIVE_BUFFER_H
#define NEW_DL_RECEIVE_BUFFER_H

#include "../platform/platform.h"
#include "site.h"

enum RecvBufferOptions {
    RECV_BUFFER_DATA_LENGTH_CHUNK, RECV_BUFFER_DATA_LENGTH_KNOWN, RECV_BUFFER_DATA_LENGTH_TOKEN
};

typedef struct RecvBufferLengthChunk {
    PlatformFileOffset escape, next;
} RecvBufferLengthChunk;

typedef struct RecvBufferLengthKnown {
    PlatformFileOffset escape, total;
} RecvBufferLengthKnown;

typedef struct RecvBufferLengthUnknown {
    size_t escape, limit;
} RecvBufferLengthUnknown;

typedef struct RecvBufferLengthToken {
    const char *token;
    size_t length;
} RecvBufferLengthToken;

typedef union RecvBufferLength {
    RecvBufferLengthChunk chunk;
    RecvBufferLengthKnown known;
    RecvBufferLengthToken token;
    RecvBufferLengthUnknown unknown;
} RecvBufferLength;

typedef struct RecvBuffer {
    FILE *buffer;
    SocketAddress serverAddress;
    SOCKET serverSocket;
    int options;
    union RecvBufferLength length;
} RecvBuffer;

/**
 * Read data out of the socket and append to the buffer
 * @param self The buffer to start or append more network data into the buffer of
 * @param len The length of the network data to fetch
 * @return NULL on success, user friendly error message otherwise
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
 * Update the server socket to another socket or a reconnected socket
 * @param self The buffer to update to socket with
 * @param socket The socket to update the buffer with
 * @remark Aside from the switched out socket the rest of the buffer state remains unchanged.
 * Use recvBufferClear to reset the buffer as needed
 */
void recvBufferUpdateSocket(RecvBuffer *self, const SOCKET *socket);

/**
 * Create a socket buffer to receive data on
 * @param serverSocket In: The socket this buffer should manage
 * @param serverAddress In: The address this buffer should manage
 * @param options In: The options to set on the new socketBuffer
 * @return The new socket buffer (stack allocated)
 * @note One socket buffer per socket and socketAddress
 */
RecvBuffer recvBufferNew(SOCKET serverSocket, SocketAddress serverAddress, int options);

/**
 * Create a socket buffer to receive data on from a socket address.
 * A socket will be connected as part of the creation process
 * @param self In: The buffer to setup
 * @param serverAddress In: The address this buffer should manage
 * @param options In: The options to set on the new socketBuffer
 * @return NULL on success, user friendly error message otherwise
 * @note One socket buffer per socket and socketAddress
 */
char *recvBufferNewFromSocketAddress(RecvBuffer *self, SocketAddress serverAddress, int options);

/**
 * Set this buffer to receive a http data request where length of the data is determined by the HTTP 1.1 chunk standard
 * @param self The buffer to set in chunk length mode
 */
void recvBufferSetLengthChunk(RecvBuffer *self);

/**
 * Set this buffer to receive a request where length of the data has been predetermined in bytes
 * @param self The buffer to set the data length of
 * @param length The length of the expected data buffer
 */
void recvBufferSetLengthKnown(RecvBuffer *self, PlatformFileOffset length);

/**
 * Set this buffer to receive a request where the end of the data is determined by a token
 * @param self The buffer to set to an known data length state
 * @param token The token to represent the end of the data buffer
 * @param length The length of the token string to match
 */
void recvBufferSetLengthToken(RecvBuffer *self, const char *token, size_t length);

/**
 * Set this buffer to receive a request where the length of data is not known
 * @param self The buffer to set to an known data length state
 * @param limit The maximum size the buffer can grow to
 */
void recvBufferSetLengthUnknown(RecvBuffer *self, size_t limit);

#endif /* NEW_DL_RECEIVE_BUFFER_H */
