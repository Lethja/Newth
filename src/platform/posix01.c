#include <ifaddrs.h>
#include <net/if.h>
#include <time.h>
#include <ctype.h>
#include "platform.h"
#include "posix01.h"
#include "../server/event.h"

#define MAX_LISTEN 32

#pragma region Manual Interface Binding Compatibility

#ifdef MANUAL_IFACE_LISTEN

static inline SOCKET *BindAllSockets(sa_family_t af, char *bindName, const char *bindPort, char **err) {
    struct addrinfo hints, *res, *r;
    int error, maxs, *s, *socks;

    memset(&hints, 0, sizeof(hints));
    hints.ai_flags = AI_PASSIVE;
    hints.ai_family = af;
    hints.ai_socktype = SOCK_STREAM;
    error = getaddrinfo(bindName, bindPort, &hints, &res);
    if (error) {
        *err = "Unable to get address information";
        return NULL;
    }

    for (maxs = 0, r = res; r; r = r->ai_next, maxs++);

    socks = (int *) malloc((maxs + 1) * sizeof(int));
    if (!socks) {
        freeaddrinfo(res);
        *err = strerror(errno);
        return NULL;
    }

    *socks = 0, s = socks + 1;
    for (r = res; r; r = r->ai_next) {
        *s = socket(r->ai_family, r->ai_socktype, r->ai_protocol);
        if (*s < 0)
            continue;

        if (bind(*s, r->ai_addr, r->ai_addrlen) < 0) {
            close(*s);
            continue;
        }
        (*socks)++;
        s++;
    }

    if (res) freeaddrinfo(res);

    if (*socks == 0) {
        free(socks);
        *err = "No adapters to bind to";
        return NULL;
    }
    return (socks);
}

static inline int ListenAllSockets(SOCKET *listenSocket, fd_set *listenSocketSet, SOCKET *maxSocket, char **err) {
    SOCKET i;
    for (i = 1; i <= *listenSocket; i++) {
        FD_SET(listenSocket[i], listenSocketSet);
        if (listen(listenSocket[i], MAX_LISTEN) < 0) {
            *err = strerror(errno);
            return -1;
        }

        if (*maxSocket < listenSocket[i])
            *maxSocket = listenSocket[i];
    }
    return 0;
}

static SOCKET *BindAllPortsManually(sa_family_t family, char *ports, SOCKET *max, char **err) {
    fd_set socketSet;
    SOCKET *sockets;
    char *comma = strchr(ports, ',');

    if (comma)
        comma[0] = '\0';

    sockets = BindAllSockets(family, NULL, ports, err);
    if (sockets)
        if (ListenAllSockets(sockets, &socketSet, max, err))
            free(sockets), sockets = NULL;

    return sockets;
}

#endif

#pragma endregion

typedef struct PlatformDir {
    DIR *dir;
    char *path;
} PlatformDir;

void platformPathCombine(char *output, const char *path1, const char *path2) {
    const char *pathDivider = "/";
    size_t len = strlen(path1), idx = len - 1;
    const char *p2;

    /* Copy first path into output buffer for manipulation */
    strcpy(output, path1);

    while (output[idx] == pathDivider[0] && idx)
        --idx;

    if (idx) {
        output[idx + 1] = '\0';
        strcat(output, pathDivider);
    } else if (output[0] == pathDivider[0] && output[1] == pathDivider[0])
        output[1] = '\0';

    /* Jump over any leading dividers in the second path then concatenate it */
    len = strlen(path2), idx = 0;

    while (path2[idx] == pathDivider[0] && idx < len)
        ++idx;

    if (idx == len)
        return;

    p2 = &path2[idx];
    strcat(output, p2);
}

#pragma region Network Bind & Listen

#ifdef PLATFORM_NET_LISTEN

int platformAcceptConnection(int fromSocket) {
    const char blocking = 0;
    socklen_t addressSize = sizeof(struct sockaddr_in);
    int clientSocket;
    struct sockaddr_in clientAddress;

    clientSocket = accept(fromSocket, (struct sockaddr *) &clientAddress, &addressSize);
    platformSocketSetBlock(clientSocket, blocking);

    eventSocketAcceptInvoke(&clientSocket);

    return clientSocket;
}

