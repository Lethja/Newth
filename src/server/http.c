#include "http.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <time.h>

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