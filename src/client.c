#include "platform/unix.h"

int main(int argc, char **argv) {
    int sockfd;
    size_t sendbytes;
    struct sockaddr_in servaddr;
    char sendline[MAX_LINE], recvline[MAX_LINE];

    if (argc != 2)
        goto errorOut;

    if ((sockfd = socket(AF_INET, SOCK_STREAM, 0)) < 0)
        goto errorOut;

    memset(&servaddr, 0, sizeof(servaddr));
    servaddr.sin_family = AF_INET;
    servaddr.sin_port = htons(SERVER_PORT);

    if (inet_pton(AF_INET, argv[1], &servaddr.sin_addr) <= 0)
        goto errorOut;

    if (connect(sockfd, (SA *) &servaddr, sizeof(servaddr)) < 0)
        goto errorOut;

    sprintf(sendline, "GET / HTTP/1.1" HTTP_EOL);
    sendbytes = strlen(sendline);

    if (write(sockfd, sendline, sendbytes) != sendbytes)
        goto errorOut;

    memset(recvline, 0, MAX_LINE);

    while ((read(sockfd, recvline, MAX_LINE - 1) > 0)) {
        printf("%s", recvline);
        memset(recvline, 0, MAX_LINE);
    }

    fflush(stdout);

    return 0;

    errorOut:
    fprintf(stderr, "ERR\n");
    return 1;
}
