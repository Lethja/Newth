#include "http.h"
#include "../uri.h"

#include <stddef.h>
#include <stdlib.h>

int httpSiteSchemeChangeDirectory(HttpSite *self, const char *path) {
    char *newPath, *newUri, *header, *scheme, *response;
    UriDetails details = uriDetailsNewFrom(self->fullUri);

    if (!details.path)
        details.path = malloc(2), details.path[0] = '/', details.path[1] = '\0';

    if (!(newPath = uriPathAbsoluteAppend(details.path, path))) {
        uriDetailsFree(&details);
        return -1;
    }

    if (ioHttpHeadRequest(&self->socket, details.path, NULL) || ioHttpHeadRead(&self->socket, &header) ||
        HttpGetEssentialResponse(header, &scheme, &response)) {
        uriDetailsFree(&details), free(newPath);
        return -1;
    }

    if (!(response[0] == '2' && response[1] == '0' && response[2] == '0')) {
        uriDetailsFree(&details), free(newPath);
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
    char *header, *scheme, *response;

    if (!details.path)
        details.path = malloc(2), details.path[0] = '/', details.path[1] = '\0';

    if (uriDetailsCreateSocketAddress(&details, &self->address, SCHEME_HTTP) ||
        ioCreateSocketFromSocketAddress(&self->address, &self->socket) ||
        connect(self->socket, &self->address.address.sock, sizeof(self->address.address.sock)) == -1)
        goto httpSiteSchemeNew_abort;

    header = scheme = response = NULL;
    if (ioHttpHeadRequest(&self->socket, details.path, NULL) || ioHttpHeadRead(&self->socket, &header) ||
        HttpGetEssentialResponse(header, &scheme, &response))
        goto httpSiteSchemeNew_closeSocketAndAbort;

    if (!(response[0] == '2' && response[1] == '0' && response[2] == '0'))
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

