#ifndef NEW_DL_TEST_SOCKET_BUFFER_H
#define NEW_DL_TEST_SOCKET_BUFFER_H

#include "../src/client/recvbufr.h"
#include <errno.h>
#include <limits.h>
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

/**
 * This test checks that recvBufferCopyBetween() copies data over into another buffer a sane way
 */
static void RecvBufferCopyBetween(void **state) {
    char *copy;

    RecvBuffer socketBuffer = {0};
    char input[] = "The quick brown fox jumps over the lazy dog";
    char output[sizeof(input)] = {0};

    socketBuffer.max = socketBuffer.len = strlen(input + 1);
    assert_non_null(socketBuffer.buffer = malloc(strlen(input) + 1));
    assert_non_null(strcpy(socketBuffer.buffer, input));
    assert_non_null(copy = recvBufferCopyBetween(&socketBuffer, 4, 20));
    assert_memory_equal(&input[4], copy, 15);
    free(copy), memset(output, 0, 15);

    assert_non_null(copy = recvBufferCopyBetween(&socketBuffer, 35, 45));
    assert_memory_equal(&input[35], copy, 8);
    free(copy), recvBufferClear(&socketBuffer);
}

/**
 * This test checks that the heap buffer is freed correctly
 */
static void RecvBufferMemoryFree(void **state) {
    RecvBuffer socketBuffer = {0};

#ifdef MOCK
    mockReset();
#endif

    socketBuffer.buffer = malloc(1);
    assert_non_null(&socketBuffer.buffer);
    recvBufferFailFree(&socketBuffer);

#ifdef MOCK
    assert_ptr_equal(socketBuffer.buffer, mockLastFree);
#endif
}

/**
 * This test checks that the heap buffer is cleared correctly
 */
static void RecvBufferClear(void **state) {
    RecvBuffer socketBuffer = {0};
    char buf[10];

#ifdef MOCK
    char *buffer;
    mockReset();
    buffer =
#endif

    socketBuffer.buffer = malloc(10);
    assert_non_null(&socketBuffer.buffer);
    recvBufferClear(&socketBuffer);
    assert_null(socketBuffer.buffer);
    assert_non_null(recvBufferFetch(&socketBuffer, buf, 0, 10));

#ifdef MOCK
    assert_ptr_equal(buffer, mockLastFree);
#endif
}

/**
 * This test ensures that recvBufferSetLengthChunk() sets the socket buffer into chunk parsing mode correctly
 */
static void ReceiveSetLengthChunk(void **state) {
    RecvBuffer socketBuffer;

    memset(&socketBuffer, 0, sizeof(RecvBuffer));
    recvBufferSetLengthChunk(&socketBuffer);
    assert_true(socketBuffer.options);
    assert_true(socketBuffer.options & RECV_BUFFER_DATA_LENGTH_CHUNK);
    assert_false(socketBuffer.options & RECV_BUFFER_DATA_LENGTH_KNOWN);
    assert_false(socketBuffer.options & RECV_BUFFER_DATA_LENGTH_TOKEN);
    assert_false(socketBuffer.options & RECV_BUFFER_DATA_LENGTH_COMPLETE);
    assert_int_equal(socketBuffer.length.chunk.next, -1);
    assert_int_equal(socketBuffer.length.chunk.total, 0);

    memset(&socketBuffer, CHAR_MIN, sizeof(RecvBuffer)), socketBuffer.buffer = 0;
    recvBufferSetLengthChunk(&socketBuffer);
    assert_true(socketBuffer.options);
    assert_true(socketBuffer.options & RECV_BUFFER_DATA_LENGTH_CHUNK);
    assert_false(socketBuffer.options & RECV_BUFFER_DATA_LENGTH_KNOWN);
    assert_false(socketBuffer.options & RECV_BUFFER_DATA_LENGTH_TOKEN);
    assert_false(socketBuffer.options & RECV_BUFFER_DATA_LENGTH_COMPLETE);
    assert_int_equal(socketBuffer.length.chunk.next, -1);
    assert_int_equal(socketBuffer.length.chunk.total, 0);
}

