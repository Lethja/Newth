#include "ws2.h"
#include "../server/event.h"

#include <iphlpapi.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>

typedef AdapterAddressArray *(*adapterInformationIpv6)(sa_family_t family,
                                                       void (*arrayAdd)(AdapterAddressArray *, char *, sa_family_t,
                                                                        char *), void (*nTop)(const void *, char *));

adapterInformationIpv6 getAdapterInformationIpv6 = NULL;
HMODULE ws2ipv6 = NULL;
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

/*
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
*/

void platformIpStackExit(void) {
    WSACleanup();
    if (ws2ipv6)
        FreeLibrary(ws2ipv6);
}

int platformIpStackInit(void) {
    /* Runtime link dual-stack functions that are not available in all 32 bit versions of Windows */
    ws2ipv6 = LoadLibrary(TEXT("thwsipv6.dll"));
    if (ws2ipv6)
        getAdapterInformationIpv6 = (adapterInformationIpv6) GetProcAddress(ws2ipv6,
                                                                            "platformGetAdapterInformationIpv6");
    return WSAStartup(MAKEWORD(2, 0), &wsaData);
}

int platformServerStartup(SOCKET *listenSocket, sa_family_t family, char *ports) {
    struct sockaddr_storage serverAddress;
    int iResult;

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

static AdapterAddressArray *platformGetAdapterInformationIpv4(void) {
    AdapterAddressArray *array = NULL;
    PIP_ADAPTER_INFO pAdapterInfo = NULL;
    PIP_ADAPTER_INFO pAdapter = NULL;
    ULONG dwRetVal, ulOutBufLen = sizeof(IP_ADAPTER_INFO);

    pAdapterInfo = (IP_ADAPTER_INFO *) malloc(sizeof(IP_ADAPTER_INFO));
    if (pAdapterInfo == NULL) {
        printf("Error fetching 'GetAdaptersInfo'\n");
    }

    if (GetAdaptersInfo(pAdapterInfo, &ulOutBufLen) == ERROR_BUFFER_OVERFLOW) {
        free(pAdapterInfo);
        pAdapterInfo = (IP_ADAPTER_INFO *) malloc(ulOutBufLen);
        if (pAdapterInfo == NULL) {
            printf("Error fetching 'GetAdaptersInfo'\n");
        }
    }

    if ((dwRetVal = GetAdaptersInfo(pAdapterInfo, &ulOutBufLen)) == NO_ERROR) {
        pAdapter = pAdapterInfo;
        array = malloc(sizeof(AdapterAddressArray));
        array->size = 0;

        while (pAdapter) {
            char ip[INET6_ADDRSTRLEN];
            strncpy(ip, pAdapter->IpAddressList.IpAddress.String, INET6_ADDRSTRLEN);

            if ((strcmp(ip, "0.0.0.0")) != 0) {
                char desc[BUFSIZ];
                strcpy(desc, pAdapter->Description);
                platformFindOrCreateAdapterIp(array, desc, 0, ip);
            }

            pAdapter = pAdapter->Next;
        }
    } else
        printf("GetAdaptersInfo failed with error: %ld\n", dwRetVal);

    if (pAdapterInfo)
        free(pAdapterInfo);

    return array;
}

AdapterAddressArray *platformGetAdapterInformation(sa_family_t family) {
    switch (family) {
        case AF_UNSPEC:
            if (getAdapterInformationIpv6)
                case AF_INET6:
                    return getAdapterInformationIpv6(family, platformFindOrCreateAdapterIp, ipv6NTop);
        default:
        case AF_INET:
            return platformGetAdapterInformationIpv4();
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
