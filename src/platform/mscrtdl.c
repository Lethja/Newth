#include "mscrtdl.h"
#include "../server/event.h"
#include "../common/debug.h"

#ifdef PORTABLE_WIN32

#include "winsock/wsock1.h"

#endif

#include <ctype.h>
#include <iphlpapi.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>

typedef AdapterAddressArray *(*adapterInformationIpv6)(sa_family_t family,
                                                       void (*arrayAdd)(AdapterAddressArray *, char *, sa_family_t,
                                                                        char *), void (*nTop)(const void *, char *));

typedef AdapterAddressArray *(*adapterInformationIpv4)(
        void (*arrayAdd)(AdapterAddressArray *, char *, sa_family_t, char *));

adapterInformationIpv4 getAdapterInformationIpv4 = NULL;
adapterInformationIpv6 getAdapterInformationIpv6 = NULL;
HMODULE wsIpv4 = NULL;
HMODULE wsIpv6 = NULL;
WSADATA wsaData;

char *platformPathCombine(char *path1, char *path2) {
    const char pathDivider = '/', pathDivider2 = '\\';
    size_t a = strlen(path1), b = strlen(path2), path2Jump = 0;
    char *returnPath;
    DEBUGPRT("platformPathCombine(%s, %s)", path1, path2);

    if ((path1[a - 1] != pathDivider && path1[a - 1] != pathDivider2) &&
        (path2[0] != pathDivider && path2[0] != pathDivider2))
        ++path2Jump;
    else if ((path1[a - 1] == pathDivider || path1[a - 1] == pathDivider2) &&
             (path2[0] == pathDivider || path2[0] == pathDivider2))
        ++path2;

    DEBUGPRT("platformPathCombine:path1 = %s\nplatformPathCombine:path2 = %s\nplatformPathCombine:path2Jump = %u\n)",
             path1, path2, path2Jump);
    returnPath = malloc(a + b + path2Jump);
    memcpy(returnPath, path1, a);
    if (path2Jump > 1)
        returnPath[a] = pathDivider;

    memcpy(returnPath + a + path2Jump, path2, b + 1);

    return returnPath;
}

void platformCloseBindSockets(fd_set *sockets, SOCKET max) {
    SOCKET i;
    for (i = 0; i <= max; i++) {
        if (FD_ISSET(i, sockets)) {
            eventSocketCloseInvoke(&i);
            closesocket(i);
        }
    }
    WSACleanup();
}

#pragma clang diagnostic push
#pragma ide diagnostic ignored "ConstantParameter"

static char platformVersionAbove(int major, int minor) {
    DWORD dwVersion;
    DWORD dwMajorVersion;
    DWORD dwMinorVersion;

    dwVersion = GetVersion();

    dwMajorVersion = (DWORD) (LOBYTE(LOWORD(dwVersion)));
    dwMinorVersion = (DWORD) (HIBYTE(LOWORD(dwVersion)));

    if (dwMajorVersion > major)
        return 2;
    else if (major == dwMajorVersion && dwMinorVersion >= minor)
        return 1;
    else
        return 0;
}

#pragma clang diagnostic pop

void platformIpStackExit(void) {
    WSACleanup();
    if (wsIpv6)
        FreeLibrary(wsIpv6);
}

