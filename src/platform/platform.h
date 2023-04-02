#ifndef OPEN_WEB_PLATFORM_H
#define OPEN_WEB_PLATFORM_H

#define SERVER_PORT 8080
#define MAX_LINE 4096
#define SA struct sockaddr

#ifndef __STDC_VERSION__
#define inline
#endif

#ifdef _WIN32

#include "ws2.h"

#else

#include "unix.h"

#endif

#define ADAPTER_NAME_LENGTH 128

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
 * Start up the server
 * @param listenSocket Out: the socket the server has been bound to
 * @param ports In: A port of comma seperated ports to try to bind to from left to right
 * @return 0 on success, other on error
 */
int platformServerStartup(SOCKET *listenSocket, char *ports);

/**
 * Attach signals to comment interrupts
 * @param noAction The function to callback on pipe errors
 * @param shutdownCrash The function to callback on crashes
 * @param shutdownProgram The function to callback on graceful exiting signals
 */
void platformConnectSignals(void(*noAction)(int), void(*shutdownCrash)(int), void(*shutdownProgram)(int));

void platformGetIpString(struct sockaddr *addr, char ipStr[INET6_ADDRSTRLEN]);

unsigned short platformGetPort(struct sockaddr *addr);

AdapterAddressArray *platformGetAdapterInformation(void);

void platformFreeAdapterInformation(AdapterAddressArray *array);

void platformFindOrCreateAdapterIp(AdapterAddressArray *array, char adapter[ADAPTER_NAME_LENGTH], sa_family_t type,
                                   char ip[INET6_ADDRSTRLEN]);

SOCKET platformAcceptConnection(SOCKET fromSocket);

#ifdef _DIRENT_HAVE_D_TYPE

#define IS_ENTRY_DIRECTORY(rootPath, webPath, entry) entry->d_type == DT_DIR

#else

#define IS_ENTRY_DIRECTORY(rootPath, webPath, entry) platformIsEntryDirectory(rootPath, webPath, entry)

#include <dirent.h>

char platformIsEntryDirectory(char *rootPath, char *webPath, struct dirent *entry);

#endif /* _DIRENT_HAVE_D_TYPE */

#endif /* OPEN_WEB_PLATFORM_H */
