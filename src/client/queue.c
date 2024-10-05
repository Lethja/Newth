#include <time.h>
#include "err.h"
#include "queue.h"
#include "uri.h"

#pragma region Callbacks

void (*queueCallbackStateChange)(QueueEntry *) = NULL;

void (*queueCallbackProgress)(QueueEntry *, PlatformFileOffset) = NULL;

void (*queueCallBackTotalSize)(QueueEntry *, PlatformFileOffset) = NULL;

#pragma endregion

static inline QueueEntry *EntrySearch(QueueEntryArray *array, QueueEntry *entry) {
    size_t i;

    for (i = 0; i < array->len; ++i) {
        QueueEntry *it = &array->entry[i];
        if (it->destinationSite == entry->destinationSite && it->sourceSite == entry->sourceSite &&
            strcmp(it->destinationPath, entry->destinationPath) == 0 && strcmp(it->sourcePath, entry->sourcePath) == 0)
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
    srcSite = siteArrayGetByUriHost(array, src), dstSite = dst ? siteArrayGetByUriHost(array, dst)
              : siteArrayActiveGetWrite(array);

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

char *queueEntryGetUri(Site *site, char *path) {
    char *uri;
    UriDetails d = uriDetailsNewFrom(siteWorkingDirectoryGet(site));
    if (d.path)
        free(d.path);

    d.path = path;
    uri = uriDetailsCreateString(&d), d.path = NULL, uriDetailsFree(&d);

    return uri;
}

const char *queueEntryDownloadNoClobber(QueueEntry *entry) {
    const char *e, *fromPath, *toPath;
    Site *from, *to;
    SiteFileMeta *fromMeta, *toMeta;
    SOCK_BUF_TYPE i;
    char buf[SB_DATA_SIZE];
    time_t last;

    if (!(entry->state & QUEUE_STATE_QUEUED))
        return ErrQueueEntryNotReady;

    to = entry->destinationSite, toPath = entry->destinationPath;
    if (!(toMeta = siteStatOpenMeta(to, toPath)))
        return ErrUnableToResolvePath;

    /* A file exists, skip */
    if (toMeta->type != SITE_FILE_TYPE_NOTHING) {
        siteFileMetaFree(toMeta), free(toMeta);
        return strerror(EEXIST);
    }
    siteFileMetaFree(toMeta), free(toMeta);

    from = entry->sourceSite, fromPath = entry->sourcePath;
    if ((e = siteFileOpenRead(from, fromPath, -1, -1)))
        return e;

    if (!(fromMeta = siteFileOpenMeta(from)) || fromMeta->type == SITE_FILE_TYPE_UNKNOWN || !fromMeta->name) {
        siteFileClose(from);
        return ErrHeaderNotFound;
    }

    if (fromMeta->type != SITE_FILE_TYPE_FILE) {
        siteFileClose(from);
        return strerror(EISDIR);
    }

    if ((e = siteFileOpenWrite(to, toPath))) {
        siteFileClose(from);
        return e;
    }

    if (queueCallbackProgress)
        last = time(NULL);

    if (queueCallBackTotalSize)
        queueCallBackTotalSize(entry, fromMeta->length);

    while ((i = siteFileRead(from, buf, SB_DATA_SIZE))) {
        if (i != -1) {
            time_t now = time(NULL);

            if (siteFileWrite(to, buf, i) != -1) {
                if (queueCallbackProgress && now > last)
                    queueCallbackProgress(entry, (PlatformFileOffset) i), last = now;

                continue;
            }

            siteFileClose(from), siteFileClose(to);
            goto queueEntryDownloadNoClobber_Failed;
        } else if (siteFileAtEnd(from))
            break;

        siteFileClose(from), siteFileClose(to);
        goto queueEntryDownloadNoClobber_Failed;
    }

    siteFileClose(from), siteFileClose(to);
    entry->state = (char) ((entry->state | QUEUE_STATE_FINISHED) & ~QUEUE_STATE_QUEUED);

    if (queueCallbackStateChange)
        queueCallbackStateChange(entry);

    return NULL;

queueEntryDownloadNoClobber_Failed:
    siteFileClose(from), siteFileClose(to);
    entry->state = (char) ((entry->state | QUEUE_STATE_FAILED) & ~QUEUE_STATE_QUEUED);

    if (queueCallbackStateChange)
        queueCallbackStateChange(entry);

    return strerror(errno);
}

const char *queueEntryDownloadMirror(QueueEntry *entry) {
    const char *e, *fromPath, *toPath;
    Site *from, *to;
    SiteFileMeta *fromMeta, *toMeta;
    SOCK_BUF_TYPE i;
    char buf[SB_DATA_SIZE];
    time_t last;

    if (!(entry->state & QUEUE_STATE_QUEUED))
        return ErrQueueEntryNotReady;

    to = entry->destinationSite, toPath = entry->destinationPath;
    if (!(toMeta = siteStatOpenMeta(to, toPath)))
        return ErrUnableToResolvePath;

    siteFileMetaFree(toMeta), free(toMeta);
    from = entry->sourceSite, fromPath = entry->sourcePath;

    if ((e = siteFileOpenRead(from, fromPath, -1, -1)))
        return e;

    if (!(fromMeta = siteFileOpenMeta(from)) || fromMeta->type == SITE_FILE_TYPE_UNKNOWN || !fromMeta->name) {
        siteFileClose(from);
        return ErrHeaderNotFound;
    }

    if (fromMeta->type != SITE_FILE_TYPE_FILE) {
        siteFileClose(from);
        return strerror(EISDIR);
    }

    if ((e = siteFileOpenWrite(to, toPath))) {
        siteFileClose(from);
        return e;
    }

    if (queueCallbackProgress)
        last = time(NULL);

    if (queueCallBackTotalSize)
        queueCallBackTotalSize(entry, fromMeta->length);

    while ((i = siteFileRead(from, buf, SB_DATA_SIZE))) {
        if (i != -1) {
            time_t now = time(NULL);

            if (siteFileWrite(to, buf, i) != -1) {
                if (queueCallbackProgress && now > last)
                    queueCallbackProgress(entry, (PlatformFileOffset) i), last = now;

                continue;
            }

            siteFileClose(from), siteFileClose(to);
            goto queueEntryDownloadMirror_Failed;
        } else if (siteFileAtEnd(from))
            break;

        siteFileClose(from), siteFileClose(to);
        goto queueEntryDownloadMirror_Failed;
    }

    siteFileClose(from), siteFileClose(to);
    entry->state = (char) ((entry->state | QUEUE_STATE_FINISHED) & ~QUEUE_STATE_QUEUED);

    if (queueCallbackStateChange)
        queueCallbackStateChange(entry);

    return NULL;

    queueEntryDownloadMirror_Failed:
    siteFileClose(from), siteFileClose(to);
    entry->state = (char) ((entry->state | QUEUE_STATE_FAILED) & ~QUEUE_STATE_QUEUED);

    if (queueCallbackStateChange)
        queueCallbackStateChange(entry);

    return strerror(errno);
}

const char *queueEntryDownloadUpdate(QueueEntry *entry) {
    const char *e, *fromPath, *toPath;
    Site *from, *to;
    SiteFileMeta *fromMeta, *toMeta;
    SOCK_BUF_TYPE i;
    char buf[SB_DATA_SIZE];
    time_t last;

    if (!(entry->state & QUEUE_STATE_QUEUED))
        return ErrQueueEntryNotReady;

    to = entry->destinationSite, toPath = entry->destinationPath;
    if (!(toMeta = siteStatOpenMeta(to, toPath)))
        return ErrUnableToResolvePath;

    from = entry->sourceSite, fromPath = entry->sourcePath;

    /* A file exists, check if source is newer */
    if (toMeta->type != SITE_FILE_TYPE_NOTHING) {
        if ((fromMeta = siteStatOpenMeta(from, fromPath)) && fromMeta->type != SITE_FILE_TYPE_NOTHING) {
            if (platformTimeStructCompare(toMeta->modifiedDate, fromMeta->modifiedDate) >= 0) {
                entry->state = (char) ((entry->state | QUEUE_STATE_FINISHED) & ~QUEUE_STATE_QUEUED);
                siteFileMetaFree(fromMeta), free(fromMeta);
                return NULL;
            }
            siteFileMetaFree(fromMeta), free(fromMeta);
        }
    }
    siteFileMetaFree(toMeta), free(toMeta);

    if ((e = siteFileOpenRead(from, fromPath, -1, -1)))
        return e;

    if (!(fromMeta = siteFileOpenMeta(from)) || fromMeta->type == SITE_FILE_TYPE_UNKNOWN || !fromMeta->name) {
        siteFileClose(from);
        return ErrHeaderNotFound;
    }

    if (fromMeta->type != SITE_FILE_TYPE_FILE) {
        siteFileClose(from);
        return strerror(EISDIR);
    }

    if ((e = siteFileOpenWrite(to, toPath))) {
        siteFileClose(from);
        return e;
    }

    if (queueCallbackProgress)
        last = time(NULL);

    if (queueCallBackTotalSize)
        queueCallBackTotalSize(entry, fromMeta->length);

    while ((i = siteFileRead(from, buf, SB_DATA_SIZE))) {
        if (i != -1) {
            time_t now = time(NULL);

            if (siteFileWrite(to, buf, i) != -1) {
                if (queueCallbackProgress && now > last)
                    queueCallbackProgress(entry, (PlatformFileOffset) i), last = now;

                continue;
            }

            siteFileClose(from), siteFileClose(to);
            goto queueEntryDownloadUpdate_Failed;
        } else if (siteFileAtEnd(from))
            break;

        siteFileClose(from), siteFileClose(to);
        goto queueEntryDownloadUpdate_Failed;
    }

    siteFileClose(from), siteFileClose(to);
    entry->state = (char) ((entry->state | QUEUE_STATE_FINISHED) & ~QUEUE_STATE_QUEUED);

    if (queueCallbackStateChange)
        queueCallbackStateChange(entry);

    return NULL;

queueEntryDownloadUpdate_Failed:
    siteFileClose(from), siteFileClose(to);
    entry->state = (char) ((entry->state | QUEUE_STATE_FAILED) & ~QUEUE_STATE_QUEUED);

    if (queueCallbackStateChange)
        queueCallbackStateChange(entry);

    return strerror(errno);
}
