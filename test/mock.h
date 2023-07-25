#ifndef NEW_TH_TEST_MOCK_H
#define NEW_TH_TEST_MOCK_H

#include "common.h"
#include "../src/platform/platform.h"
#include <string.h>

#pragma region CMocka headers

#include <stdarg.h>
#include <stddef.h>
#include <stdint.h>
#include <setjmp.h>

#include <cmocka.h>

#pragma endregion

static void MockMalloc(void **state) {
    void *test;
    mockReset();

    test = malloc(1);
    assert_non_null(test);
    free(test);
    mockOptions = MOCK_ALLOC_NO_MEMORY;
    test = malloc(1);
    assert_null(test);
}

static void MockRealloc(void **state) {
    void *test, *new;
    mockReset();

    test = malloc(1);
    assert_non_null(test);
    new = realloc(test, 2);
    assert_non_null(new);
    mockOptions = MOCK_ALLOC_NO_MEMORY;
    test = realloc(new, 3);
    assert_null(test);
    assert_non_null(new);

    if (!test)
        free(new);
    else
        free(test);
}

static void MockSend(void **state) {
    size_t i, bufferMax = ((SB_DATA_SIZE * 2) / 32) - 1, sent;
    char junkData[SB_DATA_SIZE * 2] = "";

    strcpy(junkData, sampleText);
    for (i = 1; i < bufferMax; ++i)
        strcat(junkData, sampleText);

    i = strlen(junkData), mockReset(), mockOptions = MOCK_SEND, mockSendMaxBuf = i;

    /* Send should return the amount of data we give it */
    sent = send(0, junkData, 5, 0);
    assert_int_equal(sent, 5);

    /* The amount of data should be how much was sent this call and not accumulate */
    sent = send(0, junkData, 5, 0);
    assert_int_equal(sent, 5);

    /* When the maximum is reached it should return as much as it can fit */
    mockSendMaxBuf -= 10;
    sent = send(0, junkData, i, 0);
    assert_int_equal(sent, mockSendMaxBuf);

    /* If there's no room left a non-blocking socket should return -1 and set EAGAIN/EWOULDBLOCK */
    mockSendMaxBuf = 0;
    sent = send(0, junkData, i, 0);
    assert_int_equal(sent, -1);
    assert_int_equal(platformSocketGetLastError(), EAGAIN);
}

const struct CMUnitTest mockTest[] = {cmocka_unit_test(MockMalloc), cmocka_unit_test(MockRealloc),
                                      cmocka_unit_test(MockSend)};

#endif /* NEW_TH_TEST_MOCK_H */
