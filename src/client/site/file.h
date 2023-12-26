#ifndef NEW_DL_FILE_H
#define NEW_DL_FILE_H

typedef struct FileSite {
    char *workingDirectory;
} FileSite;

int fileSiteSchemeChangeDirectory(FileSite *self, const char *path);

char *fileSiteSchemeGetWorkingDirectory(FileSite *self);

void fileSiteSchemeFree(FileSite *self);

FileSite fileSiteSchemeNew(void);

#endif /* NEW_DL_FILE_H */
