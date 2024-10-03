#include "platform.h"
#include "doswatb.h"
#include "../server/event.h"

#include <bios.h>

#ifdef DJGPP
#include <limits.h>
#include <sys/stat.h>
#include <unistd.h>
#endif

#define MAX_LISTEN 2

char platformShouldExit(void) {
    if (_bios_keybrd(1)) {
        unsigned short key = _bios_keybrd(0);
        switch (key) {
            case 0x2d00: /* Alt + X  */
            case 0x2e03: /* Ctrl + C */
            case 0x11b:  /* Esc      */
                return 0;
                break;
            default:
                return 1;
        }
    }
    return 1;
}

void platformPathCombine(char *output, const char *path1, const char *path2) {
    const char *pathDivider = "/\\";
    size_t len = strlen(path1), idx = len - 1;
    const char *p2;

    /* Copy first path into output buffer for manipulation */
    strcpy(output, path1);

    while ((output[idx] == pathDivider[0] || output[idx] == pathDivider[1]) && idx)
        --idx;

    if (idx || (output[0] == '.' && output[1] == '\0'))
        output[idx + 1] = '\0', strcat(output, &pathDivider[1]);
    else if ((output[0] == pathDivider[0] || output[idx] == pathDivider[1]) &&
             (output[1] == pathDivider[0] || output[1] == pathDivider[1]))
        output[1] = '\0';

    /* Jump over any leading dividers in the second path then concatenate it */
    len = strlen(path2), idx = 0;

    while ((path2[idx] == pathDivider[0] || path2[idx] == pathDivider[1]) && idx < len)
        ++idx;

    if (idx == len)
        return;

    p2 = &path2[idx];
    strcat(output, p2);
}

char *platformRealPath(char *path) {
#ifdef DJGPP
    char *abs;
    if ((abs = malloc(PATH_MAX + 1)))
        _fixpath(path, abs);
    return abs;
#else /* Watcom */
    return _fullpath(NULL, path, _MAX_PATH);
#endif
}

void platformConnectSignals(void(*noAction)(int), void(*shutdownCrash)(int), void(*shutdownProgram)(int)) {
    /* See platformShouldExit() */
}

#ifdef PLATFORM_SYS_EXEC

#pragma region Real Mode Exec Helper Functions & Definitions

#ifdef __I86__

#include <malloc.h>

#define DOSCMDMAX 128
#define DOSCMDPAR "/C "

typedef struct ExecParamRec {
    unsigned short int envseg; /* Segment address of environment block (DS for .COM) */
    char far *cmdline;         /* Pointer to command line string (ES:BX) */
    unsigned short int fcb1;   /* Reserved */
    unsigned short int fcb2;
} ExecParamRec;

/**
 * Check if there's enough free base memory to attempt starting another program
 * @return 0 if there's enough memory to try run a program otherwise 1 for not enough memory or -1 for unexpected result
 */
static inline char NotEnoughMemory() {
    union REGS regs;

    /* Give as much memory as possible back to DOS */
    _heapmin();

    /* Attempt to allocate an unreasonable amount of memory so INT 21:48h fails and returns free memory amount */
    regs.x.ax = 0x4800, regs.x.bx = 0xFFFF;
    intdos(&regs, &regs);

    /* This should never be the case but might be under a forgiving emulator */
    if (regs.x.cflag == 0) {
        struct SREGS sregs;

        regs.h.ah = 0x49, sregs.es = regs.x.ax;
        intdosx(&regs, &regs, &sregs);
        return -1;
    }

    if (regs.x.ax != 0x08)
        return -1;

    if (regs.x.bx >= 0x2000)
        return 0;

    return 1;
}

static inline size_t CountArgLength(const char **args) {
    size_t i, z;

    i = z = 0;
    while (args[i])
        z += strlen(args[i]) + 1, ++i;

    --z;
    return z;
}

