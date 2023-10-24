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
    /* TODO: Find DOS portable way to do this */
    return NULL;
}

void platformCloseBindSockets(fd_set *sockets, SOCKET max) {
    /* TODO: Segregate Watt32 from DOS as a platform */
}

void platformIpStackExit(void) {
    /* TODO: Segregate Watt32 from DOS as a platform */
}

char *platformIpStackInit(void) {
    /* TODO: Segregate Watt32 from DOS as a platform */
    return 0;
}

SOCKET *platformServerStartup(sa_family_t family, char *ports, SOCKET *maxSocket, char **err) {
    /* TODO: Segregate Watt32 from DOS as a platform */
    return 0;
}

void platformConnectSignals(void(*noAction)(int), void(*shutdownCrash)(int), void(*shutdownProgram)(int)) {
    /* TODO: Connect SIGINT in realmode DOS */
}

void platformGetIpString(struct sockaddr *addr, char ipStr[INET6_ADDRSTRLEN], sa_family_t *family) {
    /* TODO: Segregate Watt32 from DOS as a platform */
}

unsigned short platformGetPort(struct sockaddr *addr) {
    /* TODO: Segregate Watt32 from DOS as a platform */
    return 0;
}

AdapterAddressArray *platformGetAdapterInformation(sa_family_t family) {
    return NULL;
}

void platformFreeAdapterInformation(AdapterAddressArray *array) {
    /* TODO: Segregate Watt32 from DOS as a platform */
}

void platformFindOrCreateAdapterIp(AdapterAddressArray *array, const char *adapter, sa_family_t type,
                                   char ip[INET6_ADDRSTRLEN], unsigned short port) {
    /* TODO: Segregate Watt32 from DOS as a platform */
}

int platformOfficiallySupportsIpv6(void) {
    /* TODO: Segregate Watt32 from DOS as a platform */
    return 0;
}

void platformGetTime(void *clock, char *timeStr) {
    /* TODO: Get time with dosdate_t and dostime_t */
}

void platformGetCurrentTime(char *timeStr) {
    /* TODO: Get time with _dos_gettime() and _dos_getdate() */
}

char platformGetTimeStruct(void *clock, PlatformTimeStruct *timeStructure) {
    /* TODO: Get time with _dos_gettime() and _dos_getdate() */
    return 1;
}

char platformTimeGetFromHttpStr(const char *str, PlatformTimeStruct *time) {
    /* TODO: Convert string to time struct */
    return 1;
}

int platformTimeStructEquals(PlatformTimeStruct *t1, PlatformTimeStruct *t2) {
    /* TODO: Compare time structs */
    return 0;
}

void *platformDirOpen(char *path) {
    DIR *dir = malloc(sizeof(DIR));
    size_t len;

    if (dir && path) {
        len = strlen(path);
        if(len > 0 && len < FILENAME_MAX - 3) {
            size_t searchPathLen;
            char searchPath[FILENAME_MAX];
            strcpy(searchPath, path);
            searchPathLen = strlen(searchPath);

            if (searchPath[searchPathLen - 1] == '\\') {
                if (searchPathLen < FILENAME_MAX - 2)
                    strcat(searchPath, "*");
            } else {
                if (searchPathLen < FILENAME_MAX - 3)
                    strcat(searchPath, "\\*");
            }

            dir->error = _dos_findfirst( path, _A_NORMAL | _A_SUBDIR, &dir->nextEntry);
        }
        return dir;
    }
    return NULL;
}

void platformDirClose(void *dirp) {
    DIR *dir = dirp;

    _dos_findclose(&dir->lastEntry);
    free(dir);
}

void *platformDirRead(void *dirp) {
    DIR *dir = dirp;

    memcpy(&dir->lastEntry, &dir->nextEntry, sizeof(PlatformDirEntry));
    switch (dir->error) {
        case 0:
            if (!_dos_findnext(&dir->nextEntry))
                ++dir->error;
            return &dir->lastEntry;
        default:
            return NULL;
    }
}

char *platformDirEntryGetName(PlatformDirEntry *entry, size_t *length) {
    *length = strlen(entry->name);
    return entry->name;
}

char platformDirEntryIsHidden(PlatformDirEntry *entry) {
    return (entry->attrib & _A_HIDDEN);
}

char platformDirEntryIsDirectory(char *rootPath, char *webPath, PlatformDirEntry *entry) {
    return (entry->attrib & _A_SUBDIR);
}

int platformFileStat(const char *path, PlatformFileStat *fileStat) {
    return stat(path, fileStat);
}

SOCKET platformAcceptConnection(SOCKET fromSocket) {
    /* TODO: Segregate Watt32 from DOS as a platform */
    return 0;
}

char platformFileStatIsDirectory(PlatformFileStat *stat) {
    return S_ISDIR(stat->st_mode);
}

char platformFileStatIsFile(PlatformFileStat *stat) {
    return S_ISREG(stat->st_mode);
}

char *platformGetRootPath(char *path) {
    /* TODO: Implement DOS path functions */
    return NULL;
}

char *platformGetWorkingDirectory(char *buffer, size_t length) {
    /* TODO: Implement DOS path functions */
    return NULL;
}

short platformPathWebToSystem(const char *rootPath, char *webPath, char *absolutePath) {
    /* TODO: Implement DOS path functions */
    return 500;
}

PlatformFile platformFileOpen(const char *fileName, const char *fileMode) {
    /* TODO: Implement DOS file functions */
    return NULL;
}

int platformFileClose(PlatformFile stream) {
    /* TODO: Implement DOS file functions */
    return 1;
}

int platformFileSeek(PlatformFile stream, PlatformFileOffset offset, int whence) {
    /* TODO: Implement DOS file functions */
    return 1;
}

PlatformFileOffset platformFileTell(PlatformFile stream) {
    /* TODO: Implement DOS file functions */
    return -1;
}

size_t platformFileRead(void *buffer, size_t size, size_t n, PlatformFile stream) {
    /* TODO: Implement DOS file functions */
    return 0;
}

int platformSocketSetBlock(SOCKET socket, char blocking) {
    /* TODO: Segregate Watt32 from DOS as a platform */
    return -1;
}

int platformSocketGetLastError(void) {
    /* TODO: Segregate Watt32 from DOS as a platform */
    return ENOMEM;
}
