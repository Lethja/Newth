#include "http.h"

#include <stddef.h>
#include <stdlib.h>

int httpSiteSchemeChangeDirectory(HttpSite *self, const char *path) {
    return 1;
}

void httpSiteSchemeFree(HttpSite *self) {
    if (self->workingPath)
        free(self->workingPath);
}

char *httpSiteSchemeGetWorkingDirectory(HttpSite *self) {
    return self->workingPath;
}

HttpSite httpSiteSchemeNew(void) {
    HttpSite self;
    self.workingPath = NULL;
    return self;
}
