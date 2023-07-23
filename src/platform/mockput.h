#ifndef NEWTH_MOCK_POSIX_UNIT_TEST_H
#define NEWTH_MOCK_POSIX_UNIT_TEST_H

#include <stdlib.h>
#include <stdio.h>
#include <sys/select.h>

#define SOCK_BUF_TYPE long int

#ifndef INET_ADDRSTRLEN
#define INET_ADDRSTRLEN 22
#endif

#ifndef INET6_ADDRSTRLEN
#define INET6_ADDRSTRLEN 65
#endif

typedef int sa_family_t;
typedef int SOCKET;

#define    ENOERR      0   /* No error */
#define    EAGAIN      11  /* Try again */
#define    ENOMEM      12  /* Out of memory */
#define    EWOULDBLOCK 41  /* Operation would block */

#define SOCKET_TRY_AGAIN EAGAIN
#define SOCKET_WOULD_BLOCK EWOULDBLOCK

#define calloc(x, y) mockError == ENOMEM ? NULL : calloc(x)
#define free(x) free(x)
#define malloc(x) mockError == ENOMEM ? NULL : malloc(x)
#define realloc(x, y) mockError == ENOMEM ? NULL : realloc(x, y)

typedef struct tm PlatformTimeStruct;
typedef struct dirent PlatformDirEntry;
typedef struct stat PlatformFileStat;
typedef off_t PlatformFileOffset;
typedef FILE *PlatformFile;

#pragma clang diagnostic push
#pragma ide diagnostic ignored "OCUnusedGlobalDeclarationInspection"
struct sockaddr {
    sa_family_t (sa_);
    char sa_data[14];
};
#pragma clang diagnostic pop

extern int mockError;
extern size_t mockCurBuf, mockMaxBuf;

void mockReset(void);

ssize_t send(int fd, const void *buf, size_t n, int flags);

int platformSocketGetLastError(void);

#endif /* NEWTH_MOCK_POSIX_UNIT_TEST_H */
