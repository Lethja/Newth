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

static void RecvBufferCopyBetween(void **state) {
    FILE *copy;

    RecvBuffer socketBuffer = {0};
    char input[] = "The quick brown fox jumps over the lazy dog";
    char output[sizeof(input)] = {0};
    size_t len = strlen(input);

    assert_non_null(socketBuffer.buffer = tmpfile());
    assert_int_equal(fwrite(input, 1, len, socketBuffer.buffer), len);
    assert_non_null(copy = recvBufferCopyBetween(&socketBuffer, 4, 20));
    assert_int_equal(fread(output, 1, 15, copy), 15);
    assert_memory_equal(&input[4], output, 15);

    fclose(copy), memset(output, 0, 15);
    assert_non_null(copy = recvBufferCopyBetween(&socketBuffer, 35, 45));
    assert_int_equal(fread(output, 1, 9, copy), 8);
    assert_memory_equal(&input[35], output, 8);
    fclose(copy);

}

static void RecvBufferDitchBetween(void **state) {
    RecvBuffer socketBuffer = {0};
    char input[] = "The quick brown fox jumps over the lazy dog";
    char buf[sizeof(input)] = {0};
    size_t len = strlen(input);
    FILE *stream;

    assert_non_null(stream = socketBuffer.buffer = tmpfile());
    assert_int_equal(fwrite(input, 1, len, socketBuffer.buffer), len);
    assert_null(recvBufferDitchBetween(&socketBuffer, 25, 5));
    assert_ptr_not_equal(socketBuffer.buffer, stream);
    assert_null(recvBufferFetch(&socketBuffer, buf, 0, sizeof(input)));
    assert_string_equal(buf, "The quick brown fox jumps the lazy dog");
    assert_null(recvBufferDitchBetween(&socketBuffer, 30, 5));
    assert_null(recvBufferFetch(&socketBuffer, buf, 0, sizeof(input)));
    assert_string_equal(buf, "The quick brown fox jumps the dog");
    assert_null(recvBufferDitchBetween(&socketBuffer, 25, 50));
    assert_null(recvBufferFetch(&socketBuffer, buf, 0, sizeof(input)));
    assert_string_equal(buf, "The quick brown fox jumps");
}

static void RecvBufferMemoryFree(void **state) {
    RecvBuffer socketBuffer = {0};

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
    RecvBuffer socketBuffer = {0};
    char buf[10];

#ifdef MOCK
    FILE *buffer;
    mockReset();
    buffer =
    #endif

    socketBuffer.buffer = platformMemoryStreamNew();
    assert_non_null(&socketBuffer.buffer);
    recvBufferClear(&socketBuffer);
    assert_null(socketBuffer.buffer);
    assert_non_null(recvBufferFetch(&socketBuffer, buf, 0, 10));

#ifdef MOCK
    assert_ptr_equal(buffer, mockLastFileClosed);
#endif
}

static void ReceiveSetLengthChunk(void **state) {
    RecvBuffer socketBuffer;

    memset(&socketBuffer, 0, sizeof(RecvBuffer));
    recvBufferSetLengthChunk(&socketBuffer);
    assert_true(socketBuffer.options);
    assert_true(socketBuffer.options & RECV_BUFFER_DATA_LENGTH_CHUNK);
    assert_false(socketBuffer.options & RECV_BUFFER_DATA_LENGTH_KNOWN);
    assert_false(socketBuffer.options & RECV_BUFFER_DATA_LENGTH_TOKEN);
    assert_int_equal(socketBuffer.length.chunk.next, 0);
    assert_int_equal(socketBuffer.length.chunk.total, 0);

    memset(&socketBuffer, CHAR_MIN, sizeof(RecvBuffer));
    recvBufferSetLengthChunk(&socketBuffer);
    assert_true(socketBuffer.options);
    assert_true(socketBuffer.options & RECV_BUFFER_DATA_LENGTH_CHUNK);
    assert_false(socketBuffer.options & RECV_BUFFER_DATA_LENGTH_KNOWN);
    assert_false(socketBuffer.options & RECV_BUFFER_DATA_LENGTH_TOKEN);
    assert_int_equal(socketBuffer.length.chunk.next, 0);
    assert_int_equal(socketBuffer.length.chunk.total, 0);
}

