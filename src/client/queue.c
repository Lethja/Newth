#include "err.h"
#include "queue.h"
#include "../platform/platform.h"

AddressQueue *addressQueue = NULL;

static inline const char *PathAppendOrCreate(PathQueue *pathQueue, const char *path) {
    size_t i;

    if (!pathQueue)
        return ErrNoQueue;

    if (pathQueue->paths == NULL) {
        if (!(pathQueue->paths = malloc(sizeof(char *))))
            return strerror(errno);

        if (!(pathQueue->paths[0] = malloc(strlen(path) + 1))) {
            free(pathQueue->paths), pathQueue->paths = NULL;
            return strerror(errno);
        }

        strcpy(pathQueue->paths[0], path);
        pathQueue->pathsLen = 1;
        return NULL;
    }

    for (i = 0; i < pathQueue->pathsLen; ++i) {
        if (pathQueue->paths[i] && strcmp(pathQueue->paths[i], path) == 0)
            return NULL;
    }

    if (i == pathQueue->pathsLen) {
        void *tmp = realloc(pathQueue->paths, sizeof(char *) * (pathQueue->pathsLen + 1));
        if (tmp) {
            pathQueue->paths = tmp, tmp = malloc(strlen(path) + 1);

            if (!tmp)
                return strerror(errno);

            strcpy(tmp, path);
            pathQueue->paths[i] = tmp;
            ++pathQueue->pathsLen;
        } else
            return strerror(errno);
    }

    return NULL;
}

static inline char *newPathQueue(PathQueue **self, SocketAddress *address) {
    PathQueue *q = malloc(sizeof(PathQueue));

    if (!q) {
        *self = NULL;
        return strerror(errno);
    }

    memcpy(&q->address, address, sizeof(SocketAddress));
    q->pathsLen = 0, q->paths = NULL;
    q->ctrlSock = q->dataSock = -1;

    *self = q;
    return NULL;
}

const char *pathQueueAppendOrCreate(SocketAddress *address, const char *path) {
    if (addressQueue) {
        size_t i;

        for (i = 0; i < addressQueue->queueLen; ++i) {
            if (memcmp(&addressQueue->queue[i]->address.address.storage, &address->address.storage,
                       sizeof(SocketAddress)) == 0)
                goto downloadQueueAppendOrCreate_Found;
        }

        downloadQueueAppendOrCreate_Found:

        if (i == addressQueue->queueLen) {
            PathQueue *q;

            if (!newPathQueue(&q, address)) {
                void *tmp = realloc(addressQueue->queue, sizeof(AddressQueue) * (addressQueue->queueLen + 1));

                if (tmp) {
                    addressQueue->queue = tmp;
                    addressQueue->queue[i] = q;
                    ++addressQueue->queueLen;
                } else {
                    if (q)
                        free(q);

                    return strerror(errno);
                }
            } else {
                if (q)
                    free(q);

                return strerror(errno);
            }
        }
        return PathAppendOrCreate(addressQueue->queue[i], path);
    } else {
        if (!(addressQueue = malloc(sizeof(AddressQueue))))
            return strerror(errno);

        if (!(addressQueue->queue = malloc(sizeof(PathQueue *))))
            goto downloadQueueAppendOrCreate_AbortNew;

        if (newPathQueue(&addressQueue->queue[0], address))
            goto downloadQueueAppendOrCreate_AbortNew;

        addressQueue->queueLen = 1;

        return PathAppendOrCreate(addressQueue->queue[0], path);

        downloadQueueAppendOrCreate_AbortNew:
        free(addressQueue), addressQueue = NULL;
        return strerror(errno);
    }
}

void *pathQueueFind(SocketAddress *address, const char *path, size_t *addressIdx, size_t *pathIdx) {
    size_t i, j;

    if (!addressQueue)
        return NULL;

    for (i = 0; i < addressQueue->queueLen; ++i) {
        if (memcmp(&addressQueue->queue[i]->address.address.storage, &address->address.storage,
                   sizeof(SocketAddress)) == 0)
            goto downloadQueueFind_AddressFound;
    }

    downloadQueueFind_AddressFound:

    if (i == addressQueue->queueLen || !addressQueue->queue[0]->paths)
        return NULL;

    for (j = 0; j < addressQueue->queue[i]->pathsLen; ++j) {
        if (strcmp(addressQueue->queue[i]->paths[j], path) == 0) {
            if (addressIdx)
                *addressIdx = i;
            if (pathIdx)
                *pathIdx = j;

            return addressQueue->queue[i]->paths[j];
        }
    }

    return NULL;
}

