#ifndef NEW_DL_QUEUE_H
#define NEW_DL_QUEUE_H

#include "io.h"
#include "site.h"

/**
 * The state of a QueueEntry
 */
typedef enum QueueState {
    /**
     * This entry has not been attempted yet
     */
    QUEUE_STATE_QUEUED,
    /**
     * This entry has been skipped for some reason
     */
    QUEUE_STATE_SKIPPED,
    /**
     * This entry has been successfully downloaded
     */
    QUEUE_STATE_FINISHED,
    /**
     * This entry has failed to download
     */
    QUEUE_STATE_FAILED,
    /**
     * This entry should recursively act on subdirectories
     */
    QUEUE_TYPE_RECURSIVE
} QueueState;

/**
 * An individual entry in the queue.
 * Contains enough information to recreate the full URI
 * for both source and destination as well as state flags for this entry.
 */
typedef struct QueueEntry {
    Site *sourceSite, *destinationSite;
    char *sourcePath, *destinationPath, state;
} QueueEntry;

/**
 * An array of queue entries to be used with the QueueEntryArray set of functions
 */
typedef struct QueueEntryArray {
    size_t len;
    QueueEntry *entry;
} QueueEntryArray;

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

/**
 * Create or append QueueEntry to an QueueEntryArray
 * @param queueEntryArray In: A pointer to the array to append to or NULL to create a new one
 * @param entry In: The QueueEntry to append
 * @return NULL on success, user friendly error message otherwise
 */
const char *queueEntryArrayAppend(QueueEntryArray **queueEntryArray, QueueEntry *entry);

void queueEntryArrayFree(QueueEntryArray *queueEntryArray);

#endif /* NEW_DL_QUEUE_H */
