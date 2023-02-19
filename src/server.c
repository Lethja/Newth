#include "platform/unix.h"

int main(int argc, char **argv) {
    int listenfd, connfd;
    size_t n;
    struct sockaddr_in servaddr;
    uint8_t buff[MAXLINE+1], recvline[MAXLINE+1];

    if((listenfd = socket(AF_INET, SOCK_STREAM, 0)) < 0)
        return 1;

    memset(&servaddr, 0, sizeof(servaddr));
    servaddr.sin_family = AF_INET;
    servaddr.sin_addr.s_addr = htonl(INADDR_ANY);
    servaddr.sin_port = htons(SERVER_PORT);

    if ((bind(listenfd, (SA *) &servaddr, sizeof servaddr)) < 0)
        return 1;

    if ((listen(listenfd, 10)) < 0)
        return 1;

    memset(buff, 0, sizeof(buff));

    while(1) {
        connfd = accept(listenfd, (SA *) NULL, NULL);

        memset(recvline, 0, sizeof (recvline));

        while((n = read(connfd, recvline, MAXLINE-1) ) > 0)
        {
            if (recvline[n-1] == '\n')
                break;
        }

        snprintf((char*)buff, sizeof(buff), "HTTP/1.0 200 OK\r\n\r\nHello");

        write(connfd, (char*)buff, strlen((char*)buff));
        close(connfd);
    }
}