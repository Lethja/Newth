#include "http.h"
#include "../uri.h"

#include <stddef.h>
#include <stdlib.h>

int httpSiteSchemeChangeDirectory(HttpSite *self, const char *path) {
    char *newPath;

    if (!(newPath = uriPathAbsoluteAppend(self->sitePath, path)))
        return -1;

    /* TODO: HTTP GET, check if file, process html links */

    free(self->sitePath), self->sitePath = newPath;
    return 0;
}

void httpSiteSchemeFree(HttpSite *self) {
    if (self->sitePath)
        free(self->sitePath);
}

char *httpSiteSchemeGetWorkingDirectory(HttpSite *self) {
    return self->sitePath;
}

HttpSite httpSiteSchemeNew(const char *path) {
    HttpSite self;
    if (!path)
        path = "/";

    /* TODO: TCP CONNECT HERE */
    /* TODO: HTTP GET HERE */

    self.sitePath = malloc(strlen(path) + 1);
    strcpy(self.sitePath, path);
    return self;
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

