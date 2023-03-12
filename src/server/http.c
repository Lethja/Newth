#include "http.h"

#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <time.h>
#include <linux/limits.h>

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

inline void AsciiToHex(char ascii, char out[4]) {
    if (isalnum(ascii))
        out[0] = ascii, out[1] = '\0';
    else
        snprintf(out, 4, "%%%x", ascii);
}

char convertPathToUrl(char *path, size_t max) {
    size_t i = 0;
    char out[4];
    size_t outLen;

    while (path[i] != '\0') {
        if (!isalnum(path[i]) && path[i] != '/') {
            AsciiToHex(path[i], out);
            outLen = strlen(out);

            if (strlen(path) + outLen > max - 1)
                return -1;

            memmove(&path[i + outLen], &path[i + 1], strlen(&path[i]) + 1);
            memcpy(&path[i], out, outLen);

            i += outLen - 1;
        }

        i++;
    }

    return 0;
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
    strncat(header, buffer, BUFSIZ - 1);
}

void httpHeaderWriteChunkedEncoding(char header[BUFSIZ]) {
    const char *chunked = "Transfer-Encoding: chunked\n";
    strncat(header, chunked, strlen(chunked));
}

void httpHeaderWriteContentLength(char header[BUFSIZ], size_t length) {
    const size_t max = 100;
    char buffer[max];
    snprintf(buffer, max, "Content-Length: %zu\n", length);
    strncat(header, buffer, BUFSIZ - 1);
}

void httpHeaderWriteContentLengthSt(char header[BUFSIZ], struct stat *st) {
    httpHeaderWriteContentLength(header, st->st_size);
}

void httpHeaderWriteLastModified(char header[BUFSIZ], struct stat *st) {
    const size_t max = 46;
    char buffer[max];

    strftime(buffer, max, "Last-Modified: %a, %d %b %Y %H:%M:%S GMT\n", gmtime(&st->st_mtim.tv_sec));
    strncat(header, buffer, BUFSIZ - 1);
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
    strncat(header, buffer, BUFSIZ - 1);
}

void httpHeaderWriteEnd(char header[BUFSIZ]) {
    char *r = strrchr(header, '\n');
    strncpy(r, HTTP_EOL, strlen(HTTP_EOL) + 1);
}

void htmlHeaderWrite(char buffer[BUFSIZ], char *title) {
    snprintf(buffer, BUFSIZ, "<!DOCTYPE HTML PUBLIC \"-//W3C//DTD HTML 4.01//EN\"\n"
                             "\t\"http://www.w3.org/TR/html4/strict.dtd\">\n"
                             "<HTML>\n"
                             "\t<HEAD>\n"
                             "\t\t<TITLE>%s</TITLE>\n"
                             "\t</HEAD>\n\n"
                             "\t<BODY>\n", title);
}

void htmlListStart(char buffer[BUFSIZ]) {
    const char *listStart = "\t\t<UL>\n";
    strncat(buffer, listStart, strlen(listStart));
}

void htmlListEnd(char buffer[BUFSIZ]) {
    const char *listEnd = "\t\t</UL>\n";
    strcpy(buffer, listEnd);
}

void htmlFooterWrite(char buffer[BUFSIZ]) {
    const char *htmlEnd = "\t</BODY>\n" "</HTML>\n";
    strcat(buffer, htmlEnd);
}

void htmlListWritePathLink(char buffer[BUFSIZ], char *webPath) {
    char linkPath[PATH_MAX], *filePath = strrchr(webPath, '/');
    size_t pathLen = strlen(webPath) + 1;

    if (filePath[0] == '\0')
        filePath = linkPath;

    memcpy(linkPath, webPath, pathLen);

    convertPathToUrl(linkPath, PATH_MAX);
    snprintf(buffer, BUFSIZ, "\t\t\t<LI><A HREF=\"%s\">%s</A></LI>\n", linkPath, filePath + 1);
}

size_t httpBodyWriteFile(int clientSocket, FILE *file) {
    size_t bytesRead;
    char buffer[BUFSIZ];

    while ((bytesRead = fread(buffer, 1, BUFSIZ, file)) > 0) {
        if (write(clientSocket, buffer, bytesRead) == -1)
            return 1;
    }

    return 0;
}

size_t httpBodyWriteChunk(int clientSocket, char buffer[BUFSIZ]) {
    char internalBuffer[BUFSIZ];

    if (strlen(buffer) < BUFSIZ / 2) {
        snprintf(internalBuffer, BUFSIZ, "%zx\r\n%s\r\n", strlen(buffer), buffer);
        if (write(clientSocket, internalBuffer, strlen(internalBuffer)) == -1)
            return 1;
    } else {
        char hold = buffer[BUFSIZ / 2];
        snprintf(internalBuffer, BUFSIZ, "%x\r\n%s\r\n", BUFSIZ / 2, buffer);
        if (write(clientSocket, internalBuffer, strlen(internalBuffer)) == -1)
            return 1;

        buffer[BUFSIZ / 2] = hold;
        snprintf(internalBuffer, BUFSIZ, "%zx\r\n%s\r\n", strlen(&buffer[BUFSIZ / 2]), &buffer[BUFSIZ / 2]);
        if (write(clientSocket, internalBuffer, strlen(internalBuffer)) == -1)
            return 1;
    }

    return 0;
}

size_t httpBodyWriteChunkEnding(int clientSocket) {
    const char *chunkEnding = "0\r\n\r\n";
    return write(clientSocket, chunkEnding, strlen(chunkEnding)) == -1;
}

size_t httpBodyWriteText(int clientSocket, const char *text) {
    return write(clientSocket, text, strlen(text)) == -1;
}

void httpHeaderWriteFileName(char header[BUFSIZ], char *path) {
    const size_t max = FILENAME_MAX;
    char buffer[max];
    char *name = strrchr(path, '/');
    if (!name)
        name = path;

    snprintf(buffer, max, "Content-Disposition: filename=\"%s\"\n", name);
    strncat(header, buffer, BUFSIZ - 1);
}