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

/**
 * Combine two path strings together
 * @param path1 The string to combine on the left
 * @param path2 The string to combine on the right
 * @return Combined string. Use free() when done with it
 */
char *platformPathCombine(char *path1, char *path2);

/**
 * Close any open sockets in the file descriptor
 * @param sockets the fd_set to close all open sockets on
 * @param max the maximum socket number select() has seen in the session
 */
void platformCloseBindSockets(fd_set *sockets, SOCKET max);

/**
 * Start up the server
 * @param listenSocket Out: the socket the server has been bound to
 * @param port In: The port to bind to
 * @return 0 on success, other on error
 */
int platformServerStartup(SOCKET *listenSocket, short port);

/**
 * Attach signals to comment interrupts
 * @param noAction The function to callback on pipe errors
 * @param shutdownCrash The function to callback on crashes
 * @param shutdownProgram The function to callback on graceful exiting signals
 */
void platformConnectSignals(void(*noAction)(int), void(*shutdownCrash)(int), void(*shutdownProgram)(int));

SOCKET platformAcceptConnection(SOCKET fromSocket);

#ifdef _DIRENT_HAVE_D_TYPE

#define IS_ENTRY_DIRECTORY(rootPath, webPath, entry) entry->d_type == DT_DIR

#else

#define IS_ENTRY_DIRECTORY(rootPath, webPath, entry) platformIsEntryDirectory(rootPath, webPath, entry)

#include <dirent.h>

char platformIsEntryDirectory(char *rootPath, char *webPath, struct dirent *entry);

#endif /* _DIRENT_HAVE_D_TYPE */

#endif /* OPEN_WEB_PLATFORM_H */
