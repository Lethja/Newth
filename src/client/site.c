#include "site.h"

#pragma region Site Array Type & Functions

typedef struct SiteArray {
    long len, set;
    Site *array;
} SiteArray;

SiteArray Sites;

static inline char SiteCompare(Site *site1, Site *site2) {
    size_t s;
    void *p, *q;

    if (site1->type != site2->type)
        return 0;

    switch (site1->type) {
        default:
            return 0;
        case SITE_FILE:
            p = &site1->site.file, q = &site2->site.file, s = sizeof(FileSite);
            break;
        case SITE_HTTP:
            p = &site1->site.http, q = &site2->site.http, s = sizeof(HttpSite);
            break;
    }

    if (!memcmp(p, q, s))
        return 1;

    return 0;
}

Site *siteArrayActiveGet(void) {
    if (Sites.set >= Sites.len)
        return NULL;

    return &Sites.array[Sites.set];
}

long siteArrayActiveGetNth(void) {
    if (Sites.set >= Sites.len)
        return -1;

    return Sites.set;
}

void siteArrayActiveSet(Site *site) {
    long i;

    for (i = 0; i < Sites.len; ++i) {
        if (SiteCompare(&Sites.array[i], site)) {
            siteArrayActiveSetNth(i);
            return;
        }
    }
}

char *siteArrayActiveSetNth(long siteNumber) {
    if (siteNumber >= Sites.len)
        return "Invalid site";

    Sites.set = siteNumber;
    return NULL;
}

char *siteArrayInit(void) {
    Site file;
    siteNew(&file, SITE_FILE, NULL);
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

void siteArrayRemoveNth(long n) {
    if (Sites.len - 1 >= n) {
        void *p;

        memmove(&Sites.array[n], &Sites.array[n + 1], sizeof(Site) * (Sites.len - n - 1)), --Sites.len;
        if (!(p = realloc(Sites.array, sizeof(Site) * (Sites.len + 1))))
            return;

        Sites.array = p;
    }
}

void siteArrayRemove(Site *site) {
    long i;

    for (i = 0; i < Sites.len; ++i) {
        if (SiteCompare(&Sites.array[i], site)) {
            siteArrayRemoveNth(i);
            return;
        }
    }
}

Site *siteArrayPtr(long *length) {
    if (length)
        *length = Sites.len;

    return Sites.array;
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

const char *siteNew(Site *site, enum SiteType type, const char *path) {
    memset(site, 0, sizeof(Site));

    switch ((site->type = type)) {
        case SITE_FILE:
            return fileSiteSchemeNew(&site->site.file, path);
        case SITE_HTTP:
            return httpSiteNew(&site->site.http, path);
        default:
            return "Unknown mount type";
    }
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

char *siteWorkingDirectoryGet(Site *self) {
    switch (self->type) {
        case SITE_FILE:
            return fileSiteSchemeWorkingDirectoryGet(&self->site.file);
        case SITE_HTTP:
            return httpSiteSchemeWorkingDirectoryGet(&self->site.http);
        default:
            return NULL;
    }
}

int siteWorkingDirectorySet(Site *self, char *path) {
    switch (self->type) {
        case SITE_FILE:
            return fileSiteSchemeWorkingDirectorySet(&self->site.file, path);
        case SITE_HTTP:
            return httpSiteSchemeWorkingDirectorySet(&self->site.http, path);
        default:
            return 1;
    }
}

void *siteDirectoryListingOpen(Site *self, char *path) {
    switch (self->type) {
        case SITE_FILE:
            return fileSiteSchemeDirectoryListingOpen(&self->site.file, path);
        case SITE_HTTP:
            return httpSiteSchemeDirectoryListingOpen(&self->site.http, path);
        default:
            return NULL;
    }
}

SiteDirectoryEntry *siteDirectoryListingRead(Site *self, void *listing) {
    switch (self->type) {
        case SITE_FILE:
            return fileSiteSchemeDirectoryListingRead(listing);
        case SITE_HTTP:
            return httpSiteSchemeDirectoryListingRead(listing);
        default:
            return NULL;
    }
}

void siteDirectoryListingClose(Site *self, void *listing) {
    switch (self->type) {
        case SITE_FILE:
            fileSiteSchemeDirectoryListingClose(listing);
            break;
        case SITE_HTTP:
            httpSiteSchemeDirectoryListingClose(listing);
            break;
        default:
            break;
    }
}

#pragma endregion
