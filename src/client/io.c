#include <ctype.h>
#include <sys/types.h>
#include "recvbufr.h"
#include "io.h"

#pragma region Static Helper Function

/**
 * Determines if this character is valid to represent part of a hex number from '0' to 'f'
 * @param c The character to test
 * @return zero if this character is not part of the hex character otherwise non-zero
 * @remark 'x' is not considered a valid hex character.
 * If your string is formatted like "0xff" then skip the first two character when using this function in a loop
 */
static char IsValidHexCharacter(char c) {
    switch (toupper(c)) {
        case '0':
        case '1':
        case '2':
        case '3':
        case '4':
        case '5':
        case '6':
        case '7':
        case '8':
        case '9':
        case 'A':
        case 'B':
        case 'C':
        case 'D':
        case 'E':
        case 'F':
            return 1;
        default:
            return 0;
    }
}

#pragma endregion


char *ioHttpRequestGenerateSend(const char *type, const char *path, const char *extra) {
    const char *http = HTTP_RES HTTP_EOL HTTP_EOL "  ";
    size_t len = strlen(type) + strlen(path) + strlen(http);
    char *req;

    if (extra)
        len += strlen(extra);

    /* Padding to make valgrind happy on SIMD machines */
    if (!(req = malloc(len + 8)))
        return NULL;

    if (extra)
        sprintf(req, "%s %s %s%s" HTTP_EOL, type, path, HTTP_RES HTTP_EOL, extra);
    else
        sprintf(req, "%s %s %s" HTTP_EOL, type, path, HTTP_RES HTTP_EOL);

    return req;
}

char *ioHttpResponseHeaderFind(const char *headerFile, const char *header, char **variable) {
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

char *ioHttpResponseHeaderEssential(const char *headerFile, char **scheme, char **response) {
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

const char *ioHttpResponseHeaderRead(RecvBuffer *socket, char **header) {
    const char *token = HTTP_EOL HTTP_EOL, *e;
    char *t;
    size_t len = 4;

    if (!header)
        return "Header not assigned";

    recvBufferClear(socket);
    recvBufferSetLengthToken(socket, token, len);

    if ((e = recvBufferFindAndFetch(socket, token, len, 4096)))
        goto ioHttpResponseHeaderRead_abort;

    len = socket->len;
    *header = calloc(len + 1, 1);

    if ((e = recvBufferFetch(socket, *header, 0, len + 1)))
        goto ioHttpResponseHeaderRead_abort1;

    /* Strip excess allocation that might occur */
    if (!(t = strstr(*header, token))) {
        e = "No token found";
        goto ioHttpResponseHeaderRead_abort1;
    } else {
        len = (t + 2) - *header, header[0][len] = '\0';
        if (platformHeapResize((void **) header, sizeof(char), len + 1))
            goto ioHttpResponseHeaderRead_abort1;
    }

    /* If the buffer has a body after the head then jump over it so the next function is ready to read the body */
    recvBufferDitch(socket, (PlatformFileOffset) (len + 2));

    return NULL;

    ioHttpResponseHeaderRead_abort1:
    free(*header), *header = NULL;

    ioHttpResponseHeaderRead_abort:
    return e;
}

char *ioHttpRequestGenerateHead(const char *path, const char *extra) {
    return ioHttpRequestGenerateSend("HEAD", path, extra);
}

char *ioHttpRequestGenerateGet(const char *path, const char *extra) {
    return ioHttpRequestGenerateSend("GET", path, extra);
}

const char *ioHttpBodyChunkHexToSize(const char *hex, size_t *value) {
    size_t i, max = strlen(hex);
    *value = 0;

    for (i = 0; i < max; i++) {
        if (IsValidHexCharacter(hex[i])) {
            char v = (char) (((hex[i] & 0xF) + (hex[i] >> 6)) | ((hex[i] >> 3) & 0x8));
            *value = (*value << 4) | (size_t) v;
        } else
            return "Illegal hex character";
    }

    return NULL;
}

char *ioHttpBodyChunkStrip(char *data, size_t *max, size_t *len) {
    size_t i, j, l;
    char chunk[20] = {0};

    if (*len == -1) {
        ++*len;

        for (i = j = 0, l = *max > 19 ? 19 : *max; j < l; ++j) {
            if (data[j] == '\r') {
                memcpy(chunk, &data[i], j - i), chunk[j] = '\0';
                break;
            }
        }

        if (!(l = strlen(chunk)))
            return "Malformed or impractically large HTTP chunk request";

        if (!(ioHttpBodyChunkHexToSize(chunk, &j)) && j)
            *len = i + l + 2, memmove(data, &data[*len], *max - *len), *max -= *len, *len = j;
        else {
            *len = -1, *max -= 3;
            return NULL;
        }

    } else
        i = 0;

    for (; i < *max; ++i) {
        if (*len > 0)
            --*len;
        else if (data[i] == '\r' && data[i + 1] == '\n') {
            j = i + 2;

            /* Move hex string into its own buffer */
            for (l = j + 19 > *max ? *max : j + 19; j < l; ++j) {
                if (data[j] == '\r' && data[j + 1] == '\n') {
                    memcpy(chunk, &data[i + 2], j - i - 2), chunk[j - i - 2] = '\0';
                    break;
                }
            }

            if (j == l) {
                *max = i;
                return "Chunk metadata overflows buffer";
            }

            if (!(l = strlen(chunk)))
                return "Malformed or impractically large HTTP chunk request";

            /* Remove chunk from data stream so it can be processed by other functions */
            if (!(ioHttpBodyChunkHexToSize(chunk, &j)) && j) {
                *len = i + l + 4, memmove(&data[i], &data[*len], *max - *len);
                *max -= *len - i, *len = j, --i;
            } else {
                *len = -1, *max -= 5;
                return NULL;
            }
        } else
            return "Unexpected end to chunk";
    }

    return NULL;
}
