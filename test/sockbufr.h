#ifndef NEW_TH_TEST_SOCKET_BUFFER_H
#define NEW_TH_TEST_SOCKET_BUFFER_H

#include "common.h"
#include "../src/server/sockbufr.h"
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

static void SocketBufferExtend(void **state) {
    size_t i, bufferMax = ((SB_DATA_SIZE * 2) / 32) - 1, sent1, sent2, sent3, len;
    char junkData[SB_DATA_SIZE * 2] = "";
    SocketBuffer socketBuffer = socketBufferNew(0, SOC_BUF_OPT_EXTEND);

#pragma region Check sane values in new socket buffer
    assert_null(socketBuffer.extension);
    assert_false(socketBuffer.idx);
#pragma endregion

#pragma region Populate some data
    strcpy(junkData, sampleText);
    for (i = 1; i < bufferMax; ++i)
        strcat(junkData, sampleText);

    len = strlen(junkData), mockReset(), mockMaxBuf = SB_DATA_SIZE / 8;
#pragma endregion

#pragma region Create socket buffer extended buffer
    sent1 = socketBufferWriteText(&socketBuffer, junkData);

    assert_int_equal(socketBuffer.idx, SB_DATA_SIZE);
    assert_memory_equal(socketBuffer.buffer, junkData, SB_DATA_SIZE);

    assert_int_equal(sent1, len);
    assert_int_equal(socketBuffer.idx, SB_DATA_SIZE);
    assert_non_null(socketBuffer.extension);
    assert_false(socketBuffer.extension->i);
    assert_int_equal(socketBuffer.idx + socketBuffer.extension->length, sent1);
    assert_memory_equal(socketBuffer.extension->data, &junkData[socketBuffer.idx], socketBuffer.extension->length);
#pragma endregion

#pragma region Append to existing socket buffer extension
    sent2 = socketBufferWriteText(&socketBuffer, junkData);
    assert_non_null(socketBuffer.extension);
    assert_false(socketBuffer.extension->i);
    assert_int_equal(socketBuffer.idx + socketBuffer.extension->length, sent1 + sent2);

    sent3 = socketBufferWriteData(&socketBuffer, "", 1);
    assert_int_equal(socketBuffer.idx + socketBuffer.extension->length, sent1 + sent2 + sent3);
#pragma endregion

#pragma region Freed up correctly when no longer needed
    sent2 = sent1 + sent2 + sent3, sent1 = 1, sent3 = 0;

    while (sent1 != 0) {
        mockCurBuf = mockError = 0;
        sent1 = socketBufferFlush(&socketBuffer);
        sent3 += sent1;
    }

    assert_null(socketBuffer.extension);
    assert_false(socketBuffer.idx);
    assert_int_equal(sent3, sent2);
#pragma endregion

#pragma region Data appending integrity
    socketBufferWriteText(&socketBuffer, junkData);
    free(socketBuffer.extension->data), free(socketBuffer.extension), socketBuffer.extension = NULL;

    socketBufferWriteText(&socketBuffer, "ABCD");
    socketBufferWriteText(&socketBuffer, "EFGH");
    assert_memory_equal(socketBuffer.extension->data, "ABCDEFGH", socketBuffer.extension->length);

    --socketBuffer.idx, socketBuffer.buffer[SB_DATA_SIZE - 1] = '\0';
    free(socketBuffer.extension->data), free(socketBuffer.extension), socketBuffer.extension = NULL;

    socketBufferWriteText(&socketBuffer, "ABCD");
    assert_int_equal(socketBuffer.buffer[SB_DATA_SIZE - 1], 'A');
    assert_memory_equal(socketBuffer.extension->data, "BCD", socketBuffer.extension->length);

    socketBuffer.idx -= 3;
    memset(&socketBuffer.buffer[SB_DATA_SIZE - 3], 0, 3);
    free(socketBuffer.extension->data), free(socketBuffer.extension), socketBuffer.extension = NULL;

    socketBufferWriteText(&socketBuffer, "ABCD");
    assert_int_equal(socketBuffer.buffer[SB_DATA_SIZE - 3], 'A');
    assert_int_equal(socketBuffer.buffer[SB_DATA_SIZE - 2], 'B');
    assert_int_equal(socketBuffer.buffer[SB_DATA_SIZE - 1], 'C');
    assert_memory_equal(socketBuffer.extension->data, "D", socketBuffer.extension->length);
#pragma endregion

#pragma region Kill buffer
    socketBufferKill(&socketBuffer);

    assert_null(socketBuffer.extension);
    assert_false(socketBuffer.idx);
    assert_true(socketBuffer.options & SOC_BUF_ERR_FAIL);
#pragma endregion
}

static void ExtendedMemoryAppend(void **state) {
    size_t i, bufferMax = ((SB_DATA_SIZE * 2) / 32) - 1;
    char junkData[SB_DATA_SIZE * 4] = "";
    MemoryPool *memoryPool = NULL;

    strcpy(junkData, sampleText);
    for (i = 1; i < bufferMax; ++i)
        strcat(junkData, sampleText);

    i = 128, mockReset(), mockMaxBuf = SB_DATA_SIZE / 8;

    memoryPool = socketBufferMemoryPoolNew(junkData, i);

    assert_non_null(memoryPool);
    assert_non_null(memoryPool->data);
    assert_false(memoryPool->i);
    assert_int_equal(memoryPool->length, i);
    assert_memory_equal(memoryPool->data, junkData, memoryPool->length);

    free(memoryPool->data), free(memoryPool), memoryPool = NULL;

    memoryPool = socketBufferMemoryPoolAppend(memoryPool, "ABCD", 5);

    assert_non_null(memoryPool);
    assert_non_null(memoryPool->data);
    assert_false(memoryPool->i);
    assert_int_equal(memoryPool->length, 5);
    assert_memory_equal(memoryPool->data, "ABCD", memoryPool->length);

    memoryPool = socketBufferMemoryPoolAppend(memoryPool, "EFGH", 5);

    assert_non_null(memoryPool);
    assert_non_null(memoryPool->data);
    assert_false(memoryPool->i);
    assert_int_equal(memoryPool->length, 10);
    assert_memory_equal(memoryPool->data, "ABCD\0EFGH", memoryPool->length);

    mockError = ENOMEM;
    memoryPool = socketBufferMemoryPoolAppend(memoryPool, "IJKL", 5);
    assert_null(memoryPool);

    memoryPool = socketBufferMemoryPoolAppend(memoryPool, "ABCD", 5);
    assert_null(memoryPool);
}

const struct CMUnitTest socketTest[] = {cmocka_unit_test(ExtendedMemoryAppend),
                                        cmocka_unit_test(SocketBufferExtend)};


#endif /* NEW_TH_TEST_SOCKET_BUFFER_H */