/**
 * This test ensures that recvBufferSetLengthComplete() marks the socket buffer as request complete correctly
 */
static void ReceiveSetLengthComplete(void **state) {
    RecvBuffer socketBuffer;

    memset(&socketBuffer, 0, sizeof(RecvBuffer));
    recvBufferSetLengthComplete(&socketBuffer);
    assert_true(socketBuffer.options);
    assert_false(socketBuffer.options & RECV_BUFFER_DATA_LENGTH_CHUNK);
    assert_false(socketBuffer.options & RECV_BUFFER_DATA_LENGTH_KNOWN);
    assert_false(socketBuffer.options & RECV_BUFFER_DATA_LENGTH_TOKEN);
    assert_true(socketBuffer.options & RECV_BUFFER_DATA_LENGTH_COMPLETE);
    assert_int_equal(socketBuffer.length.chunk.next, -1);
    assert_int_equal(socketBuffer.length.chunk.total, 0);

    memset(&socketBuffer, CHAR_MIN, sizeof(RecvBuffer));
    recvBufferSetLengthComplete(&socketBuffer);
    assert_true(socketBuffer.options);
    assert_false(socketBuffer.options & RECV_BUFFER_DATA_LENGTH_CHUNK);
    assert_false(socketBuffer.options & RECV_BUFFER_DATA_LENGTH_KNOWN);
    assert_false(socketBuffer.options & RECV_BUFFER_DATA_LENGTH_TOKEN);
    assert_true(socketBuffer.options & RECV_BUFFER_DATA_LENGTH_COMPLETE);
    assert_int_equal(socketBuffer.length.chunk.next, -1);
    assert_int_equal(socketBuffer.length.chunk.total, 0);
}

/**
 * This test ensures that recvBufferSetLengthKnown() sets the socket buffer is set into length known mode correctly
 */
static void ReceiveSetLengthKnown(void **state) {
    RecvBuffer socketBuffer;

    memset(&socketBuffer, 0, sizeof(RecvBuffer));
    recvBufferSetLengthKnown(&socketBuffer, 69);
    assert_true(socketBuffer.options);
    assert_true(socketBuffer.options & RECV_BUFFER_DATA_LENGTH_KNOWN);
    assert_false(socketBuffer.options & RECV_BUFFER_DATA_LENGTH_CHUNK);
    assert_false(socketBuffer.options & RECV_BUFFER_DATA_LENGTH_TOKEN);
    assert_false(socketBuffer.options & RECV_BUFFER_DATA_LENGTH_COMPLETE);
    assert_int_equal(socketBuffer.length.known.total, 69);
    assert_int_equal(socketBuffer.length.known.escape, 0);

    memset(&socketBuffer, CHAR_MIN, sizeof(RecvBuffer));
    recvBufferSetLengthKnown(&socketBuffer, 69);
    assert_true(socketBuffer.options);
    assert_true(socketBuffer.options & RECV_BUFFER_DATA_LENGTH_KNOWN);
    assert_false(socketBuffer.options & RECV_BUFFER_DATA_LENGTH_CHUNK);
    assert_false(socketBuffer.options & RECV_BUFFER_DATA_LENGTH_TOKEN);
    assert_false(socketBuffer.options & RECV_BUFFER_DATA_LENGTH_COMPLETE);
    assert_int_equal(socketBuffer.length.known.total, 69);
    assert_int_equal(socketBuffer.length.known.escape, 0);
}

/**
 * This test ensures that recvBufferSetLengthToken() set the socket buffer is set into token parsing mode correctly
 */
