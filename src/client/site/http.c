#include "http.h"
#include "../uri.h"

#include <stddef.h>
#include <stdlib.h>

#pragma region Static Helper Functions

/**
 * Initialize a TCP socket ready for HTTP communication
 * @param self The site the socket is related to
 * @param details The address details
 * @return NULL on success, user friendly error message otherwise
 */
static inline const char *ConnectTo(HttpSite *self, UriDetails *details) {
    if (uriDetailsCreateSocketAddress(details, &self->address, SCHEME_HTTP) ||
        ioCreateSocketFromSocketAddress(&self->address, &self->socket) ||
        connect(self->socket, &self->address.address.sock, sizeof(self->address.address.sock)) == -1) {
        if (self->socket != INVALID_SOCKET)
            CLOSE_SOCKET(self->socket);

        return strerror(platformSocketGetLastError());
    }

    return NULL;
}

/**
 * Prepare the already connected TCP socket for sending data
 * @param self The site to send HTTP requests over
 * @param data The data to be sent
 * @param len The total length of the data to be sent
 * @return NULL on success, user friendly error message otherwise
 * @remark If the TCP connection was reset then this function will attempt to reestablish the connection internally
 */
static inline const char *WakeUpAndSend(HttpSite *self, const void *data, size_t len) {
    const char *err, maxAttempt = 3;
    char attempt = 0;
    SOCK_BUF_TYPE s = send(self->socket, data, len, 0);
    WakeUpAndSend_TryAgain:
    ++attempt;

    if (s == -1) {
        UriDetails details = uriDetailsNewFrom(self->fullUri);
        if (self->socket != INVALID_SOCKET)
            CLOSE_SOCKET(self->socket);

        err = ConnectTo(self, &details);
        uriDetailsFree(&details);
        if (err)
            return err;
        else if (attempt < maxAttempt)
            goto WakeUpAndSend_TryAgain;

        return strerror(platformSocketGetLastError());
    }

    return NULL;
}

/**
 * Check if the HTTP response is generally considered a good one that can proceed
 * @param response The response string created by HttpGetEssentialResponse()
 * @return Non-zero on good responses (200 OK), zero on bad responses (404 NOT FOUND or 500 INTERNAL SERVER ERROR)
 */
static inline char HttpResponseOk(const char *response) {
    switch (response[0]) {
        case '2':
            return 1;
        default:
            return 0;
    }
}

/**
 * Attempt to detect if this page can be considered a file directory page from information in the header
 * @param header The header to use to determine if this page is a directory listing
 * @return Zero if not a directory listing, positive number for a directory listing, negative number if unsure
 */
static inline char HttpResponseIsDir(const char *header) {
    char *ptr;
    FindHeader(header, "Content-Disposition", &ptr);
    if (ptr) {
        char *p = platformStringFindNeedle(ptr, "attachment");
        free(ptr);
        if (p)
            return 0;
    }

    FindHeader(header, "Content-Type", &ptr);

    if (ptr) {
        char *p = platformStringFindNeedle(ptr, "text/html");
        free(ptr);
        if (p)
            return 1;
    }

    return -1;
}

/**
 * Get an element in a XML document
 * @param xml The xml document to scan
 * @param element The element name to retrieve
 * @return Pointer within xml to the first instance of element found or NULL
 */
static inline char *XmlFindElement(const char *xml, char *element) {
    const char *p = xml;

    while (p && (p = strchr(p, '<'))) {
        char *next = strchr(&p[1], '<'), *end = strchr(&p[1], '>'), *t;
        if (end && end < next) {
            const char *sw = p;
            while ((*sw == ' ' || *sw == '<') && *sw != '\0')
                ++sw;

            *end = '\0', t = platformStringFindWord(sw, element), *end = '>';
            if (sw == t)
                return (char *) p;
        }
        p = next;
    }

    return NULL;
}