static void ReceiveSetLengthKnown(void **state) {
    RecvBuffer socketBuffer;

    memset(&socketBuffer, 0, sizeof(RecvBuffer));
    recvBufferSetLengthKnown(&socketBuffer, 69);
    assert_true(socketBuffer.options);
    assert_true(socketBuffer.options & RECV_BUFFER_DATA_LENGTH_KNOWN);
    assert_false(socketBuffer.options & RECV_BUFFER_DATA_LENGTH_CHUNK);
    assert_false(socketBuffer.options & RECV_BUFFER_DATA_LENGTH_TOKEN);
    assert_int_equal(socketBuffer.length.known.total, 69);
    assert_int_equal(socketBuffer.length.known.escape, 0);

    memset(&socketBuffer, CHAR_MIN, sizeof(RecvBuffer));
    recvBufferSetLengthKnown(&socketBuffer, 69);
    assert_true(socketBuffer.options);
    assert_true(socketBuffer.options & RECV_BUFFER_DATA_LENGTH_KNOWN);
    assert_false(socketBuffer.options & RECV_BUFFER_DATA_LENGTH_CHUNK);
    assert_false(socketBuffer.options & RECV_BUFFER_DATA_LENGTH_TOKEN);
    assert_int_equal(socketBuffer.length.known.total, 69);
    assert_int_equal(socketBuffer.length.known.escape, 0);
}

static void ReceiveSetLengthToken(void **state) {
    RecvBuffer socketBuffer;

    memset(&socketBuffer, 0, sizeof(RecvBuffer));
    recvBufferSetLengthToken(&socketBuffer, HTTP_EOL HTTP_EOL, 4);
    assert_true(socketBuffer.options);
    assert_true(socketBuffer.options & RECV_BUFFER_DATA_LENGTH_TOKEN);
    assert_false(socketBuffer.options & RECV_BUFFER_DATA_LENGTH_CHUNK);
    assert_false(socketBuffer.options & RECV_BUFFER_DATA_LENGTH_KNOWN);
    assert_memory_equal(socketBuffer.length.token.token, HTTP_EOL HTTP_EOL, 4);
    assert_int_equal(socketBuffer.length.token.length, 4);

    memset(&socketBuffer, CHAR_MIN, sizeof(RecvBuffer));
    recvBufferSetLengthToken(&socketBuffer, HTTP_EOL HTTP_EOL, 4);
    assert_true(socketBuffer.options);
    assert_true(socketBuffer.options & RECV_BUFFER_DATA_LENGTH_TOKEN);
    assert_false(socketBuffer.options & RECV_BUFFER_DATA_LENGTH_CHUNK);
    assert_false(socketBuffer.options & RECV_BUFFER_DATA_LENGTH_KNOWN);
    assert_memory_equal(socketBuffer.length.token.token, HTTP_EOL HTTP_EOL, 4);
    assert_int_equal(socketBuffer.length.token.length, 4);
}