static void ReceiveSetLengthToken(void **state) {
    RecvBuffer socketBuffer;

    memset(&socketBuffer, 0, sizeof(RecvBuffer));
    recvBufferSetLengthToken(&socketBuffer, HTTP_EOL HTTP_EOL, 4);
    assert_true(socketBuffer.options);
    assert_true(socketBuffer.options & RECV_BUFFER_DATA_LENGTH_TOKEN);
    assert_false(socketBuffer.options & RECV_BUFFER_DATA_LENGTH_CHUNK);
    assert_false(socketBuffer.options & RECV_BUFFER_DATA_LENGTH_KNOWN);
    assert_false(socketBuffer.options & RECV_BUFFER_DATA_LENGTH_COMPLETE);
    assert_memory_equal(socketBuffer.length.token.token, HTTP_EOL HTTP_EOL, 4);
    assert_int_equal(socketBuffer.length.token.length, 4);

    memset(&socketBuffer, CHAR_MIN, sizeof(RecvBuffer));
    recvBufferSetLengthToken(&socketBuffer, HTTP_EOL HTTP_EOL, 4);
    assert_true(socketBuffer.options);
    assert_true(socketBuffer.options & RECV_BUFFER_DATA_LENGTH_TOKEN);
    assert_false(socketBuffer.options & RECV_BUFFER_DATA_LENGTH_CHUNK);
    assert_false(socketBuffer.options & RECV_BUFFER_DATA_LENGTH_KNOWN);
    assert_false(socketBuffer.options & RECV_BUFFER_DATA_LENGTH_COMPLETE);
    assert_memory_equal(socketBuffer.length.token.token, HTTP_EOL HTTP_EOL, 4);
    assert_int_equal(socketBuffer.length.token.length, 4);
}

/**
 * This test ensures that recvBufferSetLengthUnknown() set the socket buffer is set into unknown mode correctly
 */
static void ReceiveSetLengthUnknown(void **state) {
    RecvBuffer socketBuffer;

    memset(&socketBuffer, 0, sizeof(RecvBuffer));
    recvBufferSetLengthUnknown(&socketBuffer, 69);
    assert_false(socketBuffer.options & RECV_BUFFER_DATA_LENGTH_CHUNK);
    assert_false(socketBuffer.options & RECV_BUFFER_DATA_LENGTH_KNOWN);
    assert_false(socketBuffer.options & RECV_BUFFER_DATA_LENGTH_TOKEN);
    assert_false(socketBuffer.options & RECV_BUFFER_DATA_LENGTH_COMPLETE);
    assert_int_equal(socketBuffer.length.unknown.limit, 69);
    assert_int_equal(socketBuffer.length.known.escape, 0);

    memset(&socketBuffer, CHAR_MIN, sizeof(RecvBuffer));
    recvBufferSetLengthUnknown(&socketBuffer, 69);
    assert_false(socketBuffer.options & RECV_BUFFER_DATA_LENGTH_CHUNK);
    assert_false(socketBuffer.options & RECV_BUFFER_DATA_LENGTH_KNOWN);
    assert_false(socketBuffer.options & RECV_BUFFER_DATA_LENGTH_TOKEN);
    assert_false(socketBuffer.options & RECV_BUFFER_DATA_LENGTH_COMPLETE);
    assert_int_equal(socketBuffer.length.unknown.limit, 69);
    assert_int_equal(socketBuffer.length.known.escape, 0);
}

#ifdef MOCK

/**
 * This test ensures that recvBufferDitch() only ditches the amount of bytes specified from the beginning of the buffer
 */
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

/**
 * This test ensures that recvBufferFetch() get the correct starting position and length from the requested buffer
 */
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

/**
 * This test is like the ReceiveFetch test except in chunk parsing mode
 */
static void ReceiveFetchChunk(void **state) {
    RecvBuffer socketBuffer;

    char sample[] = "4" HTTP_EOL "This" HTTP_EOL "3" HTTP_EOL " is" HTTP_EOL "2" HTTP_EOL" a" HTTP_EOL "5" HTTP_EOL " test" HTTP_EOL "0" HTTP_EOL HTTP_EOL;
    char expect[] = "This is a test", output[sizeof(sample)] = {0};
    size_t max = strlen(sample);

    mockReset(), mockOptions = MOCK_CONNECT | MOCK_RECEIVE, mockSendMaxBuf = mockReceiveMaxBuf = 1024;
    mockReceiveStream = tmpfile(), fwrite(sample, 1, max, mockReceiveStream), rewind(mockReceiveStream);

    assert_null(recvBufferNewFromUri(&socketBuffer, "http://127.0.0.1:8080", 0));
    recvBufferSetLengthChunk(&socketBuffer);

    assert_null(recvBufferAppend(&socketBuffer, 512));
    assert_null(recvBufferFetch(&socketBuffer, output, 0, 512));
    assert_string_equal(expect, output);
    assert_int_equal(socketBuffer.length.chunk.next, -1);
    recvBufferFailFree(&socketBuffer);
}

