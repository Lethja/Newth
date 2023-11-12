#include "platform.h"
#include "doswatb.h"
#include "../server/event.h"

#include <bios.h>
#ifdef DJGPP
#include <limits.h>
#include <sys/stat.h>
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
    const char pathDivider = '/', pathDivider2 = '\\';
    size_t a = strlen(path1), path2Jump = 0;

    if ((path1[a - 1] != pathDivider && path1[a - 1] != pathDivider2) &&
        (path2[0] != pathDivider && path2[0] != pathDivider2))
        ++path2Jump;
    else if ((path1[a - 1] == pathDivider || path1[a - 1] == pathDivider2) &&
             (path2[0] == pathDivider || path2[0] == pathDivider2))
        ++path2;

    LINEDBG;

    strcpy(output, path1);
    if (path2Jump > 1)
        output[a] = pathDivider;

    strcpy(output + a + path2Jump, path2);
}

char *platformRealPath(char *path) {
#ifdef DJGPP
    char *abs = malloc(PATH_MAX + 1);
    if(abs)
        realpath(path, abs);
    return abs;
#else /* Watcom */
    return _fullpath(NULL, path, _MAX_PATH);
#endif
}

void platformConnectSignals(void(*noAction)(int), void(*shutdownCrash)(int), void(*shutdownProgram)(int)) {
    /* See platformShouldExit() */
}

AdapterAddressArray *platformGetAdapterInformation(sa_family_t family) {
    struct AdapterAddressArray *array = malloc(sizeof(AdapterAddressArray));
    struct in_addr address;

    if (!array)
        return NULL;

    array->size = 0;
    address.s_addr = htonl(my_ip_addr);
    platformFindOrCreateAdapterIp(array, "Loaded Packet Driver", 0, inet_ntoa(address), 0);

    return array;
}

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

int platformTimeStructEquals(PlatformTimeStruct *t1, PlatformTimeStruct *t2) {
    return (t1->tm_year == t2->tm_year && t1->tm_mon == t2->tm_mon && t1->tm_mday == t2->tm_mday &&
            t1->tm_hour == t2->tm_hour && t1->tm_min == t2->tm_min && t1->tm_sec == t2->tm_sec);
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

void *platformDirOpen(char *path) {
    return opendir(path);
}

void platformDirClose(void *dirp) {
    closedir(dirp);
}

void *platformDirRead(void *dirp) {
    return readdir(dirp);
}

char *platformDirEntryGetName(PlatformDirEntry *entry, size_t *length) {
    *length = strlen(entry->d_name);
    return entry->d_name;
}

char platformDirEntryIsHidden(PlatformDirEntry *entry) {
#ifdef DJGPP
    return entry->d_name[0] == '.' ? 1 : 0;
#else /* Watcom */
    return (entry->d_attr & _A_HIDDEN);
#endif
}

char platformDirEntryIsDirectory(char *rootPath, char *webPath, PlatformDirEntry *entry) {
#ifdef DJGPP
    return (entry->d_type & DT_DIR);
#else /* Watcom */
    return (entry->d_attr & _A_SUBDIR);
#endif
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
        printf("No such directory \"%s\"\n", path);
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

    if (internal[absolutePathLen - 1] == '\\')
        internal[absolutePathLen - 1] = '\0';

    LINEDBG;

    strcpy(absolutePath, internal);

    LINEDBG;

    return 0;
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

#pragma region Watt32 Networking

int platformAcceptConnection(int fromSocket) {
    const char blocking = 0;
    socklen_t addressSize = sizeof(struct sockaddr_in);
    int clientSocket;
    struct sockaddr_in clientAddress;

    clientSocket = accept(fromSocket, (SA *) &clientAddress, &addressSize);
    platformSocketSetBlock(clientSocket, blocking);

    eventSocketAcceptInvoke(&clientSocket);

    LINEDBG;

    return clientSocket;
}

int platformSocketSetBlock(SOCKET socket, char blocking) {
    /* TODO: Get non-blocking sockets setup */
    LINEDBG;
    return 0;
}

int platformSocketGetLastError(void) {
    LINEDBG;
    return errno;
}

void platformCloseBindSockets(const SOCKET *sockets) {
    SOCKET i, max = sockets[0];

    for (i = 1; i <= max; ++i) {
        eventSocketCloseInvoke(&i);
        close(i);
    }
}

void platformIpStackExit(void) {
    LINEDBG;
}

char *platformIpStackInit(void) {
    LINEDBG;
    return NULL;
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
    LINEDBG;
    return 0;
}

#pragma endregion
