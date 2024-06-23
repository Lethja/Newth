#include "../client/io.h"
#include "../client/queue.h"
#include "../client/site.h"
#include "../client/uri.h"
#include "../client/err.h"
#include "../common/signal.h"

#include <ctype.h>
#include <time.h>

#if defined(WATT32) || defined(WIN32)
#define SHELL_PS1 "%ld>"
#else
#define SHELL_PS1 "%ld # "
#endif /* defined(WATT32) || defined(WIN32) */

static inline void Copy(const char **argv) {
    size_t i = 0;
    while (argv[i])
        ++i;

    /* TODO: Implement adding to the queue */
    if (i > 2)
        puts("COPY from/to not yet implemented");
    else {
        const char *e;
        Site *from = siteArrayActiveGet(), *to = siteArrayPtr(NULL);
        SiteFileMeta *fromMeta, *toMeta;
        char buf[SB_DATA_SIZE];

        if ((e = siteFileOpenRead(from, argv[1], -1, -1))) {
            puts(e);
            return;
        }

        if (!(fromMeta = siteFileOpenMeta(from)) || fromMeta->type == SITE_FILE_TYPE_UNKNOWN || !fromMeta->name) {
            siteFileClose(from), puts(ErrHeaderNotFound);
            return;
        }

        if (fromMeta->type != SITE_FILE_TYPE_FILE) {
            siteFileClose(from), puts(strerror(EISDIR));
            return;
        }

        if (!(toMeta = siteStatOpenMeta(to, fromMeta->name))) {
            siteFileClose(from), puts(strerror(errno));
            return;
        }

        /* TODO: add logic for overwrite/update behaviour */
        if (toMeta->type != SITE_FILE_TYPE_NOTHING) {
            siteFileMetaFree(toMeta), free(toMeta), siteFileClose(from), puts(strerror(EEXIST));
            return;
        }
        siteFileMetaFree(toMeta), free(toMeta);

        if ((e = siteFileOpenWrite(to, fromMeta->name))) {
            siteFileClose(from), puts(e);
            return;
        }

        while ((i = siteFileRead(from, buf, SB_DATA_SIZE))) {
            if (i != -1) {
                siteFileWrite(to, buf, i);
                continue;
            }

            puts(strerror(errno));
            return;
        }

        siteFileClose(from), siteFileClose(to);
    }
}

static inline void XCopy(const char **argv) {
    size_t i = 0;
    while (argv[i])
        ++i;

    /* TODO: Implement adding to the queue */
    if (i > 2)
        puts("XCOPY from/to not yet implemented");
    else
        puts("XCOPY from not yet implemented");
}

static inline void ListQueue(void) {
    puts("QUEUE not yet implemented"); /* TODO: Implement */
}

static inline SocketAddress *ParseUri(const char *uri, UriDetails *uriDetails) {
    UriDetails details = uriDetailsNewFrom(uri);
    SocketAddress *address = calloc(1, sizeof(SocketAddress));

    if (uriDetailsCreateSocketAddress(&details, address, SCHEME_HTTP)) {
        uriDetailsFree(&details), free(address);
        return NULL;
    }

    memcpy(uriDetails, &details, sizeof(UriDetails));
    return address;
}

static inline void ParseUris(int argc, char **argv) {
    int i;
    unsigned int e;
    for (e = 0, i = 1; i < argc; ++i) {
        UriDetails details;
        SocketAddress *address = ParseUri(argv[i], &details);
        if (address) {
            const char *err;
            if ((err = pathQueueAppendOrCreate(address, details.path ? details.path : "/")))
                printf("ERR: '%s' (%s)\n", argv[i], err), ++e;
        } else
            printf("ERR: '%s' (Couldn't parse URI)\n", argv[i]), ++e;

        uriDetailsFree(&details);
    }

    if (e)
        printf("WARN: %u addresses couldn't be parsed", e);
}

static inline void CleanInput(char *input) {
    char *newline = strchr(input, '\n');
    if (newline)
        *newline = '\0';
}