/**
 * This test ensures that HTTP chunk metadata is not interpreted as data
 */
static void ReceiveFetchChunkEmpty(void **state) {
    RecvBuffer socketBuffer;

    char sample[] = "0" HTTP_EOL;
    size_t max = strlen(sample);

    mockReset(), mockOptions = MOCK_CONNECT | MOCK_RECEIVE, mockSendMaxBuf = mockReceiveMaxBuf = 1024;
    mockReceiveStream = tmpfile(), fwrite(sample, 1, max, mockReceiveStream), rewind(mockReceiveStream);

    assert_null(recvBufferNewFromUri(&socketBuffer, "http://127.0.0.1:8080", 0));
    recvBufferSetLengthChunk(&socketBuffer);

    assert_null(recvBufferAppend(&socketBuffer, 512));
    assert_int_equal(socketBuffer.length.chunk.next, -1);
    recvBufferClear(&socketBuffer);
}

/**
 * This test ensures that HTTP chunk metadata is not interpreted as data when the requests come in aligned to the chunks
 */
static void ReceiveFetchChunkIterateAligned(void **state) {
    RecvBuffer socketBuffer;

    char sample[] = "4" HTTP_EOL "This" HTTP_EOL "3" HTTP_EOL " is" HTTP_EOL "2" HTTP_EOL" a" HTTP_EOL "5" HTTP_EOL " test" HTTP_EOL "0" HTTP_EOL HTTP_EOL;
    char output[sizeof(sample)] = {0};
    size_t max = strlen(sample);

    mockReset(), mockOptions = MOCK_CONNECT | MOCK_RECEIVE | MOCK_RECEIVE_COUNT;
    mockReceiveStream = tmpfile(), fwrite(sample, 1, max, mockReceiveStream), rewind(mockReceiveStream);

    assert_null(recvBufferNewFromUri(&socketBuffer, "http://127.0.0.1:8080", 0));
    recvBufferSetLengthChunk(&socketBuffer);

    mockReceiveMaxBuf = 4 + 3;
    assert_null(recvBufferAppend(&socketBuffer, 512));
    assert_null(recvBufferFetch(&socketBuffer, output, 0, 512));
    assert_string_equal(output, "This");
    assert_int_equal(socketBuffer.length.chunk.next, 0);

    mockReceiveMaxBuf += 3 + 5;
    assert_null(recvBufferAppend(&socketBuffer, 512));
    assert_null(recvBufferFetch(&socketBuffer, output, 0, 512));
    assert_string_equal(output, "This is");
    assert_int_equal(socketBuffer.length.chunk.next, 0);

    mockReceiveMaxBuf += 2 + 5;
    assert_null(recvBufferAppend(&socketBuffer, 512));
    assert_null(recvBufferFetch(&socketBuffer, output, 0, 512));
    assert_string_equal(output, "This is a");
    assert_int_equal(socketBuffer.length.chunk.next, 0);

    mockReceiveMaxBuf += 5 + 5;
    assert_null(recvBufferAppend(&socketBuffer, 512));
    assert_null(recvBufferFetch(&socketBuffer, output, 0, 512));
    assert_string_equal(output, "This is a test");
    assert_int_equal(socketBuffer.length.chunk.next, 0);

    mockReceiveMaxBuf += 0 + 5 + 1;
    assert_null(recvBufferAppend(&socketBuffer, 512));
    assert_null(recvBufferFetch(&socketBuffer, output, 0, 512));
    assert_string_equal(output, "This is a test");
    assert_int_equal(socketBuffer.length.chunk.next, -1);
    recvBufferClear(&socketBuffer);
}

/**
 * This test ensures that HTTP chunk metadata is not interpreted as data when the requests are unaligned to the chunks
 */