/**
 * Get an attribute in element
 * @param element Element pointer returned by XmlFindElement
 * @param attribute The tag name to retrieve
 * @return Pointer within xml to the first attribute found or NULL
 */
static inline char *XmlFindAttribute(const char *element, const char *attribute) {
    const char *p = element;
    while (p && (p = strchr(p, '<'))) {
        char *next = strchr(&p[1], '<'), *end = strchr(&p[1], '>'), *t;
        if (end && end < next) {
            *end = '\0', t = platformStringFindNeedle(p, attribute), *end = '>';
            if (t)
                return (char *) p;
        } else
            return NULL;
    }
    return NULL;
}

/**
 * Get the value of an attribute if it has one
 * @param attribute Attribute pointer return by XmlFindAttribute
 * @return String with the value or NULL
 */
static inline char *XmlExtractAttributeValue(const char *attribute) {
    size_t len = 0;
    char *eq = strchr(attribute, '='), *qp, qc;

    if (!eq)
        return NULL;

    {
        char *singleQuote = strchr(eq, '\''), *doubleQuote = strchr(eq, '"');
        if (singleQuote && doubleQuote) {
            if (singleQuote < doubleQuote)
                qp = singleQuote, qc = '\'';
            else
                qp = doubleQuote, qc = '"';
        } else if (singleQuote)
            qp = singleQuote, qc = '\'';
        else if (doubleQuote)
            qp = doubleQuote, qc = '"';
        else
            qp = NULL;
    }

    if (qp) {
        char *i = &qp[1];

        while (*i != '\0') {
            if (*i == qc)
                break;
            ++len, ++i;
        }

        if ((i = malloc(len + 1)))
            strncpy(i, &qp[1], len), i[len] = '\0';

        return i;
    } else {
        char *i = &eq[1];

        while (*i != '\0' || *i == qc)
            ++len, ++i;

        if ((i = malloc(len + 1)))
            strncpy(i, &qp[1], len), i[len] = '\0';

        return i;
    }
}

static inline char *HttpGetContentLength(const char *header, size_t *length) {
    char *cd, *e = FindHeader(header, "content-length", &cd);
    if (e)
        return e;

    if (cd) {
        size_t tmp = 0;
        char *i = strchr(cd, '\r');
        if (i)
            i[0] = '\0';

        i = cd, *length = 0;

        while (*i != '\0' && !isdigit(*i))
            ++i;

        while (*i != '\0') {
            switch (*i) {
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
                    tmp *= 10, tmp += (*i - '0');
                    if (tmp > *length)
                        *length = tmp, ++i;
                    else {
                        free(cd);
                        return "Content-Length value caused buffer overflow";
                    }
                    break;
                default:
                    free(cd);
                    return "Content-Length value contains non-digit character(s)";
            }
        }
        free(cd);
    }

    return NULL;
}

#pragma endregion

int httpSiteSchemeChangeDirectory(HttpSite *self, const char *path) {
    char *newPath, *newUri, *header, *scheme, *response, *send;
    UriDetails details = uriDetailsNewFrom(self->fullUri);

    if (!details.path)
        details.path = malloc(2), details.path[0] = '/', details.path[1] = '\0';

    if (!(newPath = uriPathAbsoluteAppend(details.path, path))) {
        uriDetailsFree(&details);
        return -1;
    }

    if (!(send = ioGenerateHttpHeadRequest(details.path, NULL))) {
        uriDetailsFree(&details), free(newPath);
        return -1;
    }

    if (WakeUpAndSend(self, send, strlen(send))) {
        uriDetailsFree(&details), free(send), free(newPath);
        return -1;
    }

    free(send), header = NULL;

    if (ioHttpHeadRead(&self->socket, &header)) {
        uriDetailsFree(&details), free(newPath);
        return -1;
    }

    if (HttpGetEssentialResponse(header, &scheme, &response)) {
        uriDetailsFree(&details), free(newPath), free(header);
        return -1;
    }

    if (!HttpResponseOk(response) || !HttpResponseIsDir(header)) {
        uriDetailsFree(&details), free(newPath), free(header), free(scheme);
        return 1;
    }

    free(header), free(scheme);

    if (details.path)
        free(details.path);

    details.path = newPath;

    if (!(newUri = uriDetailsCreateString(&details))) {
        uriDetailsFree(&details);
        return -1;
    }

    uriDetailsFree(&details);

    free(self->fullUri), self->fullUri = newUri;

    return 0;
}

