#include "../platform/platform.h"
#include "http.h"

#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

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

void httpHeaderWriteDate(SocketBuffer *socketBuffer) {
#define WRITE_DATE_MAX 39
    char buffer[WRITE_DATE_MAX];
    time_t rawTime;
    struct tm *timeInfo;

    time(&rawTime);
    timeInfo = gmtime(&rawTime);

    strftime(buffer, WRITE_DATE_MAX, "Date: %a, %d %b %Y %H:%M:%S GMT" HTTP_EOL, timeInfo);
    socketBufferWrite(socketBuffer, buffer);
}

void httpHeaderWriteChunkedEncoding(SocketBuffer *socketBuffer) {
    const char *chunked = "Transfer-Encoding: chunked" HTTP_EOL;
    socketBufferWrite(socketBuffer, chunked);
}

void httpHeaderWriteContentLength(SocketBuffer *socketBuffer, size_t length) {
#define CONTENT_LENGTH_MAX 100
    char buffer[CONTENT_LENGTH_MAX];
    snprintf(buffer, CONTENT_LENGTH_MAX, "Content-Length: %lu" HTTP_EOL, (unsigned long) length);
    socketBufferWrite(socketBuffer, buffer);
}

void httpHeaderWriteContentLengthSt(SocketBuffer *socketBuffer, struct stat *st) {
    httpHeaderWriteContentLength(socketBuffer, st->st_size);
}

void httpHeaderWriteLastModified(SocketBuffer *socketBuffer, struct stat *st) {
#define LAST_MODIFIED_MAX 47
    char buffer[LAST_MODIFIED_MAX];

    strftime(buffer, LAST_MODIFIED_MAX, "Last-Modified: %a, %d %b %Y %H:%M:%S GMT" HTTP_EOL,
             gmtime(&st->st_mtim.tv_sec));
    socketBufferWrite(socketBuffer, buffer);
}

void httpHeaderWriteResponse(SocketBuffer *socketBuffer, short response) {
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

    snprintf(buffer, RESPONSE_MAX, "HTTP/1.1 %s" HTTP_EOL, r);
    socketBufferWrite(socketBuffer, buffer);
}

void httpHeaderWriteEnd(SocketBuffer *socketBuffer) {
    socketBufferWrite(socketBuffer, HTTP_EOL);
}

void htmlHeaderWrite(char buffer[BUFSIZ], char *title) {
    snprintf(buffer, BUFSIZ, "<!DOCTYPE HTML PUBLIC \"-//W3C//DTD HTML 4.01//EN\"\n"
                             "\t\"http://www.w3.org/TR/html4/strict.dtd\">\n"
                             "<HTML>\n"
                             "\t<HEAD>\n"
                             "\t\t<TITLE>%s</TITLE>\n"
                             "\t\t<STYLE TYPE=\"text/css\">\n"
                             "\t\t*{\n"
                             "\t\t\tfont-family: monospace;\n"
                             "\t\t}\n"
                             "\t\t\n"
                             "\t\ta:hover,tr:hover{\n"
                             "\t\t\tfont-weight: bold;\n"
                             "\t\t}\n"
                             "\t\t</STYLE>\n"
                             "\t</HEAD>\n\n"
                             "\t<BODY>\n", title);
}

void htmlListStart(char buffer[BUFSIZ]) {
    const char *listStart = "\t\t<UL>\n";
    strncat(buffer, listStart, strlen(listStart));
}

void htmlListEnd(char buffer[BUFSIZ]) {
    const char *listEnd = "\t\t</UL>\n";
    strcat(buffer, listEnd);
}

void htmlFooterWrite(char buffer[BUFSIZ]) {
    const char *htmlEnd = "\t</BODY>\n" "</HTML>\n";
    strcat(buffer, htmlEnd);
}

