#include <ifaddrs.h>
#include "platform.h"
#include "unix.h"

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
        if (FD_ISSET(i, sockets))
            close(i);
    }
}

int platformServerStartup(int *listenSocket, short port) {
    struct sockaddr_in serverAddress;

    if ((*listenSocket = socket(AF_INET, SOCK_STREAM, 0)) < 0)
        return 1;

    serverAddress.sin_family = AF_INET;
    serverAddress.sin_addr.s_addr = htonl(INADDR_ANY);
    serverAddress.sin_port = htons(port);

    if ((bind(*listenSocket, (SA *) &serverAddress, sizeof(serverAddress))) < 0)
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

#ifndef NDEBUG
    {
        char address[INET6_ADDRSTRLEN] = "";
        inet_ntop(clientAddress.sin_family, &clientAddress.sin_addr, address, sizeof(address));
        fprintf(stdout, "Connection opened for: %s\n", address);
    }
#endif

    return clientSocket;
}

void platformConnectSignals(void(*noAction)(int), void(*shutdownCrash)(int), void(*shutdownProgram)(int)) {
    signal(SIGPIPE, noAction);
    signal(SIGSEGV, shutdownCrash);
    signal(SIGHUP, shutdownProgram);
    signal(SIGINT, shutdownProgram);
    signal(SIGTERM, shutdownProgram);
}

void platformGetIpString(struct sockaddr *addr, char ipStr[INET6_ADDRSTRLEN]) {
    if (addr->sa_family == AF_INET) {
        struct sockaddr_in *s4 = (struct sockaddr_in *) addr;
        char *ip = inet_ntoa(s4->sin_addr);
        strcpy(ipStr, ip);
    } else if (addr->sa_family == AF_INET6) {
        struct sockaddr_in6 *s6 = (struct sockaddr_in6 *) addr;
        inet_ntop(s6->sin6_family, &s6->sin6_addr, ipStr, INET6_ADDRSTRLEN);
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

AdapterAddressArray *platformGetAdapterInformation(void) {
    struct AdapterAddressArray *array;
    struct ifaddrs *ifap, *ifa;
    char addr[INET6_ADDRSTRLEN];
    if (getifaddrs(&ifap)) {
        return NULL;
    }

    array = malloc(sizeof(AdapterAddressArray));
    array->size = 0;

    for (ifa = ifap; ifa; ifa = ifa->ifa_next) {
        if (ifa->ifa_addr->sa_family == AF_INET) {
            struct sockaddr_in *sa = (struct sockaddr_in *) ifa->ifa_addr;
            char *ip4 = inet_ntoa(sa->sin_addr);
            if (strncmp(ip4, "127", 3) != 0)
                strcpy(addr, ip4);
            else
                continue;

        } else if (ifa->ifa_addr->sa_family == AF_INET6) {
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
