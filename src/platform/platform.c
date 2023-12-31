#include <stdio.h>
#include <limits.h>
#include "platform.h"

void platformFreeAdapterInformation(AdapterAddressArray *array) {
    if (array->size) {
        size_t i;
        for (i = 0; i < array->size; ++i)
            free(array->adapter[i].addresses.array);

        free(array->adapter);
    }
    free(array);
}

void platformFindOrCreateAdapterIp(AdapterAddressArray *array, const char *adapter, sa_family_t type,
                                   char ip[INET6_ADDRSTRLEN]) {
    size_t i, j;
    for (i = 0; i < array->size; ++i) {
        if (strcmp(adapter, array->adapter[i].name) == 0)
            break;
    }

    if (i == array->size) {
        array->adapter = array->size ? realloc(array->adapter, sizeof(NetworkAdapter) * (array->size + 1)) : malloc(
                sizeof(NetworkAdapter));

        if (!array->adapter)
            return;

        array->adapter[i].addresses.size = 0;
        strncpy(array->adapter[i].name, adapter, ADAPTER_NAME_LENGTH - 1);
        array->adapter[i].name[ADAPTER_NAME_LENGTH - 1] = '\0';
        ++array->size;
    }

    array->adapter[i].addresses.array = array->adapter[i].addresses.size ? realloc(array->adapter[i].addresses.array,
                                                                                   sizeof(Address) *
                                                                                   (array->adapter[i].addresses.size +
                                                                                    1)) : malloc(sizeof(Address));
    if (!array->adapter[i].addresses.array)
        return;

    j = array->adapter[i].addresses.size;
    ++array->adapter[i].addresses.size;

    array->adapter[i].addresses.array[j].type = type;
    strncpy(array->adapter[i].addresses.array[j].address, ip, INET6_ADDRSTRLEN);
}

char platformBindPort(const SOCKET *listenSocket, SA *sockAddr, char *port) {
#define PORT_SIZE_MAX 2048
    size_t portSize = 0, i;
    unsigned short portList[PORT_SIZE_MAX];
    unsigned long out[2];
    char *last = port;

    LINEDBG;

    while (*port != '\0') {
        char portStr[6] = "";
        size_t strLen;
        switch (*port) {
            case ',': /* Divider */
                errno = 0;
                strLen = port - last + 1;
                if (strLen > sizeof(portStr)) {
                    last = port += 1;
                    break;
                }

                strncpy(portStr, last, strLen - 1);
                out[0] = strtoul(portStr, NULL, 10);
                last = port += 1;
                if (!errno && out[0] < USHRT_MAX && portSize < PORT_SIZE_MAX) {
                    portList[portSize] = (unsigned short) out[0];
                    ++portSize;
                }
                break;
            case '0':
            case '1':
            case '2':
            case '3':
            case '4':
            case '5':
            case '6':
            case '7':
            case '8':
            case '9':
                break;
            default:
                /* Non-number character, error out early */
                return 1;
        }
        ++port;
    }

    if (!portSize) {
        errno = 0;
        out[0] = strtoul(last, NULL, 10);
        if (!errno && out[0] < USHRT_MAX) {
            portList[portSize] = (unsigned short) out[0];
            ++portSize;
        }
    }

    if (portSize) {
        if (sockAddr->sa_family == AF_INET) {
            struct sockaddr_in *sock = (struct sockaddr_in *) sockAddr;
            LINEDBG;
            for (i = 0; i < portSize; ++i) {
                sock->sin_port = htons(portList[i]);
                if (bind(*listenSocket, (SA *) sock, sizeof(struct sockaddr_in)) == 0)
                    return 0;
            }
        } else if (sockAddr->sa_family == AF_INET6) {
            struct SOCKIN6 *sock = (struct SOCKIN6 *) sockAddr;
            LINEDBG;
            for (i = 0; i < portSize; ++i) {
                sock->sin6_port = htons(portList[i]);
                if (bind(*listenSocket, (SA *) sock, sizeof(struct SOCKIN6)) == 0)
                    return 0;
            }
        }
    }
    return -1;
}

int platformArgvGetFlag(int argc, char **argv, char shortFlag, char *longFlag, char **optArg) {
    int i;
    for (i = 1; i < argc; ++i) {
        if (argv[i][0] == '-') {
            switch (argv[i][1]) {
                case '-':
                    if (longFlag) {
                        if (strncmp(longFlag, &argv[i][2], strlen(longFlag)) == 0) {
                            if (optArg) {
                                char *equ = strchr(argv[i], '=');
                                if (equ)
                                    *optArg = &equ[1];
                                else if (i + 1 < argc && argv[i + 1][0] != '-')
                                    *optArg = argv[i + 1];
                            }

                            return i;
                        }
                    }
                case '\0':
                    continue;
                default:
                    if (shortFlag != '\0' && argv[i][1] == shortFlag) {
                        if (optArg) {
                            if (argv[i][2] == '\0' && i + 1 < argc && argv[i + 1][0] != '-')
                                *optArg = argv[i + 1];
                            else if (argv[i][2] != '\0')
                                *optArg = &argv[i][2];
                        }
                        return i;
                    }
                    break;
            }
        }
    }

    return 0;
}

char platformHeapResize(void **heap, size_t elementSize, size_t elementNumber) {
    size_t allocSize = elementSize * elementNumber;
    if (*heap && allocSize) {
        void *test = realloc(*heap, allocSize);
        if (test) {
            *heap = test;
            return 0;
        }
    }
    return 1;
}

FILE *platformMemoryStreamNew(void) {
    return tmpfile();
}

void platformMemoryStreamFree(FILE *stream) {
    fclose(stream);
}

int platformMemoryStreamSeek(FILE *stream, long offset, int origin) {
    return fseek(stream, offset, origin);
}

long platformMemoryStreamTell(FILE *stream) {
    return ftell(stream);
}

