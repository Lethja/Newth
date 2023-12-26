#ifndef NEW_DL_SITE_H
#define NEW_DL_SITE_H

#include "site/file.h"
#include "site/http.h"

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

Site siteNew(enum SiteType type, const char *path);

Site *siteArrayGetActive(void);

size_t *siteArrayGetActiveNth(void);

char *siteArrayInit(void);

void siteFree(Site *self);

void siteArrayFree(void);

char *siteGetWorkingDirectory(Site *self);

int siteSetWorkingDirectory(Site *self, char *path);

#endif /* NEW_DL_SITE_H */
