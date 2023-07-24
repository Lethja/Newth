#include <ifaddrs.h>
#include <time.h>
#include <ctype.h>
#include "platform.h"
#include "posix01.h"
#include "../server/event.h"

void platformPathCombine(char *output, const char *path1, const char *path2) {
    const char pathDivider = '/';
    size_t a = strlen(path1), b = strlen(path2), path2Jump = 1;

    if (path1[a - 1] != pathDivider && path2[0] != pathDivider)
        ++path2Jump;
    else if (path1[a - 1] == pathDivider && path2[0] == pathDivider)
        ++path2;

    strncpy(output, path1, a);
    if (path2Jump > 1)
        output[a] = pathDivider;

    memcpy(output + a + path2Jump - 1, path2, b + 1);
}

void platformCloseBindSockets(fd_set *sockets, SOCKET max) {
    int i;

    for (i = 0; i <= max; ++i) {
        if (FD_ISSET(i, sockets)) {
            eventSocketCloseInvoke(&i);
            close(i);
        }
    }
}

char *platformServerStartup(int *listenSocket, sa_family_t family, char *ports) {
    struct sockaddr_storage serverAddress;
    memset(&serverAddress, 0, sizeof(struct sockaddr_storage));

    switch (family) {
        default:
        case AF_INET: {
            struct sockaddr_in *sock = (struct sockaddr_in *) &serverAddress;
            if ((*listenSocket = socket(AF_INET, SOCK_STREAM, 0)) < 0)
                return "Unable to acquire socket from system";

            sock->sin_family = AF_INET;
            sock->sin_addr.s_addr = htonl(INADDR_ANY);
            break;
        }
        case AF_INET6:
        case AF_UNSPEC: {
            struct sockaddr_in6 *sock = (struct sockaddr_in6 *) &serverAddress;
            size_t v6Only = family == AF_INET6;
            if ((*listenSocket = socket(AF_INET6, SOCK_STREAM, 0)) < 0)
                return "Unable to acquire socket from system";

            sock->sin6_family = AF_INET6;
            sock->sin6_addr = in6addr_any;
            setsockopt(*listenSocket, IPPROTO_IPV6, IPV6_V6ONLY, &v6Only, sizeof(v6Only));
            break;
        }
    }

    if (platformBindPort(listenSocket, (struct sockaddr *) &serverAddress, ports))
        return "Unable to bind to designated port numbers";

    if ((listen(*listenSocket, 10)) < 0)
        return "Unable to listen to assigned socket";

    return NULL;
}

int platformAcceptConnection(int fromSocket) {
    const char blocking = 0;
    socklen_t addressSize = sizeof(struct sockaddr_in);
    int clientSocket;
    struct sockaddr_in clientAddress;

    clientSocket = accept(fromSocket, (SA *) &clientAddress, &addressSize);
    platformSocketSetBlock(clientSocket, blocking);

    eventSocketAcceptInvoke(&clientSocket);

    return clientSocket;
}

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

AdapterAddressArray *platformGetAdapterInformation(sa_family_t family) {
    struct AdapterAddressArray *array;
    struct ifaddrs *ifap, *ifa;
    char address[INET6_ADDRSTRLEN];

    if (getifaddrs(&ifap)) {
        return NULL;
    }

    array = malloc(sizeof(AdapterAddressArray));

    if (!array)
        return NULL;

    array->size = 0;

    for (ifa = ifap; ifa; ifa = ifa->ifa_next) {
        if (ifa->ifa_addr->sa_family == AF_INET && family != AF_INET6) {
            struct sockaddr_in *sa = (struct sockaddr_in *) ifa->ifa_addr;
            char *ip4 = inet_ntoa(sa->sin_addr);
            if (strncmp(ip4, "127", 3) != 0)
                strcpy(address, ip4);
            else
                continue;

        } else if (ifa->ifa_addr->sa_family == AF_INET6 && family != AF_INET) {
            struct sockaddr_in6 *sa = (struct sockaddr_in6 *) ifa->ifa_addr;
            inet_ntop(sa->sin6_family, &sa->sin6_addr, address, INET6_ADDRSTRLEN);
            if (strncmp(address, "::", 2) == 0)
                continue;

        } else continue;

        platformFindOrCreateAdapterIp(array, ifa->ifa_name, ifa->ifa_addr->sa_family == AF_INET6, address);
    }
    freeifaddrs(ifap);

    return array;
}

char *platformRealPath(char *path) {
    return realpath(path, NULL);
}

void platformIpStackExit(void) {
}

char *platformIpStackInit(void) {
    return NULL;
}

int platformOfficiallySupportsIpv6(void) {
    /* TODO: POSIX IPv6 checks */
    return 1;
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
                tm->tm_mon = toupper(month[1]) == 'A' ? 0 : toupper(month[3]) == 'L' ? 6 : 5;
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
    return opendir(path);
}

void platformDirClose(void *dirp) {
    closedir(dirp);
}

void *platformDirRead(void *dirp) {
    return readdir(dirp);
}

char *platformDirEntryGetName(PlatformDirEntry *entry, size_t *length) {
    struct dirent *d = entry;
    char *r = d->d_name;

    if (length)
        *length = strlen(r);

    return r;
}

char platformDirEntryIsHidden(PlatformDirEntry *entry) {
    struct dirent *d = entry;
    return d->d_name[0] == '.' ? 1 : 0;
}

#pragma clang diagnostic push
#pragma ide diagnostic ignored "UnusedParameter"

char platformDirEntryIsDirectory(char *rootPath, char *webPath, PlatformDirEntry *entry) {
#pragma clang diagnostic pop
    struct dirent *d = entry;

#ifdef _DIRENT_HAVE_D_TYPE

    return d->d_type == DT_DIR ? 1 : 0;

#else

    struct stat st;
    char *a, *b = NULL;

    a = platformPathCombine(rootPath, webPath);

    if (!a)
        return 0;

    b = platformPathCombine(a, platformDirEntryGetName(entry, NULL));
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

#endif /* _DIRENT_HAVE_D_TYPE */

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

char *platformGetRootPath(char *path) {
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

    /* TODO: Fix conditional jump warning */
    if (absolutePathLen > 1 && internal[absolutePathLen - 1] == '/')
        internal[absolutePathLen - 1] = '\0';

    strcpy(absolutePath, internal);
    return 0;
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
    if (flags == -1) {
        LINEDBG; /* Should never be here */
        return 0;
    }

    flags = blocking ? (flags & ~O_NONBLOCK) : (flags | O_NONBLOCK);
    return fcntl(socket, F_SETFL, flags);
}

int platformSocketGetLastError(void) {
    return errno;
}

