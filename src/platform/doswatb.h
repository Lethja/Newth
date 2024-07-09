#ifndef NEW_TH_DOS_WATT32_H
#define NEW_TH_DOS_WATT32_H

#include "platform.h"

#pragma region Documentation
/* To use Watt32 with Newth. You must edit 'watt32s/src/config.h' so that 'USE_BSD_API' is defined.
 * Defining 'USE_BOOTP', 'USE_BSD_FATAL' and 'USE_DHCP' is also recommended
 * Build the large and/or flat memory model for Watcom */
#pragma endregion

#pragma region DOS/Compiler Specific headers

#include <dos.h>

#ifdef DJGPP

#include <dirent.h>
#include <sys/stat.h>

#else /* Open Watcom assumed */

#include <direct.h>

#endif

#pragma endregion

#pragma region ANSI C headers

#include <ctype.h>
#include <limits.h>
#include <stdio.h>
#include <signal.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#pragma endregion

#pragma region Watt32 library headers

#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>

#include <netdb.h>
#include <tcp.h>

#pragma endregion

typedef struct tm PlatformTimeStruct;
typedef struct dirent PlatformDirEntry;
typedef struct stat PlatformFileStat;
typedef long PlatformFileOffset;
typedef FILE *PlatformFile;

typedef struct PlatformDir {
    DIR *dir;
    char *path;
} PlatformDir;

typedef int nfds_t;

extern char platformShouldExit(void);

#define PLATFORM_SHOULD_EXIT platformShouldExit
#define PLATFORM_SELECT_MAX_TIMEOUT 1

#define SOCKIN6 sockaddr_in6
#define CLOSE_SOCKET(x) closesocket(x)
#define SOCKET int
#define SOCK_BUF_TYPE size_t

#ifdef DJGPP
#define PF_OFFSET "u"
#else /* Open Watcom */
#define PF_OFFSET "lu"
#endif

#define HEX_OFFSET "x"

#ifndef SB_DATA_SIZE
#define SB_DATA_SIZE 64
#endif

#ifndef SOCKET_TRY_AGAIN
#define SOCKET_TRY_AGAIN EAGAIN
#endif

#ifndef SOCKET_WOULD_BLOCK
#define SOCKET_WOULD_BLOCK EWOULDBLOCK
#endif

#endif /* NEW_TH_DOS_WATT32_H */
