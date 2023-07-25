#include <errno.h>
#include "mockput.h"
#include "platform.h"

#pragma region Mockup global variables

int mockOptions = 0;
size_t mockSendMaxBuf = 0;
FILE *mockSendStream = NULL;

#pragma endregion

void mockReset(void) {
    mockSendMaxBuf = mockOptions = 0;

    if (mockSendStream)
        fclose(mockSendStream), mockSendStream = NULL;
}

void mockResetError(void) {
    errno = ENOERR;
}

#pragma region System function mockup

#pragma clang diagnostic push
#pragma ide diagnostic ignored "bugprone-reserved-identifier"
#pragma ide diagnostic ignored "OCUnusedGlobalDeclarationInspection"

extern void *__real_calloc(size_t nmemb, size_t size);

extern void *__real_malloc(size_t size);

extern void *__real_realloc(void *ptr, size_t size);

extern ssize_t *__real_send(int fd, const void *buf, size_t n, int flags);

void *__wrap_calloc(size_t nmemb, size_t size) {
    if (mockOptions & MOCK_ALLOC_NO_MEMORY)
        return 0;

    return __real_calloc(nmemb, size);
}

void *__wrap_malloc(size_t size) {
    if (mockOptions & MOCK_ALLOC_NO_MEMORY)
        return 0;

    return __real_malloc(size);
}

void *__wrap_realloc(void *ptr, size_t size) {
    if (mockOptions & MOCK_ALLOC_NO_MEMORY)
        return 0;

    return __real_realloc(ptr, size);
}

ssize_t __wrap_send(int fd, const void *buf, size_t n, int flags) {
    if (mockOptions & MOCK_SEND) {
        size_t r;

        if (!mockSendMaxBuf) {
            errno = EAGAIN;
            return -1;
        }

        r = n < mockSendMaxBuf ? n : mockSendMaxBuf;

        if (mockSendStream)
            return (ssize_t) fwrite(buf, r, 1, mockSendStream);

        return (ssize_t) r;
    }

    return (ssize_t) __real_send(fd, buf, n, flags);
}

#pragma clang diagnostic pop

#pragma endregion

int platformSocketGetLastError(void) {
    return errno;
}
