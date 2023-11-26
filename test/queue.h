#ifndef NEW_DL_TEST_QUEUE_H
#define NEW_DL_TEST_QUEUE_H

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

static void QueueClear(void **state) {
    const char *Uri = "http://localhost/foo";
    SocketAddress address;
    UriDetails details = uriDetailsNewFrom(Uri);
    assert_int_equal(addressQueueGetNth(), 0);
    addressQueueClear(); /* Test early return when already cleared */
    assert_int_equal(addressQueueGetNth(), 0);

    assert_false(uriDetailsCreateSocketAddress(&details, &address, SCHEME_UNKNOWN));
    assert_null(pathQueueAppendOrCreate(&address, details.path));
    uriDetailsFree(&details);

    assert_int_equal(addressQueueGetNth(), 1);
    addressQueueClear();
    assert_int_equal(addressQueueGetNth(), 0);
}

static void QueueCreate(void **state) {
    const char *Uri = "http://localhost/foo";
    SocketAddress address;
    UriDetails details = uriDetailsNewFrom(Uri);

    memset(&address, 0, sizeof(SocketAddress));

    assert_false(uriDetailsCreateSocketAddress(&details, &address, SCHEME_UNKNOWN));

    assert_null(pathQueueAppendOrCreate(&address, details.path));
    assert_int_equal(addressQueueGetNth(), 1);
    assert_int_equal(pathQueueGetNth(&address), 1);

    assert_null(pathQueueAppendOrCreate(&address, details.path)); /* Intentional for testing */
    uriDetailsFree(&details);

    assert_int_equal(addressQueueGetNth(), 1);
    assert_int_equal(pathQueueGetNth(&address), 1);

    addressQueueClear();
}

static void QueueFind(void **state) {
    size_t i, j = i = -1;
    const char *Uri1 = "http://localhost/foo", *Uri2 = "ftp://localhost/bar";
    SocketAddress address1, address2;
    UriDetails details1 = uriDetailsNewFrom(Uri1), details2 = uriDetailsNewFrom(Uri2);

    memset(&address1, 0, sizeof(SocketAddress)), memset(&address2, 0, sizeof(SocketAddress));

    assert_false(uriDetailsCreateSocketAddress(&details1, &address1, SCHEME_UNKNOWN));
    assert_false(uriDetailsCreateSocketAddress(&details2, &address2, SCHEME_UNKNOWN));

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

static void QueueRemove(void **state) {
    const char *Uri1 = "http://localhost/foo", *Uri2 = "ftp://localhost/barbar", *Uri3 = "ftp://localhost/bar";
    SocketAddress address1, address2, address3;
    UriDetails details1 = uriDetailsNewFrom(Uri1), details2 = uriDetailsNewFrom(Uri2), details3 = uriDetailsNewFrom(
            Uri3);

    memset(&address1, 0, sizeof(SocketAddress)), memset(&address2, 0, sizeof(SocketAddress)), memset(&address3, 0,
                                                                                                     sizeof(SocketAddress));

    assert_false(uriDetailsCreateSocketAddress(&details1, &address1, SCHEME_UNKNOWN));
    assert_false(uriDetailsCreateSocketAddress(&details2, &address2, SCHEME_UNKNOWN));

    assert_null(pathQueueAppendOrCreate(&address1, details1.path));
    assert_null(pathQueueAppendOrCreate(&address2, details2.path));

    assert_non_null(pathQueueFind(&address1, details1.path, NULL, NULL));
    assert_non_null(pathQueueFind(&address2, details2.path, NULL, NULL));

    assert_int_equal(addressQueueGetNth(), 2);
    assert_int_equal(pathQueueGetNth(&address1), 1);
    assert_int_equal(pathQueueGetNth(&address2), 1);

    assert_null(pathQueueRemove(&address2, details2.path));

    assert_int_equal(addressQueueGetNth(), 1);
    assert_int_equal(pathQueueGetNth(&address1), 1);
    assert_int_equal(pathQueueGetNth(&address2), 0);

    assert_non_null(pathQueueFind(&address1, details1.path, NULL, NULL));
    assert_null(pathQueueFind(&address2, details2.path, NULL, NULL));

    assert_null(pathQueueAppendOrCreate(&address2, details2.path));

    assert_int_equal(addressQueueGetNth(), 2);
    assert_int_equal(pathQueueGetNth(&address1), 1);
    assert_int_equal(pathQueueGetNth(&address2), 1);

    assert_null(pathQueueRemove(&address1, details1.path));

    assert_int_equal(addressQueueGetNth(), 1);
    assert_int_equal(pathQueueGetNth(&address1), 0);
    assert_int_equal(pathQueueGetNth(&address2), 1);

    assert_null(pathQueueFind(&address1, details1.path, NULL, NULL));
    assert_non_null(pathQueueFind(&address2, details2.path, NULL, NULL));

    assert_false(uriDetailsCreateSocketAddress(&details3, &address3, SCHEME_UNKNOWN));
    assert_null(pathQueueAppendOrCreate(&address3, details3.path));

    assert_int_equal(addressQueueGetNth(), 1);
    assert_int_equal(pathQueueGetNth(&address1), 0);
    assert_int_equal(pathQueueGetNth(&address2), 2);

    assert_null(pathQueueRemove(&address2, details2.path));
    uriDetailsFree(&details1), uriDetailsFree(&details2), uriDetailsFree(&details3);

    assert_int_equal(addressQueueGetNth(), 1);
    assert_int_equal(pathQueueGetNth(&address2), 1);

    addressQueueClear();
}

const struct CMUnitTest queueTest[] = {cmocka_unit_test(QueueClear), cmocka_unit_test(QueueCreate),
                                       cmocka_unit_test(QueueFind), cmocka_unit_test(QueueRemove)};

#endif /* NEW_DL_TEST_QUEUE_H */