static inline void BuildCmdTail(const char **args, char *tail) {
    size_t i;
    char *s = &tail[1];

    strcpy(s, "/C");
    for (i = 0; args[i]; ++i)
        strcat(s, " "), strcat(s, args[i]);

    i = strlen(s);
    s[i] = '\n', tail[0] = i;
}
#endif

#pragma endregion

char *platformExecRunWait(const char** args) {
#ifdef __I86__
    #pragma region Real Mode Version
    char *p;
    if (!(p = getenv("COMSPEC")))
        p = "COMMAND.COM";

    if (CountArgLength(args) < DOSCMDMAX - 5) { /* Combined size of "/C " + a[0] + '\r' + 1 must not exceed 128 */
        union REGS regs;
        struct SREGS sregs;
        ExecParamRec exec;
        char command[DOSCMDMAX];

        switch (NotEnoughMemory()) {
            case 1:
                return "Command not run due to low free memory";

            case -1:
                return "Command not run due unexpected result during free memory check";
        }

        BuildCmdTail(args, &command);

        /* Clear registers and structures */
        memset(&exec, 0, sizeof(ExecParamRec)), memset(&regs, 0, sizeof(union REGS)), memset(&sregs, 0, sizeof(struct SREGS));
        exec.cmdline = command;

        regs.x.ax = 0x4b00;            /* Exec + load */
        regs.x.dx = FP_OFF(p);         /* Offset of the command line string (DS:DX) */
        sregs.ds = FP_SEG(p);          /* Segment of the command line string (DS:DX) */
        regs.x.bx = FP_OFF(&exec);     /* Offset of the ExecParamRec structure (ES:BX) */
        sregs.es = FP_SEG(&exec);      /* Segment of the ExecParamRec structure (ES:BX) */
        intdosx(&regs, &regs, &sregs); /* 0x21 Send it! */

        /* Check COMMAND.COM ran correctly, inform the user if it didn't */
        if (regs.x.cflag != 0) {
            switch (regs.x.ax) {
                default:
                    printf("E%d: ", regs.x.ax);
                    return "COMMAND.COM did not return gracefully";
                case 2: /* File not found */
                case 3: /* Path not found */
                    return "Could not find %COMSPEC% or COMMAND.COM. Check system disk or set enviroment variable";
            }
        }

        return NULL;
    } else
        return "Command is too long";
    #pragma endregion
#else
    #pragma region Standard C Version
    char **a = (char **) args, *p = NULL;
    int i = 0;

    /* Turn args** back into a continuous string */
    while (args[i])
        p = &a[i][strlen(a[i])], *p = ' ', ++i;

    /* Find and remove newline */
    if (p) {
        *p = '\0', p = a[0];
        while (*p != '\0') {
            if (*p != '\n') {
                ++p;
                continue;
            }

            *p = '\0';
            break;
        }
    }

    system(a[0]);
    return NULL;
    #pragma endregion
#endif
}

#endif

#pragma region Network Adapter Discovery

#ifdef PLATFORM_NET_ADAPTER

AdapterAddressArray *platformGetAdapterInformation(sa_family_t family) {
    struct AdapterAddressArray *array = malloc(sizeof(AdapterAddressArray));
    struct in_addr address;

    if (!array)
        return NULL;

    array->size = 0;
    address.s_addr = htonl(my_ip_addr);
    platformFindOrCreateAdapterIp(array, "Loaded Packet Driver", 0, inet_ntoa(address));

    return array;
}

#endif

#pragma endregion

void platformGetTime(void *clock, char *time) {
    struct tm *timeInfo = gmtime(clock);
    strftime(time, 30, "%a, %d %b %Y %H:%M:%S GMT", timeInfo);
}

void platformGetCurrentTime(char *timeStr) {
    time_t rawTime;
    time(&rawTime);
    platformGetTime(&rawTime, timeStr);
}

