#ifndef NEW_TH_DEFER_H
#define NEW_TH_DEFER_H

#include "../platform/platform.h"
#include "sockbufr.h"

typedef struct ExtendedBuffer {
    size_t length, i;
    char *data;
} ExtendedBuffer;

/**
 * Add a deferred socket writing job.
 * @param socketBuffer [In] The socket buffer to defer
 * @param extendedBuffer [In, Optional] The already made extended buffer related to the socket buffer or NULL
 */
void deferAdd(SocketBuffer *socketBuffer, ExtendedBuffer *extendedBuffer);

/**
 * Remove a deferred job
 * @param socket [In] The socket to remove from the deferred socket writing job
 */
void deferRemove(SOCKET socket);

/**
 * Find a deferred job that has already been added
 * @param socket [In] The socket to find from the deferred socket writing job
 * @param extendedBuffer [Out, Optinonal] The extended buffer applicable to this deferred socket writing job or NULL
 * @return A pointer to the socket buffer structure if found or NULL otherwise
 */
SocketBuffer *deferFind(SOCKET socket, ExtendedBuffer **extendedBuffer);

/**
 * Create a new extended buffer. This should only be used when a socket would block
 * but the socket buffers processing cannot be stopped
 * @param data [In] The data to write to into the extended buffer
 * @param bytes [In] The length of the data
 * @return A newly created extended buffer
 */
ExtendedBuffer *extendedBufferNew(char *data, size_t bytes);

/**
 * Append to a extended buffer. This should only be used when a socket would block
 * but the socket buffers processing cannot be stopped
 * @param self [In] The extended buffer to append to
 * @param data [In] The data to append onto the end of the extended buffer
 * @param bytes [In] The length of the data
 * @return The extended buffer with appended data
 * @remark The return point might be the same as self but it also might not, always update your pointer
 */
ExtendedBuffer *extendedBufferAppend(ExtendedBuffer *self, char *data, size_t bytes);

#endif /* NEW_TH_DEFER_H */
