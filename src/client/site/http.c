#include "../site.h"
#include "http.h"
#include "../../client/err.h"
#include "../../common/hex.h"
#include "../uri.h"
#include "../xml.h"

#include <stddef.h>
#include <stdlib.h>
#include <time.h>

#include <sys/stat.h>

#pragma region Static Helper Functions

/**
 * Check if the HTTP response is generally considered a good one that can proceed
 * @param response The response string created by ioHttpResponseHeaderEssential()
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
 * Give the closest system error message to the HTTP response
 * @param response The HTTP response to convert to a system error
 * @return A system error or -1 if there isn't a good match
 */
static inline int HttpResponseToSystemError(const char *response) {
    switch (response[0]) {
        case '1':
        case '2':
            return 0;
        case '4':
            switch (response[1]) {
                case '0':
                    switch (response[2]) {
                        case '0':
                            return EFAULT;
                        case '1':
                        case '2':
                        case '3':
                            return EPERM;
                        case '4':
                            return ENOENT;
                    }
            }
    }
    return -1;
}

/**
 * Attempt to detect if this page can be considered a file directory page from information in the header
 * @param header The header to use to determine if this page is a directory listing
 * @return Zero if not a directory listing, positive number for a directory listing, negative number if unsure
 */
static inline char HttpResponseIsDir(const char *header) {
    char *ptr;
    ioHttpResponseHeaderFind(header, "Content-Disposition", &ptr);
    if (ptr) {
        char *p = platformStringFindNeedle(ptr, "attachment");
        free(ptr);
        if (p)
            return 0;
    }

    ioHttpResponseHeaderFind(header, "Content-Type", &ptr);

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
            char *last;

            if (strlen(link) <= len) /* The same directory */
                return 0;

            /* Check if link is multiple levels down but allow trailing '/' on a single level down */
            if ((last = strchr(&link[len + 1], '/')) && last != &link[strlen(link) - 1])
                return 0;

            return 1;
        }
        return 0;
    }

    /* No choice to but trust this is a relative path */
    return 1;
}

/**
 * Create a new directory listing
 * @param uriPath The full uri path this directory listing is representing
 * @return New directory listing on success, otherwise NULL
 */
