#include "platform.h"

#ifndef _DIRENT_HAVE_D_TYPE

#include <dirent.h>
#include <stdlib.h>
#include <sys/stat.h>

char platformIsEntryDirectory(char *rootPath, char *webPath, struct dirent *entry) {
    struct stat st;
    char *a, *b = NULL;

    a = platformPathCombine(rootPath, webPath);

    if (!a)
        return 0;

    b = platformPathCombine(a, entry->d_name);
    if (!b)
        goto platformIsEntryDirectoryFail;

    free(a), a = NULL;

    if (stat(b, &st))
        goto platformIsEntryDirectoryFail;

    free(b);
    return S_ISDIR(st.st_mode);

    platformIsEntryDirectoryFail:
    if (a)
        free(a);
    if (b)
        free(b);

    return 0;
}

#endif
