#include "../site.h"
#include "http.h"
#include "../../common/hex.h"
#include "../uri.h"
#include "../xml.h"

#include <stddef.h>
#include <stdlib.h>
#include <time.h>

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
 * Verify that a link is a direct subdirectory of a particular site path
 * @param path The site path details to check again
 * @param link The link to verify
 * @return Non-zero if link is a direct subdirectory of path, otherwise zero
 */
static inline char LinkPathIsDirectSub(const UriDetails *path, const char *link) {
    char *p = strstr(link, "://");

    if (p) {
        UriDetails linkUri = uriDetailsNewFrom(link);
        if ((linkUri.scheme && path->scheme && !strcmp(linkUri.scheme, path->scheme)) &&
            (linkUri.host && path->host && !strcmp(linkUri.host, path->host)) &&
            uriDetailsGetPort(&linkUri) == uriDetailsGetPort(path)) { /* Is the same site */
            if (strstr(linkUri.path, path->path) == linkUri.path) {
                char c = strchr(&linkUri.path[strlen(path->path) + 1], '/') ? 0 : 1; /* 0 if multiple levels down */
                uriDetailsFree(&linkUri);
                return c;
            }
        }

        uriDetailsFree(&linkUri);
        return 0;
    } else if (link[0] == '.') /* No hidden or parent directories allowed */
        return 0;
    else if (link[0] == '/') {
        if (strstr(link, path->path) == link) {
            size_t len = strlen(path->path);
            if (strlen(link) <= len) /* The same directory */
                return 0;

            if (strchr(&link[len + 1], '/')) /* Multiple levels down */
                return 0;

            return 1;
        }
        return 0;
    }

    /* Not choice to but trust this is a relative path */
    return 1;
}

/**
 * Create a new directory listing
 * @param uriPath The full uri path this directory listing is representing
 * @return New directory listing on success, otherwise NULL
 */
static HttpSiteDirectoryListing *DirectoryListingCreate(const char *uriPath) {
    HttpSiteDirectoryListing *self = calloc(1, sizeof(HttpSiteDirectoryListing));
    if (self) {
        if (uriPath) {
            if ((self->fullUri = malloc(strlen(uriPath) + 1)))
                strcpy(self->fullUri, uriPath);
            else {
                free(self);
                return NULL;
            }
        }
    }
    return self;
}

/**
 * Add new DirectoryEntry to the the directory listing
 * @param self The directory listing to add to
 * @param name The name of the directory listing
 * @return NULL on success, user friendly error message otherwise
 */
static char *DirectoryListingAdd(HttpSiteDirectoryListing *self, const char *name) {
    char *n = malloc(strlen(name) + 1);
    if (!n)
        return strerror(errno);

    strcpy(n, name);
    if (!self->entry) {
        if (!(self->entry = malloc(sizeof(SiteDirectoryEntry)))) {
            free(n);
            return strerror(errno);
        }
    } else {
        void *tmp = realloc(self->entry, sizeof(SiteDirectoryEntry) * (self->len + 1));
        if (tmp)
            self->entry = tmp;
        else {
            free(n);
            return strerror(errno);
        }
    }

    self->entry[self->len].name = n, ++self->len;
    return NULL;
}

/**
 * Set the time that the directory listing was made. This can be used later to prevent needlessly requesting information over the network
 * @param self The DirectoryListing to update the time of
 */
static void DirectoryListingSetReloadDate(HttpSiteDirectoryListing *self) {
    self->asOf = time(NULL);
}

/**
 * Free all allocations inside of a Http Sites directory listing
 * @param self The directory listing to free all elements inside of
 * @remark If the directory listing is allocated on the heap then always run this function before freeing the directory listing itself
 */
static void DirectoryListingClear(HttpSiteDirectoryListing *self) {
    if (self->entry) {
        size_t i;
        for (i = 0; i < self->len; ++i) {
            if (self->entry[i].name)
                free(self->entry[i].name);
        }

        free(self->entry), self->entry = NULL;
    }

    if (self->fullUri)
        free(self->fullUri), self->fullUri = NULL;

    self->len = self->idx = self->asOf = 0;
}

