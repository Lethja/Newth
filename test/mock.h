#ifndef NEW_TH_TEST_MOCK_H
#define NEW_TH_TEST_MOCK_H

#include "common.h"
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

#pragma endregion

static void MockMalloc(void **state) {
    void *test;
    mockReset();

    assert_non_null((test = malloc(1)));
    free(test);
    mockOptions = MOCK_ALLOC_NO_MEMORY;
    assert_null((test = malloc(1)));
    assert_int_equal(platformSocketGetLastError(), ENOMEM);
    if (test)
        free(test);
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
    assert_int_equal(platformSocketGetLastError(), ENOMEM);
    assert_non_null(new);

    if (!test)
        free(new);
    else
        free(test);
}

static void MockReceive(void **state) {
    size_t i = SB_DATA_SIZE * 2, received;
    char junkData[SB_DATA_SIZE * 2] = "";

    mockReset(), mockOptions = MOCK_RECEIVE, mockReceiveMaxBuf = i;

    /* Receive should return the amount of data we ask of it */
    received = recv(0, junkData, 5, 0);
    assert_int_equal(received, 5);
    assert_memory_equal(junkData, "12345", sizeof(char) * 5);

    /* The amount of data should be how much was sent this call and not accumulate */
    received = recv(0, junkData, 5, 0);
    assert_int_equal(received, 5);
    assert_memory_equal(junkData, "12345", sizeof(char) * 5);

    /* When the maximum is reached it should return as much as it can fit */
    mockReceiveMaxBuf -= 10;
    received = recv(0, junkData, i, 0);
    assert_int_equal(received, mockReceiveMaxBuf);

    /* If there's no room left a non-blocking socket should return -1 and set EAGAIN/EWOULDBLOCK */
    mockReceiveMaxBuf = 0;
    received = recv(0, junkData, i, 0);
    assert_int_equal(received, -1);
    assert_int_equal(platformSocketGetLastError(), EAGAIN);
}

static void MockReceiveStream(void **state) {
    size_t i, received;
    char junkData[SB_DATA_SIZE] = "";
    const char *SampleData = "Lorem ipsum dolor sit amet, consectetur adipiscing elit,"
                             " sed do eiusmod tempor incididunt ut labore et dolore magna aliqua."
                             " Urna porttitor rhoncus dolor purus non enim praesent elementum facilisis."
                             " Cras sed felis eget velit aliquet sagittis id consectetur.";

    mockReset(), mockOptions = MOCK_RECEIVE, mockReceiveMaxBuf = SB_DATA_SIZE, i = strlen(
            SampleData), mockReceiveData = (char *) SampleData, mockReceiveDataMax = strlen(SampleData);

    /* Receive should return the amount of data we ask of it */
    received = recv(0, junkData, 11, 0);
    assert_int_equal(received, 11);
    assert_memory_equal(junkData, "Lorem ipsum", sizeof(char) * 11);

    /* Receive should continue from current stream position */
    received = recv(0, junkData, 15, 0);
    assert_int_equal(received, 15);
    assert_memory_equal(junkData, " dolor sit amet", sizeof(char) * 15);

    /* Receive the maximum */
    received = recv(0, junkData, SB_DATA_SIZE, 0);
    assert_int_equal(received, SB_DATA_SIZE);
    assert_memory_equal(junkData, &SampleData[i - (i - 15 - 11)], SB_DATA_SIZE);

    /* Receive less than requested */
    received = recv(0, junkData, SB_DATA_SIZE, 0);
    assert_int_equal(received, i - 15 - 11 - SB_DATA_SIZE);
    assert_memory_equal(junkData, &SampleData[i - (i - 15 - 11 - SB_DATA_SIZE)], received);

    /* No more */
    received = recv(0, junkData, SB_DATA_SIZE, 0);
    assert_int_equal(received, 0);
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

static void MockHttpBodyFindStart(void **state) {
    const char *http = "HTTP/1.1 200 OK\n"
                       "Date: Thu, 27 Jul 2023 05:57:33 GMT" HTTP_EOL
                       "Content-Type: text/html; charset=utf-8" HTTP_EOL
                       "Content-Length: 6434" HTTP_EOL HTTP_EOL;
    const char *body = "<!DOCTYPE HTML>" HTTP_EOL
                       "<html>" HTTP_EOL HTTP_EOL
                       "<head>\n"
                       "<title>There are no easter eggs here</title>\n"
                       "</head>\n"
                       "</html>";

    FILE *tmpFile1 = tmpfile(), *tmpFile2;
    long correctPosition, testPosition;

    assert_non_null(tmpFile1), fwrite(http, strlen(http), 1, tmpFile1), correctPosition = ftell(tmpFile1);

    fwrite(body, strlen(body), 1, tmpFile1);
    testPosition = findHttpBodyStart(tmpFile1);
    assert_int_equal(correctPosition, testPosition);

    assert_non_null(tmpFile2 = tmpfile()), fwrite(body, 1, strlen(body), tmpFile2), rewind(tmpFile2);
    CompareStreams(state, tmpFile1, tmpFile2);

    fclose(tmpFile1), fclose(tmpFile2);
}

#pragma clang diagnostic pop

const struct CMUnitTest mockTest[] = {cmocka_unit_test(MockHttpBodyFindStart), cmocka_unit_test(MockMalloc),
                                      cmocka_unit_test(MockRealloc), cmocka_unit_test(MockReceive),
                                      cmocka_unit_test(MockReceiveStream), cmocka_unit_test(MockSend)};

#endif /* NEW_TH_TEST_MOCK_H */
