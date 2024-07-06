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
 * This test checks that the queue can be cleared
 */
static void QueueClear(void **state) {
    const char *Uri = "http://localhost/foo";
    SocketAddress address;
    UriDetails details = uriDetailsNewFrom(Uri);
    assert_int_equal(addressQueueGetNth(), 0);
    addressQueueClear(); /* Test early return when already cleared */
    assert_int_equal(addressQueueGetNth(), 0);

    assert_null(uriDetailsCreateSocketAddress(&details, &address, SCHEME_UNKNOWN));
    assert_null(pathQueueAppendOrCreate(&address, details.path));
    uriDetailsFree(&details);

    assert_int_equal(addressQueueGetNth(), 1);
    addressQueueClear();
    assert_int_equal(addressQueueGetNth(), 0);
}

/**
 * This test checks that the queue can be created without being filled with junk data
 */
static void QueueCreate(void **state) {
    const char *Uri = "http://localhost/foo";
    SocketAddress address;
    UriDetails details = uriDetailsNewFrom(Uri);

    memset(&address, 0, sizeof(SocketAddress));

    assert_null(uriDetailsCreateSocketAddress(&details, &address, SCHEME_UNKNOWN));

    assert_null(pathQueueAppendOrCreate(&address, details.path));
    assert_int_equal(addressQueueGetNth(), 1);
    assert_int_equal(pathQueueGetNth(&address), 1);

    assert_null(pathQueueAppendOrCreate(&address, details.path)); /* Intentional for testing */
    uriDetailsFree(&details);

    assert_int_equal(addressQueueGetNth(), 1);
    assert_int_equal(pathQueueGetNth(&address), 1);

    addressQueueClear();
}

/**
 * This test checks that the queue can be searched in
 */
static void QueueFind(void **state) {
    size_t i, j = i = -1;
    const char *Uri1 = "http://localhost/foo", *Uri2 = "ftp://localhost/bar";
    SocketAddress address1, address2;
    UriDetails details1 = uriDetailsNewFrom(Uri1), details2 = uriDetailsNewFrom(Uri2);

    memset(&address1, 0, sizeof(SocketAddress)), memset(&address2, 0, sizeof(SocketAddress));

    assert_null(uriDetailsCreateSocketAddress(&details1, &address1, SCHEME_UNKNOWN));
    assert_null(uriDetailsCreateSocketAddress(&details2, &address2, SCHEME_UNKNOWN));

    assert_null(pathQueueAppendOrCreate(&address1, details1.path));
    assert_null(pathQueueAppendOrCreate(&address2, details2.path));

    assert_non_null(pathQueueFind(&address1, details1.path, NULL, NULL));
    assert_null(pathQueueFind(&address1, details2.path, NULL, NULL));
    assert_non_null(pathQueueFind(&address2, details2.path, &i, &j));
    uriDetailsFree(&details1), uriDetailsFree(&details2);

    assert_int_not_equal(i, -1);
    assert_int_not_equal(j, -1);

    addressQueueClear();
}

/**
 * This test checks that a member of the queue can be removed without leaving junk data behind
 */
static void QueueRemove(void **state) {
    const char *Uri1 = "http://localhost/foo", *Uri2 = "ftp://localhost/barbar", *Uri3 = "ftp://localhost/bar";
    SocketAddress ad1, ad2, ad3;
    UriDetails d1 = uriDetailsNewFrom(Uri1), d2 = uriDetailsNewFrom(Uri2), d3 = uriDetailsNewFrom(Uri3);

    memset(&ad1, 0, sizeof(SocketAddress)), memset(&ad2, 0, sizeof(SocketAddress)), memset(&ad3, 0, sizeof(SocketAddress));

    assert_null(uriDetailsCreateSocketAddress(&d1, &ad1, SCHEME_UNKNOWN));
    assert_null(uriDetailsCreateSocketAddress(&d2, &ad2, SCHEME_UNKNOWN));

    assert_null(pathQueueAppendOrCreate(&ad1, d1.path));
    assert_null(pathQueueAppendOrCreate(&ad2, d2.path));

    assert_non_null(pathQueueFind(&ad1, d1.path, NULL, NULL));
    assert_non_null(pathQueueFind(&ad2, d2.path, NULL, NULL));

    assert_int_equal(addressQueueGetNth(), 2);
    assert_int_equal(pathQueueGetNth(&ad1), 1);
    assert_int_equal(pathQueueGetNth(&ad2), 1);

    assert_null(pathQueueRemove(&ad2, d2.path));

    assert_int_equal(addressQueueGetNth(), 1);
    assert_int_equal(pathQueueGetNth(&ad1), 1);
    assert_int_equal(pathQueueGetNth(&ad2), 0);

    assert_non_null(pathQueueFind(&ad1, d1.path, NULL, NULL));
    assert_null(pathQueueFind(&ad2, d2.path, NULL, NULL));

    assert_null(pathQueueAppendOrCreate(&ad2, d2.path));

    assert_int_equal(addressQueueGetNth(), 2);
    assert_int_equal(pathQueueGetNth(&ad1), 1);
    assert_int_equal(pathQueueGetNth(&ad2), 1);

    assert_null(pathQueueRemove(&ad1, d1.path));

    assert_int_equal(addressQueueGetNth(), 1);
    assert_int_equal(pathQueueGetNth(&ad1), 0);
    assert_int_equal(pathQueueGetNth(&ad2), 1);

    assert_null(pathQueueFind(&ad1, d1.path, NULL, NULL));
    assert_non_null(pathQueueFind(&ad2, d2.path, NULL, NULL));

    assert_null(uriDetailsCreateSocketAddress(&d3, &ad3, SCHEME_UNKNOWN));
    assert_null(pathQueueAppendOrCreate(&ad3, d3.path));

    assert_int_equal(addressQueueGetNth(), 1);
    assert_int_equal(pathQueueGetNth(&ad1), 0);
    assert_int_equal(pathQueueGetNth(&ad2), 2);

    assert_null(pathQueueRemove(&ad2, d2.path));
    uriDetailsFree(&d1), uriDetailsFree(&d2), uriDetailsFree(&d3);

    assert_int_equal(addressQueueGetNth(), 1);
    assert_int_equal(pathQueueGetNth(&ad2), 1);

    addressQueueClear();
}

/**
 * This test checks ioHttpResponseHeaderEssential returns in a sensible way even when header data is complete non-sense
 */
static void HeaderGetEssential(void **state) {
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
static void HeaderFind(void **state) {
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
    cmocka_unit_test(HeaderFind),
    cmocka_unit_test(HeaderGetEssential),
    cmocka_unit_test(HttpChunk),
    cmocka_unit_test(HttpChunkLast),
    cmocka_unit_test(HttpChunkPartial),
    cmocka_unit_test(HttpChunkPartialOverBufferEnd),
    cmocka_unit_test(QueueClear),
    cmocka_unit_test(QueueCreate),
    cmocka_unit_test(QueueFind),
    cmocka_unit_test(QueueRemove)
};

#endif /* NEW_DL_TEST_QUEUE_H */
