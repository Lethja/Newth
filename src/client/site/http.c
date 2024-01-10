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
        connect(self->socket, &self->address.address.sock, sizeof(self->address.address.sock)) == -1)
        return strerror(platformSocketGetLastError());

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
    ssize_t s = send(self->socket, data, len, 0);
    WakeUpAndSend_TryAgain:
    ++attempt;

    if (s == -1) {
        UriDetails details = uriDetailsNewFrom(self->fullUri);
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
        char *p = strstr(ptr, "filename");
        free(ptr);
        if (p)
            return 0;
    }

    FindHeader(header, "Content-Type", &ptr);

    if (ptr) {
        char *p = strstr(ptr, "text/html");
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
static inline const char *XmlFindElement(const char *xml, char *element) {
    const char *p = xml;
    while ((p = strchr(p, '<'))) {
        char *next = strchr(p, '<'), *end = strchr(p, '>'), *t;
        if (end && end < next) {
            *end = '\0', t = strstr(p, element), *end = '>';
            if (t)
                return p;
        }
    }
    return NULL;
}

/**
 * Get an attribute in element
 * @param element Element pointer returned by XmlFindElement
 * @param attribute The tag name to retrieve
 * @return Pointer within xml to the first attribute found or NULL
 */
static inline const char *XmlFindAttribute(const char *element, const char *attribute) {
    const char *p = element;
    while ((p = strchr(p, '<'))) {
        char *next = strchr(p, '<'), *end = strchr(p, '>'), *t;
        if (end && end < next) {
            *end = '\0', t = strstr(p, attribute), *end = '>';
            if (t)
                return p;
        }
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

    if (WakeUpAndSend(self, send, strlen(send)) || ioHttpHeadRead(&self->socket, &header)) {
        uriDetailsFree(&details), free(newPath);
        return -1;
    }

    free(send);

    if (HttpGetEssentialResponse(header, &scheme, &response)) {
        uriDetailsFree(&details), free(newPath), free(header);
        return -1;
    }

    if (!HttpResponseOk(response) || !HttpResponseIsDir(header)) {
        uriDetailsFree(&details), free(newPath), free(header), free(scheme);
        return 1;
    }

    if (details.path)
        free(details.path);

    details.path = newPath;

    if (!(newUri = uriDetailsCreateString(&details))) {
        uriDetailsFree(&details);
        return -1;
    }

    free(self->fullUri), self->fullUri = newUri;

    return 0;
}

void httpSiteSchemeFree(HttpSite *self) {
    if (self->fullUri)
        free(self->fullUri);

    CLOSE_SOCKET(self->socket);
}

char *httpSiteSchemeGetWorkingDirectory(HttpSite *self) {
    return self->fullUri;
}

const char *httpSiteNew(HttpSite *self, const char *path) {
    UriDetails details = uriDetailsNewFrom(path);
    const char *err;
    char *header, *scheme, *response, *send;

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

    if (header) free(header);
    if (scheme) free(scheme);

    return NULL;

    httpSiteSchemeNew_closeSocketAndAbort:
    CLOSE_SOCKET(self->socket);
    if (header) free(header);
    if (scheme) free(scheme);

    httpSiteSchemeNew_abort:
    uriDetailsFree(&details);
    return err;
}

void *httpSiteOpenDirectoryListing(char *path) {
    /* TODO: Implement */
    return NULL;
}

void *httpSiteReadDirectoryListing(void *listing) {
    /* TODO: Implement */
    return NULL;
}

void httpSiteCloseDirectoryListing(void *listing) {
    /* TODO: Implement */
}

