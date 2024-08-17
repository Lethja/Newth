#include "../client/io.h"
#include "../client/queue.h"
#include "../client/site.h"
#include "../client/uri.h"
#include "../client/err.h"
#include "../common/signal.h"

#include <ctype.h>
#include <time.h>

#ifdef READLINE
#include <readline/history.h>
#include <readline/readline.h>
#endif

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
                if (siteFileWrite(to, buf, i) != -1)
                    continue;

                siteFileClose(from), siteFileClose(to), puts(strerror(errno));
                return;
            } else if (siteFileAtEnd(from))
                break;

            siteFileClose(from), siteFileClose(to), puts(strerror(errno));
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

#ifndef READLINE

static inline void CleanInput(char *input) {
    char *newline = strchr(input, '\n');
    if (newline)
        *newline = '\0';
}

#endif

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
            /* There should always be a `meta.name` but in the case there is not don't show anything and skip */
            if (meta.name) {
                char timeStr[30];

                if (meta.modifiedDate)
                    platformTimeStructToStr(meta.modifiedDate, timeStr);
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
    " ?            - Show this printout          #           - Change active site\n"
    " ! ...        - Run system command          +(>/!/#)    - Change default writing site\n"
    " COPY ...     - Queue file(s) for download  EXIT        - Close the program\n"
    " DIR/LS (...) - Full/Slim directory info    MOUNT (...) - Mount URI as site\n"
    " PWD (#)      - Print site current path     QUEUE       - List enqueued tasks\n"
    " UMOUNT #     - Unmount site                XCOPY ...   - Queue subdirectorie(s) for download\n";
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
    long a, b, i, len;
    Site *sites = siteArrayPtr(&len);

    if (!len)
        siteArrayInit();

    a = siteArrayActiveGetNth();
    b = siteArrayActiveGetWriteNth();

    for (i = 0; i < len; ++i) {
        printf("%c%c%ld:\t%s\n", i == b ? '+' : ' ', i == a ? '>' : ' ', i,
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
            case '+':
                switch (args[0][1]) {
                    case '\0': {
                        Site *site = siteArrayActiveGetWrite();
                        if (site)
                            puts(siteWorkingDirectoryGet(site));
                        else
                            goto processCommand_invalidId;
                        break;
                    }
                    case '!': /* Reverse direction */
                        l = siteArrayActiveGetWriteNth();

                        if (!(str = siteArrayActiveSetWriteNth(siteArrayActiveGetNth())))
                            siteArrayActiveSetNth(l);
                        else
                            puts(str);
                        break;
                    case '>':
                        if ((str = siteArrayActiveSetWriteNth(siteArrayActiveGetNth())))
                            puts(str);

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
                        errno = 0, l = strtol(&args[0][1], NULL, 10);
                        if (errno)
                            goto processCommand_invalidId;
                        else {
                            const char *e = siteArrayActiveSetWriteNth(l);
                            if (e)
                                puts(e);
                        }
                        break;
                    default:
                        /* TODO: Uri resolving */
                        goto processCommand_invalidId;
                }
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
#ifdef READLINE
                    clear_history();
#endif
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
                /* TODO: Uri resolving */
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

                    errno = 0, id = strtol(args[1], NULL, 10);
                    if (errno)
                        goto processCommand_invalidId;

                    if (siteArrayNthMounted(id))
                        puts(siteWorkingDirectoryGet(&siteArrayPtr(NULL)[id]));
                    else
                        goto processCommand_invalidId;
                } else
                    goto processCommand_notFound;
                break;
            case 'U':
                if (toupper(args[0][1]) == 'M' && toupper(args[0][2]) == 'O' && toupper(args[0][3]) == 'U' &&
                    toupper(args[0][4]) == 'N' && toupper(args[0][5]) == 'T') {
                    Site *site;
                    long id;

                    errno = 0, id = strtol(args[1], NULL, 10);
                    if (!errno) {
                        if (!(site = siteArrayGet(id)))
                            goto processCommand_invalidId;

                        siteFree(site), siteArrayRemove(site);
                    }
                    goto processCommand_invalidId;
                } else
                    goto processCommand_notFound;
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

processCommand_invalidId:
    puts("Invalid Mount ID");
    return;

processCommand_notFound:
    puts("Command not found. See valid commands with '?'");
}

#if __STDC_VERSION__ >= 201112L
_Noreturn
#endif

#pragma clang diagnostic push
#pragma ide diagnostic ignored "EndlessLoop"
#ifdef READLINE
static inline void InteractiveMode(void) {

    char **args;

    siteArrayInit();
    stifle_history(10);

    while (1) {
        char *input, *prompt;
        {
            long id = siteArrayActiveGetNth();
            size_t digits = id < 0 ? 2 : 1;

            while (id > 9)
                id /= 10, ++digits;

            prompt = malloc(strlen(SHELL_PS1) + digits + 1);
            sprintf(prompt, SHELL_PS1, siteArrayActiveGetNth());
        }

        input = readline(prompt), free(prompt);
        if (input) {
            args = platformArgvConvertString(input);
            add_history(input);
            free(input), input = NULL;

            if (args)
                ProcessCommand(args), platformArgvFree(args);
        }
    }
}
#else
static inline void InteractiveMode(void) {
    char input[BUFSIZ];
    char **args;

    siteArrayInit();

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
}
#endif
#pragma clang diagnostic pop

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
