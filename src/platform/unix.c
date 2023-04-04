#include <ifaddrs.h>
#include "platform.h"
#include "unix.h"
#include "../server/event.h"

char *platformPathCombine(char *path1, char *path2) {
    const char pathDivider = '/';
    size_t a = strlen(path1), b = strlen(path2), path2Jump = 1;
    char *returnPath;

    if (path1[a - 1] != pathDivider && path2[0] != pathDivider)
        path2Jump++;

    returnPath = malloc(a + b + path2Jump);
    memcpy(returnPath, path1, a);
    if (path2Jump > 1)
        returnPath[a] = pathDivider;

    memcpy(returnPath + a + path2Jump - 1, path2, b + 1);

    return returnPath;
}

void platformCloseBindSockets(fd_set *sockets, SOCKET max) {
    int i;
    for (i = 0; i <= max; i++) {
        if (FD_ISSET(i, sockets)) {
            eventSocketCloseInvoke(&i);
            close(i);
        }

    }
}

int platformServerStartup(int *listenSocket, sa_family_t family, char *ports) {
    struct sockaddr_storage serverAddress;
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
            sock->sin6_addr = in6addr_any;
            setsockopt(*listenSocket, IPPROTO_IPV6, IPV6_V6ONLY, &v6Only, sizeof(v6Only));
            break;
        }
    }

    if (platformBindPort(listenSocket, (struct sockaddr *) &serverAddress, ports))
        return 1;

    if ((listen(*listenSocket, 10)) < 0)
        return 1;

    return 0;
}

int platformAcceptConnection(int fromSocket) {
    socklen_t addrSize = sizeof(struct sockaddr_in);
    int clientSocket;
    struct sockaddr_in clientAddress;

    clientSocket = accept(fromSocket, (SA *) &clientAddress, &addrSize);

    eventSocketAcceptInvoke(&clientSocket);

    return clientSocket;
}

void platformConnectSignals(void(*noAction)(int), void(*shutdownCrash)(int), void(*shutdownProgram)(int)) {
    signal(SIGPIPE, noAction);
    signal(SIGSEGV, shutdownCrash);
    signal(SIGHUP, shutdownProgram);
    signal(SIGINT, shutdownProgram);
    signal(SIGTERM, shutdownProgram);
}

void platformGetIpString(struct sockaddr *addr, char ipStr[INET6_ADDRSTRLEN], sa_family_t *family) {
    if (addr->sa_family == AF_INET) {
        struct sockaddr_in *s4 = (struct sockaddr_in *) addr;
        char *ip = inet_ntoa(s4->sin_addr);
        strcpy(ipStr, ip);
        *family = AF_INET;
    } else if (addr->sa_family == AF_INET6) {
        const char ipv4[8] = "::ffff:";
        struct sockaddr_in6 *s6 = (struct sockaddr_in6 *) addr;
        inet_ntop(s6->sin6_family, &s6->sin6_addr, ipStr, INET6_ADDRSTRLEN);
        if (strncmp(ipStr, ipv4, sizeof(ipv4) - 1) == 0) {
            memmove(ipStr, &ipStr[7], INET_ADDRSTRLEN);
            *family = AF_INET;
        } else
            *family = AF_INET6;
    } else
        strcpy(ipStr, "???");
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
    struct AdapterAddressArray *array;
    struct ifaddrs *ifap, *ifa;
    char addr[INET6_ADDRSTRLEN];
    if (getifaddrs(&ifap)) {
        return NULL;
    }

    array = malloc(sizeof(AdapterAddressArray));
    array->size = 0;

    for (ifa = ifap; ifa; ifa = ifa->ifa_next) {
        if (ifa->ifa_addr->sa_family == AF_INET && family != AF_INET6) {
            struct sockaddr_in *sa = (struct sockaddr_in *) ifa->ifa_addr;
            char *ip4 = inet_ntoa(sa->sin_addr);
            if (strncmp(ip4, "127", 3) != 0)
                strcpy(addr, ip4);
            else
                continue;

        } else if (ifa->ifa_addr->sa_family == AF_INET6 && family != AF_INET) {
            struct sockaddr_in6 *sa = (struct sockaddr_in6 *) ifa->ifa_addr;
            inet_ntop(sa->sin6_family, &sa->sin6_addr, addr, INET6_ADDRSTRLEN);
            if (strncmp(addr, "::", 2) == 0)
                continue;

        } else continue;

        platformFindOrCreateAdapterIp(array, ifa->ifa_name, ifa->ifa_addr->sa_family == AF_INET6, addr);
    }
    freeifaddrs(ifap);

    return array;
}

char *platformRealPath(char *path) {
    return realpath(path, NULL);
}
