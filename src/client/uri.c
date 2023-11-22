#include "uri.h"

#include <ctype.h>

UriScheme uriDetailsGetScheme(UriDetails *details) {
    size_t len;

    if (!details->scheme)
        return PROTOCOL_UNKNOWN;

    len = strlen(details->scheme);
    switch (len) {
        default:
            return PROTOCOL_UNKNOWN;
        case 3:
            switch (toupper(details->scheme[0])) {
                case 'F':
                    if (toupper(details->scheme[1]) == 'T' && toupper(details->scheme[2]) == 'P')
                        return PROTOCOL_FTP;
            }
            break;
        case 4:
            switch (toupper(details->scheme[0])) {
                case 'F':
                    if (toupper(details->scheme[1]) == 'T' && toupper(details->scheme[2]) == 'P' &&
                        toupper(details->scheme[3]) == 'S')
                        return PROTOCOL_FTPS;
                case 'H':
                    if (toupper(details->scheme[1]) == 'T' && toupper(details->scheme[2]) == 'T' &&
                        toupper(details->scheme[3]) == 'P')
                        return PROTOCOL_HTTP;
            }
            break;
        case 5:
            switch (toupper(details->scheme[0])) {
                case 'H':
                    if (toupper(details->scheme[1]) == 'T' && toupper(details->scheme[2]) == 'T' &&
                        toupper(details->scheme[3]) == 'P' && toupper(details->scheme[4]) == 'S')
                        return PROTOCOL_HTTPS;
            }
    }

    return PROTOCOL_UNKNOWN;
}

char *uriDetailsGetHostAddr(UriDetails *details) {
    struct hostent *ent;

    if (!details || !(ent = gethostbyname(details->host)))
        return NULL;

    switch (ent->h_addrtype) {
        case AF_INET: {
            char *addr = inet_ntoa(*(struct in_addr *) ent->h_addr);
            char *r = malloc(strlen(addr) + 1);

            if (r)
                strcpy(r, addr);

            return r;
        }
        case AF_INET6: {
            char *addr = malloc(INET6_ADDRSTRLEN + 1);
            inet_ntop(AF_INET6, ent->h_addr, addr, INET6_ADDRSTRLEN);
            return addr;
        }
        default:
            return NULL;
    }
}

unsigned short uriDetailsGetPort(UriDetails *details) {
    size_t i, r, len;

    if (!details->port)
        return uriDetailsGetScheme(details);

    len = strlen(details->port);
    for (r = i = 0; i < len; ++i)
        r *= 10, r += (details->port[i] - '0');

    if (r > USHRT_MAX)
        return 0;

    return r;
}

UriDetails uriDetailsNewFrom(const char *uri) {
    UriDetails details;
    char *p, *q;

    details.scheme = details.host = details.port = details.path = NULL;

    /* Get Scheme */
    p = strstr(uri, "://");
    if (p) {
        size_t len = p - uri;
        details.scheme = malloc(len + 1);

        if (details.scheme)
            strncpy(details.scheme, uri, len), details.scheme[len] = '\0';

        q = p + 3;
    } else
        q = (char *) uri;

    /* Get Host */
    p = strchr(q, '/');
    if (p) {
        size_t len;
        char *i = strchr(q, ':');
        if (i && i < p) {
            len = i - q;
            details.host = malloc(len + 1);

            if (details.host)
                strncpy(details.host, q, len), details.host[len] = '\0';

            /* Optional Port */
            len = p - i - 1;
            details.port = malloc(len + 1);

            if (details.port)
                strncpy(details.port, i + 1, len), details.port[len] = '\0';

        } else {
            len = p - q;
            details.host = malloc(len + 1);

            if (details.host)
                strncpy(details.host, q, len), details.host[len] = '\0';

        }

        len = strlen(p);
        details.path = malloc(len + 1);

        if (details.path)
            strcpy(details.path, p);

    } else if (strlen(q)) {
        details.host = malloc(strlen(q) + 1);

        if (details.host)
            strcpy(details.host, q);

    }

    return details;
}

void uriDetailsFree(UriDetails *details) {
    if (details->scheme) free(details->scheme), details->scheme = NULL;
    if (details->host) free(details->host), details->host = NULL;
    if (details->port) free(details->port), details->port = NULL;
    if (details->path) free(details->path), details->path = NULL;
}
