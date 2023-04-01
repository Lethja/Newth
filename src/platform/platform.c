#include <limits.h>
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

char platformBindPort(const SOCKET *listenSocket, SA *sockaddr, char *port) {
#define PORT_SIZE_MAX 2048
    size_t portSize = 0, i;
    unsigned short portList[PORT_SIZE_MAX];
    unsigned long out[2];
    char *last = port;

    while (*port != '\0') {
        switch (*port) {
            case ',': /* Divider */
                errno = 0;
                *port = '\0';
                out[0] = strtoul(last, NULL, 10);
                *port = ',';
                last = port += 1;
                if (!errno && out[0] < USHRT_MAX && portSize < PORT_SIZE_MAX) {
                    portList[portSize] = out[0];
                    ++portSize;
                }
                break;
        }
        ++port;
    }

    if (!portSize) {
        errno = 0;
        out[0] = strtoul(last, NULL, 10);
        if (!errno && out[0] < USHRT_MAX) {
            portList[portSize] = out[0];
            ++portSize;
        }
    }

    if (portSize) {
        if (sockaddr->sa_family == AF_INET) {
            struct sockaddr_in *sock = (struct sockaddr_in *) sockaddr;
            for (i = 0; i < portSize; ++i) {
                sock->sin_port = htons(portList[i]);
                if (bind(*listenSocket, (SA *) sock, sizeof(struct sockaddr_in)) == 0)
                    return 0;
            }
        } else if (sockaddr->sa_family == AF_INET6) {
            struct sockaddr_in6 *sock = (struct sockaddr_in6 *) sockaddr;
            for (i = 0; i < portSize; ++i) {
                sock->sin6_port = htons(portList[i]);
                if (bind(*listenSocket, (SA *) sock, sizeof(struct sockaddr_in6)) == 0)
                    return 0;
            }
        }
    }
    return -1;
}
