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
    int i;
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

static inline void PrintDirectoryFilesDosLike(Site *site) {
    void *dir;
    SiteDirectoryEntry *entry;

    /* TODO: Allow parameters to determine path directory */
    if (!(dir = siteDirectoryListingOpen(site, ".")))
        return;

    printf(" Site number is %ld\n Working directory is '%s'\n\n", siteArrayActiveGetNth(),
           siteWorkingDirectoryGet(site));

    while ((entry = siteDirectoryListingRead(site, dir))) {
        PlatformFileStat st;
        if (!siteDirectoryListingEntryStat(site, dir, entry, &st)) {
            char timeStr[30];
            platformGetTime(&st.st_mtime, (char *) &timeStr);

            if (platformFileStatIsDirectory(&st))
                printf("%-30s %12s %s\n", timeStr, "<DIR>", entry->name);
            else
                printf("%-30s %12" PF_OFFSET" %s\n", timeStr, st.st_size, entry->name);
        } else
            printf("%-30s %12s %s\n", "<\?\?\?>", "<\?\?\?>", entry->name);

        siteDirectoryEntryFree(entry);
    }

    siteDirectoryListingClose(site, dir), putc('\n', stdout);
}

static inline void PrintDirectoryFilesUnixLike(Site *site) {
    void *dir;
    SiteDirectoryEntry *entry;

    /* TODO: Allow parameters to determine path directory */
    if (!(dir = siteDirectoryListingOpen(site, ".")))
        return;

    putc(' ', stdout);
    while ((entry = siteDirectoryListingRead(site, dir))) {
        char *folder = entry->isDirectory ? "/" : "";
        if (strchr(entry->name, ' '))
            printf("'%s%s'   ", entry->name, folder);
        else
            printf("%s%s   ", entry->name, folder);
        siteDirectoryEntryFree(entry);
    }

    siteDirectoryListingClose(site, dir), putc('\n', stdout);
}

static inline const char *mountSite(const char *parameter) {
    UriDetails details = uriDetailsNewFrom(parameter);
    enum SiteType type;
    Site site;
    const char *err;

    if (!details.host)
        return "Address not understood";

    switch (uriDetailsGetScheme(&details)) {
        case SCHEME_HTTP:
        case SCHEME_HTTPS:
            type = SITE_HTTP;
            break;
        default:
        case SCHEME_UNKNOWN:
            return "Scheme not recognized";
    }

    uriDetailsFree(&details);

    if ((err = siteNew(&site, type, parameter)) || (err = siteArrayAdd(&site)))
        return err;

    siteArrayActiveSet(&site);
    return NULL;
}

static inline void mountList(void) {
    long a, i, len;
    Site *sites = siteArrayPtr(&len);
    a = siteArrayActiveGetNth();
    for (i = 0; i < len; ++i) {
        printf("%c%ld:\t%s\n", i == a ? '>' : ' ', i,
               sites[i].type == SITE_HTTP ? sites[i].site.http.fullUri : sites[i].site.file.fullUri);

    }
}

static inline void processCommand(char **args) {
    if (args[0] == NULL)
        return;

    if (args[1] == NULL || args[1][0] == '-') {
        long l;
        char *str;
        switch (toupper(args[0][0])) {
            case 'D':
                if (toupper(args[0][1]) == 'I' && toupper(args[0][2]) == 'R') {
                    PrintDirectoryFilesDosLike(siteArrayActiveGet());
                } else
                    goto processCommand_notFound;
                break;
            case 'E':
                if (toupper(args[0][1]) == 'X' && toupper(args[0][2]) == 'I' && toupper(args[0][3]) == 'T') {
                    siteArrayFree();
                    exit(0);
                } else
                    goto processCommand_notFound;
            case 'L':
                if (toupper(args[0][1]) == 'S')
                    PrintDirectoryFilesUnixLike(siteArrayActiveGet());
                else
                    goto processCommand_notFound;
                break;
            case 'M':
                if (toupper(args[0][1]) == 'O' && toupper(args[0][2]) == 'U' && toupper(args[0][3]) == 'N' &&
                    toupper(args[0][4]) == 'T')
                    mountList();
                break;
            case 'P':
                if (toupper(args[0][1]) == 'W' && toupper(args[0][2]) == 'D')
                    puts(siteWorkingDirectoryGet(siteArrayActiveGet()));
                else
                    goto processCommand_notFound;
                break;
            case 'U':
                if (toupper(args[0][1]) == 'M' && toupper(args[0][2]) == 'O' && toupper(args[0][3]) == 'U' &&
                    toupper(args[0][4]) == 'N' && toupper(args[0][5]) == 'T') {
                    Site *site;

                    if (!(site = siteArrayActiveGet()))
                        return;

                    siteFree(site), siteArrayRemove(site);
                    siteArrayActiveSetNth(0);
                } else
                    goto processCommand_notFound;
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
                    if ((str = siteArrayActiveSetNth(l)))
                        puts(str);
                break;
            default:
                goto processCommand_notFound;
        }
    } else if (args[2] == NULL || args[2][0] == '-') {
        switch (toupper(args[0][0])) {
            case 'C':
                if (toupper(args[0][1]) == 'D')
                    siteWorkingDirectorySet(siteArrayActiveGet(), args[1]);
                else
                    goto processCommand_notFound;
                break;
            case 'M':
                if (toupper(args[0][1]) == 'O' && toupper(args[0][2]) == 'U' && toupper(args[0][3]) == 'N' &&
                    toupper(args[0][4]) == 'T') {
                    const char *err;
                    if ((err = mountSite(args[1]))) {
                        puts(err);
                        return;
                    }
                } else
                    goto processCommand_notFound;
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
        printf(SHELL_PS1, siteArrayActiveGetNth());
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
#ifndef _WIN32
            if (isatty(fileno(stdout)))
#endif /* _WIN32 */
                interactiveMode();
#ifndef _WIN32
            else
                puts("Nothing queued for download");
#endif /* _WIN32 */
            return 1;
        case 1:
        default:
            puts("Not yet implemented");
            break;
    }

    return 0;
}
