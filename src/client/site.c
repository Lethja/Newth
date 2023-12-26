#include "site.h"
#include "../platform/platform.h"

typedef struct SiteArray {
    size_t len, set;
    Site *array;
} SiteArray;

SiteArray Sites;

Site siteNew(enum SiteType type, const char *path) {
    Site self;

    self.type = type;

    switch (self.type) {
        case SITE_FILE:
            self.site.file = fileSiteSchemeNew();
            break;
    }

    return self;
}

Site *siteArrayGetActive(void) {
    if (Sites.set >= Sites.len)
        return NULL;

    return &Sites.array[Sites.set];
}

char *siteArrayInit(void) {
    Sites.array = malloc(sizeof(Site)), Sites.len = 1, Sites.set = 0;
    if (!Sites.array)
        return strerror(errno);

    Sites.array[0] = siteNew(SITE_FILE, NULL);
    return NULL;
}

void siteFree(Site *self) {
    switch (self->type) {
        case SITE_FILE:
            fileSiteSchemeFree(&self->site.file);
            break;
    }
}

void siteArrayFree(void) {
    size_t i;
    for (i = 0; i < Sites.len; ++i)
        siteFree(&Sites.array[i]);
    free(Sites.array);
}

char *siteGetWorkingDirectory(Site *self) {
    switch (self->type) {
        case SITE_FILE:
            return fileSiteSchemeGetWorkingDirectory(&self->site.file);
        case SITE_HTTP:
            return "'pwd' for HTTP not implemented yet...";
        default:
            return NULL;
    }
}

int siteSetWorkingDirectory(Site *self, char *path) {
    switch (self->type) {
        case SITE_FILE:
            return fileSiteSchemeChangeDirectory(&self->site.file, path);
        default:
            return 1;
    }
}