static void ReceiveSetLengthUnknown(void **state) {
    RecvBuffer socketBuffer;

    memset(&socketBuffer, 0, sizeof(RecvBuffer));
    recvBufferSetLengthUnknown(&socketBuffer, 69);
    assert_false(socketBuffer.options & RECV_BUFFER_DATA_LENGTH_CHUNK);
    assert_false(socketBuffer.options & RECV_BUFFER_DATA_LENGTH_KNOWN);
    assert_false(socketBuffer.options & RECV_BUFFER_DATA_LENGTH_TOKEN);
    assert_int_equal(socketBuffer.length.unknown.limit, 69);
    assert_int_equal(socketBuffer.length.known.escape, 0);

    memset(&socketBuffer, CHAR_MIN, sizeof(RecvBuffer));
    recvBufferSetLengthUnknown(&socketBuffer, 69);
    assert_false(socketBuffer.options & RECV_BUFFER_DATA_LENGTH_CHUNK);
    assert_false(socketBuffer.options & RECV_BUFFER_DATA_LENGTH_KNOWN);
    assert_false(socketBuffer.options & RECV_BUFFER_DATA_LENGTH_TOKEN);
    assert_int_equal(socketBuffer.length.unknown.limit, 69);
    assert_int_equal(socketBuffer.length.known.escape, 0);
}

#ifdef MOCK

