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

    free(src);
    if (dst)
        free(dst);

    if (!srcSite || !dstSite) {
        if (srcSite)
            free(srcSite);
        if (dstSite)
            free(dstSite);
        return ErrAddressNotUnderstood;
    }

    self->destinationSite = dstSite, self->sourceSite = srcSite;
    details = uriDetailsNewFrom(siteWorkingDirectoryGet(srcSite));
    self->sourcePath = details.path, details.path = NULL, uriDetailsFree(&details);
    details = uriDetailsNewFrom(siteWorkingDirectoryGet(dstSite));
    self->destinationPath = details.path, details.path = NULL, uriDetailsFree(&details);
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

void queueEntryArrayFree(QueueEntryArray **queueEntryArray) {
    QueueEntryArray *a;
    if (!queueEntryArray || !*queueEntryArray)
        return;

    a = *queueEntryArray;
    if (a->entry)
        free(a->entry);

    free(a), *queueEntryArray = NULL;
}
