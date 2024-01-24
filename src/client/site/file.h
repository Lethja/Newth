#ifndef NEW_DL_FILE_H
#define NEW_DL_FILE_H

typedef struct FileSite {
    char *fullUri;
} FileSite;

void fileSiteSchemeFree(FileSite *self);

char *fileSiteSchemeNew(FileSite *self, const char *path);

void fileSiteSchemeDirectoryListingClose(void *listing);

void *fileSiteSchemeDirectoryListingOpen(FileSite *self, char *path);

void *fileSiteSchemeDirectoryListingRead(void *listing);

char *fileSiteSchemeWorkingDirectoryGet(FileSite *self);

int fileSiteSchemeWorkingDirectorySet(FileSite *self, const char *path);

#endif /* NEW_DL_FILE_H */
