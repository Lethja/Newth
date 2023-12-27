#ifndef NEW_DL_SITE_H
#define NEW_DL_SITE_H

#include "site/file.h"
#include "site/http.h"
#include "../platform/platform.h"

#include <stddef.h>

enum SiteType {
    SITE_FILE, SITE_HTTP
};

typedef struct Site {
    int type;
    union site {
        FileSite file;
        HttpSite http;
    } site;
} Site;

typedef struct SiteDirectoryEntry {
    char *name;
} SiteDirectoryEntry;

Site siteNew(enum SiteType type, const char *path);

Site *siteArrayGetActive(void);

size_t *siteArrayGetActiveNth(void);

char *siteArrayInit(void);

void siteFree(Site *self);

void siteArrayFree(void);

char *siteGetWorkingDirectory(Site *self);

int siteSetWorkingDirectory(Site *self, char *path);

void *siteOpenDirectoryListing(Site *self, char *path);

SiteDirectoryEntry *siteReadDirectoryListing(Site *self, void *listing);

void siteCloseDirectoryListing(Site *self, void *listing);

void siteDirectoryEntryFree(void *entry);

#endif /* NEW_DL_SITE_H */
