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
	QUEUE_STATE_QUEUED = 1 << 0,
	/**
	 * This entry has been skipped for some reason
	 */
	QUEUE_STATE_SKIPPED = 1 << 1,
	/**
	 * This entry has been successfully downloaded
	 */
	QUEUE_STATE_FINISHED = 1 << 2,
	/**
	 * This entry has failed to download
	 */
	QUEUE_STATE_FAILED = 1 << 3,
	/**
	 * This entry should recursively act on subdirectories
	 */
	QUEUE_TYPE_RECURSIVE = 1 << 4
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
	unsigned long len;
	QueueEntry *entry;
} QueueEntryArray;

#pragma region Callbacks

extern void (*queueCallbackStateChange)(QueueEntry *);

extern void (*queueCallbackProgress)(QueueEntry *, PlatformFileOffset);

extern void (*queueCallBackTotalSize)(QueueEntry *, PlatformFileOffset);

#pragma endregion

/**
 * Create or append QueueEntry to an QueueEntryArray
 * @param queueEntryArray In: A pointer to the array to append to or NULL to create a new one
 * @param entry In: The QueueEntry to append
 * @return NULL on success, user friendly error message otherwise
 */
const char *queueEntryArrayAppend(QueueEntryArray **queueEntryArray, QueueEntry *entry);

/**
 * Search for an exact entry index inside an array
 * @param array The array to search in
 * @param entry The entry to find in the array
 * @return The position in the array that the entry is or -1 when not found
 */
unsigned long queueEntryArrayFindNth(QueueEntryArray *array, QueueEntry *entry);

/**
 * Free the entry array and its elements
 * @param queueEntryArray In-Out: A double pointer to an entry array to free from memory, the value will be set to NULL
 */
void queueEntryArrayFree(QueueEntryArray **queueEntryArray);

/**
 * Free the entry array without freeing the contained elements.
 * @param queueEntryArray In-Out: A double pointer to an entry array to free from memory, the value will be set to NULL
 * @remark This will cause memory leaks if you don't know what you're doing. If in doubt try queueEntryArrayFree() first.
 */
void queueEntryArrayFreeArrayOnly(QueueEntryArray **queueEntryArray);

/**
 * Remove a entry by element
 * @param queueEntryArray The array to remove the element from
 * @param entry The entry to remove
 * @return NULL on success, user friendly error message otherwise
 */
const char *queueEntryArrayRemove(QueueEntryArray **queueEntryArray, QueueEntry *entry);

/**
 * Find a queue index or part of a source/destination uri
 * @param array The QueueEntryArray to search in
 * @param found The QueueEntryArray to store finds in, the source pointer should be set to null on the first query
 * @param search The string or string number to search for
 * @return NULL on success, user friendly error message otherwise
 * @remark Even when an error occurs, found might be allocated to the heap in a incomplete state
 */
const char *queueEntryArraySearch(QueueEntryArray *array, QueueEntryArray **found, const char *search);

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

/**
 * Download the entry in no clobber mode (existing files won't be overwritten)
 * @param entry The entry to do the download on
 * @return NULL on success, user friendly error message otherwise
 */
const char *queueEntryDownloadNoClobber(QueueEntry *entry);

/**
 * Download the entry in mirror mode (all existing files are overwritten)
 * @param entry The entry to do the download on
 * @return NULL on success, user friendly error message otherwise
 */
const char *queueEntryDownloadMirror(QueueEntry *entry);

/**
 * Download the entry in update mode (only newer files will be overwritten)
 * @param entry The entry to do the download on
 * @return NULL on success, user friendly error message otherwise
 */
const char *queueEntryDownloadUpdate(QueueEntry *entry);

/**
 * Get the full URI of a site and path members of a QueueEntry
 * @param site The site to get the beginning of the uri from
 * @param path The path to get the end of the uri from
 * @return A string of the URI on success otherwise NULL
 */
char *queueEntryGetUri(Site *site, char *path);

#endif /* NEW_DL_QUEUE_H */
