#ifndef NEWTH_MOCK_POSIX_UNIT_TEST_H
#define NEWTH_MOCK_POSIX_UNIT_TEST_H

#pragma clang diagnostic push
#pragma ide diagnostic ignored "bugprone-reserved-identifier"

#ifdef _WIN32
#include "mscrtdl.h"
#else
#define SOCKET_WOULD_BLOCK 41

#include "posix01.h"

#endif

void *__wrap_calloc(size_t nmemb, size_t size);

void *__wrap_malloc(size_t size);

void *__wrap_realloc(void *ptr, size_t size);

int __wrap_connect(int sockfd, const struct sockaddr *addr, __socklen_t addrlen);

ssize_t __wrap_recv(int fd, void *buf, size_t len, int flags);

ssize_t __wrap_send(int fd, const void *buf, size_t n, int flags);

#pragma clang diagnostic pop

#ifndef INET_ADDRSTRLEN
#define INET_ADDRSTRLEN 22
#endif

#ifndef INET6_ADDRSTRLEN
#define INET6_ADDRSTRLEN 65
#endif

#define    ENOERR      0   /* No error */
#define    EAGAIN      11  /* Try again */
#define    ENOMEM      12  /* Out of memory */

enum mockOptions {
    MOCK_ALLOC_NO_MEMORY = 1, MOCK_CONNECT = 2, MOCK_RECEIVE = 4, MOCK_SEND = 8, MOCK_RECEIVE_COUNT = 16, MOCK_SEND_COUNT = 32
};

extern int mockOptions, mockConnectError, mockSendError, mockErrorReset, mockReceiveError;
extern size_t mockReceiveMaxBuf, mockSendMaxBuf;
extern FILE *mockSendStream, *mockLastFileClosed, *mockReceiveStream;
extern void *mockLastFree;

void mockDumpFile(FILE *file);

void mockReset(void);

void mockResetError(void);

#endif /* NEWTH_MOCK_POSIX_UNIT_TEST_H */
