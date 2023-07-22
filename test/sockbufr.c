#include <criterion/criterion.h>
#include "../src/server/sockbufr.h"

static char *sampleText = "Glum-Schwartzkopf,vex'd_by NJ&IQ"; /* 32 characters, each unique */

Test(Mockup, Send) {
    size_t i, bufferMax = ((SB_DATA_SIZE * 2) / 32) - 1, sent;
    char junkData[SB_DATA_SIZE * 2] = "";

    strcpy(junkData, sampleText);
    for (i = 1; i < bufferMax; ++i)
        strcat(junkData, sampleText);

    i = strlen(junkData);

    sent = send(0, junkData, 5, 0);
    cr_assert_eq(sent, 5, "Expected %d got %lu", 5, sent);

    sent = send(0, junkData, 5, 0);
    cr_assert_eq(sent, 5, "Expected %d got %lu", 5, sent);

    sent = send(0, junkData, i, 0);
    cr_assert_eq(sent, mockMaxBuf - 10, "Expected %lu got %lu", mockMaxBuf - 10, sent);

    sent = send(0, junkData, i, 0);
    cr_assert_eq(sent, -1, "Expected %d got %lu", -1, sent);
    cr_assert_eq(platformSocketGetLastError(), EAGAIN);
}

Test(SocketBuffer, Extend) {
    size_t i, bufferMax = ((SB_DATA_SIZE * 2) / 32) - 1, sent1, sent2, sent3, len;
    char junkData[SB_DATA_SIZE * 2] = "";
    SocketBuffer socketBuffer = socketBufferNew(0, SOC_BUF_OPT_EXTEND);
    mockMaxBuf = SB_DATA_SIZE / 4;
    cr_log_info("Socket Buffer data size is %d", SB_DATA_SIZE);

#pragma region Check sane values in new socket buffer

    cr_assert_null(socketBuffer.extension);
    cr_assert_not(socketBuffer.idx);

#pragma endregion

#pragma region Populate some data

    strcpy(junkData, sampleText);
    for (i = 1; i < bufferMax; ++i)
        strcat(junkData, sampleText);

    len = strlen(junkData);

#pragma endregion

#pragma region Create socket buffer extended buffer

    sent1 = socketBufferWriteText(&socketBuffer, junkData);

    cr_assert_eq(socketBuffer.idx, SB_DATA_SIZE);
    cr_assert_not(memcmp(socketBuffer.buffer, junkData, SB_DATA_SIZE));

    cr_assert_eq(sent1, len, "Expected %zu got %lu", len, sent1);
    cr_assert_eq(socketBuffer.idx, SB_DATA_SIZE, "Expected %d got %d", SB_DATA_SIZE, socketBuffer.idx);
    cr_assert_not_null(socketBuffer.extension);
    cr_assert_eq(socketBuffer.extension->i, 0);
    cr_assert_eq(socketBuffer.extension->length, sent1 - mockMaxBuf, "Expected %lu got %lu", sent1 - mockMaxBuf,
                 socketBuffer.extension->length);
    cr_assert_not(memcmp(socketBuffer.extension->data, &junkData[SB_DATA_SIZE / 4], SB_DATA_SIZE - (SB_DATA_SIZE / 4)));

    /* Manual checking info */
    cr_log_info("Pass 1");
    cr_log_info("Mock sent internal buffer is %zu", mockMaxBuf);
    cr_log_info("Socket Buffer data IDX is %d", socketBuffer.idx);
    cr_log_info("Socket Buffer Extension Length is %zu", socketBuffer.extension->length);
    cr_log_info("Reported data sent is %zu", sent1);
    cr_log_info("%d + %zu + %zu = %zu?", socketBuffer.idx, socketBuffer.extension->length, mockMaxBuf, sent1);

#pragma endregion

#pragma region Append to existing socket buffer extension

    sent2 = socketBufferWriteText(&socketBuffer, junkData);

    cr_assert_not_null(socketBuffer.extension);
    cr_assert_eq(socketBuffer.extension->i, 0);
    cr_assert_eq(socketBuffer.extension->length, sent1 + sent2 - mockMaxBuf, "Expected %lu got %lu",
                 sent1 + sent2 - mockMaxBuf, socketBuffer.extension->length);

    /* Manual checking info */
    cr_log_info("Pass 2");
    cr_log_info("Mock sent internal buffer is %zu", mockMaxBuf);
    cr_log_info("Socket Buffer data IDX is %d", socketBuffer.idx);
    cr_log_info("Socket Buffer Extension Length is %zu", socketBuffer.extension->length);
    cr_log_info("Reported data sent is %zu", sent2);
    cr_log_info("%d + %zu + %zu = %zu?", socketBuffer.idx, socketBuffer.extension->length, mockMaxBuf, sent2);

    sent3 = socketBufferWriteData(&socketBuffer, "", 1);
    cr_assert_eq(socketBuffer.extension->length, sent1 + sent2 + sent3 - mockMaxBuf, "Expected %lu got %lu",
                 sent1 + sent2 + sent3 - mockMaxBuf, socketBuffer.extension->length);

#pragma endregion
}

Test(ExtendedMemory, Append) {
    size_t i, bufferMax = ((SB_DATA_SIZE * 2) / 32) - 1;
    char junkData[SB_DATA_SIZE * 2] = "";
    MemoryPool *memoryPool = NULL;

    strcpy(junkData, sampleText);
    for (i = 1; i < bufferMax; ++i)
        strcat(junkData, sampleText);

    i = strlen(junkData);

    memoryPool = socketBufferMemoryPoolNew(junkData, i);

    cr_assert_not_null(memoryPool);
    cr_assert_not_null(memoryPool->data);
    cr_assert_eq(memoryPool->i, 0);
    cr_assert_eq(memoryPool->length, i);

    free(memoryPool->data), free(memoryPool), memoryPool = NULL;

    memoryPool = socketBufferMemoryPoolAppend(memoryPool, junkData, i);

    cr_assert_not_null(memoryPool);
    cr_assert_not_null(memoryPool->data);
    cr_assert_eq(memoryPool->i, 0);
    cr_assert_eq(memoryPool->length, i);

    memoryPool = socketBufferMemoryPoolAppend(memoryPool, junkData, i);

    cr_assert_not_null(memoryPool);
    cr_assert_not_null(memoryPool->data);
    cr_assert_eq(memoryPool->i, 0);
    cr_assert_eq(memoryPool->length, i * 2);

    free(memoryPool->data), free(memoryPool);
}
