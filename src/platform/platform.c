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

void platformFreeAdapterInformation(AdapterAddressArray *array) {
    size_t i;
    for (i = 0; i < array->size; ++i)
        free(array->adapter[i].addresses.array);

    free(array->adapter);
    free(array);
}

void platformFindOrCreateAdapterIp(AdapterAddressArray *array, char adapter[ADAPTER_NAME_LENGTH], short type,
                                   char ip[INET6_ADDRSTRLEN]) {
    size_t i, j;
    for (i = 0; i < array->size; ++i) {
        if (strcmp(adapter, array->adapter[i].name) == 0)
            break;
    }

    if (i == array->size) {
        array->adapter = array->size ? realloc(array->adapter, sizeof(NetworkAdapter) * (array->size + 1)) : malloc(
                sizeof(NetworkAdapter));

        array->adapter[i].addresses.size = 0;
        strncpy(array->adapter[i].name, adapter, ADAPTER_NAME_LENGTH - 1);
        array->adapter[i].name[ADAPTER_NAME_LENGTH - 1] = '\0';
        ++array->size;
    }

    array->adapter[i].addresses.array = array->adapter[i].addresses.size ? realloc(array->adapter[i].addresses.array,
                                                                                   sizeof(Address) *
                                                                                   (array->adapter[i].addresses.size +
                                                                                    1)) : malloc(sizeof(Address));
    j = array->adapter[i].addresses.size;
    ++array->adapter[i].addresses.size;

    array->adapter[i].addresses.array[j].type = type;
    strncpy(array->adapter[i].addresses.array[j].address, ip, INET6_ADDRSTRLEN);
}