int platformIpStackInit(void) {
    int error;
    char libAbs[FILENAME_MAX] = "", *filePoint;
    DEBUGPRT("%s ", "platformIPStackInit");

    GetModuleFileName(NULL, libAbs, FILENAME_MAX);
    filePoint = strrchr(libAbs, '\\') + 1;
    DEBUGPRT("libAbs = %s\nfilePoint = %s", libAbs, filePoint);
    if (!filePoint)
        return 1;

    /* Runtime link. Dual-stack functions that are not available in all 32 bit versions of Windows */
    *filePoint = '\0';
    strncat(filePoint, "thwsipv6.dll", FILENAME_MAX);
    wsIpv6 = LoadLibrary(TEXT(libAbs));
    DEBUGPRT("wsIpv6 (%s) = %p", libAbs, (void *) wsIpv6);
    if (wsIpv6)
        getAdapterInformationIpv6 = (adapterInformationIpv6) GetProcAddress(wsIpv6,
                                                                            "platformGetAdapterInformationIpv6");

    /* Choose between WinSock 1 (Windows 95 support) and WinSock 2 */
    *filePoint = '\0';
    strncat(filePoint, "thwsock2.dll", FILENAME_MAX);
    wsIpv4 = LoadLibrary(TEXT(libAbs));
    DEBUGPRT("wsIpv4 (%s) = %p", libAbs, (void *) wsIpv4);
    if (wsIpv4)
        getAdapterInformationIpv4 = (adapterInformationIpv4) GetProcAddress(wsIpv4,
                                                                            "platformGetAdapterInformationIpv4");

#ifdef PORTABLE_WIN32
    else {
        getAdapterInformationIpv4 = platformGetAdapterInformationIpv4;
        DEBUGPRT("getAdapterInformationIpv4 (Internal) = %p", getAdapterInformationIpv4);
    }
#endif

    if (!getAdapterInformationIpv4) {
        char err[255] = "";
        FormatMessage(FORMAT_MESSAGE_FROM_SYSTEM, NULL, GetLastError(), MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), err,
                      255, NULL);
        DEBUGPRT("%s %s\n", "No Ipv4 handler could be established:", err);
        return 1;
    }

    DEBUGPRT("%s\n","WSAStartup() about to be called");
    error = WSAStartup(MAKEWORD(1, 1), &wsaData);
    DEBUGPRT("WSAStartup() = %d\n", error);

    if(error) {
        error = WSAGetLastError();
        DEBUGPRT("WSAStartup() failed with error %d\n", error);
    }

    return error;
}

int platformOfficiallySupportsIpv6(void) {
    if (platformVersionAbove(6, 0))
        return 1;
    return 0;
}

int platformServerStartup(SOCKET *listenSocket, sa_family_t family, char *ports) {
    struct sockaddr_storage serverAddress;
    int iResult;

    DEBUGPRT("%s", "platformServerStartup()");
    /* Force start in IPV4 mode if IPV6 functions cannot be loaded */
    if (family != AF_INET && (!getAdapterInformationIpv6)) {
        if (family == AF_INET6) {
            puts("This build of Windows does not have sufficient IPv6 support");
            return 1;
        }
        family = AF_INET;
    }

    ZeroMemory(&serverAddress, sizeof(serverAddress));

    switch (family) {
        default:
        case AF_INET: {
            struct sockaddr_in *sock = (struct sockaddr_in *) &serverAddress;
            if ((*listenSocket = socket(AF_INET, SOCK_STREAM, 0)) < 0)
                return 1;

            sock->sin_family = AF_INET;
            sock->sin_addr.s_addr = htonl(INADDR_ANY);
            break;
        }
        case AF_INET6:
        case AF_UNSPEC: {
            struct sockaddr_in6 *sock = (struct sockaddr_in6 *) &serverAddress;
            size_t v6Only = family == AF_INET6;
            if ((*listenSocket = socket(AF_INET6, SOCK_STREAM, 0)) < 0)
                return 1;

            sock->sin6_family = AF_INET6;
            setsockopt(*listenSocket, IPPROTO_IPV6, IPV6_V6ONLY, (char *) &v6Only, sizeof(v6Only));
            break;
        }
    }

    if (*listenSocket == INVALID_SOCKET) {
        return 1;
    }

    if (platformBindPort(listenSocket, (struct sockaddr *) &serverAddress, ports)) {
        closesocket(*listenSocket);
        return 1;
    }

    iResult = listen(*listenSocket, SOMAXCONN);
    if (iResult == SOCKET_ERROR) {
        closesocket(*listenSocket);
        return 1;
    }

    return 0;
}

#pragma clang diagnostic push
#pragma ide diagnostic ignored "UnusedParameter"

void platformConnectSignals(void(*noAction)(int), void(*shutdownCrash)(int), void(*shutdownProgram)(int)) {
#pragma clang diagnostic pop
    signal(SIGSEGV, shutdownCrash);
    signal(SIGBREAK, shutdownProgram);
    signal(SIGINT, shutdownProgram);
    signal(SIGTERM, shutdownProgram);
}