static void ReceiveFetchChunkIterateUnaligned(void **state) {
    RecvBuffer socketBuffer;

    char sample[] = "4" HTTP_EOL "This" HTTP_EOL "3" HTTP_EOL " is" HTTP_EOL "2" HTTP_EOL" a" HTTP_EOL "5" HTTP_EOL " test" HTTP_EOL "0" HTTP_EOL HTTP_EOL;
    char output[sizeof(sample)] = {0};
    size_t max = strlen(sample);

    mockReset(), mockOptions = MOCK_CONNECT | MOCK_RECEIVE | MOCK_RECEIVE_COUNT;
    mockReceiveStream = tmpfile(), fwrite(sample, 1, max, mockReceiveStream), rewind(mockReceiveStream);

    assert_null(recvBufferNewFromUri(&socketBuffer, "http://127.0.0.1:8080", 0));
    recvBufferSetLengthChunk(&socketBuffer);

    mockReceiveMaxBuf = 3 + 3;
    assert_null(recvBufferAppend(&socketBuffer, 512));
    assert_null(recvBufferFetch(&socketBuffer, output, 0, 512));
    assert_string_equal(output, "Thi");
    assert_int_equal(socketBuffer.length.chunk.next, 1);

    mockReceiveMaxBuf += 3 + 5;
    assert_null(recvBufferAppend(&socketBuffer, 512));
    assert_null(recvBufferFetch(&socketBuffer, output, 0, 512));
    assert_string_equal(output, "This i");
    assert_int_equal(socketBuffer.length.chunk.next, 1);

    mockReceiveMaxBuf += 2 + 5;
    assert_null(recvBufferAppend(&socketBuffer, 512));
    assert_null(recvBufferFetch(&socketBuffer, output, 0, 512));
    assert_string_equal(output, "This is ");
    assert_int_equal(socketBuffer.length.chunk.next, 1);

    mockReceiveMaxBuf += 5 + 5;
    assert_null(recvBufferAppend(&socketBuffer, 512));
    assert_null(recvBufferFetch(&socketBuffer, output, 0, 512));
    assert_string_equal(output, "This is a tes");
    assert_int_equal(socketBuffer.length.chunk.next, 1);

    mockReceiveMaxBuf += 1 + 5 + 1;
    assert_null(recvBufferAppend(&socketBuffer, 512));
    assert_null(recvBufferFetch(&socketBuffer, output, 0, 512));
    assert_string_equal(output, "This is a test");
    assert_int_equal(socketBuffer.length.chunk.next, -1);
    recvBufferClear(&socketBuffer);
}

/**
 * This test ensures that recvBufferAppend() fails gracefully when HTTP chunk metadata is corrupted
 */
static void ReceiveFetchChunkMalformed(void **state) {
    RecvBuffer socketBuffer;

    char sample[] = "4" HTTP_EOL "This" HTTP_EOL "3" HTTP_EOL " is" HTTP_EOL "3" HTTP_EOL" a" HTTP_EOL "4" HTTP_EOL " test" HTTP_EOL "0" HTTP_EOL HTTP_EOL;
    size_t max = strlen(sample);

    mockReset(), mockOptions = MOCK_CONNECT | MOCK_RECEIVE, mockSendMaxBuf = mockReceiveMaxBuf = 1024;
    mockReceiveStream = tmpfile(), fwrite(sample, 1, max, mockReceiveStream), rewind(mockReceiveStream);

    assert_null(recvBufferNewFromUri(&socketBuffer, "http://127.0.0.1:8080", 0));
    recvBufferSetLengthChunk(&socketBuffer);

    assert_ptr_equal(recvBufferAppend(&socketBuffer, 512), ErrIllegalHexCharacter);
    recvBufferClear(&socketBuffer);
}

/**
 * This test ensures that recvBufferAppend() fails gracefully when HTTP chunk metadata is malformed at the start
 */