static inline void PrintDirectoryFilesDosLike(Site *site, char *path) {
    void *dir;
    char *pathRes = NULL, *unknownPath = "Unknown";
    SiteFileMeta *entry;
    UriDetails details;

    if (!path)
        path = ".";

    if (!(dir = siteDirectoryListingOpen(site, path)))
        return;

    details = uriDetailsNewFrom(siteWorkingDirectoryGet(site));

    if (details.path) {
        char *tmp = uriPathAbsoluteAppend(details.path, path);
        if (tmp)
            free(details.path), details.path = tmp;

        pathRes = uriDetailsCreateString(&details);
    }

    uriDetailsFree(&details);

    if (!pathRes)
        pathRes = unknownPath;

    printf(" Site: %ld\n Path: '%s'\n\n", siteArrayActiveGetNth(), pathRes);

    if (pathRes != unknownPath)
        free(pathRes);

    while ((entry = siteDirectoryListingRead(site, dir))) {
        SiteFileMeta meta;
        if (!siteDirectoryListingEntryStat(site, dir, entry, &meta)) {
            char timeStr[30];
            if (meta.name) {
                /* There should always be a `meta.name` but in the case there is not this code is never run */

                if (meta.modifiedDate)
                    /* TODO: Platform independent to convert meta.modifiedDate into */
                    strftime(timeStr, 30, "%a, %d %b %Y %H:%M:%S GMT", meta.modifiedDate);
                else
                    strcpy(timeStr, "???, ?? ??? ???? ??:??:?? GMT");

                switch (meta.type) {
                    case SITE_FILE_TYPE_DIRECTORY:
                        printf("%-30s %12s %s\n", timeStr, "<DIR>", meta.name);
                        break;
                    case SITE_FILE_TYPE_FILE:
                        printf("%-30s %12" PF_OFFSET" %s\n", timeStr, meta.length, meta.name);
                        break;
                    default:
                        printf("%-30s %12s %s\n", "<\?\?\?>", "<\?\?\?>", meta.name);
                        break;
                }
            }

            siteFileMetaFree(&meta);
            siteDirectoryEntryFree(entry);
        }
    }
    siteDirectoryListingClose(site, dir), putc('\n', stdout);
}

static inline void PrintDirectoryFilesUnixLike(Site * site, char *path) {
    void *dir;
    SiteFileMeta *entry;

    if (!path)
        path = ".";

    if (!(dir = siteDirectoryListingOpen(site, path)))
        return;

    putc(' ', stdout);
    while ((entry = siteDirectoryListingRead(site, dir))) {
        if (strchr(entry->name, ' '))
            printf("'%s'   ", entry->name);
        else
            printf("%s   ", entry->name);
        siteDirectoryEntryFree(entry);
    }

    siteDirectoryListingClose(site, dir), putc('\n', stdout);
}

static inline void PrintHelp(void) {
    const char *legend =
    " # = id, ... = parameter(s), () = parameter is optional."
    " All commands are case-insensitive.\n";
    const char *help =
    " ?            - Show this printout       #           - Change active site\n"
    " ! ...        - Run system command       COPY ...    - Queue file(s) for download\n"
    " DIR (...)    - Full directory info      EXIT        - Close the program\n"
    " LS (...)     - Slim directory info      MOUNT (...) - Mount URI as site\n"
    " PWD (#)      - Print site current path  QUEUE       - List enqueued tasks\n"
    " UMOUNT #     - Unmount site             XCOPY ...   - Queue subdirectorie(s) for download\n";
    puts(help), puts(legend);
}

