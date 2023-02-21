#include <sys/stat.h>
#include <time.h>
#include "platform/unix.h"

int serverSocket;

void noAction() {}

void shutdownProgram() {
    if (serverSocket)
        shutdown(serverSocket, SHUT_RDWR);
    exit(0);
}

int serverStartup(short port) {
    int newSocket;
    struct sockaddr_in serverAddress;

    if ((newSocket = socket(AF_INET, SOCK_STREAM, 0)) < 0)
        exit(1);

    serverAddress.sin_family = AF_INET;
    serverAddress.sin_addr.s_addr = htonl(INADDR_ANY);
    serverAddress.sin_port = htons(port);

    if ((bind(newSocket, (SA *) &serverAddress, sizeof(serverAddress))) < 0)
        goto ServerStartupError;

    if ((listen(newSocket, 10)) < 0)
        goto ServerStartupError;

    return newSocket;

    ServerStartupError:
    perror("Unable to start server");
    exit(1);
}

int acceptConnection(int fromSocket) {
    socklen_t addrSize = sizeof(struct sockaddr_in);
    int clientSocket;
    struct sockaddr_in clientAddress;
    char address[16] = "";

    clientSocket = accept(fromSocket, (SA *) &clientAddress, &addrSize);
    inet_ntop(AF_INET, &clientAddress.sin_addr, address, sizeof(address));
    fprintf(stdout, "%s\n", address);
    return clientSocket;
}

inline char HexToAscii(const char *hex) {
    char value = 0;
    unsigned char i;
    for (i = 0; i < 2; i++) {
        unsigned char byte = hex[i];
        if (byte >= '0' && byte <= '9') byte = byte - '0';
        else if (byte >= 'a' && byte <= 'f') byte = byte - 'a' + 10;
        else if (byte >= 'A' && byte <= 'F') byte = byte - 'A' + 10;
        value = (char) (value << 4 | (byte & 0xF));
    }
    return value;
}

void convertUrlToPath(char *url) {
    while (*url != '\0') {
        if (*url == '%') {
            if (url[1] != '\0' && url[2] != '\0') {
                *url = HexToAscii(&url[1]);
                memmove(&url[1], &url[3], strlen(url) - 2);
            }
        }
        url++;
    }
}

char *httpClientReadUri(char *request) {
    char *start, *end;
    size_t len;

    start = strchr(request, ' ');
    if (!start)
        return NULL;

    end = strchr(start + 1, ' ');
    len = end - start;

    char *path = malloc(len + 1);
    memcpy(path, start + 1, len - 1);
    path[len - 1] = '\0';

    convertUrlToPath(path);

    return path;
}

void httpHeaderWriteDate(int clientSocket) {
    const char max = 36;
    char buffer[max];
    time_t rawTime;
    struct tm *timeInfo;

    time(&rawTime);
    timeInfo = gmtime(&rawTime);

    strftime(buffer, max, "Date: %a, %d %b %H:%M:%S GMT\n", timeInfo);
    write(clientSocket, buffer, strlen(buffer));
}

void httpHeaderWriteContentLength(int clientSocket, struct stat *st) {
    const size_t max = 100;
    char buffer[max];
    snprintf(buffer, max, "Content-Length: %zu\n", st->st_size);
    write(clientSocket, buffer, strlen(buffer));
}

void httpHeaderWriteResponseOK(int clientSocket) {
    write(clientSocket, "HTTP/1.1 200 OK\n", strlen("HTTP/1.1 200 OK\n"));
}

void httpHeaderWriteEnd(int clientSocket) {
    write(clientSocket, HTTP_EOL, strlen(HTTP_EOL));
}

void httpBodyWriteFile(int clientSocket, FILE *file) {
    size_t bytesRead;
    char buffer[BUFSIZ];

    while ((bytesRead = fread(buffer, 1, BUFSIZ, file)) > 0) {
        write(clientSocket, buffer, bytesRead);
    }
}

void handleDir(int clientSocket, char *path, struct stat *st) {
    /* TODO: Implementation */
}

void handleFile(int clientSocket, char *path, struct stat *st) {
    FILE *fp = fopen(path, "rb");

    if (fp == NULL)
        return;

    /* Headers */
    httpHeaderWriteResponseOK(clientSocket);
    httpHeaderWriteDate(clientSocket);
    httpHeaderWriteContentLength(clientSocket, st);
    httpHeaderWriteEnd(clientSocket);

    /* Body */
    httpBodyWriteFile(clientSocket, fp);

    /* Cleanup */
    fclose(fp);
}

void handlePath(int clientSocket, char *path) {
    struct stat st;

    if (stat(path, &st) != 0)
        return;

    if (S_ISDIR(st.st_mode))
        handleDir(clientSocket, path, &st);
    else if (S_ISREG(st.st_mode))
        handleFile(clientSocket, path, &st);
}

void handleConnection(int clientSocket) {
    char buffer[BUFSIZ] = "";
    size_t bytesRead, messageSize = 0;
    char *uriPath;

    while ((bytesRead = read(clientSocket, buffer + messageSize, sizeof(buffer) - messageSize - 1))) {
        messageSize += bytesRead;
        if (messageSize > BUFSIZ - 1 || buffer[messageSize - 1] == '\n') break;
    }

    buffer[messageSize - 1] = 0;
    printf("REQUEST: %s\n", buffer);
    fflush(stdout);

    uriPath = httpClientReadUri(buffer);
    if(uriPath) {
        handlePath(clientSocket, uriPath + 1);
        free(uriPath);
    }

    close(clientSocket);
}

int main(int argc, char **argv) {
    signal(SIGHUP, shutdownProgram);
    signal(SIGINT, shutdownProgram);
    signal(SIGPIPE, noAction);

    serverSocket = serverStartup(SERVER_PORT);

    fd_set currentSockets, readySockets;
    FD_ZERO(&currentSockets);
    FD_SET(serverSocket, &currentSockets);

    while (1) {
        size_t i;
        int clientSocket;
        readySockets = currentSockets;

        if (select(FD_SETSIZE, &readySockets, NULL, NULL, NULL) < 0) {
            perror("Select");
            exit(1);
        }

        for (i = 0; i < FD_SETSIZE; i++) {
            if (FD_ISSET(i, &readySockets)) {
                if (i == serverSocket) {
                    clientSocket = acceptConnection(serverSocket);
                    FD_SET(clientSocket, &currentSockets);
                } else {
                    handleConnection(clientSocket);
                    FD_CLR(i, &currentSockets);
                }
            }
        }
    }
}