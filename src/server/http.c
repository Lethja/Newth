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

void httpHeaderWriteDate(char header[BUFSIZ]) {
    const char max = 37;
    char buffer[max];
    time_t rawTime;
    struct tm *timeInfo;

    time(&rawTime);
    timeInfo = gmtime(&rawTime);

    strftime(buffer, max, "Date: %a, %d %b %Y %H:%M:%S GMT\n", timeInfo);
    strncat(header, buffer, BUFSIZ-1);
}

void httpHeaderWriteChunkedEncoding(char header[BUFSIZ]) {
    const char *chunked = "Transfer-Encoding: chunked\n";
    strncat(header, chunked, strlen(chunked));
}

void httpHeaderWriteContentLength(char header[BUFSIZ], size_t length) {
    const size_t max = 100;
    char buffer[max];
    snprintf(buffer, max, "Content-Length: %zu\n", length);
    strncat(header, buffer, BUFSIZ-1);
}

void httpHeaderWriteContentLengthSt(char header[BUFSIZ], struct stat *st) {
    httpHeaderWriteContentLength(header, st->st_size);
}

void httpHeaderWriteLastModified(char header[BUFSIZ], struct stat *st) {
    const size_t max = 46;
    char buffer[max];

    strftime(buffer, max, "Last-Modified: %a, %d %b %Y %H:%M:%S GMT\n", gmtime(&st->st_mtim.tv_sec));
    strncat(header, buffer, BUFSIZ-1);
}

void httpHeaderWriteResponse(char header[BUFSIZ], short response) {
    const size_t max = 128;
    char buffer[max], *r;

    switch (response) {
        case 200:
            r = "200 OK";
            break;
        case 204:
            r = "204 No Content";
            break;
        case 404:
            r = "404 Not Found";
            break;
        case 431:
            r = "431 Request Header Fields Too Large";
            break;
        case 500:
        default:
            r = "500 Internal Server Error";
            break;
    }

    snprintf(buffer, max, "HTTP/1.1 %s\n", r);
    strncat(header, buffer, BUFSIZ-1);
}

void httpHeaderWriteEnd(char header[BUFSIZ]) {
    char *r = strrchr(header, '\n');
    strncpy(r, HTTP_EOL, strlen(HTTP_EOL) + 1);
}

void httpBodyWriteFile(int clientSocket, FILE *file) {
    size_t bytesRead;
    char buffer[BUFSIZ];

    while ((bytesRead = fread(buffer, 1, BUFSIZ, file)) > 0) {
        if (write(clientSocket, buffer, bytesRead) == -1) {
            perror("Error writing small file body");
            break;
        }
    }
}

size_t httpBodyWriteText(int clientSocket, const char *text) {
    return write(clientSocket, text, strlen(text));
}

void httpHeaderWriteFileName(char header[BUFSIZ], char *path) {
    const size_t max = FILENAME_MAX;
    char buffer[max];
    char *name = strrchr(path, '/');
    if (!name)
        name = path;

    snprintf(buffer, max, "Content-Disposition: filename=\"%s\"\n", name);
    strncat(header, buffer, BUFSIZ-1);
}