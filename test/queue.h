#ifndef NEW_DL_TEST_QUEUE_H
#define NEW_DL_TEST_QUEUE_H

#include "../src/client/err.h"
#include "../src/client/queue.h"
#include "../src/client/uri.h"

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
 * This test checks ioHttpResponseHeaderEssential returns in a sensible way even when header data is complete non-sense
 */
static void HttpHeaderGetEssential(void **state) {
    const char *minHeader = "HTTP/1.1 200 OK" HTTP_EOL HTTP_EOL, *invalidHeader1 = "Hello", *invalidHeader2 = "World" HTTP_EOL HTTP_EOL, *emptyHeader = HTTP_EOL HTTP_EOL;
    char *scheme, *response = scheme = NULL;
    assert_null(ioHttpResponseHeaderEssential(minHeader, &scheme, &response));
    assert_non_null(scheme);
    assert_non_null(response);
    assert_memory_equal(scheme, "HTTP/1.1", 9);
    assert_memory_equal(response, "200 OK", 7);
    free(scheme);

    assert_non_null(ioHttpResponseHeaderEssential(invalidHeader1, &scheme, &response));
    assert_null(scheme);
    assert_null(response);

    assert_non_null(ioHttpResponseHeaderEssential(invalidHeader2, &scheme, &response));
    assert_null(scheme);
    assert_null(response);

    assert_non_null(ioHttpResponseHeaderEssential(emptyHeader, &scheme, &response));
    assert_null(scheme);
    assert_null(response);
}

/**
 * This test checks ioHttpResponseHeaderFind can find and extract HTTP header property values as their own strings
 */
static void HttpHeaderFind(void **state) {
    const char *header = "HTTP/1.1 200 OK" HTTP_EOL
                         "Date: Wed, 12 Aug 1981 00:00:00 GMT" HTTP_EOL
                         "Content-Length: 5150" HTTP_EOL;
    char *value = NULL;

    assert_null(ioHttpResponseHeaderFind(header, HTTP_EOL "Date", &value));
    assert_non_null(value);
    assert_string_equal(value, "Wed, 12 Aug 1981 00:00:00 GMT");
    free(value), value = NULL;

    assert_null(ioHttpResponseHeaderFind(header, HTTP_EOL "Content-Length", &value));
    assert_non_null(value);
    assert_string_equal(value, "5150");
    free(value), value = NULL;

    assert_non_null(ioHttpResponseHeaderFind(header, HTTP_EOL "Content-Disposition", &value));
    assert_null(value);
}

/**
 * This test checks ioHttpBodyChunkStrip can parse a chuck body
 */
static void HttpChunk(void **state) {
    char sample[] = "4" HTTP_EOL "This" HTTP_EOL "3" HTTP_EOL " is" HTTP_EOL "2" HTTP_EOL" a" HTTP_EOL "5" HTTP_EOL " test" HTTP_EOL "0" HTTP_EOL;
    size_t len = -1, max = strlen(sample);

    assert_null(ioHttpBodyChunkStrip((char *) &sample, &max, &len));
    assert_int_equal(len, -1);
    assert_int_equal(max, 14);
    assert_memory_equal(sample, "This is a test", max);
}

/**
 * This test checks the behaviour of ioHttpBodyChunkStrip when the stream hangs mid-chunk
 */
static void HttpChunkPartial(void **state) {
    char sample1[] = "4" HTTP_EOL "This" HTTP_EOL "3" HTTP_EOL " i", sample2[] = "s" HTTP_EOL "2" HTTP_EOL" a" HTTP_EOL "5" HTTP_EOL " test" HTTP_EOL "0" HTTP_EOL;
    size_t len = -1, max = strlen(sample1);

    assert_null(ioHttpBodyChunkStrip((char *) &sample1, &max, &len));
    assert_int_equal(len, 1);
    assert_int_equal(max, 6);
    assert_memory_equal(sample1, "This i", max), max = strlen(sample2);
    assert_null(ioHttpBodyChunkStrip((char *) &sample2, &max, &len));
    assert_int_equal(len, -1);
    assert_int_equal(max, 8);
    assert_memory_equal(sample2, "s a test", max);
}

