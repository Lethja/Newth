#include "uri.h"
#include "../platform/platform.h"

#include <ctype.h>
#include <limits.h>

#pragma region Static Helper Functions

#ifdef GETHOSTBYNAME_CANT_IPV4STR

#ifdef UNIT_TEST

char isValidIpv4Str(const char *str) {

#else

static inline char isValidIpv4Str(const char *str) {

#endif
    unsigned int i, j, k;

    for (j = k = i = 0; str[i] != '\0'; ++i) {
        switch (str[i]) {
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
                j += str[i] - '0';
                break;
            case '.':
                if (j > 12)
                    return 0;

                j = 0, ++k;
                break;
            default:
                return 0;
        }
    }

    if (k != 3)
        return 0;

    return 1;
}

#endif /* GETHOSTBYNAME_CANT_IPV4STR */

#pragma endregion

UriScheme uriDetailsGetScheme(const UriDetails *details) {
    size_t len;

    if (!details->scheme)
        return SCHEME_UNKNOWN;

    len = strlen(details->scheme);
    switch (len) {
        default:
            return SCHEME_UNKNOWN;
        case 3:
            switch (toupper(details->scheme[0])) {
                case 'F':
                    if (toupper(details->scheme[1]) == 'T' && toupper(details->scheme[2]) == 'P')
                        return SCHEME_FTP;
            }
            break;
        case 4:
            switch (toupper(details->scheme[0])) {
                case 'F':
                    if (toupper(details->scheme[1]) == 'T' && toupper(details->scheme[2]) == 'P' &&
                        toupper(details->scheme[3]) == 'S')
                        return SCHEME_FTPS;
                case 'H':
                    if (toupper(details->scheme[1]) == 'T' && toupper(details->scheme[2]) == 'T' &&
                        toupper(details->scheme[3]) == 'P')
                        return SCHEME_HTTP;
            }
            break;
        case 5:
            switch (toupper(details->scheme[0])) {
                case 'H':
                    if (toupper(details->scheme[1]) == 'T' && toupper(details->scheme[2]) == 'T' &&
                        toupper(details->scheme[3]) == 'P' && toupper(details->scheme[4]) == 'S')
                        return SCHEME_HTTPS;
            }
    }

    return SCHEME_UNKNOWN;
}

char *uriDetailsGetHostAddr(UriDetails *details) {
    struct hostent *ent;

    if (!details || !details->host)
        return NULL;

#ifdef GETHOSTBYNAME_CANT_IPV4STR
    /* This implementation of gethostbyname() can resolve literal IPV4, check manually */
    if (isValidIpv4Str(details->host)) {
        char *r = malloc(strlen(details->host) + 1);

        if (r)
            strcpy(r, details->host);

        return r;
    }
#endif

    if (!(ent = gethostbyname(details->host)))
        return NULL;

    switch (ent->h_addrtype) {
        case AF_INET: {
            char *addr = inet_ntoa(*(struct in_addr *) ent->h_addr);
            char *r = malloc(strlen(addr) + 1);

            if (r)
                strcpy(r, addr);

            return r;
        }
#ifdef ENABLE_IPV6
        case AF_INET6: {
            char *addr = malloc(INET6_ADDRSTRLEN + 1);
            inet_ntop(AF_INET6, ent->h_addr, addr, INET6_ADDRSTRLEN);
            return addr;
        }
#endif
        default:
            return NULL;
    }
}

unsigned short uriDetailsGetPort(const UriDetails *details) {
#if UINT_MAX > USHRT_MAX
    unsigned int r;
#else
    unsigned long r;
#endif
    size_t i, len;

    if (!details->port)
        return uriDetailsGetScheme(details);

    len = strlen(details->port);
    for (r = i = 0; i < len; ++i)
        switch (details->port[i]) {
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
                r *= 10, r += (details->port[i] - '0');
                if (r <= USHRT_MAX)
                    break;
            default:
                return 0;
        }

    return (unsigned short) r;
}

static sa_family_t GetAddressFamily(const char *address) {
    if (strchr(address, ':'))
        return AF_INET6;
    else
        return AF_INET;
}

const char *uriDetailsCreateSocketAddress(UriDetails *self, SocketAddress *socketAddress, unsigned short scheme) {
    const char *err = "Unable to extract address information from URI";
    char *address = uriDetailsGetHostAddr(self);
    unsigned short port;

    if (!scheme)
        scheme = uriDetailsGetScheme(self);

    port = self->port ? uriDetailsGetPort(self) : scheme ? scheme : uriDetailsGetPort(self);

    if (!address || !port) {
        if (address)
            free(address);

        return err;
    }

    socketAddress->address.sock.sa_family = GetAddressFamily(address);

    switch (socketAddress->address.sock.sa_family) {
#ifdef ENABLE_IPV6
        case AF_INET6:
            if (inet_pton(AF_INET6, address, &socketAddress->ipv6.sin6_addr)) {
                socketAddress->ipv6.sin6_port = htons(port);
                break;
            } else
                goto uriDetailsCreateSocketAddress_abort;
#endif
        case AF_INET:
            if ((socketAddress->address.ipv4.sin_addr.s_addr = inet_addr(address))) {
                socketAddress->address.ipv4.sin_port = htons(port);
                break;
            }
#ifdef ENABLE_IPV6
uriDetailsCreateSocketAddress_abort:
#endif
        default:
            socketAddress->state = SA_STATE_FAILED, free(address);
            return err;
    }

    socketAddress->scheme = scheme, socketAddress->state = SA_STATE_QUEUED, free(address);
    return NULL;
}

UriDetails uriDetailsNewFrom(const char *uri) {
    UriDetails details;
    size_t len;
    char *p, *q, *i;

    details.scheme = details.host = details.port = details.path = details.query = NULL;

    if (!uri)
        return details;

    /* Get Scheme */
    p = strstr(uri, "://");
    if (p) {
        len = p - uri;

        if ((details.scheme = malloc(len + 1)))
            strncpy(details.scheme, uri, len), details.scheme[len] = '\0';

        q = p + 3;
    } else
        q = (char *) uri;

    /* Get Host */
    p = strchr(q, '/'), i = strchr(q, ':');

    if (p && i) {
        if (i < p) {
            len = i - q;

            if ((details.host = malloc(len + 1)))
                strncpy(details.host, q, len), details.host[len] = '\0';

            /* Optional Port */
            len = p - i - 1;

            if ((details.port = malloc(len + 1)))
                strncpy(details.port, i + 1, len), details.port[len] = '\0';

        } else {
            len = p - q;

            if ((details.host = malloc(len + 1)))
                strncpy(details.host, q, len), details.host[len] = '\0';

        }

        /* Get Path (optional query) */
        if ((q = strchr(p, '?'))) {
            len = q - p;
            if ((details.path = malloc(len + 1)))
                strncpy(details.path, p, len), details.path[len] = '\0';

            len = strlen(q);
            if ((details.query = malloc(len)))
                strcpy(details.query, &q[1]);
        } else {
            len = strlen(p);
            if ((details.path = malloc(len + 1)))
                strcpy(details.path, p);
        }

    } else if (p) {
        len = p - q;

        if ((details.host = malloc(len + 1)))
            strncpy(details.host, q, len), details.host[len] = '\0';

        /* Get Path (optional query) */
        if ((q = strchr(p, '?'))) {
            len = q - p;
            if ((details.path = malloc(len + 1)))
                strncpy(details.path, p, len), details.path[len] = '\0';

            len = strlen(q);
            if ((details.query = malloc(len)))
                strcpy(details.query, &q[1]);
        } else {
            len = strlen(p);
            if ((details.path = malloc(len + 1)))
                strcpy(details.path, p);
        }

    } else if (i) {
        len = i - q;

        if ((details.host = malloc(len + 1)))
            strncpy(details.host, q, len), details.host[len] = '\0';

        len = strlen(i);

        if ((details.port = malloc(len + 1)))
            strcpy(details.port, i + 1);
    } else if (strlen(q)) {
        if ((details.host = malloc(strlen(q) + 1)))
            strcpy(details.host, q);

    }

    return details;
}

char *uriDetailsCreateString(UriDetails *details) {
    char *r;
    size_t len;

    if (!details->scheme || !details->host || !details->path)
        return NULL;

    len = strlen(details->scheme) + strlen(details->host) + strlen(details->path) + 3;

    if (details->port && details->port != details->scheme) {
        len += strlen(details->port) + 1;
        if (details->query) {
            len += strlen(details->query) + 1;
            if ((r = malloc(len + 1)))
                sprintf(r, "%s://%s:%s%s?%s", details->scheme, details->host, details->port, details->path,
                        details->query);
        } else if ((r = malloc(len + 1)))
            sprintf(r, "%s://%s:%s%s", details->scheme, details->host, details->port, details->path);
    } else if (details->query) {
        len += strlen(details->query) + 1;
        if ((r = malloc(len + 1)))
            sprintf(r, "%s://%s%s?%s", details->scheme, details->host, details->path, details->query);
    } else if ((r = malloc(len + 1)))
        sprintf(r, "%s://%s%s", details->scheme, details->host, details->path);

    return r;
}

void uriDetailsFree(UriDetails *details) {
    if (details->scheme) free(details->scheme), details->scheme = NULL;
    if (details->host) free(details->host), details->host = NULL;
    if (details->port) free(details->port), details->port = NULL;
    if (details->path) free(details->path), details->path = NULL;
    if (details->query) free(details->query), details->query = NULL;
}

void uriPathCombine(char *output, const char *path1, const char *path2) {
    const char *pathDivider = "/";
    size_t len = strlen(path1), idx = len - 1;
    const char *p2;

    /* Copy first path into output buffer for manipulation */
    strcpy(output, path1);

    while (output[idx] == pathDivider[0] && idx)
        --idx;

    if (idx) {
        output[idx + 1] = '\0';
        strcat(output, pathDivider);
    } else if (output[0] == pathDivider[0] && output[1] == pathDivider[0])
        output[1] = '\0';

    /* Jump over any leading dividers in the second path then concatenate it */
    len = strlen(path2), idx = 0;

    while (path2[idx] == pathDivider[0] && idx < len)
        ++idx;

    if (idx == len)
        return;

    p2 = &path2[idx];
    strcat(output, p2);
}

static inline size_t JumpOverDotPaths(const char *path) {
    size_t r = 0;
    char skip = '/';
    while (*path != '\0') {
        if (*path != skip)
            break;
        skip = skip == '.' ? '/' : '.', ++r, ++path;
    }
    return r;
}

char *uriPathAbsoluteAppend(const char *currentPath, const char *append) {
    size_t len = strlen(currentPath) + strlen(append) + 2, i, j;
    char *c, *r;

    switch (append[0]) {
        case '/':
            if (!(r = malloc(strlen(append) + 1)))
                return NULL;

            strcpy(r, append);
            return r;
        case '.':
            switch (append[1]) {
                case '\0': /* Quick return for '.' */
                    if (!(r = malloc(strlen(currentPath) + 1)))
                        return NULL;

                    strcpy(r, currentPath);
                    return r;
                case '.': /* Explicit return if trying to go up a directory while already at trunk */
                    switch (append[3]) {
                        case '\0':
                        case '/':
                            if (currentPath[0] == '/' && currentPath[1] == '\0') {
                                if (!(r = malloc(2)))
                                    return NULL;

                                r[0] = '/', r[1] = '\0';
                                return r;
                            }
                    }
            }
    }

    if (currentPath[0] != '/' || !(c = calloc(len, sizeof(char))))
        return NULL;

    uriPathCombine(c, currentPath, append);

    if (!(r = malloc(len))) {
        free(c);
        return NULL;
    }

    for (i = j = 0; c[i] != '\0'; ++i, ++j) {
        switch (c[i]) {
            case '/':
                switch (c[i + 1]) {
                    case '.':
                        switch (c[i + 2]) {
                            case '/': {
                                r[j] = c[i];
                                i += JumpOverDotPaths(&c[i]);
                                switch (c[i]) {
                                    case '\0':
                                        goto uriPathAbsoluteAppend_finished;
                                    case '.':
                                        if (i > 3 && c[i - 1] == '.' && c[i - 2] == '/')
                                            i -= 2;
                                    default:
                                        --i;
                                }
                                break;
                            }
                            case '\0':
                                goto uriPathAbsoluteAppend_finished;
                            case '.':
                                switch (c[i + 3]) {
                                    case '/':
                                    case '\0':
                                        i += 3;
                                        if (j)
                                            do --j;
                                            while (j && r[j] != '/');

                                        if (!j) {
                                            ++j;
                                            goto uriPathAbsoluteAppend_finished;
                                        }

                                        if (c[i] == '/' && c[i + 1] == '.') {
                                            if (c[i + 2] == '/')
                                                i += JumpOverDotPaths(&c[i]);
                                            --i;
                                        } else if (c[i] == '\0')
                                            goto uriPathAbsoluteAppend_finished;

                                        break;
                                    default:
                                        r[j] = c[i];
                                        break;
                                }
                            default:
                                r[j] = c[i];
                        }
                }
            default:
                r[j] = c[i];
        }
    }

uriPathAbsoluteAppend_finished:
    r[j] = '\0', free(c);
    if (!(c = realloc(r, j + 1))) {
        free(r);
        return NULL;
    }

    return c;
}

void uriDetailsSetScheme(UriDetails *self, enum UriScheme scheme) {
    const char *str;
    switch (scheme) {
        default:
        case SCHEME_UNKNOWN:
            if (self->scheme)
                free(self->scheme), self->scheme = NULL;
            return;
        case SCHEME_HTTP:
            str = "http";
            break;
        case SCHEME_HTTPS:
            str = "https";
            break;
        case SCHEME_FTP:
            str = "ftp";
            break;
        case SCHEME_FTPS:
            str = "ftps";
            break;
        case SCHEME_FILE:
            str = "file";
            break;
    }

    if (self->scheme)
        free(self->scheme);

    if (!(self->scheme = malloc(strlen(str) + 1)))
        return;

    strcpy(self->scheme, str);
}

void uriDetailsSetAddress(UriDetails *self, struct sockaddr *socketAddress) {
    char *address, port[6], *p;
    switch (socketAddress->sa_family) {
        /* TODO: IPV6 */
        default:
            return;
        case AF_INET: {
            struct sockaddr_in *ipv4 = (struct sockaddr_in *) socketAddress;
            char *addr = inet_ntoa(ipv4->sin_addr);
            unsigned short prt;
            if (!(address = malloc(strlen(addr) + 1)))
                return;
            strcpy(address, addr);
            prt = htons(ipv4->sin_port);
            sprintf((char *) port, "%d", prt);
            break;
        }
    }

    if (!(p = malloc(strlen(port) + 1))) {
        free(address);
        return;
    }
    strcpy(p, port);

    if (self->host)
        free(self->host);
    if (self->port)
        free(self->port);

    self->host = address, self->port = p;
}
