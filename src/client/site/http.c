#include "http.h"
#include "../uri.h"

#include <stddef.h>
#include <stdlib.h>

int httpSiteSchemeChangeDirectory(HttpSite *self, const char *path) {
    char *newPath, *newUri;
    UriDetails details = uriDetailsNewFrom(self->fullUri);

    if (!(newPath = uriPathAbsoluteAppend(details.path ? details.path : "/", path)))
        return -1;

    /* TODO: HTTP GET, check if file, process html links */

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
}

char *httpSiteSchemeGetWorkingDirectory(HttpSite *self) {
    return self->fullUri;
}

char *httpSiteSchemeNew(HttpSite *self, const char *path) {
    UriDetails details = uriDetailsNewFrom(path);
    char *header, *scheme, *response;

    if (uriDetailsCreateSocketAddress(&details, &self->address, SCHEME_HTTP) ||
        ioCreateSocketFromSocketAddress(&self->address, &self->socket) ||
        connect(self->socket, &self->address.address.sock, sizeof(self->address.address.sock)) == -1)
        goto httpSiteSchemeNew_abort;

    if (ioHttpHeadRequest(&self->socket, details.path, NULL) || ioHttpHeadRead(&self->socket, &header) ||
        HttpGetEssentialResponse(header, &scheme, &response))
        goto httpSiteSchemeNew_closeSocketAndAbort;

    if (!strcmp(response, "200")) {
        self->fullUri = uriDetailsCreateString(&details);
        uriDetailsFree(&details);
        return "Remote response not compatible";
    }

    httpSiteSchemeNew_closeSocketAndAbort:
    CLOSE_SOCKET(self->socket);

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

