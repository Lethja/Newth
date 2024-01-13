#ifndef OPEN_WEB_PLATFORM_UNIX_H
#define OPEN_WEB_PLATFORM_UNIX_H

#include <arpa/inet.h>
#include <sys/ioctl.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <sys/types.h>

#include <ctype.h>
#include <dirent.h>
#include <errno.h>
#include <fcntl.h>
#include <limits.h>
#include <netdb.h>
#include <signal.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

typedef struct tm PlatformTimeStruct;
typedef struct dirent PlatformDirEntry;
typedef struct stat PlatformFileStat;
typedef off_t PlatformFileOffset;
typedef FILE *PlatformFile;

#define PLATFORM_SELECT_MAX_TIMEOUT 60

#define SOCKIN6 sockaddr_in6
#define CLOSE_SOCKET(x) close(x)
#define SOCKET int
#define SOCK_BUF_TYPE size_t
#define STD_FILE_TYPE size_t
#define PF_OFFSET "ji"
#define HEX_OFFSET "lx"

#ifndef SB_DATA_SIZE
#define SB_DATA_SIZE BUFSIZ
#endif

#ifndef INVALID_SOCKET
#define INVALID_SOCKET (-1)
#endif

#ifndef SOCKET_TRY_AGAIN
#define SOCKET_TRY_AGAIN EAGAIN
#endif

#ifndef SOCKET_WOULD_BLOCK
#define SOCKET_WOULD_BLOCK EWOULDBLOCK
#endif

#endif /* OPEN_WEB_PLATFORM_UNIX_H */
