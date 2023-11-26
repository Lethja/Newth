#include "../client/io.h"
#include "../client/queue.h"
#include "../client/uri.h"
#include "../platform/platform.h"
#include "../server/http.h"

static inline SocketAddress *parseUri(const char *uri, UriDetails *uriDetails) {
    UriDetails details = uriDetailsNewFrom(uri);
    SocketAddress *address = calloc(1, sizeof(SocketAddress));

    if (uriDetailsCreateSocketAddress(&details, address, SCHEME_HTTP)) {
        uriDetailsFree(&details), free(address);
        return NULL;
    }

    if (!uriDetails)
        uriDetailsFree(&details);
    else
        memcpy(uriDetails, &details, sizeof(UriDetails));

    return address;
}

static inline void parseUris(int argc, char **argv) {
    size_t i;
    unsigned int e;
    for (e = 0, i = 1; i < argc; ++i) {
        UriDetails details;
        SocketAddress *address = parseUri(argv[i], &details);
        if (address) {
            char *err;
            if ((err = pathQueueAppendOrCreate(address, details.path ? details.path : "/")))
                printf("ERR: '%s' (%s)\n", argv[i], err), ++e;
        } else
            printf("ERR: '%s' (Couldn't parse URI)\n", argv[i]), ++e;

        uriDetailsFree(&details);
    }

    if (e)
        printf("WARN: %u addresses couldn't be parsed", e);
}

static inline char *handleQueueEntry(void) {
    PathQueue **pathQueue = addressQueueGet(), *q = pathQueue[0];
    ServerHeaderResponse *headerResponse;
    char *err = pathQueueConnect(q);
    char *header = NULL;

    if (err)
        return err;

    if ((err = pathQueueConnect(q)))
        return err;

    if ((err = ioHttpHeadRequest(&q->ctrlSock, q->paths[0], NULL)))
        return err;

    if ((err = ioHttpHeadRead(&q->ctrlSock, &header)))
        return err;

    return NULL;
}

int main(int argc, char **argv) {
    char *err;

    parseUris(argc, argv);

    err = platformIpStackInit();
    if (err) {
        puts(err);
        return 1;
    }

    switch (addressQueueGetTotalPathRequests()) {
        case 0:
            puts("Nothing queued for download");
            return 1;
        case 1:
            if ((err = handleQueueEntry())) {
                puts(err);
                return 1;
            }
            break;
        default:
            puts("Not yet implemented");
            break;
    }

    return 0;
}