SOCKET platformAcceptConnection(SOCKET fromSocket) {
    socklen_t addrSize = sizeof(struct sockaddr_storage);
    SOCKET clientSocket;
    struct sockaddr_storage clientAddress;

    clientSocket = accept(fromSocket, (SA *) &clientAddress, &addrSize);

    eventSocketAcceptInvoke(&clientSocket);

    return clientSocket;
}

void platformPathForceForwardSlash(char *path) {
    while (*path != '\0') {
        if (*path == '\\')
            *path = '/';
        ++path;
    }
}

void platformPathForceBackwardSlash(char *path) {
    while (*path != '\0') {
        if (*path == '/')
            *path = '\\';
        ++path;
    }
}

void ipv6NTop(const void *inAddr6, char *ipStr) {
    const unsigned char *a = inAddr6;
    char buf[100];
    size_t i, j, max, best;

    if (memcmp(a, "\0\0\0\0\0\0\0\0\0\0\377\377", 12) != 0)
        snprintf(buf, sizeof(buf), "%x:%x:%x:%x:%x:%x:%x:%x", 256 * a[0] + a[1], 256 * a[2] + a[3], 256 * a[4] + a[5],
                 256 * a[6] + a[7], 256 * a[8] + a[9], 256 * a[10] + a[11], 256 * a[12] + a[13], 256 * a[14] + a[15]);
    else
        snprintf(buf, sizeof(buf), "%x:%x:%x:%x:%x:%x:%d.%d.%d.%d", 256 * a[0] + 256 * a[1], 256 * a[2] + a[3],
                 256 * a[4] + a[5], 256 * a[6] + a[7], 256 * a[8] + a[9], 256 * a[10] + a[11], a[12], a[13], a[14],
                 a[15]);

    for (i = best = 0, max = 2; buf[i]; ++i) {
        if (i && buf[i] != ':')
            continue;
        j = strspn(buf + i, ":0");
        if (j > max) best = i, max = j;
    }

    if (max > 2) {
        buf[best] = buf[best + 1] = ':';
        memmove(buf + best + 2, buf + best + max, i - best - max + 1);
    }

    strcpy(ipStr, buf);
}

void platformGetIpString(struct sockaddr *addr, char ipStr[INET6_ADDRSTRLEN], sa_family_t *family) {
    if (addr->sa_family == AF_INET) {
        struct sockaddr_in *s4 = (struct sockaddr_in *) addr;
        char *ip = inet_ntoa(s4->sin_addr);
        *family = AF_INET;
        strcpy(ipStr, ip);
    } else if (addr->sa_family == AF_INET6) {
        const char ipv4[8] = "::ffff:";
        struct sockaddr_in6 *s6 = (struct sockaddr_in6 *) addr;
        ipv6NTop(&s6->sin6_addr, ipStr);
        if (strncmp(ipStr, ipv4, sizeof(ipv4) - 1) == 0) {
            memmove(ipStr, &ipStr[7], INET_ADDRSTRLEN);
            *family = AF_INET;
        } else
            *family = AF_INET6;
    } else {
        strcpy(ipStr, "???");
    }
}

unsigned short platformGetPort(struct sockaddr *addr) {
    if (addr->sa_family == AF_INET) {
        struct sockaddr_in *s4 = (struct sockaddr_in *) addr;
        return ntohs(s4->sin_port);
    } else if (addr->sa_family == AF_INET6) {
        struct sockaddr_in *s6 = (struct sockaddr_in *) addr;
        return ntohs(s6->sin_port);
    }

    return 0;
}

AdapterAddressArray *platformGetAdapterInformation(sa_family_t family) {
    switch (family) {
        case AF_UNSPEC:
            if (getAdapterInformationIpv6)
                case AF_INET6:
                    return getAdapterInformationIpv6(family, platformFindOrCreateAdapterIp, ipv6NTop);
        default:
        case AF_INET:
            return getAdapterInformationIpv4(platformFindOrCreateAdapterIp);
    }
}

char *platformRealPath(char *path) {
    char *buf = malloc(MAX_PATH);
    DWORD e = GetFullPathName(path, MAX_PATH, buf, NULL);

    if (e != 0)
        return buf;

    free(buf);
    return NULL;
}

