#ifndef NEWTH_MOCK_POSIX_UNIT_TEST_H
#define NEWTH_MOCK_POSIX_UNIT_TEST_H

#pragma clang diagnostic push
#pragma ide diagnostic ignored "bugprone-reserved-identifier"

#include <stdio.h>

void *__wrap_calloc(size_t nmemb, size_t size);

void *__wrap_malloc(size_t size);

void *__wrap_realloc(void *ptr, size_t size);

ssize_t __wrap_send(int fd, const void *buf, size_t n, int flags);

#pragma clang diagnostic pop

#define SOCKET_WOULD_BLOCK 41

#include "posix01.h"

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
    MOCK_ALLOC_NO_MEMORY = 1, MOCK_SEND = 2, MOCK_SEND_COUNT = 4
};

extern int mockOptions, mockSendError;
extern size_t mockSendMaxBuf;
extern FILE *mockSendStream;

void mockDumpFile(FILE *file);

void mockReset(void);

void mockResetError(void);

int platformSocketGetLastError(void);

#endif /* NEWTH_MOCK_POSIX_UNIT_TEST_H */
