#ifndef NEW_DL_FILE_H
#define NEW_DL_FILE_H

typedef struct FileSite {
    char *fullUri;
    PlatformFile file;
    SiteFileMeta meta;
} FileSite;

void fileSiteSchemeFree(FileSite *self);

char *fileSiteSchemeNew(FileSite *self, const char *path);

void fileSiteSchemeDirectoryListingClose(void *listing);

char *fileSiteSchemeDirectoryListingEntryStat(void *entry, void *listing, SiteFileMeta *meta);

void *fileSiteSchemeDirectoryListingOpen(FileSite *self, char *path);

void *fileSiteSchemeDirectoryListingRead(void *listing);

char *fileSiteSchemeWorkingDirectoryGet(FileSite *self);

int fileSiteSchemeWorkingDirectorySet(FileSite *self, const char *path);

void fileSiteSchemeFileClose(FileSite *self);

SOCK_BUF_TYPE fileSiteSchemeFileRead(FileSite *self, char *buffer, SOCK_BUF_TYPE size);

const char *fileSiteSchemeFileOpenRead(FileSite *self, const char *path, PlatformFileOffset start, PlatformFileOffset end);

const char *fileSiteSchemeFileOpenWrite(FileSite *self, const char *path, PlatformFileOffset start, PlatformFileOffset end);

SOCK_BUF_TYPE fileSiteSchemeFileWrite(FileSite *self, char *buffer, SOCK_BUF_TYPE size);

#endif /* NEW_DL_FILE_H */
