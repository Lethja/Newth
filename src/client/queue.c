#include "err.h"
#include "queue.h"
#include "uri.h"

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

const char *queueEntryNewFromPath(QueueEntry *self, SiteArray *array, const char *source, const char *destination) {
    char *src, *dst;
    Site *srcSite, *dstSite;
    UriDetails details;

    if (!(src = siteArrayUserPathResolve(array, source, 0)))
        return ErrAddressNotUnderstood;

    dst = destination ? siteArrayUserPathResolve(array, destination, 1) : NULL;
    srcSite = siteArrayGetByUriHost(array, src), dstSite = dst ? siteArrayGetByUriHost(array, dst) : siteArrayActiveGetWrite(array);

    self->destinationSite = dstSite, self->sourceSite = srcSite;
    details = uriDetailsNewFrom(src), self->sourcePath = details.path, details.path = NULL, uriDetailsFree(&details);
    details = uriDetailsNewFrom(dst ? dst : siteWorkingDirectoryGet(dstSite));
    self->destinationPath = details.path, details.path = NULL, uriDetailsFree(&details);

    if (dst)
        free(dst);
    free(src);

    self->state = QUEUE_STATE_QUEUED;

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

    if (platformHeapResize((void **) &array->entry, sizeof(QueueEntry), array->len + 1))
        return strerror(errno);

    memcpy(&array->entry[array->len], entry, sizeof(QueueEntry));
    ++array->len;

    return NULL;
}

void queueEntryFree(QueueEntry *queueEntry) {
    if (queueEntry->destinationPath)
        free(queueEntry->destinationPath);
    if (queueEntry->sourcePath)
        free(queueEntry->sourcePath);
}

void queueEntryArrayFree(QueueEntryArray **queueEntryArray) {
    QueueEntryArray *a;
    if (!queueEntryArray || !*queueEntryArray)
        return;

    a = *queueEntryArray;
    if (a->entry) {
        size_t i;
        for (i = 0; i < a->len; ++i)
            queueEntryFree(&a->entry[i]);

        free(a->entry);
    }

    free(a), *queueEntryArray = NULL;
}