/**
 * This test checks the behaviour of ioHttpBodyChunkStrip when the encoding is corrupt and suggests an overrun
 */
static void HttpChunkPartialOverBufferEnd(void **state) {
    char sample1[] = "4" HTTP_EOL "This" HTTP_EOL "3" HTTP_EOL " is" HTTP_EOL "2", sample2[] = HTTP_EOL "2" HTTP_EOL " a" HTTP_EOL "5" HTTP_EOL " test" HTTP_EOL "0" HTTP_EOL;
    size_t len = -1, max = strlen(sample1);

    assert_ptr_equal(ioHttpBodyChunkStrip((char *) &sample1, &max, &len), ErrChunkMetadataOverflowsBuffer);
    assert_int_equal(len, 0);
    assert_int_equal(max, 7);
    assert_memory_equal(sample1, "This is", max), max = strlen(sample2);
    assert_null(ioHttpBodyChunkStrip((char *) &sample2, &max, &len));
    assert_int_equal(len, -1);
    assert_int_equal(max, 7);
    assert_memory_equal(sample2, " a test", max);
}

/**
 * This test checks the behaviour of ioHttpBodyChunkStrip when the last chunk is reached
 */
static void HttpChunkLast(void **state) {
    char sample[] = "0" HTTP_EOL;
    size_t len = -1, max = strlen(sample);

    assert_null(ioHttpBodyChunkStrip((char *) &sample, &max, &len));
    assert_int_equal(len, -1);
    assert_int_equal(max, 0);
}

#ifdef MOCK

/**
 * This test checks that queueEntryArrayAppend() works correctly
 */
static void QueueEntryArrayAppend(void **state) {
    Site from, to;
    QueueEntryArray *entryArray = NULL;
    QueueEntry queueEntry = {0};
    void *p = platformTempFilePath("nt_f1");
    const char *head = "HTTP/1.1 200 OK" HTTP_EOL
                       "Content-Type: text/html; charset=ISO-8859-1" HTTP_EOL
                       "Date: Thu, 1 Jan 1970 00:00:00 GMT" HTTP_EOL
                       "Length: 0" HTTP_EOL HTTP_EOL;

    mockReset(), mockOptions = MOCK_CONNECT | MOCK_SEND | MOCK_RECEIVE, mockSendMaxBuf = mockReceiveMaxBuf = 1024;
    assert_non_null(mockReceiveStream = fopen(p, "wb+")), free(p);
    assert_int_equal(fwrite(head, 1, strlen(head), mockReceiveStream), strlen(head)), rewind(mockReceiveStream);

    #pragma region Create Sites to give random pointer values to that queue entry can use

    assert_null(siteNew(&from, SITE_HTTP, "http://127.0.0.1")), fclose(mockReceiveStream), mockReceiveStream = NULL;
    assert_non_null(p = platformPathSystemToFileScheme((char *) platformTempDirectoryGet()));
    assert_null(siteNew(&to, SITE_FILE, p)), free(p), siteFree(&to), siteFree(&from);

    #pragma endregion

    #pragma region Create the first entry

    queueEntry.sourceSite = &from, queueEntry.destinationSite = &to;
    queueEntry.sourcePath = "foo", queueEntry.destinationPath = "bar";
    queueEntry.state = 0;

    assert_null(queueEntryArrayAppend(&entryArray, &queueEntry));
    assert_non_null(entryArray), assert_int_equal(entryArray->len, 1), assert_non_null(entryArray->entry);
    assert_memory_equal(&entryArray->entry[0], &queueEntry, sizeof(QueueEntry));

    #pragma endregion

    #pragma region Create the second entry

    queueEntry.sourceSite = &to, queueEntry.destinationSite = &from;
    queueEntry.sourcePath = "bar", queueEntry.destinationPath = "foo";
    queueEntry.state = 1;

    assert_null(queueEntryArrayAppend(&entryArray, &queueEntry));
    assert_non_null(entryArray), assert_int_equal(entryArray->len, 2), assert_non_null(entryArray->entry);
    assert_memory_equal(&entryArray->entry[1], &queueEntry, sizeof(QueueEntry));

    #pragma endregion

    #pragma region Create the third entry

    queueEntry.state = 2, queueEntry.sourceSite = &from;

    assert_null(queueEntryArrayAppend(&entryArray, &queueEntry));
    assert_non_null(entryArray), assert_int_equal(entryArray->len, 3), assert_non_null(entryArray->entry);
    assert_memory_equal(&entryArray->entry[2], &queueEntry, sizeof(QueueEntry));

    #pragma endregion

    #pragma region Check intergrety of second entry

    queueEntry.state = 1, queueEntry.sourceSite = &to;
    assert_memory_equal(&entryArray->entry[1], &queueEntry, sizeof(QueueEntry));

    #pragma endregion

    #pragma region Check no duplicates are appended

    assert_null(queueEntryArrayAppend(&entryArray, &queueEntry));
    assert_int_equal(entryArray->len, 3);

    #pragma endregion

    #pragma region Free all memory in entryArray

    p = entryArray;
    {
        size_t i;
        for (i = 0; i < entryArray->len; ++i)
            entryArray->entry[i].sourcePath = entryArray->entry[i].destinationPath = NULL;
    }

    queueEntryArrayFree(&entryArray);
    assert_ptr_equal(p, mockLastFree);
    assert_null(entryArray);

    #pragma endregion
}

