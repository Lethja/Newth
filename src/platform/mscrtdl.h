#ifndef OPEN_WEB_PLATFORM_WIN_SOCK_2_H
#define OPEN_WEB_PLATFORM_WIN_SOCK_2_H

#undef UNICODE

#define CLOSE_SOCKET(x) closesocket(x)
#define SOCK_BUF_TYPE int
#define FORCE_FORWARD_SLASH(path) platformPathForceForwardSlash(path)
#define FORCE_BACKWARD_SLASH(path) platformPathForceBackwardSlash(path)
#define sa_family_t short

#include <winsock2.h>
#include <ws2tcpip.h>
#include <iphlpapi.h>
#include <windows.h>

typedef SYSTEMTIME PlatformTimeStruct;
typedef WIN32_FIND_DATA PlatformDirEntry;
typedef struct winSockNativeStat {
    unsigned long st_size;
    unsigned int st_mode;
    FILETIME st_mtime;
} PlatformFileStat;

typedef struct winSockNativeDir {
    PlatformDirEntry nextEntry;
    PlatformDirEntry lastEntry;
    HANDLE directoryHandle;
} DIR;

#ifndef AF_INET6
#define AF_INET6 23
#endif

#ifndef INET_ADDRSTRLEN
#define INET_ADDRSTRLEN 22
#endif

#ifndef INET6_ADDRSTRLEN
#define INET6_ADDRSTRLEN 65
#endif

#ifndef IPV6_V6ONLY
#define IPV6_V6ONLY 27
#endif

#ifndef GAA_FLAG_INCLUDE_PREFIX
#define GAA_FLAG_INCLUDE_PREFIX 0x0010
#endif

#include <signal.h>

#include "platform.h"

void platformPathForceForwardSlash(char *path);

void platformPathForceBackwardSlash(char *path);

void ipv6NTop(const void *inAddr6, char *ipStr);

#endif /* OPEN_WEB_PLATFORM_WIN_SOCK_2_H */
