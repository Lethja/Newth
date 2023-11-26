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
    downloadQueueClear(); /* Test early return when already cleared */

    assert_false(uriDetailsCreateSocketAddress(&details, &address, SCHEME_UNKNOWN));
    uriDetailsFree(&details);

    downloadQueueClear();
}

static void QueueCreate(void **state) {
    const char *Uri = "http://localhost/foo";
    SocketAddress address;
    UriDetails details = uriDetailsNewFrom(Uri);

    memset(&address, 0, sizeof(SocketAddress));

    assert_false(uriDetailsCreateSocketAddress(&details, &address, SCHEME_UNKNOWN));

    assert_null(downloadQueueAppendOrCreate(&address, details.path));
    assert_null(downloadQueueAppendOrCreate(&address, details.path)); /* Intentional for testing */
    uriDetailsFree(&details);
    downloadQueueClear();
}

static void QueueFind(void **state) {
    size_t i, j = i = -1;
    const char *Uri1 = "http://localhost/foo", *Uri2 = "ftp://localhost/bar";
    SocketAddress address1, address2;
    UriDetails details1 = uriDetailsNewFrom(Uri1), details2 = uriDetailsNewFrom(Uri2);

    memset(&address1, 0, sizeof(SocketAddress)), memset(&address2, 0, sizeof(SocketAddress));

    assert_false(uriDetailsCreateSocketAddress(&details1, &address1, SCHEME_UNKNOWN));
    assert_false(uriDetailsCreateSocketAddress(&details2, &address2, SCHEME_UNKNOWN));

    assert_null(downloadQueueAppendOrCreate(&address1, details1.path));
    assert_null(downloadQueueAppendOrCreate(&address2, details2.path));

    assert_non_null(downloadQueueFind(&address1, details1.path, NULL, NULL));
    assert_null(downloadQueueFind(&address1, details2.path, NULL, NULL));
    assert_non_null(downloadQueueFind(&address2, details2.path, &i, &j));

    assert_int_not_equal(i, -1);
    assert_int_not_equal(j, -1);
    uriDetailsFree(&details1), uriDetailsFree(&details2);
    downloadQueueClear();
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

    assert_null(downloadQueueAppendOrCreate(&address1, details1.path));
    assert_null(downloadQueueAppendOrCreate(&address2, details2.path));

    assert_non_null(downloadQueueFind(&address1, details1.path, NULL, NULL));
    assert_non_null(downloadQueueFind(&address2, details2.path, NULL, NULL));

    assert_int_equal(downloadQueueNumberOfAddresses(), 2);
    assert_int_equal(downloadQueueAddressNumberOfRequests(&address1), 1);
    assert_int_equal(downloadQueueAddressNumberOfRequests(&address2), 1);

    assert_null(downloadQueueRemove(&address2, details2.path));

    assert_int_equal(downloadQueueNumberOfAddresses(), 1);
    assert_int_equal(downloadQueueAddressNumberOfRequests(&address1), 1);
    assert_int_equal(downloadQueueAddressNumberOfRequests(&address2), 0);

    assert_non_null(downloadQueueFind(&address1, details1.path, NULL, NULL));
    assert_null(downloadQueueFind(&address2, details2.path, NULL, NULL));

    assert_null(downloadQueueAppendOrCreate(&address2, details2.path));

    assert_int_equal(downloadQueueNumberOfAddresses(), 2);
    assert_int_equal(downloadQueueAddressNumberOfRequests(&address1), 1);
    assert_int_equal(downloadQueueAddressNumberOfRequests(&address2), 1);

    assert_null(downloadQueueRemove(&address1, details1.path));

    assert_int_equal(downloadQueueNumberOfAddresses(), 1);
    assert_int_equal(downloadQueueAddressNumberOfRequests(&address1), 0);
    assert_int_equal(downloadQueueAddressNumberOfRequests(&address2), 1);

    assert_null(downloadQueueFind(&address1, details1.path, NULL, NULL));
    assert_non_null(downloadQueueFind(&address2, details2.path, NULL, NULL));

    assert_false(uriDetailsCreateSocketAddress(&details3, &address3, SCHEME_UNKNOWN));
    assert_null(downloadQueueAppendOrCreate(&address3, details3.path));

    assert_int_equal(downloadQueueNumberOfAddresses(), 1);
    assert_int_equal(downloadQueueAddressNumberOfRequests(&address1), 0);
    assert_int_equal(downloadQueueAddressNumberOfRequests(&address2), 2);

    assert_null(downloadQueueRemove(&address2, details2.path));

    assert_int_equal(downloadQueueNumberOfAddresses(), 1);
    assert_int_equal(downloadQueueAddressNumberOfRequests(&address2), 1);

    uriDetailsFree(&details1), uriDetailsFree(&details2), uriDetailsFree(&details3);
    downloadQueueClear();
}

const struct CMUnitTest queueTest[] = {cmocka_unit_test(QueueClear), cmocka_unit_test(QueueCreate),
                                       cmocka_unit_test(QueueFind), cmocka_unit_test(QueueRemove)};

#endif /* NEW_DL_TEST_QUEUE_H */