static void ReceiveFetchChunkMalformedStart(void **state) {
    RecvBuffer socketBuffer;

    char sample[] = "3" HTTP_EOL "Error";
    size_t max = strlen(sample);

    mockReset(), mockOptions = MOCK_CONNECT | MOCK_RECEIVE, mockSendMaxBuf = mockReceiveMaxBuf = 1024;
    mockReceiveStream = tmpfile(), fwrite(sample, 1, max, mockReceiveStream), rewind(mockReceiveStream);

    assert_null(recvBufferNewFromUri(&socketBuffer, "http://127.0.0.1:8080", 0));
    recvBufferSetLengthChunk(&socketBuffer);

    assert_null(recvBufferAppend(&socketBuffer, 512));
    recvBufferClear(&socketBuffer);
}

/**
 * This test is similar to the ReceiveFetchNonBlocking test but for chunk mode
 */
static void ReceiveFetchChuckNonBlocking(void **state) {
    RecvBuffer socketBuffer = {0};
    char sample[] = "4" HTTP_EOL "This" HTTP_EOL "3" HTTP_EOL " is" HTTP_EOL "2" HTTP_EOL" a" HTTP_EOL "5" HTTP_EOL " test" HTTP_EOL "0" HTTP_EOL HTTP_EOL;
    char output[sizeof(sample)] = {0};
    size_t max = strlen(sample);

    mockReset(), mockOptions = MOCK_CONNECT | MOCK_RECEIVE, mockSendMaxBuf = mockReceiveMaxBuf = 1024;
    mockReceiveStream = tmpfile(), fwrite(sample, 1, max, mockReceiveStream), rewind(mockReceiveStream);
    recvBufferSetLengthChunk(&socketBuffer);

    #pragma region A normal request

    assert_null(recvBufferAppend(&socketBuffer, 10));
    assert_null(recvBufferFetch(&socketBuffer, output, 0, 11));
    assert_string_equal("This is a ", output);

    #pragma endregion

    #pragma region Client is too quick, no packets received

    mockReceiveError = EAGAIN, mockErrorReset = 2;
    recvBufferClear(&socketBuffer);

    assert_null(recvBufferAppend(&socketBuffer, 10));
    assert_null(recvBufferFetch(&socketBuffer, output, 0, 11));
    assert_string_equal("test", output);

    #pragma endregion

    recvBufferFailFree(&socketBuffer);
}

/**
 * This test ensures that recvBufferAppend() handles chunks that make no sense gracefully
 */
static void ReceiveFetchChunkOverflow(void **state) {
    RecvBuffer socketBuffer;

    char sample[] = "5" HTTP_EOL "Err";
    size_t max = strlen(sample);

    mockReset(), mockOptions = MOCK_CONNECT | MOCK_RECEIVE, mockSendMaxBuf = mockReceiveMaxBuf = 1024;
    mockReceiveStream = tmpfile(), fwrite(sample, 1, max, mockReceiveStream), rewind(mockReceiveStream);

    assert_null(recvBufferNewFromUri(&socketBuffer, "http://127.0.0.1:8080", 0));
    recvBufferSetLengthChunk(&socketBuffer);

    assert_null(recvBufferAppend(&socketBuffer, 512));
    assert_int_equal(socketBuffer.length.chunk.next, 2);
    recvBufferClear(&socketBuffer);
}

/**
 * This test ensures that recvBufferAppend() handles chunks that make no sense at the end of a stream gracefully
 */
static void ReceiveFetchChunkOverflowExact(void **state) {
    RecvBuffer socketBuffer;

    char sample[] = "5" HTTP_EOL "Error";
    size_t max = strlen(sample);

    mockReset(), mockOptions = MOCK_CONNECT | MOCK_RECEIVE, mockSendMaxBuf = mockReceiveMaxBuf = 1024;
    mockReceiveStream = tmpfile(), fwrite(sample, 1, max, mockReceiveStream), rewind(mockReceiveStream);

    assert_null(recvBufferNewFromUri(&socketBuffer, "http://127.0.0.1:8080", 0));
    recvBufferSetLengthChunk(&socketBuffer);

    assert_null(recvBufferAppend(&socketBuffer, 512));
    assert_int_equal(socketBuffer.length.chunk.next, 0);
    recvBufferClear(&socketBuffer);
}

/**
 * This test ensures that recvBufferAppend() handles chunks metadata that isn't a valid hex value gracefully
 */
