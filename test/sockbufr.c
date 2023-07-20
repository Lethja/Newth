#include <criterion/criterion.h>
#include "../src/server/sockbufr.h"

static char *sampleText = "Glum-Schwartzkopf,vex'd_by NJ&IQ"; /* 32 characters, each unique */

Test(Mockup, Send) {
    size_t i, bufferMax = ((BUFSIZ * 2) / 32) - 1, sent;
    char junkData[BUFSIZ * 2] = "";

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
    size_t i, bufferMax = ((BUFSIZ * 2) / 32) - 1, sent;
    char junkData[BUFSIZ * 2] = "";
    SocketBuffer socketBuffer = socketBufferNew(0, SOC_BUF_OPT_EXTEND);
    mockMaxBuf = BUFSIZ / 4;

#pragma region Check sane values in new socket buffer

    cr_assert_null(socketBuffer.extension);
    cr_assert_not(socketBuffer.idx);

#pragma endregion

#pragma region Populate some data

    strcpy(junkData, sampleText);
    for (i = 1; i < bufferMax; ++i)
        strcat(junkData, sampleText);

#pragma endregion

#pragma region Create socket buffer extended buffer

    sent = socketBufferWriteText(&socketBuffer, junkData);

    cr_assert_eq(socketBuffer.idx, BUFSIZ);
    cr_assert_not(memcmp(socketBuffer.buffer, junkData, BUFSIZ));

    cr_assert_eq(sent, BUFSIZ, "Expected %d got %lu", BUFSIZ, sent);
    cr_assert_eq(socketBuffer.idx, BUFSIZ, "Expected %d got %d", BUFSIZ, socketBuffer.idx);
    cr_assert_not_null(socketBuffer.extension);
    cr_assert_eq(socketBuffer.extension->i, 0);
    cr_assert_eq(socketBuffer.extension->length, sent - mockMaxBuf, "Expected %lu got %lu", sent - mockMaxBuf,
                 socketBuffer.extension->length);
    cr_assert_not(memcmp(socketBuffer.extension->data, &junkData[BUFSIZ / 4], BUFSIZ - (BUFSIZ / 4)));

#pragma endregion

#pragma region Append to existing socket buffer extension

    sent += socketBufferWriteText(&socketBuffer, junkData);

    cr_assert_not_null(socketBuffer.extension);
    cr_assert_eq(socketBuffer.extension->i, 0);
    cr_assert_eq(socketBuffer.extension->length, sent - mockMaxBuf, "Expected %lu got %lu", sent - mockMaxBuf,
                 socketBuffer.extension->length);

    sent = socketBuffer.extension->length;

    socketBufferWriteData(&socketBuffer, "", 1);
    cr_assert_eq(socketBuffer.extension->length, sent + 1, "Expected %lu got %lu", sent + 1,
                 socketBuffer.extension->length);

#pragma endregion
}

Test(ExtendedMemory, Append) {
    size_t i, bufferMax = ((BUFSIZ * 2) / 32) - 1;
    char junkData[BUFSIZ * 2] = "";
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
