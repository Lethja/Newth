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

#define ENOERR 0
#define EAGAIN 1
#define EWOULDBLOCK 2

#define SOCKET_TRY_AGAIN EAGAIN
#define SOCKET_WOULD_BLOCK EWOULDBLOCK

typedef struct tm PlatformTimeStruct;
typedef struct dirent PlatformDirEntry;
typedef struct stat PlatformFileStat;
typedef off_t PlatformFileOffset;
typedef FILE *PlatformFile;

struct sockaddr {
    sa_family_t (sa_);
    char sa_data[14];
};

extern int mockError;
extern size_t mockCurBuf, mockMaxBuf;

void mockReset(void);

ssize_t send(int fd, const void *buf, size_t n, int flags);

int platformSocketGetLastError(void);

#endif /* NEWTH_MOCK_POSIX_UNIT_TEST_H */
