#include "../client/io.h"
#include "../client/queue.h"
#include "../client/site.h"
#include "../client/uri.h"
#include "../server/http.h"

#include <ctype.h>

#if defined(WATT32) || defined(WIN32)
#define SHELL_PS1 "%ld>"
#else
#define SHELL_PS1 "%ld # "
#endif /* defined(WATT32) || defined(WIN32) */

static inline SocketAddress *parseUri(const char *uri, UriDetails *uriDetails) {
    UriDetails details = uriDetailsNewFrom(uri);
    SocketAddress *address = calloc(1, sizeof(SocketAddress));

    if (uriDetailsCreateSocketAddress(&details, address, SCHEME_HTTP)) {
        uriDetailsFree(&details), free(address);
        return NULL;
    }

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
    /* ServerHeaderResponse *headerResponse; */
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

static inline void processInput(char *input, char **args) {
    const char *delimiters = " \t\n";
    int i = 0;

    while (*input != '\0') {
        if (i >= 4)
            break;

        args[i++] = input;

        while (*input != '\0' && strchr(delimiters, *input) == NULL)
            ++input;

        if (*input != '\0')
            *input = '\0', ++input;
    }

    args[i] = NULL;
}

static inline void PrintDirectoryFiles(Site *site) {
    void *dir;
    SiteDirectoryEntry *entry;

    /* TODO: Allow parameters to determine path directory */
    if (!(dir = siteOpenDirectoryListing(site, ".")))
        return;

    putc(' ', stdout);
    while ((entry = siteReadDirectoryListing(site, dir))) {
        if (strchr(entry->name, ' '))
            printf("'%s'   ", entry->name);
        else
            printf("%s   ", entry->name);
        siteDirectoryEntryFree(entry);
    }

    siteCloseDirectoryListing(site, dir), putc('\n', stdout);
}

static inline void processCommand(char **args) {
    if (args[0] == NULL)
        return;

    if (args[1] == NULL || args[1][0] == '-') {
        long l;
        char *str;
        switch (toupper(args[0][0])) {
            case 'E':
                siteArrayFree();
                exit(0);
            case 'L':
                if (toupper(args[0][1]) == 'S')
                    PrintDirectoryFiles(siteArrayGetActive());
                break;
            case 'P':
                if (toupper(args[0][1]) == 'W' && toupper(args[0][2]) == 'D')
                    puts(siteGetWorkingDirectory(siteArrayGetActive()));
                break;
            case '0':
            case '1':
            case '2':
            case '3':
            case '4':
            case '5':
            case '6':
            case '7':
            case '8':
            case '9':
                errno = 0, l = strtol(args[0], NULL, 10);
                if (!errno)  /* Must be a site switch */
                    if ((str = siteArraySetActiveNth(l)))
                        puts(str);
                break;
            default:
                goto processCommand_notFound;
        }
    } else if (args[2] == NULL || args[2][0] == '-') {
        switch (toupper(args[0][0])) {
            case 'C':
                if (toupper(args[0][1]) == 'D')
                    siteSetWorkingDirectory(siteArrayGetActive(), args[1]);
                break;
            default:
                goto processCommand_notFound;
        }

    }
    return;

    processCommand_notFound:
    puts("Command not found");
}

# if __STDC_VERSION__ >= 201112L
_Noreturn
#endif

static inline void interactiveMode(void) {
    char input[BUFSIZ];
    char *args[5];

    siteArrayInit();

#pragma clang diagnostic push
#pragma ide diagnostic ignored "EndlessLoop"
    while (1) {
        printf(SHELL_PS1, siteArrayGetActiveNth());
        if (fgets(input, sizeof(input), stdin)) {
            processInput(input, (char **) args);
            processCommand(args);
        }
    }
#pragma clang diagnostic pop
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
            if (isatty(fileno(stdout)))
                interactiveMode();
            else
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
