#include "../platform/platform.h"
#include "http.h"

#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <time.h>
#include <linux/limits.h>

static inline char HexToAscii(const char *hex) {
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

static inline void AsciiToHex(char ascii, char out[4]) {
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
    char *start, *end, *path;
    size_t len;

    start = strchr(request, ' ');
    if (!start)
        return NULL;

    end = strchr(start + 1, ' ');
    len = end - start;

    path = malloc(len + 1);
    memcpy(path, start + 1, len - 1);
    path[len - 1] = '\0';

    convertUrlToPath(path);

    return path;
}

void httpHeaderWriteDate(char header[BUFSIZ]) {
#define WRITE_DATE_MAX 37
    char buffer[WRITE_DATE_MAX];
    time_t rawTime;
    struct tm *timeInfo;

    time(&rawTime);
    timeInfo = gmtime(&rawTime);

    strftime(buffer, WRITE_DATE_MAX, "Date: %a, %d %b %Y %H:%M:%S GMT\n", timeInfo);
    strncat(header, buffer, BUFSIZ - 1);
}

void httpHeaderWriteChunkedEncoding(char header[BUFSIZ]) {
    const char *chunked = "Transfer-Encoding: chunked\n";
    strncat(header, chunked, strlen(chunked));
}

void httpHeaderWriteContentLength(char header[BUFSIZ], size_t length) {
#define CONTENT_LENGTH_MAX 100
    char buffer[CONTENT_LENGTH_MAX];
    snprintf(buffer, CONTENT_LENGTH_MAX, "Content-Length: %lu\n", (unsigned long) length);
    strncat(header, buffer, BUFSIZ - 1);
}

void httpHeaderWriteContentLengthSt(char header[BUFSIZ], struct stat *st) {
    httpHeaderWriteContentLength(header, st->st_size);
}

void httpHeaderWriteLastModified(char header[BUFSIZ], struct stat *st) {
#define LAST_MODIFIED_MAX 46
    char buffer[LAST_MODIFIED_MAX];

    strftime(buffer, LAST_MODIFIED_MAX, "Last-Modified: %a, %d %b %Y %H:%M:%S GMT\n", gmtime(&st->st_mtim.tv_sec));
    strncat(header, buffer, BUFSIZ - 1);
}

void httpHeaderWriteResponse(char header[BUFSIZ], short response) {
#define RESPONSE_MAX 128
    char buffer[RESPONSE_MAX], *r;

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

    snprintf(buffer, RESPONSE_MAX, "HTTP/1.1 %s\n", r);
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

static inline void getPathName(const char *path, size_t maxPaths, char linkPath[PATH_MAX], char displayPath[PATH_MAX]) {
    size_t i, max = strlen(path) + 1, currentPath = 0;
    memcpy(linkPath, path, max);

    for (i = 0; i < max; ++i) {
        if (linkPath[i] == '/') {
            if (currentPath == maxPaths) {
                if (i == 0) {
                    linkPath[1] = '\0';
                    memcpy(displayPath, linkPath, 2);
                    return;
                } else
                    linkPath[i + 1] = '\0';

                getPathNameRewind:
                if(linkPath[i] == '/')
                    linkPath--;

                for (;; --i) {
                    if (linkPath[i] == '/') {
                        memcpy(displayPath, &linkPath[i + 1], strlen(&linkPath[i + 1]) + 1);
                        return;
                    }
                }
            } else
                ++currentPath;
        }
    }
    goto getPathNameRewind;
}

static inline size_t getPathCount(const char *path) {
    size_t r = 0;
    while (*path != '\0') {
        if (*path == '/')
            ++r;
        ++path;
    }
    return r;
}

void htmlBreadCrumbWrite(char buffer[BUFSIZ], const char *webPath) {
    size_t i, max = getPathCount(webPath) + 1;

    for (i = 0; i < max; ++i) {
        char internalBuffer[BUFSIZ], linkPath[PATH_MAX], displayPath[PATH_MAX];
        getPathName(webPath, i, linkPath, displayPath);
        convertPathToUrl(linkPath, PATH_MAX);
        snprintf(internalBuffer, BUFSIZ, "\t\t<A HREF=\"%s\">%s</A>\n", linkPath, displayPath);
        strncat(buffer, internalBuffer, strlen(internalBuffer) + 1);
    }

    strncat(buffer, "\t\t<HR>\n", 8);
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
#define HEX_MAX 32
    size_t bufLen = strlen(buffer), hexLen;
    char hex[HEX_MAX];
    snprintf(hex, HEX_MAX, "%lx\r\n", (unsigned long) bufLen);
    hexLen = strlen(hex);

    /* Reduce system calls if possible */
    if (bufLen + hexLen + 3 < BUFSIZ) {
        memmove(buffer + hexLen, buffer, bufLen);
        memcpy(buffer + hexLen + bufLen, "\r\n", 3);
        memcpy(buffer, hex, hexLen);
        if (write(clientSocket, buffer, hexLen + bufLen + 2) == -1)
            return 1;
    } else if (write(clientSocket, hex, hexLen) == -1 || write(clientSocket, buffer, bufLen) == -1)
        return 1;

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
    char buffer[FILENAME_MAX];
    char *name = strrchr(path, '/');

    if (!name)
        name = path;
    else
        name++;

    snprintf(buffer, FILENAME_MAX, "Content-Disposition: filename=\"%s\"\n", name);
    strncat(header, buffer, BUFSIZ - 1);
}