static void systemTimeToStr(SYSTEMTIME *timeStruct, char *timeStr) {
    char day[4], month[4];
    switch (timeStruct->wDayOfWeek) {
        case 0:
            memcpy(&day, "Sun", 4);
            break;
        case 1:
            memcpy(&day, "Mon", 4);
            break;
        case 2:
            memcpy(&day, "Tue", 4);
            break;
        case 3:
            memcpy(&day, "Wed", 4);
            break;
        case 4:
            memcpy(&day, "Thu", 4);
            break;
        case 5:
            memcpy(&day, "Fri", 4);
            break;
        case 6:
            memcpy(&day, "Sat", 4);
            break;
    }

    switch (timeStruct->wMonth) {
        case 1:
            memcpy(&month, "Jan", 4);
            break;
        case 2:
            memcpy(&month, "Feb", 4);
            break;
        case 3:
            memcpy(&month, "Mar", 4);
            break;
        case 4:
            memcpy(&month, "Apr", 4);
            break;
        case 5:
            memcpy(&month, "May", 4);
            break;
        case 6:
            memcpy(&month, "Jun", 4);
            break;
        case 7:
            memcpy(&month, "Jul", 4);
            break;
        case 8:
            memcpy(&month, "Aug", 4);
            break;
        case 9:
            memcpy(&month, "Sep", 4);
            break;
        case 10:
            memcpy(&month, "Oct", 4);
            break;
        case 11:
            memcpy(&month, "Nov", 4);
            break;
        case 12:
            memcpy(&month, "Dec", 4);
            break;
    }

    snprintf(timeStr, 30, "%s, %02hu %s %04hu %02hu:%02hu:%02hu GMT", day, timeStruct->wDay, month, timeStruct->wYear,
             timeStruct->wHour, timeStruct->wMinute, timeStruct->wSecond);
    DEBUGPRT("timeStr = '%s'", timeStr);
}

void platformGetTime(void *clock, char *timeStr) {
    SYSTEMTIME timeStruct;
    FileTimeToSystemTime(clock, &timeStruct);
    systemTimeToStr(&timeStruct, timeStr);
}

void platformGetCurrentTime(char *timeStr) {
    SYSTEMTIME timeStruct;
    GetSystemTime(&timeStruct);
    systemTimeToStr(&timeStruct, timeStr);
}

char platformGetTimeStruct(void *clock, PlatformTimeStruct *timeStructure) {
    return FileTimeToSystemTime(clock, timeStructure) ? 0 : 1;
}

int platformTimeStructEquals(PlatformTimeStruct *t1, PlatformTimeStruct *t2) {
    return (t1->wYear == t2->wYear && t1->wMonth == t2->wMonth && t1->wDay == t2->wDay && t1->wHour == t2->wHour &&
            t1->wMinute == t2->wMinute && t1->wSecond == t2->wSecond);
}

void *platformDirOpen(char *path) {
    DIR *dir = malloc(sizeof(DIR));
    size_t len;
    DEBUGPRT("platformDirOpen(%s)", path);
    if (dir && path) {
        len = strlen(path);
        if (len > 0 && len < FILENAME_MAX - 3) {
            size_t searchPathLen;
            char searchPath[FILENAME_MAX];
            strcpy(searchPath, path);
            searchPathLen = strlen(searchPath);

            if (searchPath[searchPathLen - 1] == '\\') {
                if (searchPathLen < FILENAME_MAX - 2)
                    strcat(searchPath, "*");
            } else {
                if (searchPathLen < FILENAME_MAX - 3)
                    strcat(searchPath, "\\*");
            }

            DEBUGPRT("platformDirOpen:searchPath = %s", searchPath);

            dir->error = 0;
            dir->directoryHandle = FindFirstFile(searchPath, &dir->nextEntry);
            if (dir->directoryHandle != INVALID_HANDLE_VALUE)
                return dir;
        }
    }

    if (dir)
        free(dir);

    return NULL;
}

void platformDirClose(void *dirp) {
    DIR *dir = dirp;
    DEBUGPRT("platformDirClose(%p)", dirp);
    FindClose(dir->directoryHandle);
    free(dir);
}

