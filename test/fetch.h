#ifndef NEW_DL_TEST_FETCH_H
#define NEW_DL_TEST_FETCH_H

#include "../src/client/uri.h"

#pragma region CMocka headers

#include <stdarg.h>
#include <stddef.h>
#include <stdint.h>
#include <setjmp.h>

#include <cmocka.h>

#pragma endregion

static void ExtractUriMinimum(void **state) {
    const char *Uri = "localhost/foo";

    UriDetails details = UriDetailsNewFrom(Uri);
    assert_null(details.scheme);
    assert_string_equal(details.host, "localhost");
    assert_null(details.port);
    assert_string_equal(details.path, "/foo");

    UriDetailsFree(&details);
    assert_null(details.scheme);
    assert_null(details.host);
    assert_null(details.port);
    assert_null(details.path);
}

static void ExtractUriPathless(void **state) {
    const char *Uri = "localhost";

    UriDetails details = UriDetailsNewFrom(Uri);
    assert_null(details.scheme);
    assert_string_equal(details.host, "localhost");
    assert_null(details.port);
    assert_null(details.path);

    UriDetailsFree(&details);
    assert_null(details.scheme);
    assert_null(details.host);
    assert_null(details.port);
    assert_null(details.path);
}

static void ExtractUriVerbose(void **state) {
    const char *Uri = "http://localhost:8080/foo";

    UriDetails details = UriDetailsNewFrom(Uri);
    assert_string_equal(details.scheme, "http");
    assert_string_equal(details.host, "localhost");
    assert_string_equal(details.port, "8080");
    assert_string_equal(details.path, "/foo");

    UriDetailsFree(&details);
    assert_null(details.scheme);
    assert_null(details.host);
    assert_null(details.port);
    assert_null(details.path);
}

const struct CMUnitTest fetchTest[] = {cmocka_unit_test(ExtractUriMinimum), cmocka_unit_test(ExtractUriPathless),
                                       cmocka_unit_test(ExtractUriVerbose)};

#endif /* NEW_DL_TEST_FETCH_H */
