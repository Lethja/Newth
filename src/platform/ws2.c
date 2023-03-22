#include "ws2.h"

#include <signal.h>
#include <stdio.h>

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

void platformCloseBindSockets(fd_set *sockets) {
    int i;
    for (i = 0; i < FD_SETSIZE; i++) {
        if (FD_ISSET(i, sockets))
            shutdown(i, SD_BOTH);
        closesocket(i);
    }
    WSACleanup();
}

int platformServerStartup(SOCKET *listenSocket, short port) {
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
        char address[16] = "";
        WSAAddressToStringA((SA *) &clientAddress.sin_addr, sizeof(clientAddress.sin_addr),
                            NULL, address, (LPDWORD) sizeof(address));
        fprintf(stdout, "Connection opened for: %s\n", address);
    }
#endif

    return clientSocket;
}