/**
 * Return a new string that contains and omits the last '/' found in the uri link path
 * @param link The link used on that instance of LinkPathIsDirectSub
 * @return Formatted subdirectory string on success, otherwise NULL
 * @remark Check that 'link' is compatible with LinkPathIsDirectSub() before calling this function
 * @remark Returned string should be freed before leaving scope
 */
static inline char *LinkPathConvertToRelativeSubdirectory(const char *link) {
    char *r, *p;

    if ((p = strrchr(link, '/')))
        ++p;
    else
        p = (char *) link;

    if ((r = malloc(strlen(p) + 1)))
        strcpy(r, p);

    return r;
}

/**
 * Fast forward socket stream to the next instance of element or the end of stream
 * @param self HttpSite to forward the socket of
 * @param element Element to find
 * @remark Use with care, TCP sockets cannot be rewound, everything before the element will be lost
 * if there's no element then everything will be lost.
 */
static inline SOCK_BUF_TYPE FastForwardToElement(HttpSite *self, const char *element) {
    char buf[2048] = {0}, *match;
    SOCK_BUF_TYPE bytesGot, totalBytes = 0;

    while ((bytesGot = recv(self->socket, buf, 2047, MSG_PEEK)) > 0) {
        if ((match = XmlFindElement(buf, element))) {
            size_t len = 0;
            char *i = buf;

            while (i < match)
                ++i, ++len;

            totalBytes += recv(self->socket, buf, len, 0);
            break;
        } else
            totalBytes += recv(self->socket, buf, bytesGot, 0);
    }

    return totalBytes;
}

/**
 * Fast forward socket stream to after the next instance of element or the end of stream
 * @param self HttpSite to forward the socket of
 * @param element Element to find and jump over
 * @remark Use with care, TCP sockets cannot be rewound, everything before the element will be lost
 * if there's no element or the nothing after the element then everything will be lost.
 */
static inline SOCK_BUF_TYPE FastForwardOverElement(HttpSite *self, const char *element) {
    SOCK_BUF_TYPE r = 0;
    char buf[2048] = {0}, *found;

    FastForwardToElement(self, element);
    recv(self->socket, buf, 2047, MSG_PEEK);

    if ((found = XmlExtractElement(buf, element))) {
        size_t len = strlen(found);
        free(found), r = recv(self->socket, buf, len, 0);
    }

    return r;
}

/**
 * Extract the links from an 'a' element containing a 'href' attribute
 * @param socket A network socket positioned at the beginning of the html body
 * @param length The length of the html body if known
 * @return The value of the next <a href> in the scope if there is one, otherwise NULL
 * @remark Returned string should be freed before leaving scope
 */
static inline char *HtmlExtractNextLink(HttpSite *self, size_t *written) {
    char *a, *e, *v, buf[2048] = {0};
    size_t got;

    *written += FastForwardToElement(self, "a");
    got = recv(self->socket, buf, 2047, MSG_PEEK);

    if (got && (e = XmlFindElement(buf, "a")) && (a = XmlFindAttribute(e, "href"))) {
        if ((v = XmlExtractAttributeValue(a))) {
            *written += FastForwardOverElement(self, "a");
            return v;
        }
    }

    return NULL;
}

