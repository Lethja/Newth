#ifndef NEW_DL_QUEUE_H
#define NEW_DL_QUEUE_H

#include "io.h"

typedef struct PathQueue {
    SocketAddress address;
    size_t pathsLen;
    char **paths;
} PathQueue;

typedef struct AddressQueue {
    size_t queueLen;
    PathQueue **queue;
} AddressQueue;

extern char *downloadQueueAppendOrCreate(SocketAddress *address, const char *path);

extern void *downloadQueueFind(SocketAddress *address, const char *path, size_t *addressIdx, size_t *pathIdx);

extern char *downloadQueueRemove(SocketAddress *address, const char *path);

extern void downloadQueueClear(void);

extern size_t downloadQueueNumberOfAddresses(void);

extern size_t downloadQueueAddressNumberOfRequests(SocketAddress *address);

#endif /* NEW_DL_QUEUE_H */
