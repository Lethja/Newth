#include <errno.h>
#include "mockput.h"

#pragma region Mockup global variables

int mockOptions = 0, mockConnectError = 0, mockReceiveError = 0, mockSendError = 0, mockErrorReset = 0;
size_t mockSendMaxBuf = 0, mockSendCountBuf = 0, mockReceiveMaxBuf = 0, mockReceiveCountBuf = 0;
FILE *mockSendStream = NULL, *mockLastFileClosed = NULL, *mockReceiveStream = NULL;
void *mockLastFree = NULL;

#pragma endregion

void mockReset(void) {
    mockConnectError = mockReceiveError = mockSendError = mockErrorReset = mockOptions = 0;
    mockReceiveCountBuf = mockReceiveMaxBuf = mockSendCountBuf = mockSendMaxBuf = mockOptions;
    mockLastFree = mockLastFileClosed = NULL;

    if (mockReceiveStream)
        fflush(mockReceiveStream), fclose(mockReceiveStream), mockReceiveStream = NULL;

    if (mockSendStream)
        fflush(mockSendStream), fclose(mockSendStream), mockSendStream = NULL;
}

void mockResetError(void) {
    errno = ENOERR;
    mockErrorReset = 0;
}

#pragma region System function mockup

#pragma clang diagnostic push
#pragma ide diagnostic ignored "bugprone-reserved-identifier"
#pragma ide diagnostic ignored "OCUnusedGlobalDeclarationInspection"

extern int __real_connect(int sockfd, const struct sockaddr *addr, __socklen_t addrlen);

extern void *__real_calloc(size_t nmemb, size_t size);

extern void *__real_malloc(size_t size);

extern void *__real_realloc(void *ptr, size_t size);

extern ssize_t *__real_recv(int fd, void *buf[], size_t len, int flags);

extern ssize_t *__real_send(int fd, const void *buf, size_t n, int flags);

extern void __real_free(void *ptr);

extern int __real_fclose(FILE *ptr);

int __wrap_connect(int sockfd, const struct sockaddr *addr, socklen_t addrlen) {
    if (mockOptions & MOCK_CONNECT) {
        if (mockConnectError) {
            errno = mockConnectError;

            if (mockErrorReset)
                if (!--mockErrorReset)
                    mockConnectError = ENOERR;
            return -1;
        }
        return 4;
    }

    return __real_connect(sockfd, addr, addrlen);
}

void *__wrap_calloc(size_t nmemb, size_t size) {
    if (mockOptions & MOCK_ALLOC_NO_MEMORY) {
        errno = ENOMEM;
        return 0;
    }

    return __real_calloc(nmemb, size);
}

void *__wrap_malloc(size_t size) {
    if (mockOptions & MOCK_ALLOC_NO_MEMORY) {
        errno = ENOMEM;
        return 0;
    }

    return __real_malloc(size);
}

void *__wrap_realloc(void *ptr, size_t size) {
    if (mockOptions & MOCK_ALLOC_NO_MEMORY) {
        errno = ENOMEM;
        return 0;
    }

    return __real_realloc(ptr, size);
}

void __wrap_free(void *ptr) {
    mockLastFree = ptr, __real_free(ptr);
}

int __wrap_fclose(FILE *file) {
    mockLastFileClosed = file;
    return __real_fclose(file);
}

ssize_t __wrap_recv(int fd, void *buf, size_t len, int flags) {
    if (mockOptions & MOCK_RECEIVE) {
        size_t r;

        if (mockReceiveError) {
            errno = mockReceiveError;

            if(mockErrorReset)
                if(!--mockErrorReset)
                    mockReceiveError = ENOERR;

            return -1;
        }

        if (!mockReceiveMaxBuf) {
            errno = EAGAIN;
            return -1;
        }

        if (mockOptions & MOCK_RECEIVE_COUNT) {
            if (mockReceiveCountBuf > mockReceiveMaxBuf) {
                errno = EAGAIN;
                return -1;
            }

            r = len < mockReceiveMaxBuf - mockReceiveCountBuf ? len : mockReceiveMaxBuf - mockReceiveCountBuf;
            mockReceiveCountBuf += r;
        } else
            r = len < mockReceiveMaxBuf ? len : mockReceiveMaxBuf;

        if (mockReceiveStream) {
            size_t s = fread(buf, sizeof(char), r, mockReceiveStream);
            if (flags & MSG_PEEK) {
                fseek(mockReceiveStream, (long) -s, SEEK_CUR);
                mockReceiveCountBuf -= s;
            }

            return (ssize_t) s;
        } else {
            char j, *b = (char *) buf;
            size_t i;

            for (i = 0, j = '1'; i < r; ++i, ++j)
                j = (char) (j > '9' ? '0' : j), b[i] = j;
        }

        return (ssize_t) r;
    }

    return (ssize_t) __real_recv(fd, buf, len, flags);
}

ssize_t __wrap_send(int fd, const void *buf, size_t n, int flags) {
    if (mockOptions & MOCK_SEND) {
        size_t r;

        if (mockSendError) {
            errno = mockSendError;

            if(mockErrorReset)
                if(!--mockErrorReset)
                    mockSendError = ENOERR;

            return -1;
        }

        if (!mockSendMaxBuf) {
            errno = EAGAIN;
            return -1;
        }

        if (mockOptions & MOCK_SEND_COUNT) {
            if (mockSendCountBuf >= mockSendMaxBuf) {
                errno = EAGAIN;
                return -1;
            }

            r = n < mockSendMaxBuf - mockSendCountBuf ? n : mockSendMaxBuf - mockSendCountBuf;
            mockSendCountBuf += mockSendMaxBuf;
        } else
            r = n < mockSendMaxBuf ? n : mockSendMaxBuf;

        if (mockSendStream)
            return (ssize_t) fwrite(buf, 1, r, mockSendStream);

        return (ssize_t) r;
    }

    return (ssize_t) __real_send(fd, buf, n, flags);
}

#pragma clang diagnostic pop

#pragma endregion
