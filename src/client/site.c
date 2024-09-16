#include "err.h"
#include "site.h"
#include "uri.h"

#pragma region Site Array Type & Functions

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

Site *siteArrayActiveGet(const SiteArray *self) {
    if (self->activeRead < 0 || self->activeRead >= self->len || !self->array)
        return NULL;

    return &self->array[self->activeRead];
}

long siteArrayActiveGetNth(const SiteArray *self) {
    if (self->activeRead < 0 || self->activeRead >= self->len || !self->array)
        return -1;

    return self->activeRead;
}

Site *siteArrayActiveGetWrite(const SiteArray *self) {
    if (self->activeWrite < 0 || self->activeWrite >= self->len || !self->array)
        return NULL;

    return &self->array[self->activeWrite];
}

long siteArrayActiveGetWriteNth(const SiteArray *self) {
    if (self->activeWrite < 0 || self->activeWrite >= self->len || !self->array)
        return -1;

    return self->activeWrite;
}

Site *siteArrayGet(const SiteArray *self, long id) {
    if (id < 0 || id >= self->len || !self->array)
        return NULL;

    return &self->array[id];
}

/**
 * Find a site that serve this a URI based on resolved address
 * @param uri The full URI to resolve
 * @return Site ID on success -1 on failure
 */
static inline long SiteArrayGetByUriHostNth(const SiteArray *self, const char *uri) {
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
                uriDetailsFree(&details);
                return -1;
        }

        uriDetailsFree(&details);
    }

    {
        long i;
        for (i = 0; i < self->len; ++i) {
            Site *site = &self->array[i];
            if (type == site->type) {
                switch (type) {
                    case SITE_FILE: /* Any file scheme site is valid */
                        return i;
                    case SITE_HTTP: {
                        UriDetails details = uriDetailsNewFrom(site->site.http.fullUri);
                        char *siteAddress = uriDetailsGetHostAddr(&details);
                        uriDetailsFree(&details);

                        if (siteAddress) {
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

Site *siteArrayGetFromInput(const SiteArray *self, const char *input) {
    long idx;

    if (isdigit(input[0])) {
        errno = 0, idx = strtol(input, NULL, 10);
        if (!errno && idx >= 0 && idx < self->len)
            return &self->array[idx];
    }

    return siteArrayGetByUriHost(self, input);
}

long siteArrayGetFromInputNth(const SiteArray *self, const char *input) {
    long idx;

    if (isdigit(input[0])) {
        errno = 0, idx = strtol(input, NULL, 10);
        if (!errno && idx >= 0 && idx < self->len)
            return idx;
    }

    return SiteArrayGetByUriHostNth(self, input);
}

Site *siteArrayGetByUriHost(const SiteArray *self, const char *uri) {
    long i = SiteArrayGetByUriHostNth(self, uri);
    return i >= 0 ? &self->array[i] : NULL;
}

void siteArrayActiveSet(SiteArray *self, Site *site) {
    long i;

    for (i = 0; i < self->len; ++i) {
        if (SiteCompare(&self->array[i], site)) {
            siteArrayActiveSetNth(self, i);
            return;
        }
    }
}

void siteArrayActiveSetWrite(SiteArray *self, Site *site) {
    long i;

    for (i = 0; i < self->len; ++i) {
        if (SiteCompare(&self->array[i], site)) {
            siteArrayActiveSetNth(self, i);
            return;
        }
    }
}

const char *siteArrayActiveSetNth(SiteArray *self, long siteNumber) {
    if (siteNumber >= self->len)
        return ErrInvalidSite;

    self->activeRead = siteNumber;
    return NULL;
}

const char *siteArrayActiveSetWriteNth(SiteArray *self, long siteNumber) {
    if (siteNumber >= self->len)
        return ErrInvalidSite;

    self->activeWrite = siteNumber;
    return NULL;
}

char siteArrayNthMounted(const SiteArray *self, long siteNumber) {
    if (siteNumber < 0 || siteNumber >= self->len)
        return 0;
    return 1;
}

char *siteArrayInit(SiteArray *self) {
    Site file;
    siteNew(&file, SITE_FILE, NULL);
    memset(self, 0, sizeof(SiteArray));
    return siteArrayAdd(self, &file);
}

void siteArrayFree(SiteArray *self) {
    int i;
    for (i = 0; i < self->len; ++i)
        siteFree(&self->array[i]);
    free(self->array);
}

char *siteArrayAdd(SiteArray *self, Site *site) {
    void *p;

    if (!(p = self->array ? realloc(self->array, sizeof(Site) * (self->len + 1)) : malloc(sizeof(Site))))
        return strerror(errno);

    self->array = p, ++self->len, memcpy(&self->array[self->len - 1], site, sizeof(Site));
    return NULL;
}

void siteArrayRemoveNth(SiteArray *self, long n) {
    if (self->len - 1 >= n) {
        void *p;

        memmove(&self->array[n], &self->array[n + 1], sizeof(Site) * (self->len - n - 1)), --self->len;
        if (!(p = realloc(self->array, sizeof(Site) * (self->len + 1))))
            return;

        self->array = p;
    }

    if (self->activeRead == n)
        self->activeRead = -1;

    if (self->activeWrite == n)
        self->activeWrite = -1;
}

void siteArrayRemove(SiteArray *self, Site *site) {
    long i;

    for (i = 0; i < self->len; ++i) {
        if (SiteCompare(&self->array[i], site)) {
            siteArrayRemoveNth(self, i);
            return;
        }
    }
}

Site *siteArrayPtr(const SiteArray *self, long *length) {
    if (length)
        *length = self->len;

    return self->array;
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

char *siteArrayUserPathResolve(const SiteArray *array, const char *path, char write) {
    char *r;

    if (isdigit(path[0]) && strchr(&path[1], ':')) { /* Looks like a relative site path */
        long l;
        UriDetails details;
        char *c;
        Site *site;

        errno = 0, l = strtol(path, &c, 10);
        if (errno || !(site = siteArrayGet(array, l)))
            return NULL;

        details = uriDetailsNewFrom(siteWorkingDirectoryGet(site));
        if (!(r = uriDetailsCreateStringBase(&details))) {
            uriDetailsFree(&details);
            return NULL;
        }

        if (c[0] == ':' && c[1] != '\0') {
            size_t len = strlen(r) + strlen(c);

            if (platformHeapResize((void **) &r, sizeof(char), len)) {
                free(r);
                return NULL;
            }

            strcat(r, &c[1]);
            return r;
        }

        return NULL;
    } else if (strstr(path, "://")) { /* Looks like an absolute path */
        UriDetails details = uriDetailsNewFrom(path);

        if (details.path) {
            r = uriDetailsCreateString(&details), uriDetailsFree(&details);
            return r;
        }
        uriDetailsFree(&details);
        return NULL;
    } else { /* Looks like a relative path on current site */
        Site *site = write ? siteArrayActiveGetWrite(array) : siteArrayActiveGet(array);
        UriDetails d;

        if (!site)
            return NULL;

        d = uriDetailsNewFrom(siteWorkingDirectoryGet(site));
        if (d.path) {
            char *p = uriPathAbsoluteAppend(d.path, path);
            free(d.path);
            if (p)
                d.path = p, r = uriDetailsCreateString(&d);
            else
                r = NULL;
        } else
            r = NULL;

        uriDetailsFree(&d);

        return r;
    }
}

Site *siteArrayUserPathResolveToQueue(const SiteArray *array, const char *path, char write, char **sitePath) {
    char *a;

    if ((a = siteArrayUserPathResolve(array, path, write))) {
        UriDetails d = uriDetailsNewFrom(a);
        char *host = uriDetailsGetHostAddr(&d);

        if (sitePath)
            *sitePath = d.path, d.path = NULL;

        uriDetailsFree(&d), free(a);
        if (host) {
            Site *s = siteArrayGetByUriHost(array, host);
            free(host);
            return s;
        } else if (sitePath)
            free(*sitePath), *sitePath = NULL;
    }

    return NULL;
}

#pragma endregion
