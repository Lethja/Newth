#include "platform.h"

/* TODO: Implement everything */

void platformPathCombine(char *output, const char *path1, const char *path2) {

}

char *platformRealPath(char *path) {
    return NULL;
}

void platformCloseBindSockets(fd_set *sockets, SOCKET max) {

}

void platformIpStackExit(void) {

}

char *platformIpStackInit(void) {
    return 0;
}

SOCKET *platformServerStartup(sa_family_t family, char *ports, SOCKET *maxSocket, char **err) {
    return 0;
}

void platformConnectSignals(void(*noAction)(int), void(*shutdownCrash)(int), void(*shutdownProgram)(int)) {

}

void platformGetIpString(struct sockaddr *addr, char ipStr[INET6_ADDRSTRLEN], sa_family_t *family) {

}

unsigned short platformGetPort(struct sockaddr *addr) {
    return 0;
}

AdapterAddressArray *platformGetAdapterInformation(sa_family_t family) {
    return NULL;
}

void platformFreeAdapterInformation(AdapterAddressArray *array) {

}

void platformFindOrCreateAdapterIp(AdapterAddressArray *array, const char *adapter, sa_family_t type,
                                   char ip[INET6_ADDRSTRLEN], unsigned short port) {

}

int platformOfficiallySupportsIpv6(void) {
    return 0;
}

void platformGetTime(void *clock, char *timeStr) {

}

void platformGetCurrentTime(char *timeStr) {

}

char platformGetTimeStruct(void *clock, PlatformTimeStruct *timeStructure) {
    return 1;
}

char platformTimeGetFromHttpStr(const char *str, PlatformTimeStruct *time) {
    return 1;
}

int platformTimeStructEquals(PlatformTimeStruct *t1, PlatformTimeStruct *t2) {
    return 0;
}

void *platformDirOpen(char *path) {
    return NULL;
}

void platformDirClose(void *dirp) {

}

void *platformDirRead(void *dirp) {
    return NULL;
}

char *platformDirEntryGetName(PlatformDirEntry *entry, size_t *length) {
    return NULL;
}

char platformDirEntryIsHidden(PlatformDirEntry *entry) {
    return 0;
}

char platformDirEntryIsDirectory(char *rootPath, char *webPath, PlatformDirEntry *entry) {
    return 0;
}

int platformFileStat(const char *path, PlatformFileStat *fileStat) {
    return 1;
}

SOCKET platformAcceptConnection(SOCKET fromSocket) {
    return 0;
}

char platformFileStatIsDirectory(PlatformFileStat *stat) {
    return 0;
}

char platformFileStatIsFile(PlatformFileStat *stat) {
    return 0;
}

char *platformGetRootPath(char *path) {
    return NULL;
}

char *platformGetWorkingDirectory(char *buffer, size_t length) {
    return NULL;
}

short platformPathWebToSystem(const char *rootPath, char *webPath, char *absolutePath) {
    return 500;
}

PlatformFile platformFileOpen(const char *fileName, const char *fileMode) {
    return NULL;
}

int platformFileClose(PlatformFile stream) {
    return 1;
}

int platformFileSeek(PlatformFile stream, PlatformFileOffset offset, int whence) {
    return 1;
}

PlatformFileOffset platformFileTell(PlatformFile stream) {
    return -1;
}

size_t platformFileRead(void *buffer, size_t size, size_t n, PlatformFile stream) {
    return 0;
}

FILE *platformMemoryStreamNew(void) {
    return NULL;
}

void platformMemoryStreamFree(FILE *stream) {

}

int platformMemoryStreamSeek(FILE *stream, long offset, int origin) {
    return 0;
}

long platformMemoryStreamTell(FILE *stream) {
    return -1;
}

int platformSocketSetBlock(SOCKET socket, char blocking) {
    return -1;
}

int platformSocketGetLastError(void) {
    return ENOMEM;
}
