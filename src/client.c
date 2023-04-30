#include "common/http.h"
#include "platform/platform.h"

#include <ctype.h>
#include <stdio.h>

#ifdef WIN32

static WSADATA wsaData;

#endif

static char getAddressAndPort(char *uri, char *address, unsigned short *port) {
    char *it = uri;
    size_t hold, len = hold = 0, i, j;
    char portStr[6] = "";

    while (*it != '\0') {
        switch (*it) {
            case '[':
                ++hold;
                break;
            case ']':
                if (!hold)
                    return 1;
                --hold;
                break;
            case ':':
                goto getAddressAndPortBreakOutLoop;
        }
        ++len;
        ++it;
    }

    getAddressAndPortBreakOutLoop:
    if (len == 0 || len >= FILENAME_MAX)
        return 1;

    hold = it - uri;
    memcpy(address, uri, hold);
    address[hold] = '\0';

    if (*it == '\0')
        portStr[0] = '8', portStr[1] = '0', portStr[2] = '\0';
    else {
        for (i = 1, j = 0; j < 5; ++i, ++j) {
            if (it[i] == '\0' || it[i] == '/' || !isdigit(it[i])) {
                if (j)
                    portStr[j] = '\0';
                else
                    portStr[0] = '8', portStr[1] = '0', portStr[2] = '\0';
                break;
            } else {
                portStr[j] = it[i];
            }
        }
    }

    *port = (unsigned short) atoi(portStr);

    return 0;
}

char clientConnectSocketTo(const SOCKET *socket, char *uri, int type) {
    struct sockaddr_storage addrStruct;
    char address[FILENAME_MAX];
    unsigned short port = 80;
    int i;
    struct hostent *host;

    if (getAddressAndPort(uri, (char *) &address, &port))
        return -1;

    host = gethostbyname(address);
    if (!host || host->h_addrtype != type || host->h_length == 0)
        return -1;

    if(type == AF_INET) {
        struct sockaddr_in *serverAddress = (struct sockaddr_in *) &addrStruct;
        memset(serverAddress, 0, sizeof(addrStruct));

        serverAddress->sin_family = type;
        serverAddress->sin_port = htons(port);

        for (i = 0; host->h_addr_list[i] != NULL; ++i) {
            memcpy(&serverAddress->sin_addr.s_addr, host->h_addr_list[0], host->h_length);

            if (connect(*socket, (SA *) serverAddress, sizeof(addrStruct)) == 0)
                return 0;
        }
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

    if ((socketFd = socket(AF_INET, SOCK_STREAM, 0)) == -1)
        goto errorOut;

    memset(&serverAddress, 0, sizeof(serverAddress));

    if (clientConnectSocketTo(&socketFd, argv[1], AF_INET))
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