/**
 * Get the content length header if applicable
 * @param header The header to search for content length of
 * @param length Out: The length of the http body of getting content length was successful
 * @return NULL on success, user friendly error message otherwise
 */
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

    if (self->directory)
        DirectoryListingClear(self->directory), free(self->directory);

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

    self->directory = NULL, details = uriDetailsNewFrom(path);

    if (!details.path)
        details.path = malloc(2), details.path[0] = '/', details.path[1] = '\0';

    if ((err = ConnectTo(self, &details)))
        goto httpSiteSchemeNew_abort;

    header = scheme = response = NULL, send = ioGenerateHttpHeadRequest(details.path, NULL);
    if (!send) {
        err = strerror(errno);
        goto httpSiteSchemeNew_abort;
    }

    if ((err = WakeUpAndSend(self, send, strlen(send)))) {
        free(send);
        goto httpSiteSchemeNew_closeSocketAndAbort;
    }

    free(send);

    if ((err = ioHttpHeadRead(&self->socket, &header)) || (err = HttpGetEssentialResponse(header, &scheme, &response)))
        goto httpSiteSchemeNew_closeSocketAndAbort;

    if (!(HttpResponseOk(response))) {
        err = "Server reply not acceptable";
        goto httpSiteSchemeNew_closeSocketAndAbort;
    }

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
    size_t len, write = len = 0;
    char *absPath, *header = NULL, *request, *response, *scheme, *file;
    UriDetails details;

    if (!self->fullUri)
        return NULL;

    details = uriDetailsNewFrom(self->fullUri);

    if (!(absPath = uriPathAbsoluteAppend(details.path, path)))
        goto httpSiteOpenDirectoryListing_abort1;

    if (!(request = ioGenerateHttpGetRequest(absPath, NULL)))
        goto httpSiteOpenDirectoryListing_abort2;

    if (WakeUpAndSend(self, request, strlen(request)))
        goto httpSiteOpenDirectoryListing_abort2;

    free(request), request = NULL, scheme = NULL;

    if (ioHttpHeadRead(&self->socket, &header))
        goto httpSiteOpenDirectoryListing_abort3;

    if (HttpGetEssentialResponse(header, &scheme, &response) ||
        !(HttpResponseOk(response) || !HttpResponseIsDir(header)))
        goto httpSiteOpenDirectoryListing_abort3;

    /* TODO: Handle chunked transfer encoding */
    HttpGetContentLength(header, &len);
    free(header), free(scheme);

    write += FastForwardToElement(self, "body");
    write += FastForwardOverElement(self, "body");

    if (self->directory)
        DirectoryListingClear(self->directory), free(self->directory);

    if (!(self->directory = DirectoryListingCreate(absPath))) {
        free(absPath), uriDetailsFree(&details);
        return NULL;
    }

    if (details.path)
        free(details.path);

    details.path = absPath;

    while ((file = HtmlExtractNextLink(self, &write))) {
        if (LinkPathIsDirectSub(&details, file)) {
            char *n = LinkPathConvertToRelativeSubdirectory(file);
            free(file);
            if (n) {
                size_t pLen = strlen(n);

                hexConvertStringToAscii(n);
                if (strlen(n) < pLen) {
                    char *tmp = realloc(n, strlen(n) + 1);
                    if (tmp)
                        n = tmp;
                }

                DirectoryListingAdd(self->directory, n), free(n);
            }
        } else
            free(file);
    }

    uriDetailsFree(&details);
    DirectoryListingSetReloadDate(self->directory);
    return self->directory;

    httpSiteOpenDirectoryListing_abort3:
    if (scheme)
        free(scheme);

    free(header);
    httpSiteOpenDirectoryListing_abort2:
    if (request)
        free(request);

    free(absPath);

    httpSiteOpenDirectoryListing_abort1:
    uriDetailsFree(&details);

    return NULL;
}

void *httpSiteReadDirectoryListing(void *listing) {
    HttpSiteDirectoryListing *l = listing;
    SiteDirectoryEntry *e;

    if (l->idx < l->len) {
        if ((e = malloc(sizeof(SiteDirectoryEntry)))) {
            char *n = malloc(strlen(l->entry[l->idx].name) + 1);
            if (!n) {
                free(e);
                return NULL;
            }

            strcpy(n, l->entry[l->idx].name);
            memcpy(e, &l->entry[l->idx], sizeof(SiteDirectoryEntry)), ++l->idx, e->name = n;
            return e;
        }
    }

    return NULL;
}

void httpSiteCloseDirectoryListing(void *listing) {
    DirectoryListingClear(listing);
}