static inline const char *MountSite(const char *parameter) {
    UriDetails details = uriDetailsNewFrom(parameter);
    enum SiteType type;
    Site site;
    const char *err;

    if (!details.host)
        return ErrAddressNotUnderstood;

    switch (uriDetailsGetScheme(&details)) {
        case SCHEME_HTTP:
        case SCHEME_HTTPS:
            type = SITE_HTTP;
            break;
        case SCHEME_FILE:
            type = SITE_FILE;
            break;
        default:
        case SCHEME_UNKNOWN:
            return ErrSchemeNotRecognized;
    }

    uriDetailsFree(&details);

    if ((err = siteNew(&site, type, parameter)) || (err = siteArrayAdd(&site)))
        return err;

    siteArrayActiveSet(&site);
    return NULL;
}

static inline void MountList(void) {
    long a, i, len;
    Site *sites = siteArrayPtr(&len);

    if (!len)
        siteArrayInit();

    a = siteArrayActiveGetNth();
    for (i = 0; i < len; ++i) {
        printf("%c%ld:\t%s\n", i == a ? '>' : ' ', i,
               sites[i].type == SITE_HTTP ? sites[i].site.http.fullUri : sites[i].site.file.fullUri);

    }
}

/**
 * Shifts every byte in the args array back one so that '!' is removed, pointers are also adjusted to this change
 * @param args The args created by platformArgvConvertString() to remove the first character from
 * @remark It's up to the caller to check that if this function should be used or not.
 * It will remove the first character of the first parameter no matter what it is
 * @code
 * char **result;
 * if (args[0][0] == '!'`) {
 *     if (args[0][1] != '\0')
 *        StripBang(args), result = args;
 *     else
 *        result = &args[1];
 * } else
 *     result = args;
 * @endcode
 */
static inline void StripBang(char **args) {
    char *e;
    size_t i = 1;

    while (args[i])
        args[i] = args[i] - 1, ++i;

    --i, e = &args[i][strlen(&args[i][1]) + 1];
    memmove(&args[0][0], &args[0][1], e - &args[0][0]);
}

