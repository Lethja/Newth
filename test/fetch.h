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

static void UriDetailsToString(void **state) {
    const char *Uri = "http://localhost/foo";
    char *res;

    UriDetails details = uriDetailsNewFrom(Uri);
    assert_non_null((res = uriDetailsCreateString(&details)));
    assert_string_equal(Uri, res);
    free(res), uriDetailsFree(&details);
}

static void UriDetailsToStringWithPort(void **state) {
    const char *Uri = "http://localhost:8080/foo";
    char *res;

    UriDetails details = uriDetailsNewFrom(Uri);
    assert_non_null((res = uriDetailsCreateString(&details)));
    assert_string_equal(Uri, res);
    free(res), uriDetailsFree(&details);
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
    assert_int_equal(address.scheme, SCHEME_HTTP);
    assert_int_equal(address.address.sock.sa_family, AF_INET);
    assert_int_equal(address.address.ipv4.sin_port, htons(SCHEME_HTTP));
    assert_string_equal(inet_ntoa(address.address.ipv4.sin_addr), "127.0.0.1");
    uriDetailsFree(&details);
}

static void UriConvertToSocketAddressWithScheme(void **state) {
    SocketAddress address;
    UriDetails details = uriDetailsNewFrom("localhost");
    assert_false(uriDetailsCreateSocketAddress(&details, &address, SCHEME_HTTPS));
    assert_int_equal(address.scheme, SCHEME_HTTPS);
    assert_int_equal(address.address.sock.sa_family, AF_INET);
    assert_int_equal(address.address.ipv4.sin_port, htons(SCHEME_HTTPS));
    assert_string_equal(inet_ntoa(address.address.ipv4.sin_addr), "127.0.0.1");
    uriDetailsFree(&details);
}

static void UriConvertToSocketAddressWithPort(void **state) {
    SocketAddress address;
    UriDetails details = uriDetailsNewFrom("localhost:1");
    assert_string_equal(details.host, "localhost");
    assert_string_equal(details.port, "1");

    assert_false(uriDetailsCreateSocketAddress(&details, &address, SCHEME_FTPS));
    assert_int_equal(address.scheme, SCHEME_FTPS);
    assert_int_equal(address.address.sock.sa_family, AF_INET);
    assert_int_equal(address.address.ipv4.sin_port, htons(1));
    assert_string_equal(inet_ntoa(address.address.ipv4.sin_addr), "127.0.0.1");
    uriDetailsFree(&details);
}

static void UriPathCombineString(void **state) {
    const char *path1 = "foo", *path2 = "bar";
    char output[FILENAME_MAX];

    platformPathCombine(output, path1, path2);
    assert_string_equal(output, "foo/bar");
}

static void UriPathCombineStringNoDivider(void **state) {
    const char *path1 = "/foo", *path2 = "bar/";
    char output[FILENAME_MAX];

    platformPathCombine(output, path1, path2);
    assert_string_equal(output, "/foo/bar/");
}

static void UriPathCombineStringTrailingDivider(void **state) {
    const char *path1 = "/foo/", *path2 = "bar/";
    char output[FILENAME_MAX];

    platformPathCombine(output, path1, path2);
    assert_string_equal(output, "/foo/bar/");
}

static void UriPathCombineStringLeadingDivider(void **state) {
    const char *path1 = "/foo", *path2 = "/bar/";
    char output[FILENAME_MAX];

    platformPathCombine(output, path1, path2);
    assert_string_equal(output, "/foo/bar/");
}

static void UriPathCombineStringBothDividers(void **state) {
    const char *path1 = "/foo/", *path2 = "/bar/";
    char output[FILENAME_MAX];

    platformPathCombine(output, path1, path2);
    assert_string_equal(output, "/foo/bar/");
}

static void UriPathCombineStringJustDividers(void **state) {
    const char *path1 = "////", *path2 = path1;
    char output[FILENAME_MAX];

    platformPathCombine(output, path1, path2);
    assert_string_equal(output, "/");
}

static void UriPathCombineStringDumbInput(void **state) {
    const char *path1, *path2 = path1 = "/";
    char output[FILENAME_MAX];

    platformPathCombine(output, path1, path2);
    assert_string_equal(output, "/");
}

static void UriPathAppend(void **state) {
    const char *path1 = "/animal/dog", *path2 = "bone";
    char *r;
    assert_non_null((r = uriPathAbsoluteAppend(path1, path2)));
    assert_string_equal(r, "/animal/dog/bone");
    free(r);
}

static void UriPathAppendAbsolute(void **state) {
    const char *path1 = "/animal/dog", *path2 = "/bone";
    char *r;
    assert_non_null((r = uriPathAbsoluteAppend(path1, path2)));
    assert_string_equal(r, path2);
    free(r);
}

static void UriPathAppendGoNowhere(void **state) {
    const char *path1 = "/animal/dog", *path2 = ".";
    char *r;
    assert_non_null((r = uriPathAbsoluteAppend(path1, path2)));
    assert_string_equal(r, "/animal/dog");
    free(r);
}

static void UriPathAppendGoNowhereDouble(void **state) {
    const char *path1 = "/animal/dog", *path2 = "././";
    char *r;
    assert_non_null((r = uriPathAbsoluteAppend(path1, path2)));
    assert_string_equal(r, "/animal/dog");
    free(r);
}

static void UriPathAppendGoNowhereYetSomewhere(void **state) {
    const char *path1 = "/animal/dog", *path2 = "./bone";
    char *r;
    assert_non_null((r = uriPathAbsoluteAppend(path1, path2)));
    assert_string_equal(r, "/animal/dog/bone");
    free(r);
}

