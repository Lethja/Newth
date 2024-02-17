#ifndef NEW_DL_TEST_SOCKET_BUFFER_H
#define NEW_DL_TEST_SOCKET_BUFFER_H

#include "../src/client/recvbufr.h"
#include <errno.h>
#include <string.h>

#pragma clang diagnostic push
#pragma ide diagnostic ignored "UnusedParameter"

#pragma region CMocka headers

#include <stdarg.h>
#include <stddef.h>
#include <stdint.h>
#include <setjmp.h>

#include <cmocka.h>

#pragma endregion

static void RecvBufferMemoryFree(void **state) {
    RecvBuffer socketBuffer = recvBufferNew(0, 0);

#ifdef MOCK
    mockReset();
#endif

    socketBuffer.buffer = platformMemoryStreamNew();
    assert_non_null(&socketBuffer.buffer);
    recvBufferFailFree(&socketBuffer);

#ifdef MOCK
    assert_ptr_equal(socketBuffer.buffer, mockLastFileClosed);
#endif
}

static void RecvBufferClear(void **state) {
    RecvBuffer socketBuffer = recvBufferNew(0, 0);

#ifdef MOCK
    FILE *buffer;
    mockReset();
    buffer =
#endif

    socketBuffer.buffer = platformMemoryStreamNew();
    assert_non_null(&socketBuffer.buffer);
    recvBufferClear(&socketBuffer);

#ifdef MOCK
    assert_ptr_equal(buffer, mockLastFileClosed);
#endif
}

#pragma clang diagnostic pop

const struct CMUnitTest recvBufferSocketTest[] = {cmocka_unit_test(RecvBufferClear),
                                                  cmocka_unit_test(RecvBufferMemoryFree)
#ifdef MOCK
        /* TODO: Mocking tests */
#endif
};

#endif /* NEW_DL_TEST_SOCKET_BUFFER_H */
