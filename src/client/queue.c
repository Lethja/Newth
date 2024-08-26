#include "err.h"
#include "queue.h"
#include "../platform/platform.h"

static inline QueueEntry *EntrySearch(QueueEntryArray *array, QueueEntry *entry) {
    size_t i;

    for (i = 0; i < array->len; ++i) {
        QueueEntry *it = &array->entry[i];
        if (it->destinationSite == entry->destinationSite && it->sourceSite == entry->sourceSite
            && strcmp(it->destinationPath, entry->destinationPath) == 0
            && strcmp(it->sourcePath, entry->sourcePath) == 0)
            return it;
    }

    return NULL;
}

const char *queueEntryArrayAppend(QueueEntryArray **queueEntryArray, QueueEntry *entry) {
    QueueEntryArray *array = *queueEntryArray;

    if (!array) {
        if (!(array = malloc(sizeof(QueueEntryArray))))
            return strerror(errno);

        if (!(array->entry = malloc(sizeof(QueueEntry)))) {
            free(array);
            return strerror(errno);
        }

        memcpy(array->entry, entry, sizeof(QueueEntry));
        array->len = 1, *queueEntryArray = array;

        return NULL;
    }

    if (EntrySearch(array, entry))
        return NULL;

    if (platformHeapResize((void **) array->entry, sizeof(QueueEntry), array->len + 1))
        return strerror(errno);

    memcpy(&array->entry[array->len], entry, sizeof(QueueEntry));
    ++array->len;

    return NULL;
}

void queueEntryArrayFree(QueueEntryArray *queueEntryArray) {
    if (!queueEntryArray)
        return;

    if (queueEntryArray->entry)
        free(queueEntryArray->entry);

    free(queueEntryArray);
}
