#ifndef OPEN_WEB_PLATFORM_WIN_SOCK_2_H
#define OPEN_WEB_PLATFORM_WIN_SOCK_2_H

#undef UNICODE

#define WIN32_LEAN_AND_MEAN
#define CLOSE_SOCKET(x) closesocket(x)
#define SOCK_BUF_TYPE int
#define FORCE_FORWARD_SLASH(path) platformPathForceForwardSlash(path)
#define FORCE_BACKWARD_SLASH(path) platformPathForceBackwardSlash(path)
#define sa_family_t short

#include <winsock2.h>
#include <windows.h>

typedef FILETIME PlatformTimeStruct;

#include <signal.h>

#include <windows.h>
#include <winsock2.h>
#include <ws2tcpip.h>
#include <iphlpapi.h>

#include "platform.h"

#ifndef IPV6_V6ONLY
#define IPV6_V6ONLY 27
#endif

#ifndef GAA_FLAG_INCLUDE_PREFIX
#define GAA_FLAG_INCLUDE_PREFIX 0x0010
#endif

void platformPathForceForwardSlash(char *path);

void platformPathForceBackwardSlash(char *path);

void ipv6NTop(const void *inAddr6, char *ipStr);

#endif /* OPEN_WEB_PLATFORM_WIN_SOCK_2_H */