static void UriPathAppendGoNowhereYetSomewhereDouble(void **state) {
    const char *path1 = "/animal/dog", *path2 = "././bone";
    char *r;
    assert_non_null((r = uriPathAbsoluteAppend(path1, path2)));
    assert_string_equal(r, "/animal/dog/bone");
    free(r);
}

static void UriPathAppendGoNowhereAndUp(void **state) {
    const char *path1 = "/animal/dog", *path2 = "././../bone";
    char *r;
    assert_non_null((r = uriPathAbsoluteAppend(path1, path2)));
    assert_string_equal(r, "/animal/bone");
    free(r);
}

static void UriPathAppendGoUpAndNowhere(void **state) {
    const char *path1 = "/animal/dog", *path2 = "../././bone";
    char *r;
    assert_non_null((r = uriPathAbsoluteAppend(path1, path2)));
    assert_string_equal(r, "/animal/bone");
    free(r);
}

static void UriPathAppendGoUp(void **state) {
    const char *path1 = "/animal/dog", *path2 = "..";
    char *r;
    assert_non_null((r = uriPathAbsoluteAppend(path1, path2)));
    assert_string_equal(r, "/animal");
    free(r);
}

static void UriPathAppendGoUpDouble(void **state) {
    const char *path1 = "/animal/dog", *path2 = "../..";
    char *r;
    assert_non_null((r = uriPathAbsoluteAppend(path1, path2)));
    assert_string_equal(r, "/");
    free(r);
}

static void UriPathAppendGoUpTooFar(void **state) {
    const char *path1 = "/animal/dog", *path2 = "../../..";
    char *r;
    assert_non_null((r = uriPathAbsoluteAppend(path1, path2)));
    assert_string_equal(r, "/");
    free(r);
}

static void UriPathAppendGoSideways(void **state) {
    const char *path1 = "/animal/dog", *path2 = "../cat";
    char *r;
    assert_non_null((r = uriPathAbsoluteAppend(path1, path2)));
    assert_string_equal(r, "/animal/cat");
    free(r);
}

static void UriPathAppendGoSidewaysDouble(void **state) {
    const char *path1 = "/animal/dog/bone", *path2 = "../../cat/fish";
    char *r;
    assert_non_null((r = uriPathAbsoluteAppend(path1, path2)));
    assert_string_equal(r, "/animal/cat/fish");
    free(r);
}

#ifdef GETHOSTBYNAME_CANT_IPV4STR

static void Ipv4Validate(void **state) {
    const char *bad1 = "256.255.255.255", *bad2 = "0", *bad3 = "0000", *bad4 = "Connect please", *loop = "127.0.0.1", *max = "255.255.255.255", *min = "0.0.0.0";

    assert_false(isValidIpv4Str(bad1));
    assert_false(isValidIpv4Str(bad2));
    assert_false(isValidIpv4Str(bad3));
    assert_false(isValidIpv4Str(bad4));
    assert_true(isValidIpv4Str(loop));
    assert_true(isValidIpv4Str(min));
    assert_true(isValidIpv4Str(max));
}

#endif

#pragma clang diagnostic pop

const struct CMUnitTest fetchTest[] = {
#ifdef GETHOSTBYNAME_CANT_IPV4STR

        cmocka_unit_test(Ipv4Validate),

#endif
        cmocka_unit_test(UriConvertToSocketAddress), cmocka_unit_test(UriConvertToSocketAddressWithPort),
        cmocka_unit_test(UriConvertToSocketAddressWithScheme), cmocka_unit_test(UriGetAddressFromAddress),
        cmocka_unit_test(UriDetailsToString), cmocka_unit_test(UriDetailsToStringWithPort),
        cmocka_unit_test(UriGetAddressFromHost), cmocka_unit_test(UriGetPort), cmocka_unit_test(UriGetPortInvalid),
        cmocka_unit_test(UriGetScheme), cmocka_unit_test(UriNewMinimum), cmocka_unit_test(UriNewNoString),
        cmocka_unit_test(UriNewPathless), cmocka_unit_test(UriNewVerbose), cmocka_unit_test(UriPathCombineString),
        cmocka_unit_test(UriPathAppend), cmocka_unit_test(UriPathAppendAbsolute),
        cmocka_unit_test(UriPathAppendGoNowhere), cmocka_unit_test(UriPathAppendGoNowhereAndUp),
        cmocka_unit_test(UriPathAppendGoNowhereDouble), cmocka_unit_test(UriPathAppendGoNowhereYetSomewhere),
        cmocka_unit_test(UriPathAppendGoNowhereYetSomewhereDouble), cmocka_unit_test(UriPathAppendGoSideways),
        cmocka_unit_test(UriPathAppendGoSidewaysDouble), cmocka_unit_test(UriPathAppendGoUp),
        cmocka_unit_test(UriPathAppendGoUpAndNowhere), cmocka_unit_test(UriPathAppendGoUpDouble),
        cmocka_unit_test(UriPathAppendGoUpTooFar), cmocka_unit_test(UriPathCombineStringBothDividers),
        cmocka_unit_test(UriPathCombineStringDumbInput), cmocka_unit_test(UriPathCombineStringJustDividers),
        cmocka_unit_test(UriPathCombineStringLeadingDivider), cmocka_unit_test(UriPathCombineStringNoDivider),
        cmocka_unit_test(UriPathCombineStringTrailingDivider)};

#endif /* NEW_DL_TEST_FETCH_H */
