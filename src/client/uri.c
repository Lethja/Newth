#include "uri.h"

#include <ctype.h>

UriScheme uriDetailsGetScheme(UriDetails *details) {
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

unsigned short uriDetailsGetPort(UriDetails *details) {
    size_t i, len;
    unsigned int r;

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
                break;
            default:
                return 0;
        }

    if (r > USHRT_MAX)
        return 0;

    return (unsigned short) r;
}

static sa_family_t GetAddressFamily(const char *address) {
    if (strchr(address, ':'))
        return AF_INET6;
    else
        return AF_INET;
}

char uriDetailsCreateSocketAddress(UriDetails *self, SocketAddress *socketAddress, unsigned short scheme) {
    char *address = uriDetailsGetHostAddr(self);
    unsigned short port = self->port ? uriDetailsGetPort(self) : scheme ? scheme : uriDetailsGetPort(self);

    if (!port)
        port = scheme;

    if (!address || !port)
        return 1;

    socketAddress->sock.sa_family = GetAddressFamily(address);

    switch (socketAddress->sock.sa_family) {
#ifdef ENABLE_IPV6
        case AF_INET6:
            if (inet_pton(AF_INET6, address, &socketAddress->ipv6.sin6_addr)) {
                socketAddress->ipv6.sin6_port = htons(port);
                break;
            } else
                goto uriDetailsCreateSocketAddress_abort;
#endif
        case AF_INET:
            if ((socketAddress->ipv4.sin_addr.s_addr = inet_addr(address))) {
                socketAddress->ipv4.sin_port = htons(port);
                break;
            }
#ifdef ENABLE_IPV6
        uriDetailsCreateSocketAddress_abort:
#endif
        default:
            free(address);
            return 1;
    }

    free(address);
    return 0;
}

UriDetails uriDetailsNewFrom(const char *uri) {
    UriDetails details;
    size_t len;
    char *p, *q, *i;

    details.scheme = details.host = details.port = details.path = NULL;

    /* Get Scheme */
    p = strstr(uri, "://");
    if (p) {
        len = p - uri;
        details.scheme = malloc(len + 1);

        if (details.scheme)
            strncpy(details.scheme, uri, len), details.scheme[len] = '\0';

        q = p + 3;
    } else
        q = (char *) uri;

    /* Get Host */
    p = strchr(q, '/'), i = strchr(q, ':');

    if (p && i) {
        if (i < p) {
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

    } else if (p) {
        len = p - q;
        details.host = malloc(len + 1);

        if (details.host)
            strncpy(details.host, q, len), details.host[len] = '\0';

        len = strlen(p);
        details.path = malloc(len + 1);

        if (details.path)
            strcpy(details.path, p);
    } else if (i) {
        len = i - q;
        details.host = malloc(len + 1);

        if (details.host)
            strncpy(details.host, q, len), details.host[len] = '\0';

        len = strlen(i);
        details.port = malloc(len + 1);

        if (details.port)
            strcpy(details.port, i + 1);
    }
    else if (strlen(q)) {
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
