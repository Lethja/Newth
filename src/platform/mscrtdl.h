#ifndef OPEN_WEB_PLATFORM_WIN_SOCK_2_H
#define OPEN_WEB_PLATFORM_WIN_SOCK_2_H

#undef UNICODE

#define CLOSE_SOCKET(x) closesocket(x)
#define SOCK_BUF_TYPE int
#define sa_family_t short

#include <winsock2.h>
#include <ws2tcpip.h>
#include <windows.h>

typedef SYSTEMTIME PlatformTimeStruct;
typedef WIN32_FIND_DATA PlatformDirEntry;
#ifndef MSVC89

typedef long long PlatformFileOffset;

#else

typedef DWORD PlatformFileOffset;
typedef int ssize_t;

#define sockaddr_storage sockaddr

#endif

#define in_addr_t long

typedef struct winSockNativeStat {
    unsigned long st_size;
    unsigned int st_mode;
    FILETIME st_mtime;
} PlatformFileStat;

/* TODO: use platform neutral typedef in case off_t is not declared */
/*
#ifndef	_OFF_T_
#define	_OFF_T_
typedef long _off_t;
#ifndef	_NO_OLDNAMES
typedef _off_t	off_t;
#endif
#endif
*/

typedef struct winSockNativeDir {
    PlatformDirEntry nextEntry;
    PlatformDirEntry lastEntry;
    HANDLE directoryHandle;
    int error;
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

#ifndef IPPROTO_IPV6
#define IPPROTO_IPV6 41
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

void platformRemoveTrailingSlashes(char *path, size_t len);

void ipv6NTop(const void *inAddr6, char *ipStr);

#endif /* OPEN_WEB_PLATFORM_WIN_SOCK_2_H */