static inline void ProcessCommand(char **args) {
    const char *str;

    if (args[0] == NULL)
        return;

    if (args[1] == NULL) { /* No parameter commands */
        long l;
        switch (toupper(args[0][0])) {
            case '!':
                memmove(&args[0][0], &args[0][1], strlen(&args[0][1]) + 1);
                str = platformExecRunWait((const char **) args);
                if (str)
                    puts(str);
                break;
            case '?':
                PrintHelp();
                break;
            case 'D':
                if (toupper(args[0][1]) == 'I' && toupper(args[0][2]) == 'R')
                    PrintDirectoryFilesDosLike(siteArrayActiveGet(), NULL);
                else
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
                    PrintDirectoryFilesUnixLike(siteArrayActiveGet(), NULL);
                else
                    goto processCommand_notFound;
                break;
            case 'M':
                if (toupper(args[0][1]) == 'O' && toupper(args[0][2]) == 'U' && toupper(args[0][3]) == 'N' &&
                    toupper(args[0][4]) == 'T')
                    MountList();
                break;
            case 'P':
                if (toupper(args[0][1]) == 'W' && toupper(args[0][2]) == 'D')
                    puts(siteWorkingDirectoryGet(siteArrayActiveGet()));
                else
                    goto processCommand_notFound;
                break;
            case 'Q':
                if (toupper(args[0][1]) == 'U' && toupper(args[0][2]) == 'E' && toupper(args[0][3]) == 'U' &&
                    toupper(args[0][4]) == 'E')
                    ListQueue();
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
            case '!':
                if (args[0][1] != '\0') {
                    StripBang(args);
                    str = platformExecRunWait((const char **) args);
                } else
                    str = platformExecRunWait((const char **) &args[1]);

                if (str)
                    puts(str);
                break;
            case 'C':
                if (toupper(args[0][1]) == 'D') { /* CD */
                    if (siteWorkingDirectorySet(siteArrayActiveGet(), args[1]))
                        puts(strerror(errno));
                } else if (toupper(args[0][1]) == 'P' ||
                           (toupper(args[0][1]) == 'O' && toupper(args[0][2]) == 'P' && toupper(args[0][3]) == 'Y'))
                    Copy((const char **) args);
                else
                    goto processCommand_notFound;
                break;
            case 'D':
                if (toupper(args[0][1]) == 'I' && toupper(args[0][2]) == 'R')
                    PrintDirectoryFilesDosLike(siteArrayActiveGet(), args[1]);
                else
                    goto processCommand_notFound;
                break;
            case 'L':
                if (toupper(args[0][1]) == 'S')
                    PrintDirectoryFilesUnixLike(siteArrayActiveGet(), args[1]);
                else
                    goto processCommand_notFound;
                break;
            case 'M':
                if (toupper(args[0][1]) == 'O' && toupper(args[0][2]) == 'U' && toupper(args[0][3]) == 'N' &&
                    toupper(args[0][4]) == 'T') {
                    const char *err;
                    if ((err = MountSite(args[1]))) {
                        puts(err);
                        return;
                    }
                } else
                    goto processCommand_notFound;
                break;
            case 'P':
                if (toupper(args[0][1]) == 'W' && toupper(args[0][2]) == 'D') {
                    long id;
                    if (args[1][0] == '0' && args[1][1] == '\0')
                        id = 0;
                    else if (!(id = atol(args[1]))) { /* NOLINT(*-err34-c) */
                        puts("Invalid Mount ID");
                        return;
                    }

                    if (siteArrayNthMounted(id))
                        puts(siteWorkingDirectoryGet(&siteArrayPtr(NULL)[id]));
                    else
                        puts("Invalid Mount ID");
                } else
                    goto processCommand_notFound;
                break;
            case 'X':
                if (toupper(args[0][1]) == 'C' ||
                    (toupper(args[0][2]) == 'O' && toupper(args[0][3]) == 'P' && toupper(args[0][4]) == 'Y'))
                    XCopy((const char **) args);
                else
                    goto processCommand_notFound;
                break;
            default:
                goto processCommand_notFound;
        }
    } else { /* Infinite commands */
        switch (toupper(args[0][0])) {
            case '!':
                if (args[0][1] != '\0') {
                    StripBang(args);
                    str = platformExecRunWait((const char **) args);
                } else
                    str = platformExecRunWait((const char **) &args[1]);

                if (str)
                    puts(str);
                break;
            case 'C':
                if (toupper(args[0][1]) == 'P' ||
                    (toupper(args[0][1]) == 'O' && toupper(args[0][2]) == 'P' && toupper(args[0][3]) == 'Y'))
                    Copy((const char **) args);
                else
                    goto processCommand_notFound;
                break;
            case 'X':
                if (toupper(args[0][1]) == 'C' ||
                    (toupper(args[0][2]) == 'O' && toupper(args[0][3]) == 'P' && toupper(args[0][4]) == 'Y'))
                    XCopy((const char **) args);
                else
                    goto processCommand_notFound;
                break;
            default:
                goto processCommand_notFound;
        }
    }
    return;

processCommand_notFound:
    puts("Command not found. See valid commands with '?'");
}

# if __STDC_VERSION__ >= 201112L
_Noreturn
#endif

static inline void InteractiveMode(void) {
    char input[BUFSIZ];
    char **args;

    siteArrayInit();

#pragma clang diagnostic push
#pragma ide diagnostic ignored "EndlessLoop"
    while (1) {
        printf(SHELL_PS1, siteArrayActiveGetNth());
        if (fgets(input, sizeof(input), stdin)) {
            CleanInput(input);
            if ((args = platformArgvConvertString(input))) {
                ProcessCommand(args);
                platformArgvFree(args);
            }
        }
    }
#pragma clang diagnostic pop
}

int main(int argc, char **argv) {
    char *err;

    ParseUris(argc, argv);

    err = platformIpStackInit();
    if (err) {
        puts(err);
        return 1;
    }

    platformConnectSignals(signalNoAction, NULL, NULL);

    switch (addressQueueGetTotalPathRequests()) {
        case 0:
#ifndef _WIN32
            if (isatty(fileno(stdout)))
#endif /* _WIN32 */
                InteractiveMode();
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
