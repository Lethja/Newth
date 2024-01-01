#ifndef NEW_DL_HTTP_H
#define NEW_DL_HTTP_H

typedef struct HttpSite {
    char *workingPath;
} HttpSite;

int httpSiteSchemeChangeDirectory(HttpSite *self, const char *path);

char *httpSiteSchemeGetWorkingDirectory(HttpSite *self);

void httpSiteSchemeFree(HttpSite *self);

void *httpSiteOpenDirectoryListing(char *path);

void *httpSiteReadDirectoryListing(void *listing);

void httpSiteCloseDirectoryListing(void *listing);

HttpSite httpSiteSchemeNew(void);

#endif /* NEW_DL_HTTP_H */
