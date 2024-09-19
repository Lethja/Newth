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

#define QUEUE_PRINT "%s %s > %s\n"

SiteArray siteArray = {0};
QueueEntryArray *queueEntryArray = NULL;

static inline void Copy(const char **argv) {
    const char *e;
    QueueEntry entry;
    size_t i = 0;

    while (argv[i])
        ++i;

    if (i > 2)
        puts("COPY from/to not yet implemented");
    else {
        char *s = siteArrayUserPathResolve(&siteArray, argv[1], 0), *d;

        if (!s) {
            puts(ErrUnableToResolvePath);
            return;
        }

        {
            Site *w = siteArrayActiveGetWrite(&siteArray);
            char *l;

            if (!w) {
                free(s);
                puts(ErrUnableToGetSystemsWorkingDirectory);
                return;
            }

            if ((l = uriPathLast(s))) {
                UriDetails u = uriDetailsNewFrom(siteWorkingDirectoryGet(w));
                char *p;

                p = u.path ? uriPathAbsoluteAppend(u.path, l) : NULL;
                if (l < s || l > &s[strlen(s) + 1])
                    free(l);

                free(u.path), u.path = p ? p : NULL;
                d = u.path ? uriDetailsCreateString(&u) : NULL;
                uriDetailsFree(&u);
            } else
                d = NULL;
        }

        if (!d) {
            free(s);
            puts(ErrUnableToGetSystemsWorkingDirectory);
            return;
        }

        if ((e = queueEntryNewFromPath(&entry, &siteArray, s, d))) {
            free(s), free(d);
            puts(e);
            return;
        }

        entry.state = QUEUE_STATE_QUEUED;
        if ((e = queueEntryArrayAppend(&queueEntryArray, &entry))) {
            free(s), free(d);
            puts(e);
            return;
        }

        printf("QUEUE #%ld: " QUEUE_PRINT, queueEntryArray->len - 1, "COPY", s, d);

        free(s), free(d);
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
    unsigned long i;
    int width;

    if (!queueEntryArray) {
        puts("No queue entries");
        return;
    }

    /* Get the width of the longest number */
    {
        unsigned long l = queueEntryArray->len;
        width = 1;

        while (l > 0)
            l /= 10, ++width;
    }

    for (i = 0; i < queueEntryArray->len; ++i) {
        QueueEntry *entry = &queueEntryArray->entry[i];
        UriDetails d = uriDetailsNewFrom(siteWorkingDirectoryGet(entry->sourceSite));
        char *src, *dst;

        if (d.path)
            free(d.path);

        d.path = entry->sourcePath;
        src = uriDetailsCreateString(&d), d.path = NULL, uriDetailsFree(&d);

        if (!src)
            goto ListQueue_LoopError1;

        d = uriDetailsNewFrom(siteWorkingDirectoryGet(entry->destinationSite));

        if (d.path)
            free(d.path);

        d.path = entry->destinationPath;
        dst = uriDetailsCreateString(&d), d.path = NULL, uriDetailsFree(&d);

        if (!dst)
            goto ListQueue_LoopError2;

        printf(" %*ld: " QUEUE_PRINT, width, i, entry->state & QUEUE_TYPE_RECURSIVE ? "XCOPY" : "COPY", src, dst);
        free(src), free(dst);
        continue;

        ListQueue_LoopError2:
        free(src);

        ListQueue_LoopError1:
        uriDetailsFree(&d);
        printf("%lu: data corruption\n", i);
    }
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

    printf(" Site: %ld\n Path: '%s'\n\n", siteArrayActiveGetNth(&siteArray), pathRes);

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

static inline void PrintDirectoryFilesUnixLike(Site *site, char *path) {
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
    const char *legend = " # = id, ... = parameter(s), () = parameter is optional."
                         " All commands are case-insensitive.\n";
    const char *help = " ?            - Show this printout          #|>...      - Change active site\n"
                       " ! ...        - Run system command          +(>|!|#)    - Change default writing site\n"
                       " COPY ...     - Queue file(s) for download  EXIT        - Close the program\n"
                       " DIR|LS (...) - Full|Slim directory info    MOUNT (...) - Mount URI as site\n"
                       " PWD (#)      - Print site current path     QUEUE (...) - List enqueued tasks\n"
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

    if ((err = siteNew(&site, type, parameter)) || (err = siteArrayAdd(&siteArray, &site)))
        return err;

    siteArrayActiveSet(&siteArray, &site);
    return NULL;
}

static inline void MountList(void) {
    long a, b, i, len;
    Site *sites = siteArrayPtr(&siteArray, &len);

    if (!len)
        siteArrayInit(&siteArray);

    a = siteArrayActiveGetNth(&siteArray);
    b = siteArrayActiveGetWriteNth(&siteArray);

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
            case '>':
                if (args[0][1] != '\0') {
                    if ((l = siteArrayGetFromInputNth(&siteArray, &args[0][1])) != -1) {
                        if ((str = siteArrayActiveSetNth(&siteArray, l)))
                            puts(str);
                    } else
                        goto processCommand_invalidId;
                }
                break;
            case '+':
                switch (args[0][1]) {
                    case '\0': {
                        Site *site = siteArrayActiveGetWrite(&siteArray);
                        if (site)
                            puts(siteWorkingDirectoryGet(site));
                        else
                            goto processCommand_invalidId;
                        break;
                    }
                    case '!': /* Reverse direction */
                        l = siteArrayActiveGetWriteNth(&siteArray);

                        if (!(str = siteArrayActiveSetWriteNth(&siteArray, siteArrayActiveGetNth(&siteArray))))
                            siteArrayActiveSetNth(&siteArray, l);
                        else
                            puts(str);
                        break;
                    case '>':
                        if ((str = siteArrayActiveSetWriteNth(&siteArray, siteArrayActiveGetNth(&siteArray))))
                            puts(str);

                        break;
                    default:
                        if ((l = siteArrayGetFromInputNth(&siteArray, &args[0][1])) != -1) {
                            const char *e = siteArrayActiveSetWriteNth(&siteArray, l);
                            if (e)
                                puts(e);
                        } else
                            goto processCommand_invalidId;
                }
                break;
            case 'D':
                if (toupper(args[0][1]) == 'I' && toupper(args[0][2]) == 'R') {
                    Site *site = siteArrayActiveGet(&siteArray);
                    if (site)
                        PrintDirectoryFilesDosLike(site, NULL);
                    else
                        goto processCommand_invalidId;
                } else
                    goto processCommand_notFound;
                break;
            case 'E':
                if (toupper(args[0][1]) == 'X' && toupper(args[0][2]) == 'I' && toupper(args[0][3]) == 'T') {
                    siteArrayFree(&siteArray);
#ifdef READLINE
                    clear_history();
#endif
                    exit(0);
                } else
                    goto processCommand_notFound;
            case 'L':
                if (toupper(args[0][1]) == 'S') {
                    Site *site = siteArrayActiveGet(&siteArray);
                    if (site)
                        PrintDirectoryFilesUnixLike(siteArrayActiveGet(&siteArray), NULL);
                    else
                        goto processCommand_invalidId;
                } else
                    goto processCommand_notFound;
                break;
            case 'M':
                if (toupper(args[0][1]) == 'O' && toupper(args[0][2]) == 'U' && toupper(args[0][3]) == 'N' &&
                    toupper(args[0][4]) == 'T')
                    MountList();
                break;
            case 'P':
                if (toupper(args[0][1]) == 'W' && toupper(args[0][2]) == 'D')
                    puts(siteWorkingDirectoryGet(siteArrayActiveGet(&siteArray)));
                else
                    goto processCommand_notFound;
                break;
            case 'Q':
                if (toupper(args[0][1]) == 'U' && toupper(args[0][2]) == 'E' && toupper(args[0][3]) == 'U' &&
                    toupper(args[0][4]) == 'E')
                    /* TODO: Subcommands */
                    ListQueue();
                else
                    goto processCommand_notFound;
                break;
            case 'U':
                if (toupper(args[0][1]) == 'M' && toupper(args[0][2]) == 'O' && toupper(args[0][3]) == 'U' &&
                    toupper(args[0][4]) == 'N' && toupper(args[0][5]) == 'T') {
                    Site *site;

                    if (!(site = siteArrayActiveGet(&siteArray)))
                        return;

                    siteFree(site), siteArrayRemove(&siteArray, site);
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
                    if ((str = siteArrayActiveSetNth(&siteArray, l)))
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
                    if (siteWorkingDirectorySet(siteArrayActiveGet(&siteArray), args[1]))
                        puts(strerror(errno));
                } else if (toupper(args[0][1]) == 'P' ||
                           (toupper(args[0][1]) == 'O' && toupper(args[0][2]) == 'P' && toupper(args[0][3]) == 'Y'))
                    Copy((const char **) args);
                else
                    goto processCommand_notFound;
                break;
            case 'D':
                if (toupper(args[0][1]) == 'I' && toupper(args[0][2]) == 'R')
                    PrintDirectoryFilesDosLike(siteArrayActiveGet(&siteArray), args[1]);
                else
                    goto processCommand_notFound;
                break;
            case 'L':
                if (toupper(args[0][1]) == 'S')
                    PrintDirectoryFilesUnixLike(siteArrayActiveGet(&siteArray), args[1]);
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

                    if (siteArrayNthMounted(&siteArray, id))
                        puts(siteWorkingDirectoryGet(&siteArrayPtr(&siteArray, NULL)[id]));
                    else
                        goto processCommand_invalidId;
                } else
                    goto processCommand_notFound;
                break;
            case 'U':
                if (toupper(args[0][1]) == 'M' && toupper(args[0][2]) == 'O' && toupper(args[0][3]) == 'U' &&
                    toupper(args[0][4]) == 'N' && toupper(args[0][5]) == 'T') {
                    Site *site = siteArrayGetFromInput(&siteArray, args[1]);

                    if (site)
                        siteFree(site), siteArrayRemove(&siteArray, site);
                    else
                        goto processCommand_invalidId;
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

    processCommand_invalidId:
    puts("Invalid site. See currently valid sites with 'MOUNT'");
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

    siteArrayInit(&siteArray);
    stifle_history(10);

    while (1) {
        char *input, *prompt;
        {
            long id = siteArrayActiveGetNth(&siteArray);
            size_t digits = id < 0 ? 2 : 1;

            while (id > 9)
                id /= 10, ++digits;

            prompt = malloc(strlen(SHELL_PS1) + digits + 1);
            sprintf(prompt, SHELL_PS1, siteArrayActiveGetNth(&siteArray));
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

    siteArrayInit(&siteArray);

    while (1) {
        printf(SHELL_PS1, siteArrayActiveGetNth(&siteArray));
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

    err = platformIpStackInit();
    if (err) {
        puts(err);
        return 1;
    }

    platformConnectSignals(signalNoAction, NULL, NULL);

#ifndef _WIN32
    if (isatty(fileno(stdout)))
#endif /* _WIN32 */
        InteractiveMode();

    return 0;
}