static void ReceiveFetchChunkOverflowMalformed(void **state) {
    RecvBuffer socketBuffer;
    const char *e;

    char sample[] = "5Error" HTTP_EOL;
    size_t max = strlen(sample);

    mockReset(), mockOptions = MOCK_CONNECT | MOCK_RECEIVE, mockSendMaxBuf = mockReceiveMaxBuf = 1024;
    mockReceiveStream = tmpfile(), fwrite(sample, 1, max, mockReceiveStream), rewind(mockReceiveStream);

    assert_null(recvBufferNewFromUri(&socketBuffer, "http://127.0.0.1:8080", 0));
    recvBufferSetLengthChunk(&socketBuffer);

    assert_non_null(e = recvBufferAppend(&socketBuffer, 512));
    assert_string_equal(e, "Illegal hex character");
    recvBufferClear(&socketBuffer);
}

/**
 * This test ensures that recvBufferAppend() waits for packets if it's requesting them too fast
 */
static void ReceiveFetchNonBlocking(void **state) {
    RecvBuffer socketBuffer = {0};
    char buf[11] = {0};

    mockReset(), mockOptions = MOCK_CONNECT | MOCK_RECEIVE, mockSendMaxBuf = mockReceiveMaxBuf = 1024;

    #pragma region A normal request

    assert_null(recvBufferAppend(&socketBuffer, 10));
    assert_null(recvBufferFetch(&socketBuffer, buf, 0, 11));
    assert_string_equal("1234567890", buf);

    #pragma endregion

    #pragma region Client is too quick, no packets at the moment, should block until at least a byte is ready

    mockReceiveError = EAGAIN, mockErrorReset = 2;
    recvBufferClear(&socketBuffer);

    assert_null(recvBufferAppend(&socketBuffer, 10));
    assert_null(recvBufferFetch(&socketBuffer, buf, 0, 11));
    assert_string_equal("1234567890", buf);

    #pragma endregion

    #pragma region There is some packets but less then requested

    mockReceiveError = ENOERR, mockErrorReset = 0, mockReceiveMaxBuf = 2;
    recvBufferClear(&socketBuffer), buf[0] = buf[1] = buf[2] = '\0';

    assert_null(recvBufferAppend(&socketBuffer, 10));
    assert_null(recvBufferFetch(&socketBuffer, buf, 0, 11));
    assert_string_equal("12", buf);

    #pragma endregion

    recvBufferFailFree(&socketBuffer);
}

/**
 * This test checks that recvBufferFind() can retrieve position of certain strings in the buffer
 */
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

/**
 * This test checks that recvBufferFindAndDitch() can find and remove everything before a certain string in the buffer
 */
static void ReceiveFindDitch(void **state) {
    const char *data = "The quick brown fox jumps over the lazy dog";
    size_t len = strlen(data);

    RecvBuffer socketBuffer = {0};
    mockReset(), mockOptions = MOCK_CONNECT | MOCK_RECEIVE, mockReceiveMaxBuf = len;
    mockReceiveStream = tmpfile(), fwrite(data, 1, len, mockReceiveStream), rewind(mockReceiveStream);

    assert_null(recvBufferFindAndDitch(&socketBuffer, "The", 3, NULL));
    assert_null(recvBufferFindAndDitch(&socketBuffer, "he", 2, NULL));
    assert_null(recvBufferFindAndDitch(&socketBuffer, "fox", 3, NULL));
    assert_null(recvBufferFindAndDitch(&socketBuffer, "ox", 2, NULL));
    assert_null(recvBufferFindAndDitch(&socketBuffer, "dog", 3, NULL));
    assert_non_null(recvBufferFindAndDitch(&socketBuffer, "fox", 3, NULL)); /* Buffer eaten, fox is gone! */

    recvBufferFailFree(&socketBuffer);
}

/**
 * This test checks that recvBufferFindAndFetch() can find bytes in the buffer
 */
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

/**
 * This test ensures that recvBufferSend() can send a message to the remote on the RecvBuffer socket
 */