static HttpSiteDirectoryListing *DirectoryListingCreate(const HttpSite *site, const char *uriPath) {
    HttpSiteDirectoryListing *self = calloc(1, sizeof(HttpSiteDirectoryListing));
    if (self) {
        self->site = site;
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
        SiteDirectoryEntry *entry = malloc(sizeof(SiteDirectoryEntry));
        if (!entry) {
            free(n);
            return strerror(errno);
        }
        self->entry = entry;
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
    const char *p, *i = p = link;
    char *r;

    /* Get last not tailing '/' */
    while (*i != '\0') {
        if (*i == '/') {
            switch (i[1]) {
                case '\0':
                case '/':
                    break;
                default:
                    p = &i[1];
                    break;
            }
        }
        ++i;
    }

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
    SOCK_BUF_TYPE totalBytes = 0;
    size_t s = strlen(element);
    char *e = malloc(s + 3);

    if (!e)
        return -1;

    e[0] = '<', memcpy(&e[1], element, s), e[s + 1] = '\0';
    if (recvBufferFindAndDitch(&self->socket, e, strlen(e), &totalBytes)) {
        free(e);
        return -1;
    }

    free(e);
    return totalBytes;
}

/**
 * Make sure the end of this XML element is in the buffer before continuing
 * @param self HttpSite to find the end element tag in
 * @param element The element to find the end tag of
 * @return NULL on success, user friendly error message otherwise
 */
static inline const char *BufferToEndElement(HttpSite *self, const char *element) {
    const char *r;
    char *e;
    size_t s = strlen(element);

    if (!(e = malloc(s + 4)))
        return strerror(errno);

    e[0] = '<', e[1] = '/', memcpy(&e[2], element, s), e[s + 2] = '>', e[s + 3] = '\0';
    r = recvBufferFindAndFetch(&self->socket, e, strlen(e), 2048);
    free(e);

    return r;
}

/**
 * Fast forward socket stream to after the next instance of element or the end of stream
 * @param self HttpSite to forward the socket of
 * @param element Element to find and jump over
 * @remark Use with care, TCP sockets cannot be rewound, everything before the element will be lost
 * if there's no element or the nothing after the element then everything will be lost.
 */
static inline SOCK_BUF_TYPE FastForwardOverElement(HttpSite *self, const char *element) {
    SOCK_BUF_TYPE r;
    char buf[2048] = {0}, *found;

    r = FastForwardToElement(self, element);
    recvBufferFetch(&self->socket, buf, 0, 2047);

    if ((found = XmlExtractElement(buf, element))) {
        size_t len = strlen(found);
        free(found), recvBufferDitch(&self->socket, (PlatformFileOffset) len);
        r += len;
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
    const char *element = "a";
    char *a, *e, *v, buf[2048] = {0};
    *written += FastForwardToElement(self, element);
    if (BufferToEndElement(self, element))
        return NULL;

    if (!recvBufferFetch(&self->socket, buf, 0, 2048) && (e = XmlFindElement(buf, "a")) &&
        (a = XmlFindAttribute(e, "href"))) {
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
static inline char *HttpGetContentLength(const char *header, PlatformFileOffset *length) {
    char *cd, *e = ioHttpResponseHeaderFind(header, "content-length", &cd);
    if (e)
        return e;

    if (cd) {
        PlatformFileOffset tmp = 0;
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

/**
 * Populate the header response with any and all found headers
 * @param header The header response received from the remote
 * @param headerResponse The response header populated with whatever is found
 */
static inline void HeadersPopulate(const char *header, HttpResponseHeader *headerResponse) {
    char *v;

    ioHttpResponseHeaderFind(header, "Transfer-Encoding", &v);
    if (v) {
        if (platformStringFindWord(v, "Chunked"))
            headerResponse->options |= SA_PROTOCOL_ALT_MODE;
        free(v);
    } else
        HttpGetContentLength(header, &headerResponse->length);

    ioHttpResponseHeaderFind(header, "Last-Modified", &v);
    if (v) {
        if (!headerResponse->modifiedDate)
            headerResponse->modifiedDate = calloc(1, sizeof(PlatformTimeStruct));

        platformTimeGetFromHttpStr(v, headerResponse->modifiedDate), free(v);
    }

    ioHttpResponseHeaderFind(header, "Content-Disposition", &v);
    if (v) {
        char *p = platformStringFindWord(v, "filename");
        if (p) {
            if ((p = strchr(p, '='))) {
                size_t len;

                ++p, len = strlen(p);
                if ((headerResponse->fileName = malloc(len + 1)))
                    strcpy(headerResponse->fileName, p);
            }
        }
        free(v);
    }

    headerResponse->length = 0, ioHttpResponseHeaderFind(header, "Content-Length", &v);
    if (v) {
        size_t l = strlen(v), i;
        for (i = 0; i < l; ++i)
            if (!isdigit(v[i])) {
                free(v);
                return;
            }

        for (i = 0; i < l; ++i)
            headerResponse->length *= 10, headerResponse->length += v[i] - '0';

        free(v);
    }
}

/**
 * Check and compare header metadata of this request against local files, setup site to receive files if necessary
 * @param self The Site to compute headers for
 * @param headerResponse The header to compute from
 * @param mode The mode this header is responding to
 */
static inline void HeadersCompute(HttpSite *self, HttpResponseHeader *headerResponse, const char *mode) {
    /* TODO: Check filename if exists, extract from url otherwise */
    /* TODO: Check modification date against local file if exists */
    if (toupper(mode[0]) == 'G' && toupper(mode[1]) == 'E' && toupper(mode[2]) == 'T') {
        if (headerResponse->options & SA_PROTOCOL_ALT_MODE)
            recvBufferSetLengthChunk(&self->socket);
        else {
            if (headerResponse->length == self->socket.len)
                recvBufferSetLengthComplete(&self->socket);
            else {
                /* Add any existing buffer that was picked up with the header */
                recvBufferSetLengthKnown(&self->socket, headerResponse->length);
                self->socket.length.known.escape = (PlatformFileOffset) self->socket.len;
            }
        }
    } else
        recvBufferSetLengthComplete(&self->socket);
}

/**
 * Free internal structures of a header
 * @param headerResponse The header to free the internals of
 * @remark If the header was allocated on the heap it will still need to be freed
 */
static inline void HeadersFree(HttpResponseHeader *headerResponse) {
    if (headerResponse->modifiedDate)
        free(headerResponse->modifiedDate);

    if (headerResponse->fileName)
        free(headerResponse->fileName);
}

/**
 * Generate a host request header
 * @param details The hostname to generate for
 * @return A host request header to use on success, otherwise NULL
 */
static inline const char *GenerateHostHeader(char **self, UriDetails *details) {
    size_t len = 8; /* Host: \r\n */
    char *tmp;
    if (!details->host)
        return "No host information";

    len += strlen(details->host);
    if (!(tmp = malloc(len + 1)))
        return strerror(errno);

    sprintf(tmp, "Host: %s" HTTP_EOL, details->host);
    return platformHeapStringAppendAndFree(self, tmp);
}

/**
 * Generate a HTTP keep alive request header
 * @param self The header string to concatenate onto the end of
 * @return NULL on success, user friendly error message otherwise
 */
static inline const char *GenerateConnectionHeader(char **self) {
    const char *h = "Connection: Keep-Alive" HTTP_EOL;
    return platformHeapStringAppend(self, h);
}

#pragma endregion

int httpSiteSchemeWorkingDirectorySet(HttpSite *self, const char *path) {
    char *newPath, *newUri, *header, *scheme, *response, *send;
    UriDetails details = uriDetailsNewFrom(self->fullUri);

    if (!details.path)
        details.path = malloc(2), details.path[0] = '/', details.path[1] = '\0';

    if (!(newPath = uriPathAbsoluteAppend(details.path, path))) {
        uriDetailsFree(&details);
        errno = EFAULT;
        return -1;
    }

    if (!(send = ioHttpRequestGenerateHead(newPath, NULL))) {
        uriDetailsFree(&details), free(newPath);
        return -1;
    }

    if (recvBufferSend(&self->socket, send, strlen(send), 0)) {
        uriDetailsFree(&details), free(newPath), free(send);
        return -1;
    }

    free(send), header = NULL;

    if (ioHttpResponseHeaderRead(&self->socket, &header)) {
        uriDetailsFree(&details), free(newPath);
        return -1;
    }

    if (ioHttpResponseHeaderEssential(header, &scheme, &response)) {
        uriDetailsFree(&details), free(newPath), free(header);
        return -1;
    }

    if (!HttpResponseOk(response)) {
        errno = HttpResponseToSystemError(response);
        uriDetailsFree(&details), free(newPath), free(header), free(scheme);
        return 1;
    }

    if (!HttpResponseIsDir(header)) {
        uriDetailsFree(&details), free(newPath), free(header), free(scheme);
        errno = ENOTDIR;
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

    if (self->file)
        httpSiteSchemeFileClose(self);

    recvBufferClear(&self->socket);
}

char *httpSiteSchemeWorkingDirectoryGet(HttpSite *self) {
    return self->fullUri;
}

const char *httpSiteSchemeNew(HttpSite *self, const char *path) {
    UriDetails details;
    const char *err;
    char *header, *scheme, *response, *send;

    if (!path) {
        self->socket.serverSocket = INVALID_SOCKET, self->fullUri = NULL;
        memset(&self->address, 0, sizeof(SocketAddress));
        return ErrNoUriSpecified;
    }

    details = uriDetailsNewFrom(path);

    if (!details.path && (details.path = malloc(2)))
        details.path[0] = '/', details.path[1] = '\0';

    if ((err = recvBufferNewFromUriDetails(&self->socket, (void *) &details, 0)))
        goto httpSiteSchemeNew_abort;

    header = NULL, GenerateHostHeader(&header, &details), GenerateConnectionHeader(&header);
    send = ioHttpRequestGenerateHead(details.path, header);

    if (header)
        free(header), header = NULL;

    if (!send) {
        err = strerror(errno);
        goto httpSiteSchemeNew_abort;
    }

    scheme = response = NULL;

    if ((err = recvBufferConnect(&self->socket)) || (err = recvBufferSend(&self->socket, send, strlen(send), 0))) {
        free(send);
        goto httpSiteSchemeNew_closeSocketAndAbort;
    }

    free(send);

    if ((err = ioHttpResponseHeaderRead(&self->socket, &header)) ||
        (err = ioHttpResponseHeaderEssential(header, &scheme, &response)))
        goto httpSiteSchemeNew_closeSocketAndAbort;

    if (!(HttpResponseOk(response))) {
        err = ErrServerReplyNotAcceptable;
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
    if (self->socket.serverSocket == INVALID_SOCKET)
        CLOSE_SOCKET(self->socket.serverSocket);

    if (header)
        free(header);

    if (scheme)
        free(scheme);

httpSiteSchemeNew_abort:
    uriDetailsFree(&details);

    return err;
}

void httpSiteSchemeDirectoryListingClose(void *listing) {
    DirectoryListingClear(listing), free(listing);
}

const char *httpSiteSchemeDirectoryListingEntryStat(void *listing, void *entry, PlatformFileStat *st) {
    HttpSiteDirectoryListing *l = listing;
    SiteDirectoryEntry *e = entry;

    char *entryPath, *request, *response;
    HttpResponseHeader header;
    UriDetails details;

    details = uriDetailsNewFrom(l->fullUri);
    if (!details.path) {
        uriDetailsFree(&details);
        return ErrUriWasNotUnderstood;
    }

    if (!(entryPath = malloc(strlen(details.path) + strlen(e->name) + 2))) {
        uriDetailsFree(&details);
        return strerror(errno);
    }

    platformPathCombine(entryPath, details.path, e->name), uriDetailsFree(&details);
    if (!(request = ioHttpRequestGenerateHead(entryPath, NULL))) {
        free(entryPath);
        return strerror(errno);
    }

    free(entryPath);
    if ((recvBufferSend((RecvBuffer *) &l->site->socket, request, strlen(request), 0))) {
        free(request);
        return strerror(platformSocketGetLastError());
    }

    free(request);
    if ((ioHttpResponseHeaderRead((RecvBuffer *) &l->site->socket, &response)))
        return strerror(platformSocketGetLastError());

    memset(&header, 0, sizeof(HttpResponseHeader));
    HeadersPopulate(response, &header);


    /* Assuming a HTTP path without a Content-Disposition filename isn't intended to be a downloadable file */
    st->st_mode = header.fileName ? S_IFREG : S_IFDIR;
    st->st_size = header.length;

    if (header.modifiedDate)
        st->st_mtime = mktime(header.modifiedDate), free(header.modifiedDate);
    else
        st->st_mtime = 0;

    if (header.fileName)
        free(header.fileName);

    return NULL;
}

void *httpSiteSchemeDirectoryListingOpen(HttpSite *self, char *path) {
    size_t write = 0;
    char *absPath, *header = NULL, *request, *response, *scheme, *file;
    UriDetails details;
    HttpResponseHeader headerResponse = {0};
    HttpSiteDirectoryListing *directoryListing;

    if (!self->fullUri)
        return NULL;

    details = uriDetailsNewFrom(self->fullUri);

    if (!(absPath = uriPathAbsoluteAppend(details.path, path)))
        goto httpSiteOpenDirectoryListing_abort1;

    scheme = NULL, GenerateHostHeader(&scheme, &details), GenerateConnectionHeader(&scheme);

    if (!(request = ioHttpRequestGenerateGet(absPath, scheme)))
        goto httpSiteOpenDirectoryListing_abort3;

    free(scheme);
    recvBufferClear(&self->socket);

    if (recvBufferSend(&self->socket, request, strlen(request), 0))
        goto httpSiteOpenDirectoryListing_abort2;

    free(request), request = NULL, scheme = NULL;

    if (ioHttpResponseHeaderRead(&self->socket, &header))
        goto httpSiteOpenDirectoryListing_abort3;

    if (ioHttpResponseHeaderEssential(header, &scheme, &response) ||
        !(HttpResponseOk(response) || !HttpResponseIsDir(header)))
        goto httpSiteOpenDirectoryListing_abort3;

    HeadersPopulate(header, &headerResponse);
    HeadersCompute(self, &headerResponse, "GET");
    HeadersFree(&headerResponse);

    free(header), free(scheme);

    write += FastForwardToElement(self, "body");
    write += FastForwardOverElement(self, "body");

    if (!(directoryListing = DirectoryListingCreate(self, absPath))) {
        free(absPath), uriDetailsFree(&details);
        return NULL;
    }

    if (details.path)
        free(details.path);

    details.path = absPath;

    while ((file = HtmlExtractNextLink(self, &write))) {
        char *q = strchr(file, '?');
        if (q) {
            *q = '\0';

            if (!strlen(file)) {
                free(file);
                continue;
            }
        }

        if (LinkPathIsDirectSub(&details, file)) {
            char *n = LinkPathConvertToRelativeSubdirectory(file);
            free(file);
            if (n) {
                size_t pLen = strlen(n);
                char *p;

                /* Strip trailing spaces */
                while ((p = strrchr(n, '/')))
                    p[0] = '\0';

                hexConvertStringToAscii(n);
                if (strlen(n) < pLen) {
                    char *tmp = realloc(n, strlen(n) + 1);
                    if (tmp)
                        n = tmp;
                }

                DirectoryListingAdd(directoryListing, n), free(n);
            }
        } else
            free(file);
    }

    uriDetailsFree(&details);
    DirectoryListingSetReloadDate(directoryListing);

    if (!directoryListing->len)
        httpSiteSchemeDirectoryListingClose(directoryListing), directoryListing = NULL;

    recvBufferClear(&self->socket);

    return directoryListing;

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
    recvBufferClear(&self->socket);

    return NULL;
}

void *httpSiteSchemeDirectoryListingRead(void *listing) {
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
            e->isDirectory = -1, e->modifiedDate = 0;
            memcpy(e, &l->entry[l->idx], sizeof(SiteDirectoryEntry)), ++l->idx, e->name = n;
            return e;
        }
    }

    return NULL;
}

void httpSiteSchemeFileClose(HttpSite *self) {
    if (self->file) {
        if (self->file->fullUri)
            free(self->file->fullUri);

        free(self->file), self->file = NULL;
    }
}

SOCK_BUF_TYPE httpSiteSchemeFileRead(HttpSite *self, char *buffer, SOCK_BUF_TYPE size) {
    SOCK_BUF_TYPE bufferSize;

    /* Determine if the requested size buffer is feasible */
    if (self->socket.len < size) {
        const char *e;

        /* Try get some more */
        if ((e = recvBufferAppend(&self->socket, size - self->socket.len)) && e != ErrNoDataToBeRetrieved)
            return -1;

        bufferSize = self->socket.len > size ? size : self->socket.len;
    } else
        bufferSize = size;

    /* Copy and ditch */
    memcpy(buffer, self->socket.buffer, bufferSize);
    recvBufferDitch(&self->socket, (PlatformFileOffset) bufferSize);

    /* Bytes copied */
    return bufferSize;
}

const char *httpSiteSchemeFileOpenRead(HttpSite *self, const char *path, PlatformFileOffset start, PlatformFileOffset end) {
    const char *e;
    UriDetails d = uriDetailsNewFrom(self->fullUri);
    HttpResponseHeader headerResponse = {0};
    char *resolvedPath, *request, *response, *scheme, *header;

    if (d.path) {
        resolvedPath = uriPathAbsoluteAppend(d.path, path);
        scheme = NULL, GenerateHostHeader(&scheme, &d);
        uriDetailsFree(&d);
    } else {
        uriDetailsFree(&d);
        return ErrUriWasNotUnderstood;
    }

    GenerateConnectionHeader(&scheme);

    /* TODO: byte range code when `if (start != -1 || end != -1)` is true */

    if (!(request = ioHttpRequestGenerateGet(resolvedPath, scheme))) {
        free(resolvedPath), free(scheme);
        return strerror(errno);
    }

    free(scheme);
    recvBufferClear(&self->socket);

    if ((e = recvBufferSend(&self->socket, request, strlen(request), 0))) {
        free(request), free(resolvedPath);
        return e;
    }

    free(request), request = NULL, scheme = NULL;

    if ((e = ioHttpResponseHeaderRead(&self->socket, &header))) {
        free(resolvedPath);
        return e;
    }

    if ((e = ioHttpResponseHeaderEssential(header, &scheme, &response))) {
        free(header), free(resolvedPath);
        return e;
    }

    /* TCP stream is now an the beginning of the HTTP body */
    HeadersPopulate(header, &headerResponse);
    free(header), free(scheme);
    HeadersCompute(self, &headerResponse, "GET");

    httpSiteSchemeFileClose(self);
    self->file = malloc(sizeof(HttpSiteOpenFile));
    self->file->fullUri = resolvedPath;
    self->file->start = start;
    self->file->end = end;

    HeadersFree(&headerResponse);

    return NULL;
}

const char *httpSiteSchemeFileOpenWrite(HttpSite *self, const char *path, PlatformFileOffset start, PlatformFileOffset end) {
    /* HTTP implementation is read only */
    return strerror(EACCES);
}

SOCK_BUF_TYPE httpSiteSchemeFileWrite(HttpSite *self, char *buffer, SOCK_BUF_TYPE size) {
    errno = EBADF;
    return -1;
}
