#ifndef NEW_DL_HTTP_H
#define NEW_DL_HTTP_H

#include "../io.h"
#include "../site.h"

typedef struct HttpSiteDirectoryListing {
    time_t asOf; /* Date & Time of last reload */
    size_t idx, len; /* Position and Length of 'entry' array */
    SiteDirectoryEntry *entry; /* Array of entries */
    char *fullUri; /* uri of current listing in the 'entry' array */
} HttpSiteDirectoryListing;

typedef struct HttpSite {
    SocketAddress address;
    HttpSiteDirectoryListing *directory;
    char *fullUri;
    SOCKET socket;
} HttpSite;

int httpSiteSchemeWorkingDirectorySet(HttpSite *self, const char *path);

char *httpSiteSchemeWorkingDirectoryGet(HttpSite *self);

void httpSiteSchemeFree(HttpSite *self);

void *httpSiteSchemeDirectoryListingOpen(HttpSite *self, char *path);

void *httpSiteSchemeDirectoryListingRead(void *listing);

void httpSiteSchemeDirectoryListingClose(void *listing);

const char *httpSiteNew(HttpSite *self, const char *path);

#endif /* NEW_DL_HTTP_H */
