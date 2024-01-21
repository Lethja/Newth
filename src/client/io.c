#include <ctype.h>
#include <sys/types.h>
#include "io.h"

char *generateSendRequest(const char *type, const char *path, const char *extra) {
    const char *http = HTTP_RES HTTP_EOL HTTP_EOL "  ";
    size_t len = strlen(type) + strlen(path) + strlen(http);
    char *req;

    if (extra)
        len += strlen(extra);

    /* Padding due to SIMD */
    if (!(req = malloc(len + 8)))
        return NULL;

    if (extra)
        sprintf(req, "%s %s %s%s" HTTP_EOL, type, path, http, extra);
    else
        sprintf(req, "%s %s %s" HTTP_EOL, type, path, http);

    return req;
}

char *FindHeader(const char *headerFile, const char *header, char **variable) {
    char *i, *s, *e;
    if ((i = platformStringFindNeedle(headerFile, header))) {
        if ((s = strchr(i, ':'))) {
            if ((e = strchr(s, '\r'))) {
                if (e[1] == '\n') {
                    size_t len;
                    char *r;

                    /* Skip any whitespace between ':' and value */
                    do
                        ++s;
                    while (*s == ' ' || *s == '\0');

                    len = e - s;
                    r = malloc(len + 1);
                    if (r) {
                        strncpy(r, s, len);
                        r[len] = '\0';
                        *variable = r;
                        return NULL;
                    } else
                        return strerror(errno);
                }
            }
        }
    }
    *variable = NULL;
    return "Header not found";
}

char *HttpGetEssentialResponse(const char *headerFile, char **scheme, char **response) {
    char *p = strstr(headerFile, HTTP_EOL), *s = NULL;
    size_t len;

    if (p)
        len = p - headerFile;
    else
        goto HttpGetEssentialResponse_Invalid;

    *scheme = *response = NULL;

    if (!(s = calloc(len + 1, sizeof(char))))
        return strerror(errno);

    strncpy(s, headerFile, len);
    if ((p = strchr(s, ' '))) {
        *response = &p[1], p[0] = '\0', *scheme = s;
        return NULL;
    }

    HttpGetEssentialResponse_Invalid:
    if (s)
        free(s);

    *scheme = *response = NULL;
    return "Not a valid header line";
}

char *ioCreateSocketFromSocketAddress(SocketAddress *self, SOCKET *sock) {
    if ((*sock = socket(self->address.sock.sa_family, SOCK_STREAM, 0)) == -1)
        return strerror(platformSocketGetLastError());

    return NULL;
}

char *ioHttpHeadRead(const SOCKET *socket, char **header) {
    const size_t headerMax = 4096;
    unsigned int totalBytes = 0;
    ssize_t bytesReceived;
    char *buf = malloc(SB_DATA_SIZE), *headEnd;
    if (!buf)
        return strerror(errno);

    if (!*header) {
        if (!(*header = calloc(SB_DATA_SIZE, sizeof(char)))) {
            free(buf);
            return strerror(errno);
        }
    }

    headEnd = NULL;

    while ((bytesReceived = recv(*socket, buf, SB_DATA_SIZE - 1, MSG_PEEK)) > 0 && !headEnd) {
        if (bytesReceived == -1)
            return strerror(platformSocketGetLastError());

        /* Check if the end of header was part of this buffer and truncate anything after if so */
        buf[bytesReceived] = '\0';
        if ((headEnd = strstr(buf, HTTP_EOL HTTP_EOL)) != NULL)
            headEnd[2] = '\0', bytesReceived = (ssize_t) strlen(buf);

        /* Write header buffer into return buffer */
        totalBytes += bytesReceived;

        {
            char *tmp = realloc(*header, totalBytes + 1);
            if (tmp)
                *header = tmp;
            else {
                free(buf), free(*header), *header = NULL;
                return strerror(errno);
            }
        }

        strncat(*header, buf, totalBytes);

        /* Non-peek to move buffer along */
        recv(*socket, buf, bytesReceived, 0);

        if (totalBytes >= headerMax) {
            free(buf), free(*header), *header = NULL;
            return "Header too large";
        }
    }

    /* If the buffer has a body after the head then jump over it so the next function is ready to read the body */
    if (headEnd)
        recv(*socket, buf, 2, 0);

    free(buf);

    return NULL;
}

char *ioGenerateHttpHeadRequest(const char *path, const char *extra) {
    return generateSendRequest("HEAD", path, extra);
}

char *ioGenerateHttpGetRequest(const char *path, const char *extra) {
    return generateSendRequest("GET", path, extra);
}