void htmlListWritePathLink(char buffer[BUFSIZ], char *webPath) {
    char linkPath[FILENAME_MAX], *filePath = strrchr(webPath, '/');
    size_t pathLen = strlen(webPath) + 1;

    if (filePath[0] == '\0')
        filePath = linkPath;

    memcpy(linkPath, webPath, pathLen);

    convertPathToUrl(linkPath, FILENAME_MAX);
    snprintf(buffer, BUFSIZ, "\t\t\t<LI><A HREF=\"%s\">%s</A></LI>\n", linkPath, filePath + 1);
}

static inline void
getPathName(const char *path, size_t maxPaths, char linkPath[FILENAME_MAX], char displayPath[FILENAME_MAX]) {
    size_t i, max = strlen(path) + 1, currentPath = 0;
    memcpy(linkPath, path, max);

    for (i = 0; i < max; ++i) {
        if (linkPath[i] != '/')
            continue;
        else {
            if (currentPath != maxPaths)
                currentPath++;
            else {
                if (i)
                    linkPath[i + 1] = '\0';
                else {
                    linkPath[1] = '\0';
                    memcpy(displayPath, linkPath, 2);
                    return;
                }

                break;
            }
        }
    }

    if (currentPath) {
        do
            --i;
        while (linkPath[i] != '/');

        memcpy(displayPath, &linkPath[i + 1], max - i + 1);
    } else
        memcpy(displayPath, linkPath, max);
}

static inline size_t getPathCount(const char *path) {
    size_t r = 0;
    while (*path != '\0') {
        if (path[0] == '/' && path[1] != '\0')
            ++r;
        ++path;
    }
    return r;
}

void htmlBreadCrumbWrite(char buffer[BUFSIZ], const char *webPath) {
    size_t i, max = getPathCount(webPath) + 1;

    strncat(buffer, "\t\t<DIV>\n", 9);

    for (i = 0; i < max; ++i) {
        char internalBuffer[8210], linkPath[FILENAME_MAX], displayPath[FILENAME_MAX];
        getPathName(webPath, i, linkPath, displayPath);
        convertPathToUrl(linkPath, FILENAME_MAX);
        snprintf(internalBuffer, 8210, "\t\t\t<A HREF=\"%s\">%s</A>\n", linkPath, displayPath);
        strncat(buffer, internalBuffer, BUFSIZ - 1);
    }

    strncat(buffer, "\t\t</DIV>\n\t\t<HR>\n", 17);
}

size_t httpBodyWriteFile(SocketBuffer *socketBuffer, FILE *file) {
    char buffer[BUFSIZ];

    while (fread(buffer, 1, BUFSIZ, file) > 0) {
        if (socketBufferWrite(socketBuffer, buffer))
            return 1;
    }

    if (socketBufferFlush(socketBuffer))
        return 1;

    return 0;
}

size_t httpBodyWriteChunk(SocketBuffer *socketBuffer, char buffer[BUFSIZ]) {
#define HEX_MAX 32
    size_t bufLen = strlen(buffer);
    char internalBuffer[HEX_MAX + BUFSIZ + 1];
    snprintf(internalBuffer, sizeof(internalBuffer), "%zx\r\n%s\r\n", bufLen, buffer);
    return socketBufferWrite(socketBuffer, internalBuffer);
}

size_t httpBodyWriteChunkEnding(SocketBuffer *socketBuffer) {
    const char *chunkEnding = "0" HTTP_EOL HTTP_EOL;
    if (socketBufferWrite(socketBuffer, chunkEnding))
        return 1;
    return socketBufferFlush(socketBuffer);
}

size_t httpBodyWriteText(SocketBuffer *socketBuffer, const char *text) {
    return socketBufferWrite(socketBuffer, text);
}

void httpHeaderWriteFileName(SocketBuffer *socketBuffer, char *path) {
    char buffer[FILENAME_MAX];
    char *name = strrchr(path, '/');

    if (!name)
        name = path;
    else
        name++;

    snprintf(buffer, FILENAME_MAX, "Content-Disposition: filename=\"%s\"\n", name);
    socketBufferWrite(socketBuffer, buffer);
}