char platformGetTimeStruct(void *clock, PlatformTimeStruct *timeStructure) {
    PlatformTimeStruct *tm = gmtime(clock);

    if (tm) {
        memcpy(timeStructure, tm, sizeof(PlatformTimeStruct));
        return 0;
    }
    return 1;
}

void platformTimeStructToStr(PlatformTimeStruct *time, char *str) {
    strftime(str, 30, "%a, %d %b %Y %H:%M:%S GMT", time);
}

void platformSleep(unsigned int ms) {
#ifdef DJGPP
    usleep((ms) * 1000);
#else /* Watcom */
    delay(ms);
#endif
}

int platformTimeStructCompare(PlatformTimeStruct *t1, PlatformTimeStruct *t2) {
    int x;

    if (t1->tm_year < t2->tm_year) return 1;
    if (t1->tm_year > t2->tm_year) return -1;

    x = ((t1->tm_mon * 31) + t1->tm_mday) - ((t2->tm_mon * 31) + t2->tm_mday);

    if (x < 0) return 1;
    if (x > 0) return -1;

    x = ((t1->tm_hour * 3600) + (t1->tm_min * 60) + t1->tm_sec) -
        ((t2->tm_hour * 3600) + (t2->tm_min * 60) + t2->tm_sec);

    if (x < 0) return 1;
    if (x > 0) return -1;

    return 0;
}

char platformTimeGetFromHttpStr(const char *str, PlatformTimeStruct *time) {
    struct tm *tm = time;
    char day[4] = "", month[4] = "";
    if (sscanf(str, "%3s, %d %3s %d %d:%d:%d GMT", day, &tm->tm_mday, month, /* NOLINT(cert-err34-c) */
               &tm->tm_year, /* NOLINT(cert-err34-c) */
               &tm->tm_hour, /* NOLINT(cert-err34-c) */
               &tm->tm_min, &tm->tm_sec)) {
        switch (toupper(day[0])) {
            case 'F':
                tm->tm_wday = 5;
                break;
            case 'M':
                tm->tm_wday = 1;
                break;
            case 'S':
                tm->tm_wday = toupper(day[1]) == 'A' ? 6 : 0;
                break;
            case 'T':
                tm->tm_wday = toupper(day[1]) == 'U' ? 2 : 4;
                break;
            case 'W':
                tm->tm_wday = 3;
                break;
            default:
                return 1;
        }

        switch (toupper(month[0])) {
            case 'A':
                tm->tm_mon = toupper(month[1]) == 'P' ? 3 : 7;
                break;
            case 'D':
                tm->tm_mon = 11;
                break;
            case 'F':
                tm->tm_mon = 1;
                break;
            case 'J':
                tm->tm_mon = toupper(month[1]) == 'A' ? 0 : toupper(month[2]) == 'L' ? 6 : 5;
                break;
            case 'M':
                tm->tm_mon = toupper(month[2]) == 'R' ? 2 : 4;
                break;
            case 'N':
                tm->tm_mon = 10;
                break;
            case 'O':
                tm->tm_mon = 9;
                break;
            case 'S':
                tm->tm_mon = 8;
                break;
            default:
                return 1;
        }

        tm->tm_year -= 1900;
        return 0;
    }
    return 1;
}

PlatformDir *platformDirOpen(char *path) {
    PlatformDir *self;
    DIR *d = opendir(path);
    char *p;

    if (!d)
        return NULL;

    if (!(self = malloc(sizeof(PlatformDir)))) {
        closedir(d);
        return NULL;
    }

    if (!(p = platformRealPath(path))) {
        closedir(d), free(self);
        return NULL;
    }

    self->dir = d, self->path = p;
    return self;
}

void platformDirClose(PlatformDir *self) {
    closedir(self->dir);
    free(self->path);
}

const char *platformDirPath(PlatformDir *self) {
    return self->path;
}

PlatformDirEntry *platformDirRead(PlatformDir *self) {
    return readdir(self->dir);
}