static void ReceiveDitch(void **state) {
    RecvBuffer socketBuffer = {0};
    char buf[11] = {0};

    mockReset(), mockOptions = MOCK_CONNECT | MOCK_RECEIVE, mockSendMaxBuf = mockReceiveMaxBuf = 1024;

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

static void ReceiveFetch(void **state) {
    RecvBuffer socketBuffer = {0};
    char buf[11] = {0};

    mockReset(), mockOptions = MOCK_CONNECT | MOCK_RECEIVE, mockSendMaxBuf = mockReceiveMaxBuf = 1024;

    assert_null(recvBufferAppend(&socketBuffer, 10));

    assert_null(recvBufferFetch(&socketBuffer, buf, 0, 11));
    assert_string_equal("1234567890", buf);

    assert_null(recvBufferFetch(&socketBuffer, buf, 1, 5));
    assert_string_equal("2345", buf);

    assert_null(recvBufferFetch(&socketBuffer, buf, 5, 4));
    assert_string_equal("678", buf);

    recvBufferFailFree(&socketBuffer);
}

static void ReceiveFind(void **state) {
    const char *data = "The quick brown fox jumps over the lazy dog";
    size_t len = strlen(data);

    RecvBuffer socketBuffer = {0};
    PlatformFileOffset pos;

    mockReset(), mockOptions = MOCK_CONNECT | MOCK_RECEIVE, mockReceiveMaxBuf = len;
    mockReceiveStream = tmpfile(), fwrite(data, 1, len, mockReceiveStream), rewind(mockReceiveStream);
    recvBufferAppend(&socketBuffer, len);

    assert_int_equal(recvBufferFind(&socketBuffer, 0, "The", 3), 0);
    assert_int_equal(recvBufferFind(&socketBuffer, 0, "he", 2), 1);
    assert_int_equal((pos = recvBufferFind(&socketBuffer, 0, "fox", 3)), 16);
    assert_int_equal((pos = recvBufferFind(&socketBuffer, pos, "ox", 2)), 17);
    assert_int_equal(recvBufferFind(&socketBuffer, pos, "dog", 3), 40);

    recvBufferFailFree(&socketBuffer);
}

static void ReceiveFindDitch(void **state) {
    const char *data = "The quick brown fox jumps over the lazy dog";
    size_t len = strlen(data);

    RecvBuffer socketBuffer = {0};
    mockReset(), mockOptions = MOCK_CONNECT | MOCK_RECEIVE, mockReceiveMaxBuf = len;
    mockReceiveStream = tmpfile(), fwrite(data, 1, len, mockReceiveStream), rewind(mockReceiveStream);

    assert_null(recvBufferFindAndDitch(&socketBuffer, "The", 3));
    assert_null(recvBufferFindAndDitch(&socketBuffer, "he", 2));
    assert_null(recvBufferFindAndDitch(&socketBuffer, "fox", 3));
    assert_null(recvBufferFindAndDitch(&socketBuffer, "ox", 2));
    assert_null(recvBufferFindAndDitch(&socketBuffer, "dog", 3));
    assert_non_null(recvBufferFindAndDitch(&socketBuffer, "fox", 3)); /* Buffer eaten, fox is gone! */

    recvBufferFailFree(&socketBuffer);
}

static void ReceiveFindFetch(void **state) {
    const char *data = "The quick brown fox jumps over the lazy dog";
    size_t len = strlen(data);

    RecvBuffer socketBuffer = {0};
    mockReset(), mockOptions = MOCK_CONNECT | MOCK_RECEIVE, mockReceiveMaxBuf = len;
    mockReceiveStream = tmpfile(), fwrite(data, 1, len, mockReceiveStream), rewind(mockReceiveStream);

    assert_null(recvBufferFindAndFetch(&socketBuffer, "The", 3, len));
    assert_null(recvBufferFindAndFetch(&socketBuffer, "he", 2, len));
    assert_null(recvBufferFindAndFetch(&socketBuffer, "fox", 3, len));
    assert_null(recvBufferFindAndFetch(&socketBuffer, "ox", 2, len));
    assert_null(recvBufferFindAndFetch(&socketBuffer, "dog", 3, len));
    assert_null(recvBufferFindAndFetch(&socketBuffer, "fox", 3, len)); /* Buffer expanded, fox is still here */

    recvBufferFailFree(&socketBuffer);
}

static void ReceiveUpdateSocket(void **state) {
    SocketAddress address;
    RecvBuffer socketBuffer1, socketBuffer2;

    UriDetails details = uriDetailsNewFrom("http://127.0.0.1:8080/foo?bar");
    SOCKET sock = 1;

    mockReset(), mockOptions = MOCK_CONNECT | MOCK_RECEIVE, mockReceiveMaxBuf = 1024;
    memset(&address, CHAR_MIN, sizeof(SocketAddress));

    assert_false(uriDetailsCreateSocketAddress(&details, &address, 0));
    uriDetailsFree(&details);

    socketBuffer1 = recvBufferNew(0, address, 0);
    recvBufferSetLengthKnown(&socketBuffer1, 99);
    assert_null(recvBufferAppend(&socketBuffer1, 10));
    socketBuffer2 = socketBuffer1;
    recvBufferUpdateSocket(&socketBuffer1, &sock);

    assert_ptr_equal(socketBuffer1.buffer, socketBuffer2.buffer);
    assert_int_not_equal(socketBuffer1.serverSocket, socketBuffer2.serverSocket);
    assert_memory_equal(&socketBuffer1.serverAddress, &socketBuffer2.serverAddress, sizeof(SocketAddress));
    assert_memory_equal(&socketBuffer1.length, &socketBuffer2.length, sizeof(RecvBufferLength));
    assert_int_equal(socketBuffer1.options, socketBuffer2.options);
}

#endif /* MOCK */

#pragma clang diagnostic pop

const struct CMUnitTest recvBufferSocketTest[] = {cmocka_unit_test(RecvBufferClear), cmocka_unit_test(RecvBufferCopyBetween),
                                                  cmocka_unit_test(RecvBufferDitchBetween),
                                                  cmocka_unit_test(RecvBufferMemoryFree),
                                                  cmocka_unit_test(ReceiveSetLengthChunk),
                                                  cmocka_unit_test(ReceiveSetLengthKnown),
                                                  cmocka_unit_test(ReceiveSetLengthToken),
                                                  cmocka_unit_test(ReceiveSetLengthUnknown)
#ifdef MOCK
        , cmocka_unit_test(ReceiveFetch), cmocka_unit_test(ReceiveFind), cmocka_unit_test(ReceiveDitch),
                                                  cmocka_unit_test(ReceiveFindDitch),
                                                  cmocka_unit_test(ReceiveFindFetch),
                                                  cmocka_unit_test(ReceiveUpdateSocket)
#endif
};

#endif /* NEW_DL_TEST_SOCKET_BUFFER_H */
