#include "platform/unix.h"

int listenfd = 0;

void shutdownProgram() {
    if (listenfd)
        shutdown(listenfd, SHUT_RDWR);
    exit(0);
}

int main(int argc, char **argv) {
    signal(SIGHUP, shutdownProgram);
    signal(SIGINT, shutdownProgram);
    int connfd;
    size_t n;
    struct sockaddr_in servaddr, addr;
    socklen_t addrLen = sizeof(addr);
    uint8_t buff[MAX_LINE + 1], recvline[MAX_LINE + 1];
    char address[24];

    if ((listenfd = socket(AF_INET, SOCK_STREAM, 0)) < 0)
        return 1;

    memset(&servaddr, 0, sizeof(servaddr));
    memset(&addr, 0, sizeof(addr));

    servaddr.sin_family = AF_INET;
    servaddr.sin_addr.s_addr = htonl(INADDR_ANY);
    servaddr.sin_port = htons(SERVER_PORT);

    if ((bind(listenfd, (SA *) &servaddr, sizeof servaddr)) < 0)
        return 1;

    if ((listen(listenfd, 10)) < 0)
        return 1;

    memset(buff, 0, sizeof(buff));

    while (1) {
        connfd = accept(listenfd, (SA *) &addr, &addrLen);

        inet_ntop(AF_INET, &addr.sin_addr, address, sizeof(address));

        fprintf(stdout, "%s\n", address);

        memset(recvline, 0, sizeof(recvline));

        while ((n = read(connfd, recvline, MAX_LINE - 1)) > 0) {
            if (recvline[n - 1] == '\n')
                break;
        }

        snprintf((char *) buff, sizeof(buff), "HTTP/1.0 200 OK" HTTP_EOL "Hello");

        write(connfd, (char *) buff, strlen((char *) buff));
        close(connfd);
    }
}