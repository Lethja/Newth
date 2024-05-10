#ifndef NEW_DL_QUEUE_H
#define NEW_DL_QUEUE_H

#include "io.h"

typedef struct PathQueue {
    SocketAddress address;
    unsigned int pathsLen;
    char **paths;
    SOCKET ctrlSock, dataSock;
} PathQueue;

typedef struct AddressQueue {
    unsigned int queueLen;
    PathQueue **queue;
} AddressQueue;

extern const char *pathQueueAppendOrCreate(SocketAddress *address, const char *path);

extern void *pathQueueFind(SocketAddress *address, const char *path, size_t *addressIdx, size_t *pathIdx);

extern char *pathQueueRemove(SocketAddress *address, const char *path);

extern void addressQueueClear(void);

extern PathQueue **addressQueueGet(void);

extern unsigned int addressQueueGetNth(void);

extern unsigned int pathQueueGetNth(SocketAddress *address);

extern unsigned int addressQueueGetTotalPathRequests(void);

extern char *pathQueueConnect(PathQueue *self);

#endif /* NEW_DL_QUEUE_H */