void *platformDirRead(void *dirp) {
    DIR *dir = dirp;
    DEBUGPRT("platformDirRead(%p)", dirp);
    memcpy(&dir->lastEntry, &dir->nextEntry, sizeof(PlatformDirEntry));
    switch (dir->error) {
        case 0:
            if (!FindNextFile(dir->directoryHandle, &dir->nextEntry))
                ++dir->error;
            return &dir->lastEntry;
        default:
            return NULL;
    }
}

char *platformDirEntryGetName(PlatformDirEntry *entry, size_t *length) {
    if (entry) {
        if (length)
            *length = strlen(entry->cFileName);

        return entry->cFileName;
    }

    return NULL;
}

char platformDirEntryIsHidden(PlatformDirEntry *entry) {
    DEBUGPRT("platformDirIsHidden(%p)", (void *) entry);
    if (entry) {
        if (entry->cFileName[0] == '.') {
            switch (entry->cFileName[1]) {
                case '\0':
                case '.':
                    return 1;
                default:
                    break;
            }
        }
        return (char) ((entry->dwFileAttributes & FILE_ATTRIBUTE_HIDDEN) > 0);
    }
    return 1;
}

#pragma clang diagnostic push
#pragma ide diagnostic ignored "UnusedParameter"

char platformDirEntryIsDirectory(char *rootPath, char *webPath, PlatformDirEntry *entry) {
    DEBUGPRT("platformDirEntryIsDirectory(%s, %s, %p)", rootPath, webPath, (void *) entry);
    if (entry)
        return (char) ((entry->dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) > 0);
    return 0;
}

#pragma clang diagnostic pop

char platformTimeGetFromHttpStr(const char *str, PlatformTimeStruct *time) {
    char day[4] = "", month[4] = "";
    if (sscanf(str, "%3s, %hu %3s %hu %hu:%hu:%hu GMT", day, &time->wDay, month, /* NOLINT(cert-err34-c) */
               &time->wYear, &time->wHour, &time->wMinute, &time->wSecond)) {
        switch (toupper(day[0])) {
            case 'F':
                time->wDayOfWeek = 5;
                break;
            case 'M':
                time->wDayOfWeek = 1;
                break;
            case 'S':
                time->wDayOfWeek = toupper(day[1]) == 'A' ? 6 : 0;
                break;
            case 'T':
                time->wDayOfWeek = toupper(day[1]) == 'U' ? 2 : 4;
                break;
            case 'W':
                time->wDayOfWeek = 3;
                break;
            default:
                return 1;
        }

        switch (toupper(month[0])) {
            case 'A':
                time->wMonth = toupper(month[1]) == 'P' ? 4 : 8;
                break;
            case 'D':
                time->wMonth = 12;
                break;
            case 'F':
                time->wMonth = 2;
                break;
            case 'J':
                time->wMonth = toupper(month[1]) == 'A' ? 1 : toupper(month[3]) == 'L' ? 7 : 6;
                break;
            case 'M':
                time->wMonth = toupper(month[2]) == 'R' ? 3 : 5;
                break;
            case 'N':
                time->wMonth = 11;
                break;
            case 'O':
                time->wMonth = 10;
                break;
            case 'S':
                time->wMonth = 9;
                break;
            default:
                return 1;
        }

        time->wMilliseconds = 0;
        return 0;
    }
    return 1;
}

int platformFileStat(const char *path, PlatformFileStat *stat) {
    DWORD attributes = GetFileAttributes(path);
    DEBUGPRT("platformFileStat(%s, %p)", path, (void *) stat);

    DEBUGPRT("platformFileStat:attributes = %lu", attributes);
    if (attributes != INVALID_FILE_ATTRIBUTES) {
        HANDLE handle = CreateFile(path, 0, FILE_SHARE_READ | FILE_SHARE_WRITE, NULL, OPEN_EXISTING,
                                   attributes & FILE_ATTRIBUTE_DIRECTORY ? FILE_FLAG_BACKUP_SEMANTICS : 0, NULL);

        DEBUGPRT("platformFileStat:handle = %p", handle);
        if (handle != INVALID_HANDLE_VALUE) {
            GetFileTime(handle, NULL, NULL, &stat->st_mtime);
            stat->st_size = GetFileSize(handle, NULL);
            stat->st_mode = attributes & FILE_ATTRIBUTE_DIRECTORY ? FILE_ATTRIBUTE_DIRECTORY : FILE_ATTRIBUTE_NORMAL;
            CloseHandle(handle);
            return 0;
        } else if (attributes & FILE_ATTRIBUTE_DIRECTORY) { /* Must be a FAT partition directory */
            stat->st_mode = FILE_ATTRIBUTE_DIRECTORY;
            stat->st_size = 0;
            GetSystemTimeAsFileTime(&stat->st_mtime); /* Avoid a false 304 condition */
            return 0;
        }
    }

    return 1;
}

