#include "../platform/platform.h"
#include "event.h"
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
        ++url;
    }
}

char *httpClientReadUri(const char *request) {
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

static inline void httpHeaderDataClamp(char **str, size_t *length) {
    char *end;
    do {
        switch (**str) {
            case ' ':
            case ':':
            case '\t':
                ++*str;
                continue;
            case '\r':
            case '\n':
                *length = 0;
                return;
            default:
                break;
        }
        break;
    } while (**str != '\0');

    end = strchr(*str, '\r');
    if (!end) {
        end = strchr(*str, '\n');
        if (!end)
            *length = 0;
    }

    *length = end - *str;
}


static inline int ValidHttpDateStr(const char *date) {
    return date[3] == ',' && date[4] == ' ' && date[7] == ' ' && date[11] == ' ' && date[16] == ' ' &&
           date[19] == ':' && date[22] == ':' && date[25] == ' ';
}

char httpHeaderReadIfModifiedSince(const char *request, struct tm *tm) {
#define READ_DATE_MAX 39
    const char *headerValue = "If-Modified-Since";
    size_t length;
    char *data = strstr(request, headerValue);

    if (data) {
        data += strlen(headerValue);
        httpHeaderDataClamp(&data, &length);

        if (length && length < READ_DATE_MAX && ValidHttpDateStr(data)) {
            char date[READ_DATE_MAX];
            char day[4] = "", month[4] = "";
            strncpy(date, data, READ_DATE_MAX);
            date[length] = '\0';
            if (sscanf(date, "%3s, %d %3s %d %d:%d:%d GMT", day, &tm->tm_mday, month,
                       &tm->tm_year, /* NOLINT(cert-err34-c) */
                       &tm->tm_hour, /* NOLINT(cert-err34-c) */
                       &tm->tm_min, &tm->tm_sec)) {
                switch (toupper(day[0])) {
                    case 'F':
                        tm->tm_wday = 5;
                        break;
                    case 'M':
                        tm->tm_wday = 1;
                        break;
                    case 'S':
                        tm->tm_wday = toupper(day[1]) == 'A' ? 6 : 0;
                        break;
                    case 'T':
                        tm->tm_wday = toupper(day[1]) == 'U' ? 2 : 4;
                        break;
                    case 'W':
                        tm->tm_wday = 3;
                        break;
                    default:
                        return 1;
                }

                switch (toupper(month[0])) {
                    case 'A':
                        tm->tm_mon = toupper(month[1]) == 'P' ? 3 : 7;
                        break;
                    case 'D':
                        tm->tm_mon = 11;
                        break;
                    case 'F':
                        tm->tm_mon = 1;
                        break;
                    case 'J':
                        tm->tm_mon = toupper(month[1]) == 'A' ? 0 : toupper(month[3]) == 'L' ? 6 : 5;
                        break;
                    case 'M':
                        tm->tm_mon = toupper(month[2]) == 'R' ? 2 : 4;
                        break;
                    case 'N':
                        tm->tm_mon = 10;
                        break;
                    case 'O':
                        tm->tm_mon = 9;
                        break;
                    case 'S':
                        tm->tm_mon = 8;
                        break;
                    default:
                        return 1;
                }

                tm->tm_year -= 1900;
                return 0;
            }
        }
    }

    return 1;
}

char httpHeaderReadRange(const char *request, off_t *start, off_t *end) {
#define MAX_RANGE_STR_BUF 128
    const char *headerValue = "Range", *parameter = "bytes=";
    size_t strLength;
    char *data = strstr(request, headerValue);

    if (data) {
        char range[MAX_RANGE_STR_BUF], *dash, *equ;
        data += strlen(headerValue);
        httpHeaderDataClamp(&data, &strLength);

        if (strLength >= MAX_RANGE_STR_BUF - 1)
            return 1;

        strncpy(range, data, strLength);
        range[strLength] = '\0';
        data = strstr(range, parameter);
        if (!data)
            return 1;

        data += strlen(parameter);
        equ = strchr(range, '=');
        dash = strchr(range, '-');

        if (!equ || !dash)
            return 1;

        *equ = *dash = '\0';

        if (equ + 1 != dash)
            *start = atol(equ + 1);
        if (dash[1] != '\0')
            *end = atol(dash + 1);
        return 0;
    }
    return 1;
}

void httpHeaderWriteAcceptRanges(SocketBuffer *socketBuffer) {
    const char *str = "Accept-Ranges: bytes" HTTP_EOL;
    socketBufferWrite(socketBuffer, str);
}

void httpHeaderWriteRange(SocketBuffer *socketBuffer, off_t start, off_t finish, off_t fileLength) {
    char buf[128];
    snprintf(buf, 128, "Content-Range: bytes %lu-%lu/%lu" HTTP_EOL, start, finish == fileLength ? finish - 1 : finish,
             fileLength);
    socketBufferWrite(socketBuffer, buf);
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

    strftime(buffer, LAST_MODIFIED_MAX, "Last-Modified: %a, %d %b %Y %H:%M:%S GMT" HTTP_EOL, gmtime(&st->st_mtime));
    socketBufferWrite(socketBuffer, buffer);
}

void httpHeaderWriteContentType(SocketBuffer *socketBuffer, char *type, char *charSet) {
    char buffer[256];
    snprintf(buffer, 256, "Content-Type: %s; %s" HTTP_EOL, type, charSet);
    socketBufferWrite(socketBuffer, buffer);
}

char *httpHeaderGetResponse(short response) {
    char *r;
    switch (response) {
        case 200:
            r = "200 OK";
            break;
        case 204:
            r = "204 No Content";
            break;
        case 206:
            r = "206 Partial Content";
            break;
        case 304:
            r = "304 Not Modified";
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
    return r;
}

void inline httpHeaderWriteResponseStr(SocketBuffer *socketBuffer, const char *response) {
#define RESPONSE_MAX 128
    char buffer[RESPONSE_MAX];

    snprintf(buffer, RESPONSE_MAX, "HTTP/1.1 %s" HTTP_EOL, response);
    socketBufferWrite(socketBuffer, buffer);
}

void httpHeaderWriteResponse(SocketBuffer *socketBuffer, short response) {
#define RESPONSE_MAX 128
    char *r = httpHeaderGetResponse(response);
    httpHeaderWriteResponseStr(socketBuffer, r);
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
    char linkPath[FILENAME_MAX], *filePath = NULL;
    size_t pathLen = strlen(webPath) + 1, i;

    memcpy(linkPath, webPath, pathLen);

    /* Find the second last '/' in the webPath, use it as the name of the HTML link */
    for (i = pathLen - 3; i > 0; --i) {
        if (webPath[i] == '/') {
            filePath = &webPath[i];
            break;
        }
    }

    /* If a filePath couldn't be set, use the full path */
    if (filePath == NULL)
        filePath = webPath;

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
                ++currentPath;
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

    if (linkPath[i - 1] == '\0') {
        --i;
        linkPath[i] = '/', linkPath[i + 1] = '\0';
    }

    if (currentPath) {
        do
            --i;
        while (linkPath[i] != '/');

        memcpy(displayPath, &linkPath[i + 1], max - i);
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

        if (strlen(buffer) + strlen(internalBuffer) + 1 > BUFSIZ - 17)
            break;

        strncat(buffer, internalBuffer, BUFSIZ - 1);
    }

    strncat(buffer, "\t\t</DIV>\n\t\t<HR>\n", 17);
}

size_t httpBodyWriteFile(SocketBuffer *socketBuffer, FILE *file, off_t start, off_t finish) {
    char buffer[BUFSIZ];
    off_t length = finish - start;
    unsigned long bytesRead;

    if (start)
        fseek(file, start, SEEK_SET);

    while ((bytesRead = fread(buffer, 1, length, file)) > 0) {
        length -= (off_t) bytesRead;
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
    snprintf(internalBuffer, sizeof(internalBuffer), "%lx\r\n%s\r\n", (unsigned long) bufLen, buffer);
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
        ++name;

    snprintf(buffer, FILENAME_MAX, "Content-Disposition: filename=\"%s\"\n", name);
    socketBufferWrite(socketBuffer, buffer);
}

char httpHeaderHandleError(SocketBuffer *socketBuffer, const char *path, char httpType, short error) {
#ifndef NDEBUG
    switch (error) {
        case 200:
        case 204:
        case 206:
        case 304:
            printf("Warning: valid HTTP request %d is in error path\n", error);
            break;
        default:
            printf("Warning: error HTTP request %d\n", error);
    }
#endif

    {
        char *errMsg = httpHeaderGetResponse(error);
        httpHeaderWriteResponseStr(socketBuffer, errMsg);
        httpHeaderWriteDate(socketBuffer);
        httpHeaderWriteContentLength(socketBuffer, strlen(errMsg));
        httpHeaderWriteEnd(socketBuffer);
        eventHttpRespondInvoke(&socketBuffer->clientSocket, path, httpType, error);

        if (httpBodyWriteText(socketBuffer, errMsg) || socketBufferFlush(socketBuffer))
            return 1;
    }
    return 0;
}

httpType httpClientReadType(const char *request) {
    switch (toupper(request[0])) {
        case 'G':
            return httpGet;
        case 'H':
            return httpHead;
        case 'P':
            return httpPost;
        default:
            return httpUnknown;
    }
}
