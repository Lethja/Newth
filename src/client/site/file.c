#include "file.h"

#include "../../platform/platform.h"

static void UpdateFileScheme(FileSite *self, char *path) {
    if (self->workingDirectory)
        free(self->workingDirectory);
    self->workingDirectory = platformPathSystemToFileScheme(path);
}

int fileSiteSchemeChangeDirectory(FileSite *self, const char *path) {
    if (!platformPathSystemChangeWorkingDirectory(path)) {
        char *wd = malloc(FILENAME_MAX);
        if (!wd) {
            free(self->workingDirectory), self->workingDirectory = NULL;
            return -1;
        }

        if (platformGetWorkingDirectory(wd, FILENAME_MAX))
            UpdateFileScheme(self, wd);

        return 0;
    }

    return 1;
}

void fileSiteSchemeFree(FileSite *self) {
    if (self->workingDirectory)
        free(self->workingDirectory);
}

char *fileSiteSchemeGetWorkingDirectory(FileSite *self) {
    return self->workingDirectory;
}

FileSite fileSiteSchemeNew(void) {
    FileSite self;
    char *wd = malloc(FILENAME_MAX);

    self.workingDirectory = NULL;
    if (!wd)
        return self;

    if (platformGetWorkingDirectory(wd, FILENAME_MAX))
        UpdateFileScheme(&self, wd);

    free(wd);
    return self;
}
