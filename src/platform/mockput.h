#ifndef NEWTH_MOCK_POSIX_UNIT_TEST_H
#define NEWTH_MOCK_POSIX_UNIT_TEST_H

#include <stdlib.h>
#include <stdio.h>
#include <sys/select.h>
#include <sys/socket.h>

#define SOCK_BUF_TYPE long int

#ifndef INET_ADDRSTRLEN
#define INET_ADDRSTRLEN 22
#endif

#ifndef INET6_ADDRSTRLEN
#define INET6_ADDRSTRLEN 65
#endif

typedef int SOCKET;

#define    ENOERR      0   /* No error */
#define    EAGAIN      11  /* Try again */
#define    ENOMEM      12  /* Out of memory */

#define SOCKET_TRY_AGAIN EAGAIN
#define SOCKET_WOULD_BLOCK EWOULDBLOCK
#pragma clang diagnostic push
#pragma ide diagnostic ignored "bugprone-reserved-identifier"

void *__wrap_calloc(size_t nmemb, size_t size);

void *__wrap_malloc(size_t size);

void *__wrap_realloc(void *ptr, size_t size);

ssize_t __wrap_send(int fd, const void *buf, size_t n, int flags);

#pragma clang diagnostic pop

typedef struct tm PlatformTimeStruct;
typedef struct dirent PlatformDirEntry;
typedef struct stat PlatformFileStat;
typedef off_t PlatformFileOffset;
typedef FILE *PlatformFile;

enum mockOptions {
    MOCK_ALLOC_NO_MEMORY = 1, MOCK_SEND = 2
};

extern int mockOptions;
extern size_t mockSendMaxBuf;
extern FILE *mockSendStream;

void mockReset(void);

void mockResetError(void);

int platformSocketGetLastError(void);

#endif /* NEWTH_MOCK_POSIX_UNIT_TEST_H */
