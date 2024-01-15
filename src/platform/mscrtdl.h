#ifndef OPEN_WEB_PLATFORM_WIN32_H
#define OPEN_WEB_PLATFORM_WIN32_H

#undef UNICODE

#define CLOSE_SOCKET(x) closesocket(x)
#define SOCK_BUF_TYPE int
#define sa_family_t short
#define PF_OFFSET "I64d"
#define HEX_OFFSET "lx"
#define SOCKET_WOULD_BLOCK WSAEWOULDBLOCK
#define snprintf _snprintf

#ifndef SB_DATA_SIZE
#define SB_DATA_SIZE 8192
#endif

#include <ctype.h>
#include <stdio.h>
#include <winsock2.h>
#include <ws2tcpip.h>
#include <windows.h>

typedef SYSTEMTIME PlatformTimeStruct;
typedef WIN32_FIND_DATA PlatformDirEntry;
typedef __int64 PlatformFileOffset;
typedef HANDLE PlatformFile;

#define PLATFORM_SELECT_MAX_TIMEOUT 60

#ifdef MSVC89

#pragma warning(disable:4068)

typedef int ssize_t;

#define INVALID_FILE_ATTRIBUTES ((DWORD)-1)

typedef unsigned long ULONG_PTR,*PULONG_PTR;

typedef int socklen_t;

#pragma region sockaddr_storage

#define _SS_MAXSIZE 128
#define _SS_ALIGNSIZE (8)

#define _SS_PAD1SIZE (_SS_ALIGNSIZE - sizeof (short))
#define _SS_PAD2SIZE (_SS_MAXSIZE - (sizeof (short) + _SS_PAD1SIZE + _SS_ALIGNSIZE))

struct sockaddr_storage {
    short ss_family;
    char __ss_pad1[_SS_PAD1SIZE];

    __int64 __ss_align;
    char __ss_pad2[_SS_PAD2SIZE];
};

#pragma endregion

#pragma region sockaddr_in6 (NT6+ support from older compilers)

#define SOCKIN6 sockaddr_in6_nt6

typedef struct _SCOPE_ID {
    union {
            struct {
                unsigned long Zone : 28;
                unsigned long Level : 4;
            };
        unsigned long Value;
    };
} SCOPE_ID, *PSCOPE_ID;

typedef struct in6_addr {
    union {
        unsigned char Byte[16];
        unsigned short Word[8];
    } u;
} IN6_ADDR_NT6, *PIN6_ADDR_NT6, *LPIN6_ADDR_NT6;

typedef struct sockaddr_in6_nt6 {
    short sin6_family;
    unsigned short sin6_port;
    unsigned long sin6_flowinfo;
    struct in6_addr sin6_addr;
    union {
        unsigned long sin6_scope_id;
        SCOPE_ID sin6_scope_struct;
    };
} sockaddr_in6_nt6;

#pragma endregion

#else

#define SOCKIN6 sockaddr_in6

#endif /* MSVC89 */

#define in_addr_t unsigned long

typedef struct winSockNativeStat {
    PlatformFileOffset st_size;
    unsigned int st_mode;
    FILETIME st_mtime;
} PlatformFileStat;

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

#ifndef INVALID_SET_FILE_POINTER
#define INVALID_SET_FILE_POINTER INVALID_FILE_ATTRIBUTES
#endif

#include <signal.h>

#include "platform.h"

#ifndef SOCKET_TRY_AGAIN
#define SOCKET_TRY_AGAIN SOCKET_WOULD_BLOCK
#endif

void ipv6NTop(const void *inAddr6, char *ipStr);

#endif /* OPEN_WEB_PLATFORM_WIN32_H */
