#ifndef OPEN_WEB_PLATFORM_H
#define OPEN_WEB_PLATFORM_H

#define SA struct sockaddr

#ifndef __STDC_VERSION__
#define inline
#endif

#ifdef _WIN32

#include "mscrtdl.h"

#else

#include "posix01.h"

#endif

#define ADAPTER_NAME_LENGTH 512

typedef struct Address {
    sa_family_t type;
    char address[INET6_ADDRSTRLEN];
} Address;

typedef struct AddressArray {
    size_t size;
    Address *array;
} AddressArray;

typedef struct Adapter {
    char name[ADAPTER_NAME_LENGTH];
    AddressArray addresses;
} NetworkAdapter;

typedef struct AdapterAddressArray {
    size_t size;
    NetworkAdapter *adapter;
} AdapterAddressArray;

/**
 * Get the argv position of the flag parameter (and optionally the argument)
 * @param argc argc from main()
 * @param argv argv from main()
 * @param shortFlag The short flag to look for or '\0' to omit
 * @param longFlag The long flag to look for or NULL to omit
 * @param optArg The argument of the flag or NULL if there wasn't one
 * @return The position of the flag in argv or 0 if the flag couldn't be found
 */
int platformArgvGetFlag(int argc, char **argv, char shortFlag, char *longFlag, char **optArg);

/**
 * Bind one of any ports mentioned in a string
 * @param listenSocket The socket to attempt to bind a listen port to
 * @param sockAddr The socket address structure get use as binding information
 * @param socketSize The size of sockAddr
 * @param port A string with all the potential ports in it
 * @return 0 on successful bind, otherwise error
 */
char platformBindPort(const SOCKET *listenSocket, SA *sockAddr, char *port);

/**
 * Wrapper around reallocation that gracefully hands control back to the caller in the event of a failure
 * @param heap [in/out] pointer to the allocation pointer
 * @param elementSize [in] size of the elements being allocated
 * @param elementNumber [in] number of elements to allocate
 * @return 0 on success other on allocation error
 * @note in the event of an allocation error the existing allocation will remain unchanged
 */
char platformHeapResize(void **heap, size_t elementSize, size_t elementNumber);

/**
 * Combine two path strings together
 * @param path1 The string to combine on the left
 * @param path2 The string to combine on the right
 * @return Combined string. Use free() when done with it
 */
char *platformPathCombine(char *path1, char *path2);

char *platformRealPath(char *path);

/**
 * Close any open sockets in the file descriptor
 * @param sockets the fd_set to close all open sockets on
 * @param max the maximum socket number select() has seen in the session
 */
void platformCloseBindSockets(fd_set *sockets, SOCKET max);

/**
 * Free any memory resources that platformServerProgramInit() allocated
 */
void platformIpStackExit(void);

/**
 * Allocate any memory resources that this platform may need to run network services
 * @return 0 on success, other on error
 */
int platformIpStackInit(void);

/**
 * Start up the server
 * @param listenSocket Out: the socket the server has been bound to
 * @param family protocol to start the server under
 * @param ports In: A port of comma seperated ports to try to bind to from left to right
 * @return 0 on success, other on error
 */
int platformServerStartup(SOCKET *listenSocket, sa_family_t family, char *ports);

/**
 * Attach signals to comment interrupts
 * @param noAction The function to callback on pipe errors
 * @param shutdownCrash The function to callback on crashes
 * @param shutdownProgram The function to callback on graceful exiting signals
 */
void platformConnectSignals(void(*noAction)(int), void(*shutdownCrash)(int), void(*shutdownProgram)(int));

void platformGetIpString(struct sockaddr *addr, char ipStr[INET6_ADDRSTRLEN], sa_family_t *family);

unsigned short platformGetPort(struct sockaddr *addr);

AdapterAddressArray *platformGetAdapterInformation(sa_family_t family);

void platformFreeAdapterInformation(AdapterAddressArray *array);

void platformFindOrCreateAdapterIp(AdapterAddressArray *array, char adapter[ADAPTER_NAME_LENGTH], sa_family_t type,
                                   char ip[INET6_ADDRSTRLEN]);

int platformOfficiallySupportsIpv6(void);

void platformGetTime(void *clock, char *timeStr);

void platformGetCurrentTime(char *timeStr);

char platformGetTimeStruct(void *clock, PlatformTimeStruct *timeStructure);

char platformTimeGetFromHttpStr(const char *str, PlatformTimeStruct *time);

int platformTimeStructEquals(PlatformTimeStruct *t1, PlatformTimeStruct *t2);

void *platformDirOpen(char *path);

void platformDirClose(void *dirp);

void *platformDirRead(void *dirp);

char *platformDirEntryGetName(PlatformDirEntry *entry, size_t *length);

char platformDirEntryIsHidden(PlatformDirEntry *entry);

char platformDirEntryIsDirectory(char *rootPath, char *webPath, PlatformDirEntry *entry);

int platformFileStat(const char *path, PlatformFileStat *fileStat);

SOCKET platformAcceptConnection(SOCKET fromSocket);

char platformFileStatIsDirectory(PlatformFileStat *stat);

char platformFileStatIsFile(PlatformFileStat *stat);

char *platformGetRootPath(char *path);

short platformPathWebToSystem(const char *rootPath, char *webPath, char *absolutePath);

short platformPathSystemToWeb(const char *rootPath, char *absolutePath, char *webPath);

#endif /* OPEN_WEB_PLATFORM_H */
