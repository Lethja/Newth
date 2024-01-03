#include "../site.h"
#include "file.h"

#include <sys/stat.h>

static void UpdateFileUri(FileSite *self, char *path) {
    if (self->fullUri)
        free(self->fullUri);
    self->fullUri = platformPathSystemToFileScheme(path);
}

int fileSiteSchemeChangeDirectory(FileSite *self, const char *path) {
    if (!platformPathSystemChangeWorkingDirectory(path)) {
        char *wd = malloc(FILENAME_MAX);
        if (!wd) {
            free(self->fullUri), self->fullUri = NULL;
            return -1;
        }

        if (platformGetWorkingDirectory(wd, FILENAME_MAX))
            UpdateFileUri(self, wd);

        free(wd);
        return 0;
    }

    return 1;
}

void fileSiteSchemeFree(FileSite *self) {
    if (self->fullUri)
        free(self->fullUri);
}

char *fileSiteSchemeGetWorkingDirectory(FileSite *self) {
    return self->fullUri;
}

FileSite fileSiteSchemeNew(const char *path) {
    FileSite self;
    char *wd;

    self.fullUri = NULL;
    if (path) {
        if (!(wd = platformRealPath((char *) path)))
            return self;

        if (strlen(wd) + 1 > FILENAME_MAX) {
            free(wd);
            return self;
        }
    } else {
        if (!(wd = malloc(FILENAME_MAX)))
            return self;

        if (!platformGetWorkingDirectory(wd, FILENAME_MAX)) {
            free(wd);
            return self;
        }
    }

    UpdateFileUri(&self, wd), free(wd);
    return self;
}

void *fileSiteOpenDirectoryListing(char *path) {
    PlatformFileStat stat;

    if (!platformFileStat(path, &stat)) {
        if (platformFileStatIsDirectory(&stat))
            return platformDirOpen(path);
    }

    return NULL;
}

void *fileSiteReadDirectoryListing(void *listing) {
    char *name;
    size_t nameLen;
    PlatformDirEntry *entry;
    SiteDirectoryEntry *siteEntry;

    fileSiteReadDirectoryListing_skip:
    if (!(entry = platformDirRead(listing)) || !(name = platformDirEntryGetName(entry, &nameLen)) || !nameLen)
        return NULL;

    if (name[0] == '.')
        goto fileSiteReadDirectoryListing_skip;

    if (!(siteEntry = malloc(sizeof(SiteDirectoryEntry))))
        return NULL;

    if (!(siteEntry->name = malloc(nameLen + 1))) {
        free(siteEntry);
        return NULL;
    }

    strcpy(siteEntry->name, name);
    siteEntry->modifiedDate = NULL, siteEntry->isDirectory = platformDirEntryIsDirectory(entry, listing);
    return siteEntry;
}

void fileSiteCloseDirectoryListing(void *listing) {
    platformDirClose(listing);
}