static void ReceiveSend(void **state) {
    RecvBuffer socketBuffer;
    const char *data = "The quick brown fox jumps over the lazy dog";
    char *res;

    mockReset(), mockOptions = MOCK_CONNECT | MOCK_RECEIVE | MOCK_SEND;
    mockReceiveMaxBuf = mockSendMaxBuf = 1024, mockSendStream = tmpfile();

    assert_null(recvBufferNewFromUri(&socketBuffer, "http://127.0.0.1", 0));
    assert_null(recvBufferSend(&socketBuffer, data, strlen(data), 0));
    assert_non_null(res = calloc(strlen(data) * 2, 1));
    rewind(mockSendStream);
    assert_int_not_equal(fread(res, sizeof(char), strlen(data) * 2, mockSendStream), 0);
    assert_string_equal(data, res);
    free(res);
}

/**
 * This test ensures that recvBufferSend() can gracefully reestablish a connection if it is possible to do so
 */
static void ReceiveSendReconnect(void **state) {
    RecvBuffer socketBuffer;
    const char *data = "The quick brown fox jumps over the lazy dog", *e;

    mockReset(), mockOptions = MOCK_CONNECT | MOCK_RECEIVE | MOCK_SEND, mockReceiveMaxBuf = mockSendMaxBuf = 1024;
    /* Initial connection establishment */
    assert_null(recvBufferNewFromUri(&socketBuffer, "http://127.0.0.1", 0));
    assert_null(recvBufferSend(&socketBuffer, data, strlen(data), 0));

    /* TCP connection re-establishment */
    mockConnectError = ECONNREFUSED, mockSendError = ECONNRESET, mockErrorReset = 2;
    assert_null(recvBufferSend(&socketBuffer, data, strlen(data), 0));

    /* Give up after too many attempts */
    mockConnectError = ECONNREFUSED, mockSendError = ECONNRESET, mockErrorReset = 10;
    assert_non_null(e = recvBufferSend(&socketBuffer, data, strlen(data), 0));
    assert_string_equal(e, strerror(ECONNREFUSED));
    assert_int_equal(mockErrorReset, 5);
}

/**
 * This test ensures that recvBufferUpdateSocket() works as intended
 */
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
    recvBufferFailFree(&socketBuffer1);
}

#endif /* MOCK */

#pragma clang diagnostic pop

const struct CMUnitTest recvBufferSocketTest[] = {
    cmocka_unit_test(RecvBufferClear),
    cmocka_unit_test(RecvBufferCopyBetween),
    cmocka_unit_test(RecvBufferMemoryFree),
    cmocka_unit_test(ReceiveSetLengthChunk),
    cmocka_unit_test(ReceiveSetLengthComplete),
    cmocka_unit_test(ReceiveSetLengthKnown),
    cmocka_unit_test(ReceiveSetLengthToken),
    cmocka_unit_test(ReceiveSetLengthUnknown)
};
#ifdef MOCK
const struct CMUnitTest recvBufferSocketTestMock[] = {
    cmocka_unit_test(ReceiveFetch),
    cmocka_unit_test(ReceiveFetchChunk),
    cmocka_unit_test(ReceiveFetchChunkEmpty),
    cmocka_unit_test(ReceiveFetchChunkIterateAligned),
    cmocka_unit_test(ReceiveFetchChunkIterateUnaligned),
    cmocka_unit_test(ReceiveFetchChunkMalformed),
    cmocka_unit_test(ReceiveFetchChunkMalformedStart),
    cmocka_unit_test(ReceiveFetchChuckNonBlocking),
    cmocka_unit_test(ReceiveFetchChunkOverflow),
    cmocka_unit_test(ReceiveFetchChunkOverflowExact),
    cmocka_unit_test(ReceiveFetchChunkOverflowMalformed),
    cmocka_unit_test(ReceiveFetchNonBlocking),
    cmocka_unit_test(ReceiveFind),
    cmocka_unit_test(ReceiveDitch),
    cmocka_unit_test(ReceiveFindDitch),
    cmocka_unit_test(ReceiveFindFetch),
    cmocka_unit_test(ReceiveSend),
    cmocka_unit_test(ReceiveSendReconnect),
    cmocka_unit_test(ReceiveUpdateSocket)
};
#endif

#endif /* NEW_DL_TEST_SOCKET_BUFFER_H */
