#include "common/http.h"
#include "platform/platform.h"

#include <stdio.h>

#ifdef WIN32

static WSADATA wsaData;

#endif

char prepareConnection(int type, char *ipStr, unsigned short port, struct sockaddr_storage *addrStruct) {
    if (type == AF_INET) {
        struct in_addr addr;
        struct sockaddr_in *serverAddress = (struct sockaddr_in *) addrStruct;
        serverAddress->sin_family = AF_INET;
        serverAddress->sin_port = htons(port);
        addr.s_addr = inet_addr(ipStr);
        if (addr.s_addr == INADDR_NONE || addr.s_addr == INADDR_ANY)
            return -1;

        /* TODO: Use a proper function */
        memcpy(&serverAddress->sin_addr, &addr, 4);
        return 0;
    }
    return -1;

}

int main(int argc, char **argv) {
    SOCKET socketFd;
    int sendBytes;
    struct sockaddr_storage serverAddress;
    char txLine[MAX_LINE], rxLine[MAX_LINE];

    if (argc != 2)
        goto errorOut;

#ifdef WIN32
    if (WSAStartup(MAKEWORD(1, 1), &wsaData))
        goto errorOut;
#endif

    if ((socketFd = socket(AF_INET, SOCK_STREAM, 0)) != 0)
        goto errorOut;

    memset(&serverAddress, 0, sizeof(serverAddress));

    if (prepareConnection(AF_INET, argv[1], SERVER_PORT, &serverAddress))
        goto errorOut;

    if (connect(socketFd, (SA *) &serverAddress, sizeof(serverAddress)) < 0)
        goto errorOut;

    /* TODO: More sophisticated header framework for HTTP requests */
    sprintf(txLine, "GET / HTTP/1.1" HTTP_EOL HTTP_EOL);
    sendBytes = (int) strlen(txLine);

    if (send(socketFd, txLine, sendBytes, 0) != sendBytes)
        goto errorOut;

    memset(rxLine, 0, MAX_LINE);

    while ((recv(socketFd, rxLine, MAX_LINE - 1, 0) > 0)) {
        printf("%s", rxLine);
        memset(rxLine, 0, MAX_LINE);
    }

    fflush(stdout);
#ifdef WIN32
    WSACleanup();
#endif
    return 0;

    errorOut:
#ifdef WIN32
    WSACleanup();
#endif
    fprintf(stderr, "ERR\n");
    return 1;
}
