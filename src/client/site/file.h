#ifndef NEW_DL_FILE_H
#define NEW_DL_FILE_H

typedef struct FileSite {
    char *fullUri;
} FileSite;

int fileSiteSchemeChangeDirectory(FileSite *self, const char *path);

char *fileSiteSchemeGetWorkingDirectory(FileSite *self);

void fileSiteSchemeFree(FileSite *self);

void *fileSiteOpenDirectoryListing(FileSite *self, char *path);

void *fileSiteReadDirectoryListing(void *listing);

void fileSiteCloseDirectoryListing(void *listing);

char *fileSiteSchemeNew(FileSite *self, const char *path);

#endif /* NEW_DL_FILE_H */
