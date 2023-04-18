#include "mscrtdl.h"
#include "../server/event.h"
#include "../common/debug.h"

#ifdef PORTABLE_WIN32

#include "winsock/wsock1.h"

#endif

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
    size_t a = strlen(path1), b = strlen(path2), path2Jump = 1;
    char *returnPath;

    if ((path1[a - 1] != pathDivider || path1[a - 1] != pathDivider2) &&
        (path2[0] != pathDivider || path2[0] != pathDivider2))
        path2Jump++;

    returnPath = malloc(a + b + path2Jump);
    memcpy(returnPath, path1, a);
    if (path2Jump > 1)
        returnPath[a] = pathDivider;

    memcpy(returnPath + a + path2Jump - 1, path2, b + 1);

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

void platformIpStackExit(void) {
    WSACleanup();
    if (wsIpv6)
        FreeLibrary(wsIpv6);
}

int platformIpStackInit(void) {
    DEBUGPRT("%s ", "platformIPStackInit");
    char libAbs[FILENAME_MAX] = "";

    GetModuleFileName(NULL, libAbs, FILENAME_MAX);
    char *filePoint = strrchr(libAbs, '\\') + 1;
    DEBUGPRT("libAbs = %s\nfilePoint = %s", libAbs, filePoint);
    if (!filePoint)
        return 1;

    /* Runtime link dual-stack functions that are not available in all 32 bit versions of Windows */
    *filePoint = '\0';
    strncat(filePoint, "thwsipv6.dll", FILENAME_MAX);
    wsIpv6 = LoadLibrary(TEXT(libAbs));
    DEBUGPRT("wsIpv6 (%s) = %p", libAbs, wsIpv6);
    if (wsIpv6)
        getAdapterInformationIpv6 = (adapterInformationIpv6) GetProcAddress(wsIpv6,
                                                                            "platformGetAdapterInformationIpv6");

    /* Choose between WinSock 1 (Windows 95 support) and WinSock 2 */
    *filePoint = '\0';
    strncat(filePoint, "thwsock2.dll", FILENAME_MAX);
    wsIpv4 = LoadLibrary(TEXT(libAbs));
    DEBUGPRT("wsIpv4 (%s) = %p", libAbs, wsIpv4);
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

    return WSAStartup(MAKEWORD(1, 1), &wsaData);
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

void platformGetTime(void *clock, char *timeStr) {
    /* TODO: Get the wall clock time and format it in http format */
    *timeStr = '\0';
}

void platformGetCurrentTime(char *time) {
    platformGetTime(NULL, time);
}

void platformGetTimeStruct(void *clock, void **timeStructure) {
    *timeStructure = NULL;
}

int platformTimeStructEquals(PlatformTimeStruct *t1, PlatformTimeStruct *t2) {
    /* TODO: Compare two FILETIMEs */
    return 0;
}

void *platformDirOpen(char *path) {
    return NULL;
}

void platformDirClose(void *dirp) {
}

void *platformDirRead(void *dirp) {
    return NULL;
}

char *platformDirEntryGetName(void *entry, size_t *length) {
    if (length)
        *length = 0;
    return NULL;
}

char platformDirEntryIsHidden(void *entry) {
    return 0;
}

char platformDirEntryIsDirectory(char *rootPath, char *webPath, void *entry) {
    return 0;
}

char platformTimeGetFromHttpStr(const char *str, PlatformTimeStruct *time) {
    return 1;
}

int platformFileStat(const char *path, PlatformFileStat *stat) {
    return 1;
}