#ifndef NEW_TH_WATT32_H
#define NEW_TH_WATT32_H

#include "platform.h"

#pragma region DOS/WatcomC Specific headers

#include <dos.h>
#include <direct.h>

#pragma endregion

#pragma region ANSI C headers

#include <ctype.h>
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
#include <tcp.h>

#pragma endregion

typedef struct tm PlatformTimeStruct;
typedef struct dirent PlatformDirEntry;
typedef struct stat PlatformFileStat;
typedef off_t PlatformFileOffset;
typedef FILE *PlatformFile;

#define SOCKIN6 sockaddr_in6
#define CLOSE_SOCKET(x) closesocket(x)
#define SOCKET int
#define SOCK_BUF_TYPE size_t
#define STD_FILE_TYPE size_t
#define PF_OFFSET "ji"

#ifndef SB_DATA_SIZE
#define SB_DATA_SIZE BUFSIZ
#endif

#ifndef SOCKET_TRY_AGAIN
#define SOCKET_TRY_AGAIN EAGAIN
#endif

#ifndef SOCKET_WOULD_BLOCK
#define SOCKET_WOULD_BLOCK EWOULDBLOCK
#endif

#endif /* NEW_TH_WATT32_H */
