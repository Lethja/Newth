#include "platform/unix.h"

int main(int argc, char **argv) {
    int socketFd;
    size_t sendBytes;
    struct sockaddr_in serverAddress;
    char sendLine[MAX_LINE], recvLine[MAX_LINE];

    if (argc != 2)
        goto errorOut;

    if ((socketFd = socket(AF_INET, SOCK_STREAM, 0)) < 0)
        goto errorOut;

    memset(&serverAddress, 0, sizeof(serverAddress));
    serverAddress.sin_family = AF_INET;
    serverAddress.sin_port = htons(SERVER_PORT);

    if (inet_pton(AF_INET, argv[1], &serverAddress.sin_addr) <= 0)
        goto errorOut;

    if (connect(socketFd, (SA *) &serverAddress, sizeof(serverAddress)) < 0)
        goto errorOut;

    sprintf(sendLine, "GET / HTTP/1.1" HTTP_EOL);
    sendBytes = strlen(sendLine);

    if (write(socketFd, sendLine, sendBytes) != sendBytes)
        goto errorOut;

    memset(recvLine, 0, MAX_LINE);

    while ((read(socketFd, recvLine, MAX_LINE - 1) > 0)) {
        printf("%s", recvLine);
        memset(recvLine, 0, MAX_LINE);
    }

    fflush(stdout);

    return 0;

    errorOut:
    fprintf(stderr, "ERR\n");
    return 1;
}
