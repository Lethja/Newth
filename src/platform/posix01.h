#ifndef OPEN_WEB_PLATFORM_UNIX_H
#define OPEN_WEB_PLATFORM_UNIX_H

#include <arpa/inet.h>
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <sys/types.h>

#include <dirent.h>
#include <errno.h>
#include <fcntl.h>
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
typedef FILE* PlatformFile;

#define SOCKIN6 sockaddr_in6
#define CLOSE_SOCKET(x) close(x)
#define SOCKET int
#define SOCK_BUF_TYPE size_t
#define FSEEK_64 fseeko
#define FTELL_64 ftello

#endif /* OPEN_WEB_PLATFORM_UNIX_H */