char *pathQueueRemove(SocketAddress *address, const char *path) {
    size_t i, j;

    if (pathQueueFind(address, path, &i, &j)) {
        free(addressQueue->queue[i]->paths[j]);

        if (j + 1 != addressQueue->queue[i]->pathsLen)
            memmove(&addressQueue->queue[i]->paths[j], &addressQueue->queue[i]->paths[j + 1],
                    (addressQueue->queue[i]->pathsLen - (j + 1)) * sizeof(char *));

        if (addressQueue->queue[i]->pathsLen < 2)
            free(addressQueue->queue[i]->paths), addressQueue->queue[i]->paths = NULL;
        else {
            void *tmp = realloc(addressQueue->queue[i]->paths, sizeof(char *) * addressQueue->queue[i]->pathsLen - 1);
            if (tmp)
                addressQueue->queue[i]->paths = tmp;
            else
                return strerror(errno);
        }

        --addressQueue->queue[i]->pathsLen;

        if (!addressQueue->queue[i]->pathsLen) {
            free(addressQueue->queue[i]);
            if (i + 1 != addressQueue->queueLen)
                memmove(&addressQueue->queue[i], &addressQueue->queue[i + 1],
                        (addressQueue->queueLen - (i + 1)) * sizeof(char *));

            if (addressQueue->queueLen < 2)
                free(addressQueue->queue), addressQueue->queue = NULL;
            else {
                void *tmp = realloc(addressQueue->queue, sizeof(char *) * (addressQueue->queueLen - 1));
                if (tmp)
                    addressQueue->queue = tmp;
                else
                    return strerror(errno);
            }

#pragma clang diagnostic push
#pragma ide diagnostic ignored "ConstantConditionsOC"
            if (addressQueue)
                --addressQueue->queueLen;
#pragma clang diagnostic pop

            if (!addressQueue->queueLen) {
                free(addressQueue->queue);
                free(addressQueue), addressQueue = NULL;
            }
        }
    }

    return NULL;
}

void addressQueueClear(void) {
    size_t i, j;

    if (!addressQueue)
        return;

    for (i = 0; i < addressQueue->queueLen; ++i) {
        for (j = 0; j < addressQueue->queue[i]->pathsLen; ++j) {
            free(addressQueue->queue[i]->paths[j]);
        }

        free(addressQueue->queue[i]->paths), free(addressQueue->queue[i]);
    }

    free(addressQueue->queue), free(addressQueue), addressQueue = NULL;
}

unsigned int addressQueueGetNth(void) {
    return addressQueue ? addressQueue->queueLen : 0;
}

PathQueue **addressQueueGet(void) {
    if (!addressQueue || !addressQueue->queue)
        return NULL;

    return addressQueue->queue;
}

unsigned int pathQueueGetNth(SocketAddress *address) {
    if (addressQueue) {
        unsigned int i;

        for (i = 0; i < addressQueue->queueLen; ++i) {
            if (memcmp(&addressQueue->queue[i]->address.address.storage, &address->address.storage,
                       sizeof(SocketAddress)) == 0)
                return addressQueue->queue[i]->pathsLen;
        }
    }
    return 0;
}

unsigned int addressQueueGetTotalPathRequests(void) {
    unsigned int t = 0;
    if (addressQueue) {
        unsigned int i, a = addressQueueGetNth();

        for (i = 0; i < a; ++i)
            t += addressQueue->queue[i]->pathsLen;
    }
    return t;
}

char *pathQueueConnect(PathQueue *self) {
    char *err;
    switch (self->address.state) {
        case SA_STATE_QUEUED:
        case SA_STATE_TRY_LATER:
            if (self->ctrlSock == -1) {
                if ((err = ioCreateSocketFromSocketAddress(&self->address, &self->ctrlSock)))
                    return err;
            }

            if (connect(self->ctrlSock, &self->address.address.sock, sizeof(self->address.address.sock)) == -1) {
                CLOSE_SOCKET(self->ctrlSock), self->ctrlSock = -1;
                return strerror(platformSocketGetLastError());
            }

            self->address.state = SA_STATE_CONNECTED;
        case SA_STATE_CONNECTED:
            return NULL;
        default:
            return "State of socket invalid";
    }
}
