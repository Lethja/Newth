#ifndef NEW_TH_TEST_REQUEST_H
#define NEW_TH_TEST_REQUEST_H

#include "common.h"
#include "../src/server/server.h"
#include "../src/platform/platform.h"
#include <string.h>

#pragma clang diagnostic push
#pragma ide diagnostic ignored "UnusedParameter"

#pragma region CMocka headers

#include <stdarg.h>
#include <stddef.h>
#include <stdint.h>
#include <setjmp.h>

#include <cmocka.h>
#include <unistd.h>

#pragma endregion

/**
 * This test ensures that a small file can be sent through servers state machine correctly
 */
static void RequestSmallFile(void **state) {
    const char *header = "GET / HTTP/1.1" HTTP_EOL HTTP_EOL;
    char *p1 = platformTempFilePath("nt_RequestSmallFile_File.txt"), *p2 = platformTempFilePath("nt_RequestSmallFile_Send.txt"), *fdStr;
    FILE *read = fopen(p1, "w+b");
    mockSendStream = fopen(p2, "w+b"), free(p2);

    mockOptions = MOCK_SEND, mockSendMaxBuf = BUFSIZ, WriteSampleFile(read, SB_DATA_SIZE);

    assert_non_null(globalRootPath = malloc(FILENAME_MAX));
    fdStr = strrchr(p1, '/'), strncpy(globalRootPath, p1, FILENAME_MAX), globalRootPath[fdStr - p1] = '\0';
    ++fdStr;

    assert_false(handlePath(0, header, fdStr));

    do {
        RoutineTick(&globalRoutineArray);
    } while (globalRoutineArray.size);

    rewind(read), FindHttpBodyStart(mockSendStream);

    CompareStreams(state, read, mockSendStream);

    free(p1), fclose(read), free(globalRootPath), globalRootPath = NULL, mockReset();
}

/**
 * This test ensures that a request asking for a byte range to resume a download of a file is handled correctly
 */
static void RequestResumeFile(void **state) {
    const char *header = "GET / HTTP/1.1" HTTP_EOL "Range: bytes=50-" HTTP_EOL HTTP_EOL;
    char *p1 = platformTempFilePath("nt_RequestResumeFile_File.txt"), *p2 = platformTempFilePath("nt_RequestResumeFile_Send.txt"), *fdStr;
    FILE *read = fopen(p1, "w+b");
    mockSendStream = fopen(p2, "w+b"), free(p2);

    mockOptions = MOCK_SEND, mockSendMaxBuf = BUFSIZ, WriteSampleFile(read, SB_DATA_SIZE);

    assert_non_null(globalRootPath = malloc(FILENAME_MAX));
    fdStr = strrchr(p1, '/'), strncpy(globalRootPath, p1, FILENAME_MAX), globalRootPath[fdStr - p1] = '\0';
    ++fdStr;

    assert_false(handlePath(0, header, fdStr));

    do {
        RoutineTick(&globalRoutineArray);
    } while (globalRoutineArray.size);

    platformFileSeek(read, 50, SEEK_SET), FindHttpBodyStart(mockSendStream);

    CompareStreams(state, read, mockSendStream);

    free(p1), fclose(read), free(globalRootPath), globalRootPath = NULL, mockReset();
}

/**
 * This test ensures that a request being interrupted immediately after request is handled gracefully by the server
 */
static void TransferInterruptStart(void **state) {
    const char *header = "GET / HTTP/1.1" HTTP_EOL HTTP_EOL;
    char *p1 = platformTempFilePath("nt_TransferInterruptStart_File.txt"), *p2 = platformTempFilePath("nt_TransferInterruptStart_Send.txt"), *fdStr;
    FILE *read = fopen(p1, "w+b");
    mockSendStream = fopen(p2, "w+b"), free(p2);

    mockOptions = MOCK_SEND, mockSendMaxBuf = BUFSIZ, WriteSampleFile(read, SB_DATA_SIZE), mockSendError = EPIPE;

    assert_non_null(read);
    assert_non_null(globalRootPath = malloc(FILENAME_MAX));
    fdStr = strrchr(p1, '/'), strncpy(globalRootPath, p1, FILENAME_MAX), globalRootPath[fdStr - p1] = '\0';
    ++fdStr;

    assert_false(handlePath(0, header, fdStr));

    do {
        RoutineTick(&globalRoutineArray);
    } while (globalRoutineArray.size);

    assert_false(platformFileTell(mockSendStream));

    free(p1), fclose(read), free(globalRootPath), globalRootPath = NULL, mockReset(), mockSendError = ENOERR;
}

/**
 * This test ensures that a request being interrupted in the middle of a request is handled gracefully by the server
 */
static void TransferInterruptMiddle(void **state) {
    const char *header = "GET / HTTP/1.1" HTTP_EOL HTTP_EOL;
    char *p1 = platformTempFilePath("nt_TransferInterruptMiddle_File.txt"), *p2 = platformTempFilePath("nt_TransferInterruptMiddle_Send.txt"), *fdStr;
    FILE *read = fopen(p1, "w+b");
    int i = 0;
    mockSendStream = fopen(p2, "w+b"), free(p2);

    mockOptions = MOCK_SEND, mockSendMaxBuf = 10, WriteSampleFile(read, SB_DATA_SIZE);

    assert_non_null(read);
    assert_non_null(globalRootPath = malloc(FILENAME_MAX));
    fdStr = strrchr(p1, '/'), strncpy(globalRootPath, p1, FILENAME_MAX), globalRootPath[fdStr - p1] = '\0';
    ++fdStr;

    assert_int_equal(platformFileTell(mockSendStream), 0);

    assert_false(handlePath(0, header, fdStr)); /* +10 bytes */

    assert_int_equal(platformFileTell(mockSendStream), 10);

    do {
        RoutineTick(&globalRoutineArray); /* +10 bytes */

        if (i == 3)
            mockSendError = EPIPE;
        else
            ++i;

    } while (globalRoutineArray.size);

    assert_int_equal(platformFileTell(mockSendStream), 50);

    free(p1), fclose(read), free(globalRootPath), globalRootPath = NULL, mockReset(), mockSendError = ENOERR;
}

#pragma clang diagnostic pop

const struct CMUnitTest requestTest[] = {
    cmocka_unit_test(RequestSmallFile),
    cmocka_unit_test(RequestResumeFile),
    cmocka_unit_test(TransferInterruptStart),
    cmocka_unit_test(TransferInterruptMiddle)
};

#endif /* NEW_TH_TEST_REQUEST_H */