char platformFileStatIsDirectory(PlatformFileStat *stat) {
    return ((stat->st_mode & FILE_ATTRIBUTE_DIRECTORY) > 0) ? 1 : 0;
}

char platformFileStatIsFile(PlatformFileStat *stat) {
    return ((stat->st_mode & FILE_ATTRIBUTE_NORMAL) > 0) ? 1 : 0;
}

void platformRemoveTrailingSlashes(char *path, size_t len) {
    if (len > 3) {
        if (path[len - 1] == '\\')
            path[len - 1] = '\0';
    }
}

char *platformGetRootPath(char *path) {
    char *test;
    if (strlen(path) == 1 || (strlen(path) < 4 && path[1] == ':' && path[2] == '\\')) {
        if (isupper(*path)) {
            test = malloc(5); /* +1 is intended */
            test[0] = *path, test[1] = ':', test[2] = '\\', test[3] = '\0';
            return test;
        }
    }

    test = platformRealPath(path);
    if (!test) {
        printf("No such directory \"%s\"\n", path);
        exit(1);
    }

    return test;
}

short platformPathWebToSystem(const char *rootPath, char *webPath, char *absolutePath) {
    size_t absolutePathLen;
    char *it, *internal;
    DEBUGPRT("platformPathWebToSystem(%s, %s, OUT)", rootPath, webPath);

    it = internal = platformPathCombine((char *) rootPath, webPath);

    if (!internal || (absolutePathLen = strlen(internal)) >= FILENAME_MAX)
        return 500;

    while (*it != '\0') {
        if (*it == '/')
            *it = '\\';
        ++it;
    }

    if (internal[absolutePathLen - 1] == '\\')
        internal[absolutePathLen - 1] = '\0';

    strcpy(absolutePath, internal);
    free(internal);
    DEBUGPRT("platformPathWebToSystem(%s, %s, %s)", rootPath, webPath, absolutePath);
    return 0;
}

short platformPathSystemToWeb(const char *rootPath, char *absolutePath, char *webPath) {
    size_t rootPathLen = strlen(rootPath), absolutePathLen = strlen(absolutePath), webPathLen;
    char *start, *it;
    DWORD attributes;
    DEBUGPRT("platformPathSystemToWeb(%s, %s, OUT)", rootPath, absolutePath);

    /* The absolutePath must be under a rootPath */
    if (rootPathLen > absolutePathLen || memcmp(rootPath, absolutePath, rootPathLen) != 0) {
        return 404;
    }

    attributes = GetFileAttributes(absolutePath);
    if (attributes == INVALID_FILE_ATTRIBUTES) {
        return 404;
    }

    /* Check if absolutePath has the folder divider where expected */
    if (absolutePath[rootPathLen - 1] == '\\' || absolutePath[rootPathLen - 1] == '/')
        start = &absolutePath[rootPathLen - 1];
    else
        start = NULL;

    /* Add if needed */
    if (start) {
        memcpy(webPath, start, strlen(start) + 1);
    } else {
        webPath[0] = '/';
        memcpy(webPath + 1, start, strlen(start) + 1);
    }

    /* Flip all DOS path dividers to web format */
    it = webPath;
    while (*it != '\0') {
        if (*it == '\\')
            *it = '/';
        ++it;
    }

    /* Add a trailing zero if it's a directory */
    if ((attributes & FILE_ATTRIBUTE_DIRECTORY) > 0) {
        webPathLen = strlen(webPath);
        if (webPath[webPathLen - 1] != '/')
            webPath[webPathLen - 1] = '/';
    }

    DEBUGPRT("platformPathSystemToWeb(%s, %s, %s)", rootPath, absolutePath, webPath);
    return 0;
}
