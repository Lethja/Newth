#include "queue.h"

AddressQueue *addressQueue = NULL;

static inline char *PathAppendOrCreate(PathQueue *pathQueue, const char *path) {
    size_t i;

    if (pathQueue->paths == NULL) {
        pathQueue->paths = malloc(sizeof(char **));
        pathQueue->paths[0] = malloc(strlen(path) + 1);
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

char *downloadQueueAppendOrCreate(SocketAddress *address, const char *path) {
    if (addressQueue) {
        size_t i;
        for (i = 0; i < addressQueue->queueLen; ++i) {
            if (memcmp(&addressQueue->queue[i]->address.storage, &address->storage, sizeof(SocketAddress)) == 0)
                goto downloadQueueAppendOrCreate_Found;
        }

        downloadQueueAppendOrCreate_Found:

        if (i == addressQueue->queueLen) {
            void *tmp = realloc(addressQueue->queue, sizeof(AddressQueue) * addressQueue->queueLen + 1);
            if (tmp) {
                addressQueue->queue = tmp;
                addressQueue->queue[i] = calloc(1, sizeof(PathQueue));

                if (!addressQueue->queue || !addressQueue->queue[i]) {
                    if (addressQueue)
                        free(addressQueue), addressQueue = NULL;

                    return strerror(errno);
                }

                ++addressQueue->queueLen;
                memcpy(&addressQueue->queue[i]->address, address, sizeof(SocketAddress));
            } else
                return strerror(errno);
        }

        return PathAppendOrCreate(addressQueue->queue[i], path);

    } else {
        addressQueue = malloc(sizeof(AddressQueue));

        if (!addressQueue)
            return strerror(errno);

        addressQueue->queue = malloc(sizeof(char *));
        addressQueue->queue[0] = calloc(1, sizeof(PathQueue));

        if (!addressQueue->queue || !addressQueue->queue[0]) {
            if (addressQueue)
                free(addressQueue), addressQueue = NULL;

            return strerror(errno);
        }

        addressQueue->queueLen = 1;
        memcpy(&addressQueue->queue[0]->address, address, sizeof(SocketAddress));

        return PathAppendOrCreate(addressQueue->queue[0], path);
    }
}

void *downloadQueueFind(SocketAddress *address, const char *path, size_t *addressIdx, size_t *pathIdx) {
    size_t i, j;
    if (!addressQueue)
        return NULL;

    for (i = 0; i < addressQueue->queueLen; ++i) {
        if (memcmp(&addressQueue->queue[i]->address.storage, &address->storage, sizeof(SocketAddress)) == 0)
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

char *downloadQueueRemove(SocketAddress *address, const char *path) {
    size_t i, j;
    if (downloadQueueFind(address, path, &i, &j)) {
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
    }

    if (!addressQueue->queue[i]->pathsLen) {
        free(addressQueue->queue[i]);
        if (i + 1 != addressQueue->queueLen)
            memmove(&addressQueue->queue[i], &addressQueue->queue[i + 1],
                    (addressQueue->queueLen - (i + 1)) * sizeof(char *));

        if (addressQueue->queueLen < 2)
            free(addressQueue->queue), addressQueue->queue = NULL;
        else {
            void *tmp = realloc(addressQueue->queue, sizeof(char *) * addressQueue->queueLen - 1);
            if (tmp)
                addressQueue->queue = tmp;
            else
                return strerror(errno);
        }

        if (addressQueue)
            --addressQueue->queueLen;
    }

    if (!addressQueue->queueLen) {
        free(addressQueue->queue);
        free(addressQueue), addressQueue = NULL;
    }

    return NULL;
}

void downloadQueueClear(void) {
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

size_t downloadQueueNumberOfAddresses(void) {
    return addressQueue->queueLen;
}

size_t downloadQueueAddressNumberOfRequests(SocketAddress *address) {
    size_t i;
    for (i = 0; i < addressQueue->queueLen; ++i) {
        if (memcmp(&addressQueue->queue[i]->address.storage, &address->storage, sizeof(SocketAddress)) == 0)
            return addressQueue->queue[i]->pathsLen;
    }
    return 0;
}
