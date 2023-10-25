#include "platform.h"

void platformPathCombine(char *output, const char *path1, const char *path2) {
    const char pathDivider = '/', pathDivider2 = '\\';
    size_t a = strlen(path1), path2Jump = 0;

    if ((path1[a - 1] != pathDivider && path1[a - 1] != pathDivider2) &&
        (path2[0] != pathDivider && path2[0] != pathDivider2))
        ++path2Jump;
    else if ((path1[a - 1] == pathDivider || path1[a - 1] == pathDivider2) &&
             (path2[0] == pathDivider || path2[0] == pathDivider2))
        ++path2;

    LINEDBG;

    strcpy(output, path1);
    if (path2Jump > 1)
        output[a] = pathDivider;

    strcpy(output + a + path2Jump, path2);
}

char *platformRealPath(char *path) {
    return _fullpath(NULL, path, _MAX_PATH);
}

void platformConnectSignals(void(*noAction)(int), void(*shutdownCrash)(int), void(*shutdownProgram)(int)) {
    signal(SIGINT, shutdownProgram);
}

AdapterAddressArray *platformGetAdapterInformation(sa_family_t family) {
    return NULL;
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
    return opendir(path);
}

void platformDirClose(void *dirp) {
    closedir(dirp);
}

void *platformDirRead(void *dirp) {
    return readdir(dirp);
}

char *platformDirEntryGetName(PlatformDirEntry *entry, size_t *length) {
    *length = strlen(entry->d_name);
    return entry->d_name;
}

char platformDirEntryIsHidden(PlatformDirEntry *entry) {
    return (entry->d_attr & _A_HIDDEN);
}

char platformDirEntryIsDirectory(char *rootPath, char *webPath, PlatformDirEntry *entry) {
    return (entry->d_attr & _A_SUBDIR);
}

int platformFileStat(const char *path, PlatformFileStat *fileStat) {
    return stat(path, fileStat);
}

char platformFileStatIsDirectory(PlatformFileStat *stat) {
    return S_ISDIR(stat->st_mode);
}

char platformFileStatIsFile(PlatformFileStat *stat) {
    return S_ISREG(stat->st_mode);
}

char *platformGetDiskPath(char *path) {
    char *test;
    if (strlen(path) == 1 || (strlen(path) < 4 && path[1] == ':' && path[2] == '\\')) {
        if (isupper(*path)) {
            test = malloc(5); /* +1 is intended */
            if (test) {
                test[0] = *path, test[1] = ':', test[2] = '\\', test[3] = '\0';
                return test;
            }
        }
    }

    test = platformRealPath(path);
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
    size_t absolutePathLen = strlen(rootPath) + strlen(webPath) + 1, i;
    char internal[FILENAME_MAX];

    LINEDBG;

    if (absolutePathLen >= FILENAME_MAX)
        return 500;

    platformPathCombine(internal, rootPath, webPath);

    LINEDBG;

    for (i = 0; internal[i] != '\0'; ++i) {
        if (internal[i] == '/')
            internal[i] = '\\';
    }

    if (internal[absolutePathLen - 1] == '\\')
        internal[absolutePathLen - 1] = '\0';

    LINEDBG;

    strcpy(absolutePath, internal);

    LINEDBG;

    return 0;
}

PlatformFile platformFileOpen(const char *fileName, const char *fileMode) {
    return fopen(fileName, fileMode);
}

int platformFileClose(PlatformFile stream) {
    return fclose(stream);
}

int platformFileSeek(PlatformFile stream, PlatformFileOffset offset, int whence) {
    return _fseeki64(stream, offset, whence);
}

PlatformFileOffset platformFileTell(PlatformFile stream) {
    return _ftelli64(stream);
}

size_t platformFileRead(void *buffer, size_t size, size_t n, PlatformFile stream) {
    return fread(buffer, size, n, stream);
}

#pragma region Watt32 Networking

SOCKET platformAcceptConnection(SOCKET fromSocket) {
    /* TODO: Segregate Watt32 from DOS as a platform */
    LINEDBG;
    return 0;
}

int platformSocketSetBlock(SOCKET socket, char blocking) {
    /* TODO: Segregate Watt32 from DOS as a platform */
    LINEDBG;
    return -1;
}

int platformSocketGetLastError(void) {
    /* TODO: Segregate Watt32 from DOS as a platform */
    LINEDBG;
    return ENOMEM;
}

void platformCloseBindSockets(fd_set *sockets, SOCKET max) {
    /* TODO: Segregate Watt32 from DOS as a platform */
    LINEDBG;
}

void platformIpStackExit(void) {
    /* TODO: Segregate Watt32 from DOS as a platform */
    LINEDBG;
}

char *platformIpStackInit(void) {
    /* TODO: Segregate Watt32 from DOS as a platform */
    LINEDBG;
    return 0;
}

SOCKET *platformServerStartup(sa_family_t family, char *ports, SOCKET *maxSocket, char **err) {
    /* TODO: Segregate Watt32 from DOS as a platform */
    LINEDBG;
    return 0;
}

void platformGetIpString(struct sockaddr *addr, char ipStr[INET6_ADDRSTRLEN], sa_family_t *family) {
    /* TODO: Segregate Watt32 from DOS as a platform */
    LINEDBG;
}

unsigned short platformGetPort(struct sockaddr *addr) {
    /* TODO: Segregate Watt32 from DOS as a platform */
    LINEDBG;
    return 0;
}

void platformFreeAdapterInformation(AdapterAddressArray *array) {
    /* TODO: Segregate Watt32 from DOS as a platform */
    LINEDBG;
}

void platformFindOrCreateAdapterIp(AdapterAddressArray *array, const char *adapter, sa_family_t type,
                                   char ip[INET6_ADDRSTRLEN], unsigned short port) {
    /* TODO: Segregate Watt32 from DOS as a platform */
    LINEDBG;
}

int platformOfficiallySupportsIpv6(void) {
    /* TODO: Segregate Watt32 from DOS as a platform */
    LINEDBG;
    return 0;
}

#pragma endregion
