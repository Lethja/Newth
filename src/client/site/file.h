#ifndef NEW_DL_FILE_H
#define NEW_DL_FILE_H

typedef struct FileSite {
    char *workingDirectory;
} FileSite;

int fileSiteSchemeChangeDirectory(FileSite *self, const char *path);

char *fileSiteSchemeGetWorkingDirectory(FileSite *self);

void fileSiteSchemeFree(FileSite *self);

void *fileSiteOpenDirectoryListing(char *path);

void *fileSiteReadDirectoryListing(void *listing);

void fileSiteCloseDirectoryListing(void *listing);

FileSite fileSiteSchemeNew(const char *path);

#endif /* NEW_DL_FILE_H */
