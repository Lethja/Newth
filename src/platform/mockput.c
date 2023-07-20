#include "mockput.h"
#include "platform.h"

#pragma region Mockup global variables

int mockError = 0;
size_t mockCurBuf = 0, mockMaxBuf = BUFSIZ / 2;

#pragma endregion

#pragma region System function mockup

/**
 * Mockup of the POSIX version of send, this mockup dosen't actually use a socket but instead can be programmed to
 * respond in certain ways that mimic a system
 * @param fd file descripter, unused in mockup, pass 0
 * @param buf The buffer, untouched can pass NULL
 * @param n The size of the buffer
 * @param flags Unused in mockup pass 0
 * @return return error state and code depending on mockup variable conditions
 */
ssize_t send(int fd, const void *buf, size_t n, int flags) {
    if (mockError) {
        return -1;
    }

    if (mockCurBuf >= mockMaxBuf) {
        mockError = EAGAIN;
        return -1;
    }

    if (mockCurBuf + n < mockMaxBuf) {
        mockCurBuf += n;
        return (ssize_t) n;
    } else {
        ssize_t r = (ssize_t) (mockMaxBuf - mockCurBuf);
        mockCurBuf = mockMaxBuf;
        return r;
    }
}

#pragma endregion

int platformSocketGetLastError(void) {
    return mockError;
}