void httpSiteSchemeFree(HttpSite *self) {
    if (self->fullUri)
        free(self->fullUri);

    if (self->socket != INVALID_SOCKET)
        CLOSE_SOCKET(self->socket);
}

char *httpSiteSchemeGetWorkingDirectory(HttpSite *self) {
    return self->fullUri;
}

const char *httpSiteNew(HttpSite *self, const char *path) {
    UriDetails details;
    const char *err;
    char *header, *scheme, *response, *send;

    if (!path) {
        self->socket = -1, self->fullUri = NULL, memset(&self->address, 0, sizeof(SocketAddress));
        return "No uri specified";
    }

    details = uriDetailsNewFrom(path);

    if (!details.path)
        details.path = malloc(2), details.path[0] = '/', details.path[1] = '\0';

    if ((err = ConnectTo(self, &details)))
        goto httpSiteSchemeNew_abort;

    header = scheme = response = NULL, send = ioGenerateHttpHeadRequest(details.path, NULL);
    if (!send) {
        err = strerror(errno);
        goto httpSiteSchemeNew_abort;
    }

    if ((err = WakeUpAndSend(self, send, strlen(send))) || (err = ioHttpHeadRead(&self->socket, &header)) ||
        (err = HttpGetEssentialResponse(header, &scheme, &response)))
        goto httpSiteSchemeNew_closeSocketAndAbort;

    free(send);

    if (!(HttpResponseOk(response)))
        goto httpSiteSchemeNew_closeSocketAndAbort;

    if (HttpResponseIsDir(header))
        self->fullUri = uriDetailsCreateString(&details);

    uriDetailsFree(&details);

    if (header)
        free(header);

    if (scheme)
        free(scheme);

    return NULL;

    httpSiteSchemeNew_closeSocketAndAbort:
    if (self->socket == INVALID_SOCKET)
        CLOSE_SOCKET(self->socket);

    if (header)
        free(header);

    if (scheme)
        free(scheme);

    httpSiteSchemeNew_abort:
    uriDetailsFree(&details);

    return err;
}

void *httpSiteOpenDirectoryListing(HttpSite *self, char *path) {
    char *absPath, *header, *r, *scheme;
    UriDetails details = uriDetailsNewFrom(self->fullUri);

    if (!(absPath = uriPathAbsoluteAppend(details.path, path)))
        goto httpSiteOpenDirectoryListing_abort1;

    if (!(r = ioGenerateHttpGetRequest(absPath, NULL)) || WakeUpAndSend(self, r, strlen(r)) ||
        ioHttpHeadRead(&self->socket, &header))
        goto httpSiteOpenDirectoryListing_abort2;

    free(r), r = NULL;

    if (HttpGetEssentialResponse(header, &scheme, &r) || !(HttpResponseOk(r) || HttpResponseIsDir(header)))
        goto httpSiteOpenDirectoryListing_abort3;

    /* TODO: Extract 'a href' links from http body and filter by only immediate subdirectories */

    httpSiteOpenDirectoryListing_abort3:
    free(header);

    httpSiteOpenDirectoryListing_abort2:
    if (r)
        free(r);

    free(absPath);

    httpSiteOpenDirectoryListing_abort1:
    uriDetailsFree(&details);

    return NULL;
}

void *httpSiteReadDirectoryListing(void *listing) {
    /* TODO: Implement */
    return NULL;
}

void httpSiteCloseDirectoryListing(void *listing) {
    /* TODO: Implement */
}

