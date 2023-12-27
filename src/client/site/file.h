#ifndef NEW_DL_FILE_H
#define NEW_DL_FILE_H

#include "../site.h"

typedef struct FileSite {
    char *workingDirectory;
} FileSite;

int fileSiteSchemeChangeDirectory(FileSite *self, const char *path);

char *fileSiteSchemeGetWorkingDirectory(FileSite *self);

void fileSiteSchemeFree(FileSite *self);

void *fileSiteOpenDirectoryListing(char *path);

void *fileSiteReadDirectoryListing(void *listing);

void fileSiteCloseDirectoryListing(void *listing);

void siteDirectoryEntryFree(void *entry);

FileSite fileSiteSchemeNew(void);

#endif /* NEW_DL_FILE_H */
