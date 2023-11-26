#ifndef NEW_DL_TEST_FETCH_H
#define NEW_DL_TEST_FETCH_H

#include "../src/client/io.h"
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

static void UriNewMinimum(void **state) {
    const char *Uri = "localhost/foo";

    UriDetails details = uriDetailsNewFrom(Uri);
    assert_null(details.scheme);
    assert_string_equal(details.host, "localhost");
    assert_null(details.port);
    assert_string_equal(details.path, "/foo");

    uriDetailsFree(&details);
    assert_null(details.scheme);
    assert_null(details.host);
    assert_null(details.port);
    assert_null(details.path);
}

static void UriNewNoString(void **state) {
    const char *Uri = "";
    UriDetails details = uriDetailsNewFrom(Uri);
    assert_null(details.scheme);
    assert_null(details.host);
    assert_null(details.port);
    assert_null(details.path);
    uriDetailsFree(&details);
}

static void UriNewPathless(void **state) {
    const char *Uri = "localhost";

    UriDetails details = uriDetailsNewFrom(Uri);
    assert_null(details.scheme);
    assert_string_equal(details.host, "localhost");
    assert_null(details.port);
    assert_null(details.path);

    uriDetailsFree(&details);
    assert_null(details.scheme);
    assert_null(details.host);
    assert_null(details.port);
    assert_null(details.path);
}

static void UriNewVerbose(void **state) {
    const char *Uri = "http://localhost:8080/foo";

    UriDetails details = uriDetailsNewFrom(Uri);
    assert_string_equal(details.scheme, "http");
    assert_string_equal(details.host, "localhost");
    assert_string_equal(details.port, "8080");
    assert_string_equal(details.path, "/foo");

    uriDetailsFree(&details);
    assert_null(details.scheme);
    assert_null(details.host);
    assert_null(details.port);
    assert_null(details.path);
}

static void UriGetAddressFromHost(void **state) {
    UriDetails details;
    char *addr;

    details.host = "localhost"; /* Should be set to 127.0.0.1 on any reasonable system */
    addr = uriDetailsGetHostAddr(&details);
    assert_non_null(addr);
    assert_string_equal(addr, "127.0.0.1");
    free(addr);
}

static void UriGetAddressFromAddress(void **state) {
    UriDetails details;
    char *addr;

    details.host = "127.0.0.1";
    addr = uriDetailsGetHostAddr(&details);
    assert_non_null(addr);
    assert_string_equal(addr, "127.0.0.1");
    free(addr);
}

static void UriGetPort(void **state) {
    UriDetails details;
    details.port = "80";
    assert_int_equal(uriDetailsGetPort(&details), 80);
}

static void UriGetPortInvalid(void **state) {
    UriDetails details;
    details.port = "65536";
    assert_int_equal(uriDetailsGetPort(&details), 0);
    details.port = "foo";
    assert_int_equal(uriDetailsGetPort(&details), 0);
}

static void UriGetScheme(void **state) {
    UriDetails details;

    details.scheme = "a";
    assert_int_equal(uriDetailsGetScheme(&details), SCHEME_UNKNOWN);

    details.scheme = "foo";
    assert_int_equal(uriDetailsGetScheme(&details), SCHEME_UNKNOWN);

    details.scheme = "ftp";
    assert_int_equal(uriDetailsGetScheme(&details), SCHEME_FTP);

    details.scheme = "ftps";
    assert_int_equal(uriDetailsGetScheme(&details), SCHEME_FTPS);

    details.scheme = "http";
    assert_int_equal(uriDetailsGetScheme(&details), SCHEME_HTTP);

    details.scheme = "https";
    assert_int_equal(uriDetailsGetScheme(&details), SCHEME_HTTPS);
}

static void UriConvertToSocketAddress(void **state) {
    SocketAddress address;
    UriDetails details = uriDetailsNewFrom("http://localhost");
    assert_false(uriDetailsCreateSocketAddress(&details, &address, SCHEME_UNKNOWN));
    assert_int_equal(address.sock.sa_family, AF_INET);
    assert_int_equal(address.ipv4.sin_port, htons(SCHEME_HTTP));
    assert_string_equal(inet_ntoa(address.ipv4.sin_addr), "127.0.0.1");
    uriDetailsFree(&details);
}

static void UriConvertToSocketAddressWithScheme(void **state) {
    SocketAddress address;
    UriDetails details = uriDetailsNewFrom("localhost");
    assert_false(uriDetailsCreateSocketAddress(&details, &address, SCHEME_HTTPS));
    assert_int_equal(address.sock.sa_family, AF_INET);
    assert_int_equal(address.ipv4.sin_port, htons(SCHEME_HTTPS));
    assert_string_equal(inet_ntoa(address.ipv4.sin_addr), "127.0.0.1");
    uriDetailsFree(&details);
}

static void UriConvertToSocketAddressWithPort(void **state) {
    SocketAddress address;
    UriDetails details = uriDetailsNewFrom("localhost:1");
    assert_string_equal(details.host, "localhost");
    assert_string_equal(details.port, "1");

    assert_false(uriDetailsCreateSocketAddress(&details, &address, SCHEME_FTPS));
    assert_int_equal(address.sock.sa_family, AF_INET);
    assert_int_equal(address.ipv4.sin_port, htons(1));
    assert_string_equal(inet_ntoa(address.ipv4.sin_addr), "127.0.0.1");
    uriDetailsFree(&details);
}

#pragma clang diagnostic pop

const struct CMUnitTest fetchTest[] = {cmocka_unit_test(UriConvertToSocketAddress),
                                       cmocka_unit_test(UriConvertToSocketAddressWithPort),
                                       cmocka_unit_test(UriConvertToSocketAddressWithScheme),
                                       cmocka_unit_test(UriGetAddressFromAddress),
                                       cmocka_unit_test(UriGetAddressFromHost), cmocka_unit_test(UriGetPort),
                                       cmocka_unit_test(UriGetPortInvalid), cmocka_unit_test(UriGetScheme),
                                       cmocka_unit_test(UriNewMinimum), cmocka_unit_test(UriNewNoString),
                                       cmocka_unit_test(UriNewPathless), cmocka_unit_test(UriNewVerbose)};

#endif /* NEW_DL_TEST_FETCH_H */