/**
 * This test checks that queueEntryArrayAppend() works correctly
 */
static void QueueEntryFromUser(void **state) {
    SiteArray siteArray = {0};
    Site from, to;
    QueueEntry queueEntry = {0};
    void *p = platformTempFilePath("nt_f1");
    const char *head = "HTTP/1.1 200 OK" HTTP_EOL
                       "Content-Type: text/html; charset=ISO-8859-1" HTTP_EOL
                       "Date: Thu, 1 Jan 1970 00:00:00 GMT" HTTP_EOL
                       "Length: 0" HTTP_EOL HTTP_EOL;

    mockReset(), mockOptions = MOCK_CONNECT | MOCK_SEND | MOCK_RECEIVE, mockSendMaxBuf = mockReceiveMaxBuf = 1024;
    assert_non_null(mockReceiveStream = fopen(p, "wb+")), free(p);
    assert_int_equal(fwrite(head, 1, strlen(head), mockReceiveStream), strlen(head)), rewind(mockReceiveStream);

    #pragma region Create Sites to give random pointer values to that queue entry can use

    assert_null(siteNew(&from, SITE_HTTP, "http://127.0.0.1")), fclose(mockReceiveStream), mockReceiveStream = NULL;
    assert_non_null(p = platformPathSystemToFileScheme((char *) platformTempDirectoryGet()));
    assert_null(siteNew(&to, SITE_FILE, p)), free(p);

    #pragma endregion

    #pragma region Add Sites to SiteArray

    siteArrayAdd(&siteArray, &from);
    siteArrayAdd(&siteArray, &to);

    #pragma endregion

    #pragma region Add/compare absolue uri entries

    assert_null(queueEntryNewFromPath(&queueEntry, &siteArray, "http://127.0.0.1/foo", "file:///bar"));
    assert_int_equal(queueEntry.state, QUEUE_STATE_QUEUED);
    assert_memory_equal(queueEntry.sourceSite, &from, sizeof(Site));
    assert_memory_equal(queueEntry.destinationSite, &to, sizeof(Site));
    assert_string_equal(queueEntry.sourcePath, "/foo");
    assert_string_equal(queueEntry.destinationPath, "/bar");

    #pragma endregion

    #pragma region Cleanup
    queueEntryFree(&queueEntry);
    siteArrayFree(&siteArray);
    #pragma endregion

}

#endif /* MOCK */

const struct CMUnitTest queueTest[] = {
    cmocka_unit_test(HttpHeaderFind),
    cmocka_unit_test(HttpHeaderGetEssential),
    cmocka_unit_test(HttpChunk),
    cmocka_unit_test(HttpChunkLast),
    cmocka_unit_test(HttpChunkPartial),
    cmocka_unit_test(HttpChunkPartialOverBufferEnd),
#ifdef MOCK
    cmocka_unit_test(QueueEntryArrayAppend),
    cmocka_unit_test(QueueEntryFromUser),
#endif /* MOCK */
};

#endif /* NEW_DL_TEST_QUEUE_H */