const char *platformDirEntryGetName(PlatformDirEntry *entry, size_t *length) {
    *length = strlen(entry->d_name);
    return entry->d_name;
}

static inline void ParseFileSchemePathToDos(char *absolutePath) {
    if (absolutePath[0] == '/' && absolutePath[1] != '\0' && absolutePath[2] == '/')
        absolutePath[0] = toupper(absolutePath[1]), absolutePath[1] = ':';

    while (*absolutePath != '\0')
        *absolutePath = *absolutePath == '/' ? '\\' : toupper(*absolutePath), ++absolutePath;
}

static inline void ParseDosToFileScheme(char *absolutePath) {
    if (absolutePath[1] == ':')
        absolutePath[1] = tolower(absolutePath[0]), absolutePath[0] = '/';

    absolutePath = &absolutePath[2];
    while (*absolutePath != '\0')
        *absolutePath = *absolutePath == '\\' ? '/' : tolower(*absolutePath), ++absolutePath;
}

char *platformPathSystemToFileScheme(char *path) {
    char *r, *abs = platformRealPath(path);
    size_t absLen;

    if (!abs || !(absLen = strlen(abs)) || !(r = malloc(absLen + 8)))
        return NULL;

    ParseDosToFileScheme(abs);
    strcpy(r, "file://"), strcat(r, abs), free(abs);
    return r;
}

char *platformPathFileSchemePathToSystem(char *path) {
    char *r;
    size_t len = strlen(path);

    if (len < 3) {
        /* Allow omitting of the trailing slash on a drives root path */
        if (len == 2 && path[0] == '/') {
            char drive = toupper(path[1]);

            /* Check if the remaining character is within the range of a DOS drive */
            if (drive < 'A' || drive > 'Z') {
                errno = ENOENT;
                return NULL;
            }

            if ((r = malloc(4)))
                r[0] = drive, r[1] = ':', r[2] = '\\', r[3] = '\0';

            return r;
        } else
            return NULL;
    }

    if ((r = malloc(len + 1))) {
        char drive;
        strcpy(r, path), drive = toupper(r[1]);
        ParseFileSchemePathToDos(r);
        if (r[0] == drive && r[1] == ':' && r[2] == '\\')
            return r;

        free(r);
        return NULL;
    }

    return r;
}

char *platformPathFileSchemeToSystem(char *path) {
    if (!(toupper(path[0]) == 'F' && toupper(path[1]) == 'I' && toupper(path[2]) == 'L' && toupper(path[3]) == 'E'
          && path[4] == ':' && path[5] == '/' && path[6] == '/' && path[7] == '/'))
        return NULL;
    else
        return platformPathFileSchemePathToSystem(&path[7]);
}

int platformPathSystemChangeWorkingDirectory(const char *path) {
    return chdir(path);
}

char platformDirEntryIsHidden(PlatformDirEntry *entry) {
#ifdef DJGPP
    return entry->d_name[0] == '.' ? 1 : 0;
#else /* Watcom */
    return (entry->d_attr & _A_HIDDEN);
#endif
}

char platformDirEntryIsDirectory(PlatformDirEntry *entry, PlatformDir *dirp, PlatformFileStat **st) {
#ifdef DJGPP
    PlatformDir *pd = dirp;
    if (!st) {
        struct stat s;
        char tmp1[PATH_MAX] = {0}, tmp2[PATH_MAX];
        strncpy((char *) &tmp1, pd->path, PATH_MAX - 1);
        platformPathCombine((char *) &tmp2, (char *) &tmp1, entry->d_name);
        _fixpath((char *) &tmp2, (char *) &tmp1);
        if (!platformFileStat((char *) &tmp1, &s))
            return platformFileStatIsDirectory(&s);
    } else
        return platformFileStatIsDirectory(*st);

    return 0;
#else /* Watcom */
    return (entry->d_attr & _A_SUBDIR);
#endif
}

