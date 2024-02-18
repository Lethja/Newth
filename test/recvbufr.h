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

#ifdef MOCK

static void ReceiveFetch(void **state) {
    RecvBuffer socketBuffer;
    char buf[11] = {0};

    mockReset(), mockOptions = MOCK_CONNECT | MOCK_RECEIVE, mockSendMaxBuf = mockReceiveMaxBuf = 1024;

    socketBuffer = recvBufferNew(0, 0);
    assert_null(recvBufferAppend(&socketBuffer, 10));

    assert_null(recvBufferFetch(&socketBuffer, buf, 0, 11));
    assert_string_equal("1234567890", buf);

    assert_null(recvBufferFetch(&socketBuffer, buf, 1, 5));
    assert_string_equal("2345", buf);

    assert_null(recvBufferFetch(&socketBuffer, buf, 5, 4));
    assert_string_equal("678", buf);

    recvBufferFailFree(&socketBuffer);
}

static void ReceiveDitch(void **state) {
    RecvBuffer socketBuffer;
    char buf[11] = {0};

    mockReset(), mockOptions = MOCK_CONNECT | MOCK_RECEIVE, mockSendMaxBuf = mockReceiveMaxBuf = 1024;

    socketBuffer = recvBufferNew(0, 0);
    assert_null(recvBufferAppend(&socketBuffer, 10));

    recvBufferDitch(&socketBuffer, 5); /* Ditch the first 5 bytes */
    assert_null(recvBufferFetch(&socketBuffer, buf, 0, 11));
    assert_string_equal("67890", buf);

    recvBufferDitch(&socketBuffer, 1); /* Ditch the next byte from the above result */
    assert_null(recvBufferFetch(&socketBuffer, buf, 0, 11));
    assert_string_equal("7890", buf);

    recvBufferDitch(&socketBuffer, 0); /* Don't ditch if zero */
    assert_null(recvBufferFetch(&socketBuffer, buf, 0, 11));
    assert_string_equal("7890", buf);

    recvBufferFailFree(&socketBuffer);
}

static void ReceiveFind(void **state) {
    const char *data = "The quick brown fox jumps over the lazy dog";
    size_t len = strlen(data);

    RecvBuffer socketBuffer;
    PlatformFileOffset pos;

    mockReset(), mockOptions = MOCK_CONNECT | MOCK_RECEIVE, mockReceiveMaxBuf = len;
    mockReceiveStream = tmpfile(), fwrite(data, 1, len, mockReceiveStream), rewind(mockReceiveStream);

    socketBuffer = recvBufferNew(0, 0);
    recvBufferAppend(&socketBuffer, len);

    assert_int_equal(recvBufferFind(&socketBuffer, 0, "The", 3), 0);
    assert_int_equal(recvBufferFind(&socketBuffer, 0, "he", 2), 1);
    assert_int_equal((pos = recvBufferFind(&socketBuffer, 0, "fox", 3)), 16);
    assert_int_equal((pos = recvBufferFind(&socketBuffer, pos, "ox", 2)), 17);
    assert_int_equal(recvBufferFind(&socketBuffer, pos, "dog", 3), 40);

    recvBufferFailFree(&socketBuffer);
}

static void ReceiveSearchFor(void **state) {
    const char *data = "The quick brown fox jumps over the lazy dog";
    size_t len = strlen(data);

    RecvBuffer socketBuffer;
    mockReset(), mockOptions = MOCK_CONNECT | MOCK_RECEIVE, mockReceiveMaxBuf = len;
    mockReceiveStream = tmpfile(), fwrite(data, 1, len, mockReceiveStream), rewind(mockReceiveStream);

    socketBuffer = recvBufferNew(0, 0);

    assert_null(recvBufferSearchFor(&socketBuffer, "The", 3));
    assert_null(recvBufferSearchFor(&socketBuffer, "he", 2));
    assert_null(recvBufferSearchFor(&socketBuffer, "fox", 3));
    assert_null(recvBufferSearchFor(&socketBuffer, "ox", 2));
    assert_null(recvBufferSearchFor(&socketBuffer, "dog", 3));
    assert_non_null(recvBufferSearchFor(&socketBuffer, "fox", 3)); /* Buffer eaten, fox is gone! */

    recvBufferFailFree(&socketBuffer);
}

static void ReceiveSearchTo(void **state) {
    const char *data = "The quick brown fox jumps over the lazy dog";
    size_t len = strlen(data);

    RecvBuffer socketBuffer;
    mockReset(), mockOptions = MOCK_CONNECT | MOCK_RECEIVE, mockReceiveMaxBuf = len;
    mockReceiveStream = tmpfile(), fwrite(data, 1, len, mockReceiveStream), rewind(mockReceiveStream);

    socketBuffer = recvBufferNew(0, 0);

    assert_null(recvBufferSearchTo(&socketBuffer, "The", 3, len));
    assert_null(recvBufferSearchTo(&socketBuffer, "he", 2, len));
    assert_null(recvBufferSearchTo(&socketBuffer, "fox", 3, len));
    assert_null(recvBufferSearchTo(&socketBuffer, "ox", 2, len));
    assert_null(recvBufferSearchTo(&socketBuffer, "dog", 3, len));
    assert_null(recvBufferSearchTo(&socketBuffer, "fox", 3, len)); /* Buffer expanded, fox is still here */

    recvBufferFailFree(&socketBuffer);
}

#endif /* MOCK */

#pragma clang diagnostic pop

const struct CMUnitTest recvBufferSocketTest[] = {cmocka_unit_test(RecvBufferClear),
                                                  cmocka_unit_test(RecvBufferMemoryFree)
#ifdef MOCK
        , cmocka_unit_test(ReceiveFetch), cmocka_unit_test(ReceiveFind), cmocka_unit_test(ReceiveDitch),
                                                  cmocka_unit_test(ReceiveSearchFor), cmocka_unit_test(ReceiveSearchTo)
#endif
};

#endif /* NEW_DL_TEST_SOCKET_BUFFER_H */
