#include "http.h"
#include "../uri.h"

#include <stddef.h>
#include <stdlib.h>

#pragma region Static Helper Functions

static inline char *ConnectTo(HttpSite *self, UriDetails *details) {
    if (uriDetailsCreateSocketAddress(details, &self->address, SCHEME_HTTP) ||
        ioCreateSocketFromSocketAddress(&self->address, &self->socket) ||
        connect(self->socket, &self->address.address.sock, sizeof(self->address.address.sock)) == -1)
        return strerror(platformSocketGetLastError());

    return NULL;
}

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

static inline char HttpResponseOk(const char *response) {
    switch (response[0]) {
        case '2':
            return 1;
        default:
            return 0;
    }
}

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

char *httpSiteSchemeNew(HttpSite *self, const char *path) {
    UriDetails details = uriDetailsNewFrom(path);
    char *header, *scheme, *response, *send;

    if (!details.path)
        details.path = malloc(2), details.path[0] = '/', details.path[1] = '\0';

    if (ConnectTo(self, &details))
        goto httpSiteSchemeNew_abort;

    header = scheme = response = NULL, send = ioGenerateHttpHeadRequest(details.path, NULL);
    if (!send || WakeUpAndSend(self, send, strlen(send)) || ioHttpHeadRead(&self->socket, &header) ||
        HttpGetEssentialResponse(header, &scheme, &response))
        goto httpSiteSchemeNew_closeSocketAndAbort;

    free(send);

    if (!(HttpResponseOk(response)))
        goto httpSiteSchemeNew_closeSocketAndAbort;

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
    return "Error connecting to HTTP server";
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

