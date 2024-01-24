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

void httpSiteSchemeDirectoryListingClose(void *listing);

char *httpSiteSchemeDirectoryListingEntryStat(void *entry, void *listing, PlatformFileStat *st);

void *httpSiteSchemeDirectoryListingOpen(HttpSite *self, char *path);

void *httpSiteSchemeDirectoryListingRead(void *listing);

void httpSiteSchemeFree(HttpSite *self);

const char *httpSiteSchemeNew(HttpSite *self, const char *path);

char *httpSiteSchemeWorkingDirectoryGet(HttpSite *self);

int httpSiteSchemeWorkingDirectorySet(HttpSite *self, const char *path);

#endif /* NEW_DL_HTTP_H */
