#include <ctype.h>
#include "io.h"

char *SendRequest(const SOCKET *socket, const char *type, const char *path, const char *extra) {
    const char *http = HTTP_RES HTTP_EOL HTTP_EOL;
    size_t i = 0, len = strlen(type) + strlen(path) + strlen(http) + 2;
    char *req;

    if (extra)
        len += strlen(extra);

    if (!(req = malloc(len + 1)))
        return strerror(errno);

    if (extra)
        sprintf(req, "%s %s %s%s", type, path, http, extra);
    else
        sprintf(req, "%s %s %s", type, path, http);

    do {
        size_t r = send(*socket, &req[i], len - i, 0);
        if (r == -1) {
            free(req);
            return strerror(platformSocketGetLastError());
        }

        i += r;
    } while (i != len);

    free(req);
    return NULL;
}

char *FindHeader(const char *headerFile, const char *header, char **variable) {
    char *i, *s, *e;
    if ((i = strstr(headerFile, header))) {
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
    PlatformFileOffset totalBytes = 0;
    ssize_t bytesReceived;
    char *buf = malloc(SB_DATA_SIZE), *headEnd;
    if (!buf)
        return strerror(errno);

    if (!*header) {
        if (!(*header = malloc(SB_DATA_SIZE))) {
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
        if ((headEnd = strstr(buf, HTTP_EOL HTTP_EOL)) != NULL) {
            bytesReceived = (headEnd - buf);
        }

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

        strncat(*header, buf, totalBytes + 1);

        /* Non-peek to move buffer along */
        recv(*socket, buf, bytesReceived, 0);
    }

    /* If the buffer has a body after the head then jump over it so the next function is ready to read the body */
    if (headEnd)
        recv(*socket, buf, 4, 0);

    return NULL;
}

char *ioHttpHeadRequest(const SOCKET *socket, const char *path, const char *extra) {
    return SendRequest(socket, "HEAD", path, extra);
}

char *ioHttpGetRequest(const SOCKET *socket, const char *path, const char *extra) {
    return SendRequest(socket, "GET", path, extra);
}
