#ifndef NEW_TH_TEST_SOCKET_BUFFER_H
#define NEW_TH_TEST_SOCKET_BUFFER_H

#include "common.h"
#include "../src/server/sockbufr.h"
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

static void SocketTextWrite(void **state) {
    const char *text = "Hello Socket Buffer Text";
    const size_t textLen = strlen(text);
    SocketBuffer socketBuffer = socketBufferNew(0, 0);
    mockOptions = MOCK_SEND, mockSendMaxBuf = BUFSIZ, mockSendStream = fopen("/tmp/nt_SocketTextWrite_Send.txt", "wb");
    assert_int_equal(textLen, socketBufferWriteText(&socketBuffer, text));
    assert_int_equal(textLen, platformFileTell(mockSendStream));
    mockReset();
}

static void SocketDataWrite(void **state) {
    const char *data = "Hello Socket Buffer Data";
    const size_t dataLen = strlen(data);
    SocketBuffer socketBuffer = socketBufferNew(0, 0);
    mockOptions = MOCK_SEND, mockSendMaxBuf = BUFSIZ, mockSendStream = fopen("/tmp/nt_SocketDataWrite_Send.txt", "wb");
    assert_int_equal(dataLen, socketBufferWriteData(&socketBuffer, data, dataLen));
    assert_int_equal(dataLen, platformFileTell(mockSendStream));
    mockReset();
}

static void SocketFlush(void **state) {
    const char *text = "Hello Socket Buffer Flush";
    const size_t textLen = strlen(text), bufSize = 5;
    SocketBuffer socketBuffer = socketBufferNew(0, 0);
    mockOptions = MOCK_SEND, mockSendMaxBuf = bufSize, mockSendStream = fopen("/tmp/nt_SocketFlush_Send.txt", "wb");

#pragma region Write some text that will overflow
    assert_int_equal(textLen, socketBufferWriteText(&socketBuffer, text));
    fflush(mockSendStream);
    assert_int_equal(bufSize, platformFileTell(mockSendStream)); /* Break here for manual verification */
#pragma endregion

#pragma region Flush where no data is sent
    mockSendMaxBuf = 0;
    assert_int_equal(0, socketBufferFlush(&socketBuffer));
    fflush(mockSendStream);
    assert_int_equal(bufSize, platformFileTell(mockSendStream)); /* Break here for manual verification */
#pragma endregion

#pragma region Flush where some data is sent
    mockSendMaxBuf = bufSize;
    assert_int_equal(bufSize, socketBufferFlush(&socketBuffer));
    fflush(mockSendStream);
    assert_int_equal(bufSize * 2, platformFileTell(mockSendStream)); /* Break here for manual verification */
#pragma endregion

#pragma region Another flush where no data is sent
    mockSendMaxBuf = 0;
    assert_int_equal(0, socketBufferFlush(&socketBuffer));
    fflush(mockSendStream);
    assert_int_equal(bufSize * 2, platformFileTell(mockSendStream)); /* Break here for manual verification */
#pragma endregion

#pragma region Another flush where some data is sent
    mockSendMaxBuf = bufSize;
    assert_int_equal(bufSize, socketBufferFlush(&socketBuffer));
    fflush(mockSendStream);
    assert_int_equal(bufSize * 3, platformFileTell(mockSendStream)); /* Break here for manual verification */
#pragma endregion

#pragma region Flush where all remaining data is sent
    mockSendMaxBuf = BUFSIZ;
    assert_int_equal(textLen - (bufSize * 3), socketBufferFlush(&socketBuffer));
    assert_int_equal(textLen, platformFileTell(mockSendStream));
    fflush(mockSendStream);
    assert_false(socketBuffer.buffer); /* Break here for manual verification */
#pragma endregion

    mockReset();
}

const struct CMUnitTest socketTest[] = {cmocka_unit_test(SocketDataWrite), cmocka_unit_test(SocketTextWrite),
                                        cmocka_unit_test(SocketFlush)};


#endif /* NEW_TH_TEST_SOCKET_BUFFER_H */
