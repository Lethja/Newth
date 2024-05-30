#include <stdio.h>
#include <limits.h>
#include "platform.h"

#pragma region Network Adapter Discovery

#ifdef PLATFORM_NET_ADAPTER

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
        array->adapter = array->size ?
                         realloc(array->adapter, sizeof(NetworkAdapter) * (array->size + 1)) : malloc(sizeof(NetworkAdapter));

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

#endif

#pragma endregion

#pragma region Network Bind & Listen

#ifdef PLATFORM_NET_LISTEN

char platformBindPort(const SOCKET *listenSocket, struct sockaddr *socketAddress, char *port) {
#define PORT_SIZE_MAX 2048
    size_t portSize = 0, i;
    unsigned short portList[PORT_SIZE_MAX];
    unsigned long out[2];
    char *last = port;

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
        if (socketAddress->sa_family == AF_INET) {
            struct sockaddr_in *sock = (struct sockaddr_in *) socketAddress;
            for (i = 0; i < portSize; ++i) {
                sock->sin_port = htons(portList[i]);
                if (bind(*listenSocket, (struct sockaddr *) sock, sizeof(struct sockaddr_in)) == 0)
                    return 0;
            }
        } else if (socketAddress->sa_family == AF_INET6) {
            struct SOCKIN6 *sock = (struct SOCKIN6 *) socketAddress;
            for (i = 0; i < portSize; ++i) {
                sock->sin6_port = htons(portList[i]);
                if (bind(*listenSocket, (struct sockaddr *) sock, sizeof(struct SOCKIN6)) == 0)
                    return 0;
            }
        }
    }
    return -1;
}

#endif

#pragma endregion

#pragma region Argv & System Execution Call

#ifdef PLATFORM_SYS_EXEC

char **platformArgvConvertString(const char *str) {
    char **r = NULL, *a, *p;
    size_t s;

    if (!str || str[0] == '\0')
        return NULL;

    s = strlen(str);
    if (!(a = malloc(s + 1)))
        return NULL;

    strcpy(a, str);
    p = a, s = 0;
    while (*p != '\0') {
        if (*p == ' ') {
            size_t amt = 0;

            ++s;
            while (p[amt] == ' ')
                ++amt;

            if (p[amt] != '\0') {
                char *d = p != a ? &p[1] : p;
                memmove(d, &p[amt], strlen(&p[amt]) + 1);
            } else {
                *p = '\0';
                break;
            }
        }
        ++p;
    }

    if (!(r = malloc(sizeof(char *) * (s + 2)))) {
        free(a);
        return NULL;
    }

    r[s + 1] = NULL, s = strlen(a);
    if (platformHeapResize((void **) &a, sizeof(char), s + 1)) {
        free(a), free(r);
        return NULL;
    }

    r[0] = p = a, s = 1;
    while (*p != '\0') {
        if (*p == ' ') {
            r[s] = p[1] != '\0' ? &p[1] : NULL;
            *p = '\0';
            ++s, ++p;
        }
        ++p;
    }

    return r;
}

void platformArgvFree(char **argv) {
    const char *str = *argv;
    if (str)
        free((void *) str);

    free(argv);
}

#endif

#pragma endregion

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

const char *platformHeapStringAppend(char **alloc, const char *append) {
    size_t len2 = strlen(append);

    if (*alloc) {
        size_t len1 = strlen(*alloc);
        if (!platformHeapResize((void **) alloc, 1, len1 + len2 + 1))
            strncat(*alloc, append, len2 + 1);
        else
            return strerror(errno);
    } else {
        if ((*alloc = malloc(len2 + 1)))
            strncpy(*alloc, append, len2 + 1);
        else
            return strerror(errno);
    }

    return NULL;
}

const char *platformHeapStringAppendAndFree(char **alloc, char *append) {
    const char *r = platformHeapStringAppend(alloc, append);
    free(append);
    return r;
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

char *platformStringFindNeedle(const char *haystack, const char *needle) {
    return platformStringFindNeedleRaw(haystack, needle, strlen(haystack), strlen(needle));
}

char *platformStringFindNeedleRaw(const char *haystack, const char *needle, size_t haystackLen, size_t needleLen) {
    size_t i, j;

    if (haystackLen == 0 || needleLen == 0 || haystackLen < needleLen)
        return NULL; /* A needle bigger then a haystack can't exist within it */

    for (i = 0; i < haystackLen; ++i) {
        if (toupper(haystack[i]) == toupper(needle[0])) {
            size_t h;
            for (h = i, j = 0; needle[j] != '\0'; ++h, ++j) {
                if (toupper(haystack[h]) != toupper(needle[j]))
                    break;
            }

            if (h - i == needleLen)
                return (char *) &haystack[i];
        }
    }

    return NULL;
}

char *platformStringFindWord(const char *haystack, const char *needle) {
    char *r = platformStringFindNeedle(haystack, needle), t[2];

    if (r) {
        unsigned char i;
        t[0] = (char) (r == haystack ? '\0' : r[-1]), t[1] = r[strlen(needle)];

        for (i = 0; i < 2; ++i) {
            switch (t[i]) {
                case '\t':
                case '\n':
                case '\0':
                case '\\':
                case '\'':
                case '"':
                case ' ':
                case '/':
                case '!':
                case '?':
                case '.':
                case '=':
                case '+':
                case '-':
                case '<':
                case '>':
                    break;
                default:
                    return NULL;
            }
        }

        return r;
    }

    return NULL;
}
