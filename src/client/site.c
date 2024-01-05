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
    Site file = siteNew(SITE_FILE, NULL);
    memset(&Sites, 0, sizeof(SiteArray));
    return siteArrayAdd(&file);
}

void siteArrayFree(void) {
    int i;
    for (i = 0; i < Sites.len; ++i)
        siteFree(&Sites.array[i]);
    free(Sites.array);
}

char *siteArrayAdd(Site *site) {
    void *p;

    if (!(p = Sites.array ? realloc(Sites.array, sizeof(Site) * (Sites.len + 1)) : malloc(sizeof(Site))))
        return strerror(errno);

    Sites.array = p, ++Sites.len, memcpy(&Sites.array[Sites.len - 1], site, sizeof(Site));
    return NULL;
}

void siteArrayRemoveNth(size_t n) {
    if (Sites.len - 1 >= n) {
        void *p;

        memmove(&Sites.array[n], &Sites.array[n + 1], sizeof(Site) * (Sites.len - n - 1)), --Sites.len;
        if (!(p = realloc(Sites.array, sizeof(Site) * (Sites.len + 1))))
            return;

        Sites.array = p;
    }
}

void siteArrayRemove(Site *site) {
    size_t i;

    for (i = 0; i < Sites.len; ++i) {
        if (Sites.array[i].type == site->type) {
            size_t s;
            void *p, *q;

            switch (site->type) {
                default:
                    return;
                case SITE_FILE:
                    p = &Sites.array[i].site.file, q = &site->site.file, s = sizeof(FileSite);
                    break;
                case SITE_HTTP:
                    p = &Sites.array[i].site.http, q = &site->site.http, s = sizeof(HttpSite);
                    break;
            }

            if (!memcmp(p, q, s)) {
                siteArrayRemoveNth(i);
                return;
            }
        }
    }
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
    Site self = {0};

    switch ((self.type = type)) {
        case SITE_FILE:
            self.site.file = fileSiteSchemeNew(path);
            break;
        case SITE_HTTP:
            self.site.http = httpSiteSchemeNew(path);
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
