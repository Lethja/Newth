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

#define CLOSE_SOCKET(x) close(x)
#define SOCKET int
#define SOCK_BUF_TYPE size_t
#define FORCE_FORWARD_SLASH(path)
#define FORCE_BACKWARD_SLASH(path)
#define REMOVE_TRAILING_SLASHES(path, len)

#endif /* OPEN_WEB_PLATFORM_UNIX_H */
