#include "../site.h"
#include "file.h"

#include <sys/stat.h>

#pragma region Static Helper Functions

static void UpdateFileUri(FileSite *self, char *path) {
    if (self->fullUri)
        free(self->fullUri);
    self->fullUri = platformPathSystemToFileScheme(path);
}

#pragma endregion

int fileSiteSchemeWorkingDirectorySet(FileSite *self, const char *path) {
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

char *fileSiteSchemeWorkingDirectoryGet(FileSite *self) {
    return self->fullUri;
}

char *fileSiteSchemeNew(FileSite *self, const char *path) {
    char *wd;

    self->fullUri = NULL;
    if (path) {
        if (!(wd = platformRealPath((char *) path)))
            return "Unable to resolve absolute path";

        if (strlen(wd) + 1 > FILENAME_MAX) {
            free(wd);
            return "Absolute path is beyond maximum path limit";
        }
    } else {
        if (!(wd = malloc(FILENAME_MAX)))
            return strerror(errno);

        if (!platformGetWorkingDirectory(wd, FILENAME_MAX)) {
            free(wd);
            return "Unable to get systems working directory";
        }
    }

    UpdateFileUri(self, wd), free(wd);
    return NULL;
}

void fileSiteSchemeDirectoryListingClose(void *listing) {
    platformDirClose(listing);
}

char *fileSiteSchemeDirectoryListingEntryStat(void *listing, void *entry, PlatformFileStat *st) {
    SiteDirectoryEntry *e = entry;
    const char *p = platformDirPath(listing);
    char *entryPath;

    if (!p || !(entryPath = malloc(strlen(e->name) + strlen(p) + 2)))
        return 0;

    platformPathCombine(entryPath, p, e->name);

    if (platformFileStat(entryPath, st)) {
        free(entryPath);
        return strerror(errno);
    }

    free(entryPath);

    return NULL;
}

#pragma clang diagnostic push
#pragma ide diagnostic ignored "UnusedParameter"
void *fileSiteSchemeDirectoryListingOpen(FileSite *self, char *path) {
    PlatformFileStat stat;

    if (!platformFileStat(path, &stat)) {
        if (platformFileStatIsDirectory(&stat))
            return platformDirOpen(path);
    }

    return NULL;
}
#pragma clang diagnostic pop

void *fileSiteSchemeDirectoryListingRead(void *listing) {
    const char *name;
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
    siteEntry->modifiedDate = 0, siteEntry->isDirectory = platformDirEntryIsDirectory(entry, listing, NULL);
    return siteEntry;
}
