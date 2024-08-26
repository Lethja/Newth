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

const struct CMUnitTest queueTest[] = {
    cmocka_unit_test(HttpHeaderFind),
    cmocka_unit_test(HttpHeaderGetEssential),
    cmocka_unit_test(HttpChunk),
    cmocka_unit_test(HttpChunkLast),
    cmocka_unit_test(HttpChunkPartial),
    cmocka_unit_test(HttpChunkPartialOverBufferEnd),
};

#endif /* NEW_DL_TEST_QUEUE_H */
