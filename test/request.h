#ifndef NEW_TH_TEST_REQUEST_H
#define NEW_TH_TEST_REQUEST_H

#include "common.h"
#include "../src/server/server.h"
#include "../src/platform/platform.h"
#include <string.h>

#pragma region CMocka headers

#include <stdarg.h>
#include <stddef.h>
#include <stdint.h>
#include <setjmp.h>

#include <cmocka.h>
#include <unistd.h>

#pragma endregion

static void RequestSmallFile(void **state) {
    const char *header = "GET / HTTP/1.1" HTTP_EOL HTTP_EOL;
    char path[FILENAME_MAX] = "/tmp/nt_RequestSmallFile_File.txt", *fdStr;
    FILE *read = fopen(path, "w+b");
    mockSendStream = fopen("/tmp/nt_RequestSmallFile_Send.txt", "w+b");

    mockOptions = MOCK_SEND, mockSendMaxBuf = BUFSIZ, writeSampleFile(read, SB_DATA_SIZE);

    globalRootPath = malloc(FILENAME_MAX);
    fdStr = strrchr(path, '/'), strncpy(globalRootPath, path, FILENAME_MAX), globalRootPath[fdStr - path] = '\0';
    ++fdStr;

    assert_false(handlePath(0, header, fdStr));

    do {
        RoutineTick(&globalRoutineArray);
    } while (globalRoutineArray.size);

    rewind(read), findHttpBodyStart(mockSendStream);

    CompareStreams(state, read, mockSendStream);

    fclose(read), free(globalRootPath), globalRootPath = NULL, mockReset();
}

static void RequestResumeFile(void **state) {
    const char *header = "GET / HTTP/1.1" HTTP_EOL "Range: bytes=50-" HTTP_EOL HTTP_EOL;
    char path[FILENAME_MAX] = "/tmp/nt_RequestResumeFile_File.txt", *fdStr;
    FILE *read = fopen(path, "w+b");
    mockSendStream = fopen("/tmp/nt_RequestResumeFile_Send.txt", "w+b");

    mockOptions = MOCK_SEND, mockSendMaxBuf = BUFSIZ, writeSampleFile(read, SB_DATA_SIZE);

    globalRootPath = malloc(FILENAME_MAX);
    fdStr = strrchr(path, '/'), strncpy(globalRootPath, path, FILENAME_MAX), globalRootPath[fdStr - path] = '\0';
    ++fdStr;

    assert_false(handlePath(0, header, fdStr));

    do {
        RoutineTick(&globalRoutineArray);
    } while (globalRoutineArray.size);

    platformFileSeek(read, 50, SEEK_SET), findHttpBodyStart(mockSendStream);

    CompareStreams(state, read, mockSendStream);

    fclose(read), free(globalRootPath), globalRootPath = NULL, mockReset();
}

static void TransferInterruptStart(void **state) {
    const char *header = "GET / HTTP/1.1" HTTP_EOL HTTP_EOL;
    char path[FILENAME_MAX] = "/tmp/nt_TransferInterruptStart_File.txt", *fdStr;
    FILE *read = fopen(path, "w+b");
    mockSendStream = fopen("/tmp/nt_TransferInterruptStart_Send.txt", "w+b");

    mockOptions = MOCK_SEND, mockSendMaxBuf = BUFSIZ, writeSampleFile(read, SB_DATA_SIZE), mockSendError = EPIPE;

    globalRootPath = malloc(FILENAME_MAX);
    fdStr = strrchr(path, '/'), strncpy(globalRootPath, path, FILENAME_MAX), globalRootPath[fdStr - path] = '\0';
    ++fdStr;

    assert_false(handlePath(0, header, fdStr));

    do {
        RoutineTick(&globalRoutineArray);
    } while (globalRoutineArray.size);

    assert_false(platformFileTell(mockSendStream));

    fclose(read), free(globalRootPath), globalRootPath = NULL, mockReset(), mockSendError = ENOERR;
}

static void TransferInterruptMiddle(void **state) {
    const char *header = "GET / HTTP/1.1" HTTP_EOL HTTP_EOL;
    char path[FILENAME_MAX] = "/tmp/nt_TransferInterruptMiddle_File.txt", *fdStr;
    FILE *read = fopen(path, "w+b");
    int i = 0;
    mockSendStream = fopen("/tmp/nt_TransferInterruptMiddle_Send.txt", "w+b");

    mockOptions = MOCK_SEND, mockSendMaxBuf = 10, writeSampleFile(read, SB_DATA_SIZE);

    globalRootPath = malloc(FILENAME_MAX);
    fdStr = strrchr(path, '/'), strncpy(globalRootPath, path, FILENAME_MAX), globalRootPath[fdStr - path] = '\0';
    ++fdStr;

    assert_false(handlePath(0, header, fdStr));

    do {
        RoutineTick(&globalRoutineArray);

        if (i == 3)
            mockSendError = EPIPE;
        else
            ++i;

    } while (globalRoutineArray.size);

    assert_int_equal(platformFileTell(mockSendStream), 40);

    fclose(read), free(globalRootPath), globalRootPath = NULL, mockReset(), mockSendError = ENOERR;
}

#pragma clang diagnostic pop

const struct CMUnitTest requestTest[] = {cmocka_unit_test(RequestSmallFile), cmocka_unit_test(RequestResumeFile),
                                         cmocka_unit_test(TransferInterruptStart),
                                         cmocka_unit_test(TransferInterruptMiddle)};

#endif /* NEW_TH_TEST_REQUEST_H */