void platformCloseBindSockets(const SOCKET *sockets) {
    SOCKET i, max = sockets[0];

    for (i = 1; i <= max; ++i) {
        eventSocketCloseInvoke(&i);
        close(i);
    }
}

SOCKET *platformServerStartup(sa_family_t family, char *ports, char **err) {
    SOCKET *r;
#ifdef MANUAL_IFACE_LISTEN
    r = BindAllPortsManually(family, ports, maxSocket, err);
    if (r)
        return r;

    return NULL;
#else
    SOCKET listenSocket;
    struct sockaddr_storage serverAddress;
    memset(&serverAddress, 0, sizeof(struct sockaddr_storage));

    switch (family) {
        default:
        case AF_INET: {
            struct sockaddr_in *sock = (struct sockaddr_in *) &serverAddress;
            if ((listenSocket = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
                *err = "Unable to acquire IPV4 socket from system";
                return NULL;
            }

            sock->sin_family = AF_INET;
            sock->sin_addr.s_addr = htonl(INADDR_ANY);
            break;
        }
        case AF_INET6:
        case AF_UNSPEC: {
            struct sockaddr_in6 *sock = (struct sockaddr_in6 *) &serverAddress;
            size_t v6Only = family == AF_INET6;
            if ((listenSocket = socket(AF_INET6, SOCK_STREAM, 0)) < 0) {
                *err = "Unable to acquire IPV6 socket from system";
                return NULL;
            }

            sock->sin6_family = AF_INET6;
            sock->sin6_addr = in6addr_any;
            setsockopt(listenSocket, IPPROTO_IPV6, IPV6_V6ONLY, &v6Only, sizeof(v6Only));
            break;
        }
    }

    if (platformBindPort(&listenSocket, (struct sockaddr *) &serverAddress, ports)) {
        *err = "Unable to bind to designated port numbers";
        return NULL;
    }

    if ((listen(listenSocket, MAX_LISTEN)) < 0) {
        *err = "Unable to listen to assigned socket";
        return NULL;
    }

    r = malloc(sizeof(SOCKET) * 2);
    if (r) {
        r[0] = 1, r[1] = listenSocket;
        return r;
    } else
        *err = strerror(errno);

    return NULL;
#endif
}

#endif /* PLATFORM_NET_LISTEN */

#pragma endregion

void platformConnectSignals(void(*noAction)(int), void(*shutdownCrash)(int), void(*shutdownProgram)(int)) {
    signal(SIGPIPE, noAction);
    signal(SIGSEGV, shutdownCrash);
    signal(SIGHUP, shutdownProgram);
    signal(SIGINT, shutdownProgram);
    signal(SIGTERM, shutdownProgram);
}

void platformGetIpString(struct sockaddr *addr, char ipStr[INET6_ADDRSTRLEN], sa_family_t *family) {
    *family = addr->sa_family;
    if (addr->sa_family == AF_INET) {
        struct sockaddr_in *s4 = (struct sockaddr_in *) addr;
        char *ip = inet_ntoa(s4->sin_addr);
        strcpy(ipStr, ip);
    } else if (addr->sa_family == AF_INET6) {
        const char ipv4[8] = "::ffff:";
        struct sockaddr_in6 *s6 = (struct sockaddr_in6 *) addr;
        inet_ntop(s6->sin6_family, &s6->sin6_addr, ipStr, INET6_ADDRSTRLEN);
        if (strncmp(ipStr, ipv4, sizeof(ipv4) - 1) == 0) {
            memmove(ipStr, &ipStr[7], INET_ADDRSTRLEN);
            *family = AF_INET;
        } else
            *family = AF_INET6;
    } else
        strcpy(ipStr, "???");
}

unsigned short platformGetPort(struct sockaddr *addr) {
    if (addr->sa_family == AF_INET) {
        struct sockaddr_in *s4 = (struct sockaddr_in *) addr;
        return ntohs(s4->sin_port);
    } else if (addr->sa_family == AF_INET6) {
        struct sockaddr_in *s6 = (struct sockaddr_in *) addr;
        return ntohs(s6->sin_port);
    }

    return 0;
}

#pragma region Network Adapter Discovery

#ifdef PLATFORM_NET_ADAPTER

/* TODO: Store all adapters with correct port numbers (for Haiku) */
/*
AdapterAddressArray *
platformSetAdapterInformationWithSocket(AdapterAddressArray *self, struct ifaddrs *interface, SOCKET socket,
                                        sa_family_t family) {
    struct sockaddr_storage sockaddr;
    socklen_t sockLen = sizeof(struct sockaddr_storage);
    char address[INET6_ADDRSTRLEN];
    unsigned short port;

    getsockname(socket, (struct sockaddr *) &sockaddr, &sockLen);

    if (interface->ifa_addr->sa_family == AF_INET && family != AF_INET6) {
        struct sockaddr_in *sa = (struct sockaddr_in *) interface->ifa_addr;
        char *ip4 = inet_ntoa(sa->sin_addr);
        strcpy(address, ip4);
    } else if (interface->ifa_addr->sa_family == AF_INET6 && family != AF_INET) {
        struct sockaddr_in6 *sa = (struct sockaddr_in6 *) interface->ifa_addr;
        inet_ntop(sa->sin6_family, &sa->sin6_addr, address, INET6_ADDRSTRLEN);
    } else
        return self;

    if (sockaddr.ss_family == AF_INET) {
        struct sockaddr_in *sock = (struct sockaddr_in *) &sockaddr;
        port = ntohs(sock->sin_port);
    } else if (sockaddr.ss_family == AF_INET6) {
        struct SOCKIN6 *sock = (struct SOCKIN6 *) &sockaddr;
        port = ntohs(sock->sin6_port);
    } else
        return self;

    if (!self)
        self = malloc(sizeof(AdapterAddressArray));

    platformFindOrCreateAdapterIp(self, interface->ifa_name, interface->ifa_addr->sa_family, address, port);

    return self;
}
*/

AdapterAddressArray *platformGetAdapterInformation(sa_family_t family) {
    struct AdapterAddressArray *array;
    struct ifaddrs *ifap, *ifa;
    char address[INET6_ADDRSTRLEN];
    /* unsigned short port; */

    if (getifaddrs(&ifap)) {
        return NULL;
    }

    array = malloc(sizeof(AdapterAddressArray));

    if (!array)
        return NULL;

    array->size = 0;

    for (ifa = ifap; ifa; ifa = ifa->ifa_next) {
        if (!ifa->ifa_name || !ifa->ifa_addr || ifa->ifa_flags & IFF_LOOPBACK || ifa->ifa_flags & IFF_POINTOPOINT)
            continue;

        if (ifa->ifa_addr->sa_family == AF_INET && family != AF_INET6) {
            struct sockaddr_in *sa = (struct sockaddr_in *) ifa->ifa_addr;
            char *ip4 = inet_ntoa(sa->sin_addr);
            /* port = sa->sin_port; */
            strcpy(address, ip4);
        } else if (ifa->ifa_addr->sa_family == AF_INET6 && family != AF_INET) {
            struct sockaddr_in6 *sa = (struct sockaddr_in6 *) ifa->ifa_addr;
            inet_ntop(sa->sin6_family, &sa->sin6_addr, address, INET6_ADDRSTRLEN);
            /* port = sa->sin6_port; */
        } else continue;

        platformFindOrCreateAdapterIp(array, ifa->ifa_name, ifa->ifa_addr->sa_family == AF_INET6, address);
    }
    freeifaddrs(ifap);

    return array;
}

#endif

#pragma endregion

char *platformRealPath(char *path) {
    return realpath(path, NULL);
}

void platformIpStackExit(void) {
}

char *platformIpStackInit(void) {
    return NULL;
}

int platformOfficiallySupportsIpv6(void) {
#ifdef __linux__
    const char *inet6File = "/proc/net/if_inet6";
    PlatformFileStat fileStat;
    if (!platformFileStat(inet6File, &fileStat) && platformFileStatIsFile(&fileStat))
        return 1;
#endif
    return 0;
}

void platformGetTime(void *clock, char *time) {
    struct tm *timeInfo = gmtime(clock);
    strftime(time, 30, "%a, %d %b %Y %H:%M:%S GMT", timeInfo);
}

void platformGetCurrentTime(char *timeStr) {
    time_t rawTime;
    time(&rawTime);
    platformGetTime(&rawTime, timeStr);
}

char platformGetTimeStruct(void *clock, PlatformTimeStruct *timeStructure) {
    PlatformTimeStruct *tm = gmtime(clock);

    if (tm) {
        memcpy(timeStructure, tm, sizeof(PlatformTimeStruct));
        return 0;
    }
    return 1;
}

int platformTimeStructEquals(PlatformTimeStruct *t1, PlatformTimeStruct *t2) {
    return (t1->tm_year == t2->tm_year && t1->tm_mon == t2->tm_mon && t1->tm_mday == t2->tm_mday &&
            t1->tm_hour == t2->tm_hour && t1->tm_min == t2->tm_min && t1->tm_sec == t2->tm_sec);
}

char platformTimeGetFromHttpStr(const char *str, PlatformTimeStruct *time) {
    struct tm *tm = time;
    char day[4] = "", month[4] = "";
    if (sscanf(str, "%3s, %d %3s %d %d:%d:%d GMT", day, &tm->tm_mday, month, /* NOLINT(cert-err34-c) */
               &tm->tm_year, /* NOLINT(cert-err34-c) */
               &tm->tm_hour, /* NOLINT(cert-err34-c) */
               &tm->tm_min, &tm->tm_sec)) {
        switch (toupper(day[0])) {
            case 'F':
                tm->tm_wday = 5;
                break;
            case 'M':
                tm->tm_wday = 1;
                break;
            case 'S':
                tm->tm_wday = toupper(day[1]) == 'A' ? 6 : 0;
                break;
            case 'T':
                tm->tm_wday = toupper(day[1]) == 'U' ? 2 : 4;
                break;
            case 'W':
                tm->tm_wday = 3;
                break;
            default:
                return 1;
        }

        switch (toupper(month[0])) {
            case 'A':
                tm->tm_mon = toupper(month[1]) == 'P' ? 3 : 7;
                break;
            case 'D':
                tm->tm_mon = 11;
                break;
            case 'F':
                tm->tm_mon = 1;
                break;
            case 'J':
                tm->tm_mon = toupper(month[1]) == 'A' ? 0 : toupper(month[2]) == 'L' ? 6 : 5;
                break;
            case 'M':
                tm->tm_mon = toupper(month[2]) == 'R' ? 2 : 4;
                break;
            case 'N':
                tm->tm_mon = 10;
                break;
            case 'O':
                tm->tm_mon = 9;
                break;
            case 'S':
                tm->tm_mon = 8;
                break;
            default:
                return 1;
        }

        tm->tm_year -= 1900;
        return 0;
    }
    return 1;
}

void *platformDirOpen(char *path) {
    PlatformDir *self;
    DIR *d = opendir(path);

    if (!d)
        return NULL;

    if (!(self = malloc(sizeof(PlatformDir)))) {
        closedir(d);
        return NULL;
    }

    if (!(self->path = malloc(strlen(path) + 1))) {
        closedir(d);
        free(self);
        return NULL;
    }

    strcpy(self->path, path);
    self->dir = d;
    return self;
}

void platformDirClose(void *dirp) {
    PlatformDir *self = dirp;

    closedir(self->dir);
    free(self->path);
    free(self);
}

void *platformDirRead(void *dirp) {
    PlatformDir *self = dirp;

    return readdir(self->dir);
}

char *platformDirEntryGetName(PlatformDirEntry *entry, size_t *length) {
    struct dirent *d = entry;
    char *r = d->d_name;

    if (length)
        *length = strlen(r);

    return r;
}

char platformDirEntryGetStats(PlatformDirEntry *entry, void *dirP, PlatformFileStat *st) {
    struct dirent *d = entry;
    PlatformDir *dir = dirP;
    char *entryPath;

    if (!(entryPath = malloc(strlen(dir->path) + strlen(d->d_name) + 2))) {
        free(dir);
        return 0;
    }

    platformPathCombine(entryPath, dir->path, d->d_name);

    if (platformFileStat(entryPath, st)) {
        free(entryPath), free(dir);
        return 0;
    }

    free(entryPath);

    return 1;
}

char platformDirEntryIsHidden(PlatformDirEntry *entry) {
    struct dirent *d = entry;
    return d->d_name[0] == '.' ? 1 : 0;
}

#pragma clang diagnostic push
#pragma ide diagnostic ignored "UnusedParameter"

char platformDirEntryIsDirectory(PlatformDirEntry *entry, void *dirP, PlatformFileStat **st) {
#pragma clang diagnostic pop
    struct dirent *d = entry;
    struct stat *s;

#ifdef _DIRENT_HAVE_D_TYPE

    if (d->d_type != DT_UNKNOWN)
        return d->d_type == DT_DIR ? 1 : 0;

#endif /* _DIRENT_HAVE_D_TYPE */

    if (st) {
        s = *st;
        return S_ISDIR(s->st_mode);
    } else if ((s = malloc(sizeof(struct stat)))) {
        if (platformDirEntryGetStats(entry, dirP, s)) {
            char mode = S_ISDIR(s->st_mode);
            free(s);
            return mode;
        }
    }

    return 0;
}

int platformFileStat(const char *path, PlatformFileStat *fileStat) {
    struct stat *st = fileStat;
    return stat(path, st);
}

char platformFileStatIsDirectory(PlatformFileStat *stat) {
    return S_ISDIR(stat->st_mode);
}

char platformFileStatIsFile(PlatformFileStat *stat) {
    return S_ISREG(stat->st_mode);
}

char *platformGetDiskPath(char *path) {
    char *test = platformRealPath(path);
    if (!test) {
        printf("No such directory \"%s\"\n", path);
        exit(1);
    }

    return test;
}

char *platformGetWorkingDirectory(char *buffer, size_t length) {
    return getcwd(buffer, length);
}

short platformPathWebToSystem(const char *rootPath, char *webPath, char *absolutePath) {
    size_t absolutePathLen = strlen(rootPath) + strlen(webPath) + 1;
    char internal[FILENAME_MAX];

    if (absolutePathLen >= FILENAME_MAX)
        return 500;

    platformPathCombine(internal, rootPath, webPath);

    if (absolutePathLen > 1 && internal[absolutePathLen - 1] == '/')
        internal[absolutePathLen - 1] = '\0';

    strcpy(absolutePath, internal);
    return 0;
}

char *platformPathSystemToFileScheme(char *path) {
    char *r, *abs = platformRealPath(path);
    size_t absLen;

    if (!abs || !(absLen = strlen(abs)) || !(r = malloc(absLen + 8)))
        return NULL;

    strcpy(r, "file://"), strcat(r, abs), free(abs);
    return r;
}

int platformPathSystemChangeWorkingDirectory(const char *path) {
    return chdir(path);
}

PlatformFile platformFileOpen(const char *fileName, const char *fileMode) {
    return fopen(fileName, fileMode);
}

int platformFileClose(PlatformFile stream) {
    return fclose(stream);
}

int platformFileSeek(PlatformFile stream, PlatformFileOffset offset, int whence) {
    return fseeko(stream, offset, whence);
}

PlatformFileOffset platformFileTell(PlatformFile stream) {
    return ftello(stream);
}

size_t platformFileRead(void *buffer, size_t size, size_t n, PlatformFile stream) {
    return fread(buffer, size, n, stream);
}

int platformSocketSetBlock(SOCKET socket, char blocking) {
    int flags = fcntl(socket, F_GETFL, 0);

    if (flags == -1)
        return 0;

    flags = blocking ? (flags & ~O_NONBLOCK) : (flags | O_NONBLOCK);
    return fcntl(socket, F_SETFL, flags);
}

int platformSocketGetLastError(void) {
    return errno;
}

