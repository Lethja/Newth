#ifndef OPEN_WEB_PLATFORM_WIN_SOCK_2_H
#define OPEN_WEB_PLATFORM_WIN_SOCK_2_H

#undef UNICODE

#define WIN32_LEAN_AND_MEAN
#define CLOSE_SOCKET(x) closesocket(x)
#define realpath(N, R) _fullpath((R),(N),MAX_PATH)
#define FD_SETSIZE 1024
#define SOCK_BUF_TYPE int
#define FORCE_FORWARD_SLASH(path) platformPathForceForwardSlash(path)
#define FORCE_BACKWARD_SLASH(path) platformPathForceBackwardSlash(path)

#include <windows.h>
#include <winsock2.h>
#include <ws2tcpip.h>
#include <iphlpapi.h>

#include "platform.h"

void platformPathForceForwardSlash(char *path);

void platformPathForceBackwardSlash(char *path);

#endif /* OPEN_WEB_PLATFORM_WIN_SOCK_2_H */
