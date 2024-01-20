#include "mscrtdl.h"
#include "../server/event.h"

#ifdef PLATFORM_NET_ADAPTER

#ifdef ENABLE_WS1

#include "winsock/wsock1.h"

#endif

#include "winsock/wsock2.h"
#include "winsock/wsipv6.h"

#endif /* PLATFORM_NET_ADAPTER */

#include <ctype.h>
#include <direct.h>
#include <fcntl.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>

#ifdef PLATFORM_NET_ADAPTER

typedef AdapterAddressArray *(*adapterInformationIpv6)(sa_family_t family,
                                                       void (*arrayAdd)(AdapterAddressArray *, const char *,
                                                                        sa_family_t, char *),
                                                       void (*nTop)(const void *, char *));

typedef AdapterAddressArray *(*adapterInformationIpv4)(
        void (*arrayAdd)(AdapterAddressArray *, const char *, sa_family_t, char *));

adapterInformationIpv4 getAdapterInformationIpv4 = NULL;
adapterInformationIpv6 getAdapterInformationIpv6 = NULL;

void (*wsIpv4)(void) = NULL;

void (*wsIpv6)(void) = NULL;

#endif /* PLATFORM_NET_ADAPTER */

#pragma region Network Bind & Listen

#ifdef PLATFORM_NET_LISTEN

SOCKET platformAcceptConnection(SOCKET fromSocket) {
    const char blocking = 1;
    socklen_t addrSize = sizeof(struct sockaddr_storage);
    SOCKET clientSocket;
    struct sockaddr_storage clientAddress;

    LINEDBG;

    clientSocket = accept(fromSocket, (struct sockaddr *) &clientAddress, &addrSize);
    platformSocketSetBlock(clientSocket, blocking);

    eventSocketAcceptInvoke(&clientSocket);

    return clientSocket;
}

void platformCloseBindSockets(const SOCKET *sockets) {
    SOCKET i, max = sockets[0];

    for (i = 1; i <= max; ++i) {
        eventSocketCloseInvoke(&i);
        closesocket(i);
    }
    WSACleanup();
}