char platformDirEntryGetStats(PlatformDirEntry *entry, PlatformDir *dirP, PlatformFileStat *st) {
    const char *p = platformDirPath(dirP);
    char *tmp;

    if ((tmp = malloc(strlen(p) + strlen(entry->d_name) + 1))) {
        platformPathCombine(tmp, p, entry->d_name);
        if (!platformFileStat(tmp, st)) {
            free(tmp);
            return 1;
        }

        free(tmp);
    }

    return 0;
}

int platformFileStat(const char *path, PlatformFileStat *fileStat) {
    return stat(path, fileStat);
}

char platformFileStatIsDirectory(PlatformFileStat *stat) {
    return S_ISDIR(stat->st_mode);
}

char platformFileStatIsFile(PlatformFileStat *stat) {
    return S_ISREG(stat->st_mode);
}

char *platformGetDiskPath(char *path) {
    char *test;
    if (strlen(path) == 1 || (strlen(path) < 4 && path[1] == ':' && path[2] == '\\')) {
        if (isupper(*path)) {
            test = malloc(5); /* +1 is intended */
            if (test) {
                test[0] = *path, test[1] = ':', test[2] = '\\', test[3] = '\0';
                return test;
            }
        }
    }

    test = platformRealPath(path);
    if (!test) {
        printf("No such directory or drive as \"%s\"\n", path);
        exit(1);
    }

    return test;
}

char *platformGetWorkingDirectory(char *buffer, size_t length) {
    return getcwd(buffer, length);
}

short platformPathWebToSystem(const char *rootPath, char *webPath, char *absolutePath) {
    size_t absolutePathLen = strlen(rootPath) + strlen(webPath) + 1, i;
    char internal[FILENAME_MAX];

    LINEDBG;

    if (absolutePathLen >= FILENAME_MAX)
        return 500;

    platformPathCombine(internal, rootPath, webPath);

    LINEDBG;

    for (i = 0; internal[i] != '\0'; ++i) {
        if (internal[i] == '/')
            internal[i] = '\\';
    }

    if (internal[i - 1] == '\\')
        internal[i - 1] = '\0';

    LINEDBG;

    strcpy(absolutePath, internal);

    LINEDBG;

    return 0;
}

int platformFileAtEnd(PlatformFile stream) {
    return feof(stream);
}

PlatformFile platformFileOpen(const char *fileName, const char *fileMode) {
    return fopen(fileName, fileMode);
}

int platformFileClose(PlatformFile stream) {
    return fclose(stream);
}

int platformFileSeek(PlatformFile stream, PlatformFileOffset offset, int whence) {
#ifdef DJGPP
    return fseek(stream, offset, whence);
#else /* Watcom */
    return _fseeki64(stream, offset, whence);
#endif
}

PlatformFileOffset platformFileTell(PlatformFile stream) {
#ifdef DJGPP
    return ftell(stream);
#else /* Watcom */
    return _ftelli64(stream);
#endif
}

size_t platformFileRead(void *buffer, size_t size, size_t n, PlatformFile stream) {
    return fread(buffer, size, n, stream);
}

#ifdef PLATFORM_SYS_WRITE

size_t platformFileWrite(void *buffer, size_t size, size_t n, PlatformFile stream) {
    return fwrite(buffer, size, n, stream);
}

#endif

const char *platformTempDirectoryGet(void) {
    const char *tmp, *fallback = ".";
    struct stat st;

    if ((tmp = getenv("TEMP")) || (getenv("TMP")) || (tmp = getenv("TMPDIR")))
        return tmp;

    if (!stat(fallback, &st)) {
        if (S_ISDIR(st.st_mode))
            return fallback;
    }

    return NULL;
}

