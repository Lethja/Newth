#include "ws2.h"

#include <iphlpapi.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>

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
            closesocket(i);
        }
    }
    WSACleanup();
}

int platformServerStartup(SOCKET *listenSocket, short port) {
    struct sockaddr_in serverAddress;
    WSADATA wsaData;
    int iResult;

    iResult = WSAStartup(MAKEWORD(2, 0), &wsaData);
    if (iResult)
        return 1;

    *listenSocket = socket(AF_INET, SOCK_STREAM, 0);
    if (*listenSocket == INVALID_SOCKET) {
        WSACleanup();
        return 1;
    }

    serverAddress.sin_family = AF_INET;
    serverAddress.sin_addr.s_addr = htonl(INADDR_ANY);
    serverAddress.sin_port = htons(port);

    iResult = bind(*listenSocket, (SA *) &serverAddress, (int) sizeof(SA));
    if (iResult == SOCKET_ERROR) {
        closesocket(*listenSocket);
        WSACleanup();
        return 1;
    }

    iResult = listen(*listenSocket, SOMAXCONN);
    if (iResult == SOCKET_ERROR) {
        closesocket(*listenSocket);
        WSACleanup();
        return 1;
    }

    return 0;
}

void platformConnectSignals(void(*noAction)(int), void(*shutdownCrash)(int), void(*shutdownProgram)(int)) {
    signal(SIGSEGV, shutdownCrash);
    signal(SIGBREAK, shutdownProgram);
    signal(SIGINT, shutdownProgram);
    signal(SIGTERM, shutdownProgram);
}

SOCKET platformAcceptConnection(SOCKET fromSocket) {
    socklen_t addrSize = sizeof(struct sockaddr_in);
    SOCKET clientSocket;
    struct sockaddr_in clientAddress;

    clientSocket = accept(fromSocket, (SA *) &clientAddress, &addrSize);

#ifndef NDEBUG
    {
/*
        char address[16] = "";
        WSAAddressToStringA((SA *) &clientAddress.sin_addr, sizeof(clientAddress.sin_addr),
                            NULL, address, (LPDWORD) sizeof(address));
        fprintf(stdout, "Connection opened for: %s\n", address);
*/
    }
#endif

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

void platformGetIpString(struct sockaddr *addr, char ipStr[INET6_ADDRSTRLEN]) {
    if (addr->sa_family == AF_INET) {
        struct sockaddr_in *s4 = (struct sockaddr_in *) addr;
        char *ip = inet_ntoa(s4->sin_addr);
        strcpy(ipStr, ip);
        /* TODO: dynamically load inet_ntop support */
        /*
    } else if (addr->sa_family == AF_INET6) {
        struct sockaddr_in6 *s6 = (struct sockaddr_in6 *) addr;
        inet_ntop(s6->sin6_family, &s6->sin6_addr, ipStr, INET6_ADDRSTRLEN);
        */
    } else {
        strcpy(ipStr, "???");
    }
}

AdapterAddressArray *platformGetAdapterInformation(void) {
    AdapterAddressArray *array = NULL;
    PIP_ADAPTER_INFO pAdapterInfo = NULL;
    PIP_ADAPTER_INFO pAdapter = NULL;
    ULONG dwRetVal, ulOutBufLen = sizeof(IP_ADAPTER_INFO);

    pAdapterInfo = (IP_ADAPTER_INFO *) malloc(sizeof(IP_ADAPTER_INFO));
    if (pAdapterInfo == NULL) {
        printf("Error fetching 'GetAdaptersinfo'\n");
    }
    if (GetAdaptersInfo(pAdapterInfo, &ulOutBufLen) == ERROR_BUFFER_OVERFLOW) {
        free(pAdapterInfo);
        pAdapterInfo = (IP_ADAPTER_INFO *) malloc(ulOutBufLen);
        if (pAdapterInfo == NULL) {
            printf("Error fetching 'GetAdaptersinfo'\n");
        }
    }
    if ((dwRetVal = GetAdaptersInfo(pAdapterInfo, &ulOutBufLen)) == NO_ERROR) {
        pAdapter = pAdapterInfo;
        array = malloc(sizeof(AdapterAddressArray));
        array->size = 0;

        while (pAdapter) {
            if ((strcmp(pAdapter->IpAddressList.IpAddress.String, "0.0.0.0")) != 0) {
                array->adapterAddress = array->size ? realloc(array->adapterAddress,
                                                              sizeof(AdapterAddress) * (array->size + 1)) : malloc(
                        sizeof(AdapterAddress));

                strncpy(array->adapterAddress[array->size].name, pAdapter->Description, INET6_ADDRSTRLEN - 1);
                if (strlen(pAdapter->Description) >= INET_ADDRSTRLEN) {
                    array->adapterAddress[array->size].name[INET6_ADDRSTRLEN - 5] = '.';
                    array->adapterAddress[array->size].name[INET6_ADDRSTRLEN - 4] = '.';
                    array->adapterAddress[array->size].name[INET6_ADDRSTRLEN - 3] = '.';
                    array->adapterAddress[array->size].name[INET6_ADDRSTRLEN - 2] = ' ';
                    array->adapterAddress[array->size].name[INET6_ADDRSTRLEN - 1] = '\0';
                }

                strncpy(array->adapterAddress[array->size].addr, pAdapter->IpAddressList.IpAddress.String,
                        INET6_ADDRSTRLEN - 1);
                array->adapterAddress[array->size].addr[INET6_ADDRSTRLEN - 1] = '\0';

                ++array->size;
            }

            pAdapter = pAdapter->Next;
        }
    } else
        printf("GetAdaptersInfo failed with error: %ld\n", dwRetVal);

    if (pAdapterInfo)
        free(pAdapterInfo);

    return array;
}
