#ifndef NEW_DL_HTTP_H
#define NEW_DL_HTTP_H

#include "../io.h"

typedef struct HttpSite {
    SocketAddress address;
    char *sitePath;
} HttpSite;

int httpSiteSchemeChangeDirectory(HttpSite *self, const char *path);

char *httpSiteSchemeGetWorkingDirectory(HttpSite *self);

void httpSiteSchemeFree(HttpSite *self);

void *httpSiteOpenDirectoryListing(char *path);

void *httpSiteReadDirectoryListing(void *listing);

void httpSiteCloseDirectoryListing(void *listing);

HttpSite httpSiteSchemeNew(const char *path);

#endif /* NEW_DL_HTTP_H */