char *platformPathLast(const char *path) {
    const char *p = NULL;
    size_t i;

    for (i = 0; path[i] != '\0'; ++i) {
        if (path[i] == '/' || path[i] == '\\') {
            if (path[i + 1] == '\0') {
                if (p && i) {
                    char *a;

                    i = strlen(p);
                    if ((a = malloc(i)))
                        --i, memcpy(a, p, i), a[i] = '\0'; /* Remove trailing '/' */
                    return a;
                }
                return NULL; /* Never return "/" */
            } else
                p = &path[i + 1];
        }
    }

    return (char *) p;
}

#pragma region Watt32 Networking

#pragma region Network Bind & Listen

#ifdef PLATFORM_NET_LISTEN

int platformAcceptConnection(int fromSocket) {
    const char blocking = 0;
    socklen_t addressSize = sizeof(struct sockaddr_in);
    int clientSocket;
    struct sockaddr_in clientAddress;

    clientSocket = accept(fromSocket, (struct sockaddr *) &clientAddress, &addressSize);
    platformSocketSetBlock(clientSocket, blocking);

    eventSocketAcceptInvoke(&clientSocket);

    LINEDBG;

    return clientSocket;
}

void platformCloseBindSockets(const SOCKET *sockets) {
    SOCKET i, max = sockets[0];

    for (i = 1; i <= max; ++i) {
        eventSocketCloseInvoke(&i);
        close(i);
    }
}

SOCKET *platformServerStartup(sa_family_t family, char *ports, char **err) {
    SOCKET *r;
    SOCKET listenSocket;
    struct sockaddr_storage serverAddress;
    memset(&serverAddress, 0, sizeof(struct sockaddr_storage));

    LINEDBG;

    switch (family) {
        default:
        case AF_INET:
        case AF_UNSPEC: {
            struct sockaddr_in *sock = (struct sockaddr_in *) &serverAddress;
            if ((listenSocket = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
                *err = "Unable to acquire IPV4 socket from system";
                return NULL;
            }

            sock->sin_family = AF_INET;
            sock->sin_addr.s_addr = htonl(INADDR_ANY);
            LINEDBG;
            break;
        }
    }

    if (platformBindPort(&listenSocket, (struct sockaddr *) &serverAddress, ports)) {
        *err = "Unable to bind to designated port numbers";
        return NULL;
    }

    if ((listen(listenSocket, MAX_LISTEN)) < 0) {
        *err = "Unable to listen to assigned socket";
        return NULL;
    }

    r = malloc(sizeof(SOCKET) * 2);
    if (r) {
        r[0] = 1, r[1] = listenSocket;

        LINEDBG;

        return r;
    } else
        *err = strerror(errno);

    return NULL;
}

#endif /* PLATFORM_NET_LISTEN */

#pragma endregion

int platformSocketSetBlock(SOCKET socket, char blocking) {
    /* TODO: Get non-blocking sockets setup */
    LINEDBG;
    return 0;
}

int platformSocketGetLastError(void) {
    LINEDBG;
    return errno;
}

void platformIpStackExit(void) {
    LINEDBG;
}

char *platformIpStackInit(void) {
    LINEDBG;
    return NULL;
}

void platformGetIpString(struct sockaddr *addr, char ipStr[INET6_ADDRSTRLEN], sa_family_t *family) {
    *family = addr->sa_family;
    if (addr->sa_family == AF_INET) {
        struct sockaddr_in *s4 = (struct sockaddr_in *) addr;
        char *ip = inet_ntoa(s4->sin_addr);
        strcpy(ipStr, ip);
    } else
        strcpy(ipStr, "???");

    LINEDBG;
}

unsigned short platformGetPort(struct sockaddr *addr) {
    if (addr->sa_family == AF_INET) {
        struct sockaddr_in *s4 = (struct sockaddr_in *) addr;
        LINEDBG;
        return ntohs(s4->sin_port);
    }

    return 0;
}

int platformOfficiallySupportsIpv6(void) {
    return 0;
}

#pragma endregion
