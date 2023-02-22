#include <sys/stat.h>
#include <time.h>
#include "platform/unix.h"

int serverSocket;

void noAction() {}

void closeBindSocket() {
    if (serverSocket)
        shutdown(serverSocket, SHUT_RDWR);
}

void shutdownCrash() {
    closeBindSocket();
    puts("Segfault");
    exit(1);
}

void shutdownProgram() {
    closeBindSocket();
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

void httpHeaderWriteContentLength(int clientSocket, size_t length) {
    const size_t max = 100;
    char buffer[max];
    snprintf(buffer, max, "Content-Length: %zu\n", length);
    write(clientSocket, buffer, strlen(buffer));
}

void httpHeaderWriteContentLengthSt(int clientSocket, struct stat *st) {
    httpHeaderWriteContentLength(clientSocket, st->st_size);
}

#define WRITESTR(c, str) write(c, str, strlen(str))

void httpHeaderWriteResponse(int clientSocket, short response) {
    WRITESTR(clientSocket, "HTTP/1.1 ");
    switch (response) {
        case 200:
            WRITESTR(clientSocket, "200 OK\n");
            break;
        case 204:
            WRITESTR(clientSocket, "204 No Content\n");
            break;
        case 404:
            WRITESTR(clientSocket, "404 Not Found\n");
            break;
        case 431:
            WRITESTR(clientSocket, "431 Request Header Fields Too Large\n");
            break;
        case 500:
        default:
            WRITESTR(clientSocket, "500 Internal Server Error\n");
            break;
    }
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

void httpBodyWriteText(int clientSocket, const char *text) {
    write(clientSocket, text, strlen(text));
}

void handleDir(int clientSocket, char *path, struct stat *st) {
    /* TODO: Implementation */
}

void handleFile(int clientSocket, char *path, struct stat *st) {
    FILE *fp = fopen(path, "rb");

    if (fp == NULL)
        return;

    /* Headers */
    httpHeaderWriteResponse(clientSocket, 200);
    httpHeaderWriteDate(clientSocket);
    httpHeaderWriteContentLengthSt(clientSocket, st);
    httpHeaderWriteEnd(clientSocket);

    /* Body */
    httpBodyWriteFile(clientSocket, fp);

    /* Cleanup */
    fclose(fp);
}

void handlePath(int clientSocket, char *path) {
    struct stat st;

    if (stat(path, &st) != 0) {
        const char *body = "Not Found";
        httpHeaderWriteResponse(clientSocket, 404);
        httpHeaderWriteDate(clientSocket);
        httpHeaderWriteContentLength(clientSocket, strlen(body));
        httpHeaderWriteEnd(clientSocket);
        httpBodyWriteText(clientSocket, body);
        return;
    }

    if (S_ISDIR(st.st_mode))
        handleDir(clientSocket, path, &st);
    else if (S_ISREG(st.st_mode))
        handleFile(clientSocket, path, &st);
}

void handleConnection(int clientSocket) {
#define BUFFER_SIZE 4096
    char buffer[BUFFER_SIZE];
    size_t bytesRead, messageSize = 0;
    char *uriPath;

    while ((bytesRead = read(clientSocket, buffer + messageSize, sizeof(buffer) - messageSize - 1))) {
        messageSize += bytesRead;
        if (messageSize > BUFFER_SIZE - 1) {
            httpHeaderWriteResponse(clientSocket, 431);
            httpHeaderWriteDate(clientSocket);
            httpHeaderWriteContentLength(clientSocket, 0);
            httpHeaderWriteEnd(clientSocket);
            return;
        }

        if (buffer[messageSize - 1] == '\n')
            break;
    }

    buffer[messageSize - 1] = 0;
    printf("REQUEST: %s\n", buffer);
    fflush(stdout);

    uriPath = httpClientReadUri(buffer);
    if (uriPath) {
        handlePath(clientSocket, uriPath + 1);
        free(uriPath);
    }
}

int main(int argc, char **argv) {
    signal(SIGHUP, shutdownProgram);
    signal(SIGINT, shutdownProgram);
    signal(SIGPIPE, noAction);
    signal(SIGSEGV, shutdownCrash);
    signal(SIGTERM, shutdownProgram);

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