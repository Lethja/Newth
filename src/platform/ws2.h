/* https://learn.microsoft.com/en-us/windows/win32/winsock/complete-server-code */

#undef UNICODE

#define WIN32_LEAN_AND_MEAN
#define realpath(N, R) _fullpath((R),(N),_MAX_PATH)
#define FD_SETSIZE 1024
#define SOCK_BUF_TYPE int
#define FORCE_FORWARD_SLASH(path) platformPathForceForwardSlash(path)

#include <windows.h>
#include <winsock2.h>
#include <ws2tcpip.h>
#include <iphlpapi.h>

#include "platform.h"

void platformPathForceForwardSlash(char *path);
