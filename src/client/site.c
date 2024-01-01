#include "site.h"

#pragma region Site Array Type & Functions

typedef struct SiteArray {
    long len, set;
    Site *array;
} SiteArray;

SiteArray Sites;

Site *siteArrayGetActive(void) {
    if (Sites.set >= Sites.len)
        return NULL;

    return &Sites.array[Sites.set];
}

long siteArrayGetActiveNth(void) {
    if (Sites.set >= Sites.len)
        return -1;

    return Sites.set;
}

char *siteArraySetActiveNth(long siteNumber) {
    if (siteNumber >= Sites.len)
        return "Invalid site";

    Sites.set = siteNumber;
    return NULL;
}

char *siteArrayInit(void) {
    Sites.array = malloc(sizeof(Site)), Sites.len = 1, Sites.set = 0;
    if (!Sites.array)
        return strerror(errno);

    Sites.array[0] = siteNew(SITE_FILE, NULL);
    return NULL;
}

void siteArrayFree(void) {
    int i;
    for (i = 0; i < Sites.len; ++i)
        siteFree(&Sites.array[i]);
    free(Sites.array);
}

#pragma endregion

#pragma region Memory Functions

void siteDirectoryEntryFree(void *entry) {
    SiteDirectoryEntry *e = (SiteDirectoryEntry *) entry;
    if (e->name)
        free(e->name);
    free(e);
}

#pragma endregion

#pragma region Site Base Functions

Site siteNew(enum SiteType type, const char *path) {
    Site self;
    self.type = type;

    switch (self.type) {
        case SITE_FILE:
            self.site.file = fileSiteSchemeNew();
            break;
        case SITE_HTTP:
            self.site.http = httpSiteSchemeNew();
            break;
    }

    return self;
}

void siteFree(Site *self) {
    switch (self->type) {
        case SITE_FILE:
            fileSiteSchemeFree(&self->site.file);
            break;
        case SITE_HTTP:
            httpSiteSchemeFree(&self->site.http);
            break;
    }
}

char *siteGetWorkingDirectory(Site *self) {
    switch (self->type) {
        case SITE_FILE:
            return fileSiteSchemeGetWorkingDirectory(&self->site.file);
        case SITE_HTTP:
            return httpSiteSchemeGetWorkingDirectory(&self->site.http);
        default:
            return NULL;
    }
}

int siteSetWorkingDirectory(Site *self, char *path) {
    switch (self->type) {
        case SITE_FILE:
            return fileSiteSchemeChangeDirectory(&self->site.file, path);
        case SITE_HTTP:
            return httpSiteSchemeChangeDirectory(&self->site.http, path);
        default:
            return 1;
    }
}

void *siteOpenDirectoryListing(Site *self, char *path) {
    switch (self->type) {
        case SITE_FILE:
            return fileSiteOpenDirectoryListing(path);
        case SITE_HTTP:
            return httpSiteOpenDirectoryListing(path);
        default:
            return NULL;
    }
}

SiteDirectoryEntry *siteReadDirectoryListing(Site *self, void *listing) {
    switch (self->type) {
        case SITE_FILE:
            return fileSiteReadDirectoryListing(listing);
        case SITE_HTTP:
            return httpSiteReadDirectoryListing(listing);
        default:
            return NULL;
    }
}

void siteCloseDirectoryListing(Site *self, void *listing) {
    switch (self->type) {
        case SITE_FILE:
            fileSiteCloseDirectoryListing(listing);
            break;
        case SITE_HTTP:
            httpSiteCloseDirectoryListing(listing);
            break;
        default:
            break;
    }
}

#pragma endregion
