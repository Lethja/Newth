#include "err.h"
#include "site.h"
#include "uri.h"

#pragma region Site Array Type & Functions

typedef struct SiteArray {
    long len, set;
    Site *array;
} SiteArray;

SiteArray Sites;

/**
 * Compare sites
 * @param site1 The first site to compare
 * @param site2 The second site to compare
 * @return 1 if sites are identical the same otherwise 0
 */
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

/**
 * Find a site that serve this a URI based on resolved address
 * @param uri The full URI to resolve
 * @return Site ID on success -1 on failure
 */
static inline long SiteArrayGetByUriHostNth(const char *uri) {
    char *desiredAddress;
    enum SiteType type;
    {
        UriDetails details = uriDetailsNewFrom(uri);

        switch (uriDetailsGetScheme(&details)) {
            case SCHEME_FILE:
                type = SITE_FILE, desiredAddress = NULL;
                break;
            case SCHEME_HTTP:
                type = SITE_HTTP, desiredAddress = uriDetailsGetHostAddr(&details);
                break;
            default:
            case SCHEME_UNKNOWN:
                return -1;
        }

        uriDetailsFree(&details);
    }

    {
        long i;
        for (i = 0; i < Sites.len; ++i) {
            Site *site = &Sites.array[i];
            if (type == site->type) {
                switch (type) {
                    case SITE_FILE: /* Any file scheme site is valid */
                        return i;
                    case SITE_HTTP: {
                        UriDetails details = uriDetailsNewFrom(site->site.http.fullUri);
                        char *siteAddress = uriDetailsGetHostAddr(&details);
                        uriDetailsFree(&details);

                        if(siteAddress) {
                            if (strcmp(desiredAddress, siteAddress) == 0) {
                                free(desiredAddress), free(siteAddress);
                                return i;
                            }

                            free(siteAddress);
                        }
                    }
                }
            }
        }
    }

    if (desiredAddress)
        free(desiredAddress);

    return -1;
}

Site *siteArrayGetByUriHost(const char *uri) {
    long i = SiteArrayGetByUriHostNth(uri);
    return i >= 0 ? &Sites.array[i] : NULL;
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

const char *siteArrayActiveSetNth(long siteNumber) {
    if (siteNumber >= Sites.len)
        return ErrInvalidSite;

    Sites.set = siteNumber;
    return NULL;
}

char siteArrayNthMounted(long siteNumber) {
    if (siteNumber < 0 || siteNumber >= Sites.len)
        return 0;
    return 1;
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

void siteFileMetaFree(SiteFileMeta *meta) {
    if (meta->name) {
        if (!meta->path || (meta->name < meta->path || meta->name > &meta->path[strlen(meta->path) + 1]))
            free(meta->name);
    }

    if (meta->path)
        free(meta->path);

    if (meta->modifiedDate)
        free(meta->modifiedDate);
}

void siteDirectoryEntryFree(void *entry) {
    SiteFileMeta *e = (SiteFileMeta *) entry;
    siteFileMetaFree(e);
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
            return httpSiteSchemeNew(&site->site.http, path);
        default:
            return ErrUnknownMountType;
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

const char *siteDirectoryListingEntryStat(Site *self, void *listing, void *entry, SiteFileMeta *meta) {
    switch (self->type) {
        case SITE_FILE:
            return fileSiteSchemeDirectoryListingEntryStat(listing, entry, meta);
        case SITE_HTTP:
            return httpSiteSchemeDirectoryListingEntryStat(listing, entry, meta);
        default:
            return NULL;
    }
}

SiteFileMeta *siteDirectoryListingRead(Site *self, void *listing) {
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

void siteFileClose(Site *self) {
    switch (self->type) {
        case SITE_FILE:
            fileSiteSchemeFileClose(&self->site.file);
        case SITE_HTTP:
            httpSiteSchemeFileClose(&self->site.http);
    }
}

int siteFileAtEnd(Site *self) {
    switch (self->type) {
        case SITE_FILE:
            return fileSiteSchemeFileAtEnd(&self->site.file);
        case SITE_HTTP:
            return httpSiteSchemeFileAtEnd(&self->site.http);
        default:
            return -1;
    }
}

SiteFileMeta *siteFileOpenMeta(Site *self) {
    switch (self->type) {
        case SITE_FILE:
            return self->site.file.file ? &self->site.file.meta : NULL;
        case SITE_HTTP:
            return self->site.http.file ? &self->site.http.file->meta : NULL;
        default:
            return NULL;
    }
}

SiteFileMeta *siteStatOpenMeta(Site *self, const char *path) {
    switch (self->type) {
        case SITE_FILE:
            return fileSiteSchemeStatOpenMeta(&self->site.file, path);
        case SITE_HTTP:
            return httpSiteSchemeStatOpenMeta(&self->site.http, path);
        default:
            return NULL;
    }
}

SOCK_BUF_TYPE siteFileRead(Site *self, char *buffer, SOCK_BUF_TYPE size) {
    switch (self->type) {
        case SITE_FILE:
            return fileSiteSchemeFileRead(&self->site.file, buffer, size);
        case SITE_HTTP:
            return httpSiteSchemeFileRead(&self->site.http, buffer, size);
        default:
            return -1;
    }
}

const char *siteFileOpenRead(Site *self, const char *path, PlatformFileOffset start, PlatformFileOffset end) {
    switch (self->type) {
        case SITE_FILE:
            return fileSiteSchemeFileOpenRead(&self->site.file, path, start, end);
        case SITE_HTTP:
            return httpSiteSchemeFileOpenRead(&self->site.http, path, start, end);
        default:
            return "Not Implemented";
    }
}

const char *siteFileOpenAppend(Site *self, const char *path, PlatformFileOffset start, PlatformFileOffset end) {
    switch (self->type) {
        case SITE_FILE:
            return fileSiteSchemeFileOpenAppend(&self->site.file, path, start, end);
        case SITE_HTTP:
            return httpSiteSchemeFileOpenWrite(&self->site.http, path);
        default:
            return "Not Implemented";
    }
}

const char *siteFileOpenWrite(Site *self, const char *path) {
    switch (self->type) {
        case SITE_FILE:
            return fileSiteSchemeFileOpenWrite(&self->site.file, path);
        case SITE_HTTP:
            return httpSiteSchemeFileOpenWrite(&self->site.http, path);
        default:
            return "Not Implemented";
    }
}

SOCK_BUF_TYPE siteFileWrite(Site *self, char *buffer, SOCK_BUF_TYPE size) {
    switch (self->type) {
        case SITE_FILE:
            return fileSiteSchemeFileWrite(&self->site.file, buffer, size);
        case SITE_HTTP:
            return httpSiteSchemeFileWrite(&self->site.http, buffer, size);
        default:
            return -1;
    }
}

#pragma endregion
