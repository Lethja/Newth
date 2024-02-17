#ifndef NEW_TH_TEST_SOCKET_BUFFER_H
#define NEW_TH_TEST_SOCKET_BUFFER_H

#include "../src/server/sendbufr.h"
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

static void SendBufferMemoryFree(void **state) {
    SendBuffer socketBuffer = sendBufferNew(0, 0);

#ifdef MOCK
    mockReset(),
#endif

    socketBuffer.buffer = platformMemoryStreamNew();
    assert_non_null(&socketBuffer.buffer);
    sendBufferFailFree(&socketBuffer);

#ifdef MOCK
    assert_ptr_equal(socketBuffer.buffer, mockLastFileClosed);
#endif
}

#ifdef MOCK

static void SendBufferTextWrite(void **state) {
    const char *text = "Hello Socket Buffer Text";
    const size_t textLen = strlen(text);
    SendBuffer socketBuffer = sendBufferNew(0, 0);
    mockOptions = MOCK_SEND, mockSendMaxBuf = BUFSIZ, mockSendStream = fopen("/tmp/nt_SocketTextWrite_Send.txt", "wb");
    assert_int_equal(textLen, sendBufferWriteText(&socketBuffer, text));
    assert_int_equal(textLen, platformFileTell(mockSendStream));
    mockReset();
}

static void SendBufferDataWrite(void **state) {
    const char *data = "Hello Socket Buffer Data";
    const size_t dataLen = strlen(data);
    SendBuffer socketBuffer = sendBufferNew(0, 0);
    mockOptions = MOCK_SEND, mockSendMaxBuf = BUFSIZ, mockSendStream = fopen("/tmp/nt_SocketDataWrite_Send.txt", "wb");
    assert_int_equal(dataLen, sendBufferWriteData(&socketBuffer, data, dataLen));
    assert_int_equal(dataLen, platformFileTell(mockSendStream));
    mockReset();
}

static void SendBufferSocketFlush(void **state) {
    const char *text = "Hello Socket Buffer Flush";
    const size_t textLen = strlen(text), bufSize = 5;
    SendBuffer socketBuffer = sendBufferNew(0, 0);
    mockOptions = MOCK_SEND, mockSendMaxBuf = bufSize, mockSendStream = fopen("/tmp/nt_SocketFlush_Send.txt", "wb");

#pragma region Write some text that will overflow
    assert_int_equal(textLen, sendBufferWriteText(&socketBuffer, text));
    fflush(mockSendStream);
    assert_int_equal(bufSize, platformFileTell(mockSendStream)); /* Break here for manual verification */
#pragma endregion

#pragma region Flush where no data is sent
    mockSendMaxBuf = 0;
    assert_int_equal(0, sendBufferFlush(&socketBuffer));
    fflush(mockSendStream);
    assert_int_equal(bufSize, platformFileTell(mockSendStream)); /* Break here for manual verification */
#pragma endregion

#pragma region Flush where some data is sent
    mockSendMaxBuf = bufSize;
    assert_int_equal(bufSize, sendBufferFlush(&socketBuffer));
    fflush(mockSendStream);
    assert_int_equal(bufSize * 2, platformFileTell(mockSendStream)); /* Break here for manual verification */
#pragma endregion

#pragma region Another flush where no data is sent
    mockSendMaxBuf = 0;
    assert_int_equal(0, sendBufferFlush(&socketBuffer));
    fflush(mockSendStream);
    assert_int_equal(bufSize * 2, platformFileTell(mockSendStream)); /* Break here for manual verification */
#pragma endregion

#pragma region Another flush where some data is sent
    mockSendMaxBuf = bufSize;
    assert_int_equal(bufSize, sendBufferFlush(&socketBuffer));
    fflush(mockSendStream);
    assert_int_equal(bufSize * 3, platformFileTell(mockSendStream)); /* Break here for manual verification */
#pragma endregion

#pragma region Flush where all remaining data is sent
    mockSendMaxBuf = BUFSIZ;
    assert_int_equal(textLen - (bufSize * 3), sendBufferFlush(&socketBuffer));
    assert_int_equal(textLen, platformFileTell(mockSendStream));
    fflush(mockSendStream);
    assert_false(socketBuffer.buffer); /* Break here for manual verification */
#pragma endregion

    mockReset();
}

#endif

#pragma clang diagnostic pop

const struct CMUnitTest sendBufferSocketTest[] = {cmocka_unit_test(SendBufferMemoryFree)
#ifdef MOCK
        , cmocka_unit_test(SendBufferDataWrite), cmocka_unit_test(SendBufferTextWrite), cmocka_unit_test(SendBufferSocketFlush)
#endif
};


#endif /* NEW_TH_TEST_SOCKET_BUFFER_H */