SOCKET *platformServerStartup(sa_family_t family, char *ports, char **err) {
    struct sockaddr_storage serverAddress;
    SOCKET listenSocket, *r;
    int iResult;

    LINEDBG;

#ifdef PLATFORM_NET_ADAPTER

    /* Force start in IPV4 mode if IPV6 functions cannot be loaded */
    if (family != AF_INET && (!getAdapterInformationIpv6)) {
        if (family == AF_INET6) {
            *err = "This build of Windows does not have sufficient IPv6 support";
            return NULL;
        }

        family = AF_INET;
    }

#endif /* PLATFORM_NET_ADAPTER */

    ZeroMemory(&serverAddress, sizeof(serverAddress));

    switch (family) {
        default:
        case AF_INET: {
            struct sockaddr_in *sock = (struct sockaddr_in *) &serverAddress;
            if ((listenSocket = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
                *err = "Unable to acquire socket from system";
                return NULL;
            }

            sock->sin_family = AF_INET;
            sock->sin_addr.s_addr = htonl(INADDR_ANY);
            LINEDBG;
            break;
        }
        case AF_INET6:
        case AF_UNSPEC: {
            struct SOCKIN6 *sock = (struct SOCKIN6 *) &serverAddress;
            size_t v6Only = family == AF_INET6;
            if ((listenSocket = socket(AF_INET6, SOCK_STREAM, 0)) < 0) {
                *err = "Unable to acquire socket from system";
                return NULL;
            }

            sock->sin6_family = AF_INET6;
            setsockopt(listenSocket, IPPROTO_IPV6, IPV6_V6ONLY, (char *) &v6Only, sizeof(v6Only));
            LINEDBG;
            break;
        }
    }

    if (listenSocket == INVALID_SOCKET) {
        *err = "Socket is invalid";
        return NULL;
    }

    if (platformBindPort(&listenSocket, (struct sockaddr *) &serverAddress, ports)) {
        closesocket(listenSocket);
        *err = "Unable to bind to designated port numbers";
        return NULL;
    }

    iResult = listen(listenSocket, SOMAXCONN);
    if (iResult == SOCKET_ERROR) {
        closesocket(listenSocket);
        *err = "Unable to listen to assigned socket";
        return NULL;
    }

    r = malloc(sizeof(SOCKET) * 2);
    if (r) {
        r[0] = 1, r[1] = listenSocket;
        return r;
    } else
        *err = strerror(errno);

    return NULL;
}

#endif /* PLATFORM_NET_LISTEN */

#pragma endregion

WSADATA wsaData;

void platformPathCombine(char *output, const char *path1, const char *path2) {
    const char *pathDivider = "/\\";
    size_t len = strlen(path1), idx = len - 1;
    const char *p2;

    /* Copy first path into output buffer for manipulation */
    strcpy(output, path1);

    while ((output[idx] == pathDivider[0] || output[idx] == pathDivider[1]) && idx)
        --idx;

    if (idx) {
        output[idx + 1] = '\0';
        strcat(output, &pathDivider[1]);
    } else if ((output[0] == pathDivider[0] || output[idx] == pathDivider[1]) && (output[1] == pathDivider[0] || output[1] == pathDivider[1]))
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

#pragma clang diagnostic push
#pragma ide diagnostic ignored "ConstantParameter"
#pragma ide diagnostic ignored "ConstantConditionsOC"

static char platformVersionAbove(DWORD major, DWORD minor) {
    DWORD dwVersion;
    DWORD dwMajorVersion;
    DWORD dwMinorVersion;

    dwVersion = GetVersion();

    dwMajorVersion = (DWORD)(LOBYTE(LOWORD(dwVersion)));
    dwMinorVersion = (DWORD)(HIBYTE(LOWORD(dwVersion)));

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
}

char *platformIpStackInit(void) {
    int error;

    LINEDBG;

#ifdef PLATFORM_NET_ADAPTER

    wsIpv6 = wSockIpv6Available();
    if (wsIpv6) {
        getAdapterInformationIpv6 = (adapterInformationIpv6) wSockIpv6GetAdapterInformation;
        LINEDBG;
    }

    /* Choose between WinSock 1 (Windows 95 support) and WinSock 2 */
    wsIpv4 = wSock2Available();
    if (wsIpv4) {
        getAdapterInformationIpv4 = (adapterInformationIpv4) wSock2GetAdapterInformation;
        LINEDBG;
    }
#ifdef ENABLE_WS1
    else {
        LINEDBG;
        wsIpv4 = wSock1Available();
        if (wsIpv4) {
            LINEDBG;
            getAdapterInformationIpv4 = (adapterInformationIpv4) wSock1GetAdapterInformation;
        }
    }
#endif

    LINEDBG;

    if (!getAdapterInformationIpv4) {
        char err[255] = "";
        FormatMessage(FORMAT_MESSAGE_FROM_SYSTEM, NULL, GetLastError(), MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), err,
                      255, NULL);

        return "Unable to retrieve TCP/IP adapter information";
    }

#endif /* PLATFORM_NET_ADAPTER */

    LINEDBG;

    error = WSAStartup(MAKEWORD(1, 1), &wsaData);

    if (error)
        return "Unable to start Windows socket interface";

    return NULL;
}

int platformOfficiallySupportsIpv6(void) {
    if (platformVersionAbove(6, 0))
        return 1;
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

void ipv6NTop(const void *inAddr6, char *ipStr) {
#define IPV6MAX 100
    const unsigned char *a = inAddr6;
    char buf[IPV6MAX];
    size_t i, j, max, best;

    if (memcmp(a, "\0\0\0\0\0\0\0\0\0\0\377\377", 12) != 0)
        snprintf(buf, IPV6MAX, "%x:%x:%x:%x:%x:%x:%x:%x", 256 * a[0] + a[1], 256 * a[2] + a[3], 256 * a[4] + a[5],
                 256 * a[6] + a[7], 256 * a[8] + a[9], 256 * a[10] + a[11], 256 * a[12] + a[13], 256 * a[14] + a[15]);
    else
        snprintf(buf, IPV6MAX, "%x:%x:%x:%x:%x:%x:%d.%d.%d.%d", 256 * a[0] + 256 * a[1], 256 * a[2] + a[3],
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
        struct SOCKIN6 *s6 = (struct SOCKIN6 *) addr;
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

#pragma region Network Adapter Discovery

#ifdef PLATFORM_NET_ADAPTER

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

#endif

#pragma endregion

#pragma region Directory Functions

void *platformDirOpen(char *path) {
    DIR *dir = malloc(sizeof(DIR));
    size_t len;

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

    FindClose(dir->directoryHandle);
    free(dir);
}

void *platformDirRead(void *dirp) {
    DIR *dir = dirp;

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

char platformDirEntryIsDirectory(PlatformDirEntry *entry, void *dirp) {

    if (entry)
        return (char) ((entry->dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) > 0);
    return 0;
}

#pragma clang diagnostic pop

#pragma endregion

#pragma region File Functions

int platformFileStat(const char *path, PlatformFileStat *stat) {
    WIN32_FILE_ATTRIBUTE_DATA fad;

    if (GetFileAttributesEx(path, GetFileExInfoStandard, &fad)) {
        HANDLE handle = CreateFile(path, 0, FILE_SHARE_READ | FILE_SHARE_WRITE, NULL, OPEN_EXISTING,
                                   fad.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY ? FILE_FLAG_BACKUP_SEMANTICS : 0,
                                   NULL);

        if (handle != INVALID_HANDLE_VALUE) {
            LARGE_INTEGER li;

            GetFileTime(handle, NULL, NULL, &stat->st_mtime);
            CloseHandle(handle);

            if (stat->st_mtime.dwHighDateTime == 0 && stat->st_mtime.dwLowDateTime == 0)
                GetSystemTimeAsFileTime(&stat->st_mtime); /* Avoid a false 304 condition */

            li.LowPart = fad.nFileSizeLow, li.HighPart = fad.nFileSizeHigh, stat->st_size = li.QuadPart;
            stat->st_mode =
                    fad.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY ? FILE_ATTRIBUTE_DIRECTORY : FILE_ATTRIBUTE_NORMAL;

            return 0;
        } else if (fad.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) { /* Might be a FAT partition directory */
            stat->st_mode = FILE_ATTRIBUTE_DIRECTORY;
            stat->st_size = 0;
            GetSystemTimeAsFileTime(&stat->st_mtime); /* Avoid a false 304 condition */
            return 0;
        }
    }
    return 1;
}

static void
platformFileOpenModeConfigure(const char *fileMode, DWORD *desiredAccess, DWORD *shareMode, DWORD *creationDisposition,
                              DWORD *flagsAndAttributes, DWORD *openFlags) {
    size_t len = strlen(fileMode), i;
    *desiredAccess = *shareMode = *creationDisposition = *openFlags = 0, *flagsAndAttributes = FILE_ATTRIBUTE_NORMAL;

    for (i = 0; i < len; ++i) {
        switch (fileMode[i]) {
            case 'r':
                *desiredAccess |= GENERIC_READ;
                *shareMode |= FILE_SHARE_READ;
                *creationDisposition = OPEN_EXISTING;
                if (fileMode[i + 1] == '+') {
                    *desiredAccess |= GENERIC_WRITE;
                    *shareMode |= GENERIC_WRITE;
                }
                break;
            case 'w':
                *desiredAccess |= GENERIC_WRITE;
                *shareMode |= FILE_SHARE_WRITE;
                if (fileMode[i + 1] == '+') {
                    *desiredAccess |= GENERIC_READ;
                    *shareMode |= GENERIC_READ;
                    *creationDisposition = OPEN_EXISTING;
                } else {
                    *creationDisposition |= CREATE_ALWAYS;
                }
                break;
            case 'a':
                *openFlags |= _O_APPEND;
                *desiredAccess |= GENERIC_WRITE;
                *shareMode |= FILE_SHARE_WRITE;
                *creationDisposition = OPEN_EXISTING;
                if (fileMode[i + 1] == '+') {
                    *desiredAccess |= GENERIC_READ;
                    *shareMode |= GENERIC_READ;
                }
                break;
            case 'b':
                *openFlags |= _O_BINARY;
                break;
        }
    }
}

PlatformFile platformFileOpen(const char *fileName, const char *fileMode) {
    DWORD desiredAccess, shareMode, creationDisposition, flagsAndAttributes, openFlags;
    HANDLE stream;
    platformFileOpenModeConfigure(fileMode, &desiredAccess, &shareMode, &creationDisposition, &flagsAndAttributes,
                                  &openFlags);
    stream = CreateFile(fileName, desiredAccess, shareMode, NULL, creationDisposition, flagsAndAttributes, NULL);

    if (stream != INVALID_HANDLE_VALUE && (openFlags & _O_APPEND) > 0)
        platformFileSeek(stream, 0, SEEK_END);

    return stream;
}

int platformFileClose(PlatformFile stream) {
    return CloseHandle(stream);
}

int platformFileSeek(PlatformFile stream, PlatformFileOffset offset, int whence) {
    LARGE_INTEGER li;

    switch (whence) {
        default:
            return -1;
        case SEEK_SET:
            whence = FILE_BEGIN;
            break;
        case SEEK_CUR:
            whence = FILE_CURRENT;
            break;
        case SEEK_END:
            whence = FILE_END;
            break;
    }

    li.QuadPart = offset;
    li.LowPart = SetFilePointer(stream, (LONG) li.LowPart, &li.HighPart, whence);

    if (li.LowPart == INVALID_SET_FILE_POINTER && GetLastError() != NO_ERROR)
        return -1;

    return 0;
}

PlatformFileOffset platformFileTell(PlatformFile stream) {
    LARGE_INTEGER li;

    ZeroMemory(&li, sizeof(LARGE_INTEGER));
    li.LowPart = SetFilePointer(stream, 0, &li.HighPart, FILE_CURRENT);

    if (li.LowPart == INVALID_SET_FILE_POINTER && GetLastError() != NO_ERROR)
        return -1;

    return li.QuadPart;
}

size_t platformFileRead(void *ptr, size_t size, size_t n, PlatformFile stream) {
    DWORD bytes, bufferSize = size * n;

    if (bufferSize > BUFSIZ)
        bufferSize = BUFSIZ;

    if (ReadFile(stream, ptr, bufferSize, &bytes, NULL))
        return bytes;

    return 0;
}

char platformFileStatIsDirectory(PlatformFileStat *stat) {
    return ((stat->st_mode & FILE_ATTRIBUTE_DIRECTORY) > 0) ? 1 : 0;
}

char platformFileStatIsFile(PlatformFileStat *stat) {
    return ((stat->st_mode & FILE_ATTRIBUTE_NORMAL) > 0) ? 1 : 0;
}

#pragma endregion

#pragma region Path Functions

char *platformRealPath(char *path) {
    char *buf = malloc(MAX_PATH);
    if (buf) {
        DWORD e = GetFullPathName(path, MAX_PATH, buf, NULL);

        if (e != 0)
            return buf;

        free(buf);
    }
    return NULL;
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
    if (GetCurrentDirectory(length, buffer))
        return buffer;
    return NULL;
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

#pragma endregion

#pragma region Time Functions

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

    sprintf(timeStr, "%s, %02hu %s %04hu %02hu:%02hu:%02hu GMT", day, timeStruct->wDay, month, timeStruct->wYear,
            timeStruct->wHour, timeStruct->wMinute, timeStruct->wSecond);
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

#pragma endregion

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

int platformPathSystemChangeWorkingDirectory(const char *path) {
    return chdir(path);
}

int platformSocketSetBlock(SOCKET socket, char blocking) {
    unsigned long mode = (unsigned char) blocking;
    return ioctlsocket(socket, FIONBIO, &mode);
}

int platformSocketGetLastError(void) {
    return WSAGetLastError();
}

