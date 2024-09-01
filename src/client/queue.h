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

/**
 * Create or append QueueEntry to an QueueEntryArray
 * @param queueEntryArray In: A pointer to the array to append to or NULL to create a new one
 * @param entry In: The QueueEntry to append
 * @return NULL on success, user friendly error message otherwise
 */
const char *queueEntryArrayAppend(QueueEntryArray **queueEntryArray, QueueEntry *entry);

/**
 * Free the entry array and its elements
 * @param queueEntryArray In-Out: A double pointer to an entry array to free from memory, the value will be set to NULL
 */
void queueEntryArrayFree(QueueEntryArray **queueEntryArray);

/**
 * Fill a QueueEntry given user input paths and a SiteArray
 * @param self Out: The QueueEntry to fill
 * @param array In: The SiteArray the QueueEntry will be a member of
 * @param source In: The user provided copy from path
 * @param destination In: The user provided copy to path
 * @return NULL on success, user friendly error message otherwise
 */
const char *queueEntryNewFromPath(QueueEntry *self, SiteArray *array, const char *source, const char *destination);

/**
 * Free internal of a queueEntry
 * @param queueEntry The QueueEntry to free the internals of
 */
void queueEntryFree(QueueEntry *queueEntry);

#endif /* NEW_DL_QUEUE_H */
