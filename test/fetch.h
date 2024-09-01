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

/**
 * This test ensures uriDetailsNewFrom() is filling out UriDetails with the minimum amount of functional data
 */
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

/**
 * This test ensures uriDetailsNewFrom() doesn't write junk data into UriDetails
 */
static void UriNewNoString(void **state) {
    const char *Uri = "";
    UriDetails details = uriDetailsNewFrom(Uri);
    assert_null(details.scheme);
    assert_null(details.host);
    assert_null(details.port);
    assert_null(details.path);
    uriDetailsFree(&details);
}

/**
 * This test ensures uriDetailsNewFrom() doesn't write junk data into UriDetails when only a host is given
 */
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

/**
 * This test ensures uriDetailsNewFrom() writes correct information into all members of UriDetails
 */
static void UriNewVerbose(void **state) {
    const char *Uri = "http://localhost:8080/foo?bar";

    UriDetails details = uriDetailsNewFrom(Uri);
    assert_string_equal(details.scheme, "http");
    assert_string_equal(details.host, "localhost");
    assert_string_equal(details.port, "8080");
    assert_string_equal(details.path, "/foo");
    assert_string_equal(details.query, "bar");

    uriDetailsFree(&details);
    assert_null(details.scheme);
    assert_null(details.host);
    assert_null(details.port);
    assert_null(details.path);
    assert_null(details.query);
}

/**
* This test ensures the string made by uriDetailsCreateString() is identical to the one put into uriDetailsNewFrom()
 */
static void UriDetailsToString(void **state) {
    const char *Uri = "http://localhost/foo";
    char *res;

    UriDetails details = uriDetailsNewFrom(Uri);
    assert_non_null((res = uriDetailsCreateString(&details)));
    assert_string_equal(Uri, res);
    free(res), uriDetailsFree(&details);
}

/**
* This test ensures file:// strings made by uriDetailsCreateString() is identical to the one put into uriDetailsNewFrom()
 */
static void UriDetailsToStringFile(void **state) {
    const char *Uri = "file:///foo";
    char *res;

    UriDetails details = uriDetailsNewFrom(Uri);
    assert_non_null((res = uriDetailsCreateString(&details)));
    assert_string_equal(Uri, res);
    free(res), uriDetailsFree(&details);
}

/**
* This test is the port version of the 'UriDetailsToString' test
 */
static void UriDetailsToStringWithPort(void **state) {
    const char *Uri = "http://localhost:8080/foo";
    char *res;

    UriDetails details = uriDetailsNewFrom(Uri);
    assert_non_null((res = uriDetailsCreateString(&details)));
    assert_string_equal(Uri, res);
    free(res), uriDetailsFree(&details);
}

/**
* This test is the query version of the 'UriDetailsToString' test
 */
static void UriDetailsToStringWithQuery(void **state) {
    const char *Uri = "http://localhost/foo?bar";
    char *res;

    UriDetails details = uriDetailsNewFrom(Uri);
    assert_non_null((res = uriDetailsCreateString(&details)));
    assert_string_equal(Uri, res);
    free(res), uriDetailsFree(&details);
}

/**
* This test ensures the string made by uriDetailsCreateString() is identical to the one put into uriDetailsNewFrom()
 */
static void UriDetailsToStringBase(void **state) {
    const char *Uri = "http://localhost/foo";
    char *res;

    UriDetails details = uriDetailsNewFrom(Uri);
    assert_non_null((res = uriDetailsCreateStringBase(&details)));
    assert_string_equal("http://localhost/", res);
    free(res), uriDetailsFree(&details);
}

/**
* This test ensures file:// strings made by uriDetailsCreateString() is identical to the one put into uriDetailsNewFrom()
 */
static void UriDetailsToStringBaseFile(void **state) {
    const char *Uri = "file:///foo";
    char *res;

    UriDetails details = uriDetailsNewFrom(Uri);
    assert_non_null((res = uriDetailsCreateStringBase(&details)));
    assert_string_equal("file:///", res);
    free(res), uriDetailsFree(&details);
}

/**
* This test is the port version of the 'UriDetailsToString' test
 */
static void UriDetailsToStringBaseWithPort(void **state) {
    const char *Uri = "http://localhost:8080/foo";
    char *res;

    UriDetails details = uriDetailsNewFrom(Uri);
    assert_non_null((res = uriDetailsCreateStringBase(&details)));
    assert_string_equal("http://localhost:8080/", res);
    free(res), uriDetailsFree(&details);
}

/**
* This test is the query version of the 'UriDetailsToString' test
 */
static void UriDetailsToStringBaseWithQuery(void **state) {
    const char *Uri = "http://localhost/foo?bar";
    char *res;

    UriDetails details = uriDetailsNewFrom(Uri);
    assert_non_null((res = uriDetailsCreateStringBase(&details)));
    assert_string_equal("http://localhost/", res);
    free(res), uriDetailsFree(&details);
}

/**
 * This test ensures that name resolution can take place and uriDetailsGetHostAddr() returns the correct address
 */
static void UriGetAddressFromHost(void **state) {
    UriDetails details;
    char *addr;

    details.host = "localhost"; /* Should be set to 127.0.0.1 on any reasonable system */
    addr = uriDetailsGetHostAddr(&details);
    assert_non_null(addr);
    assert_string_equal(addr, "127.0.0.1");
    free(addr);
}

/**
 * This test ensures that ip address resolution can take place and uriDetailsGetHostAddr() returns the correct address
 */
static void UriGetAddressFromAddress(void **state) {
    UriDetails details;
    char *addr;

    details.host = "127.0.0.1";
    addr = uriDetailsGetHostAddr(&details);
    assert_non_null(addr);
    assert_string_equal(addr, "127.0.0.1");
    free(addr);
}

/**
 * This test checks that uriDetailsGetPort() converts a string input of a port number into a unsigned short correctly
 */
static void UriGetPort(void **state) {
    UriDetails details;
    details.port = "80";
    assert_int_equal(uriDetailsGetPort(&details), 80);
}

/**
 * This test ensures that when the port string is not within the range of a unsigned short it will be set to 0
 */
static void UriGetPortInvalid(void **state) {
    UriDetails details;
    details.port = "65536";
    assert_int_equal(uriDetailsGetPort(&details), 0);
    details.port = "foo";
    assert_int_equal(uriDetailsGetPort(&details), 0);
}

/**
 * This test ensures that known uri schemes are given the correct enum value from uriDetailsGetScheme()
 */
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

/**
 * This test checks that schemes enums are set back to the correct string counterparts with uriDetailsSetScheme()
 */
static void UriSetScheme(void **state) {
    UriDetails details = uriDetailsNewFrom(NULL);
    uriDetailsSetScheme(&details, SCHEME_HTTPS);
    assert_string_equal(details.scheme, "https");
    assert_int_equal(uriDetailsGetScheme(&details), SCHEME_HTTPS);

    uriDetailsSetScheme(&details, SCHEME_FTP);
    assert_string_equal(details.scheme, "ftp");
    assert_int_equal(uriDetailsGetScheme(&details), SCHEME_FTP);

    uriDetailsSetScheme(&details, SCHEME_UNKNOWN);
    assert_null(details.scheme);
    assert_int_equal(uriDetailsGetScheme(&details), SCHEME_UNKNOWN);
}

/**
 * This test checks that address and port set back to the correct string counterparts with uriDetailsSetAddress()
 */
static void UriSetAddressAndPort(void **state) {
    UriDetails details = uriDetailsNewFrom(NULL);
    struct sockaddr_in ipv4;

    ipv4.sin_family = AF_INET, ipv4.sin_addr.s_addr = inet_addr("127.0.0.1"), ipv4.sin_port = htons(80);
    uriDetailsSetAddress(&details, (struct sockaddr *) &ipv4);
    assert_non_null(details.host);
    assert_string_equal(details.host, "127.0.0.1");
    assert_non_null(details.port);
    assert_string_equal(details.port, "80");
    free(details.host), free(details.port);
}

/**
 * This test ensures that a uri can be converted directly into a socket address if enough information is given to do so
 */
static void UriConvertToSocketAddress(void **state) {
    SocketAddress address;
    UriDetails details = uriDetailsNewFrom("http://localhost");
    assert_null(uriDetailsCreateSocketAddress(&details, &address, SCHEME_UNKNOWN));
    assert_int_equal(address.scheme, SCHEME_HTTP);
    assert_int_equal(address.address.sock.sa_family, AF_INET);
    assert_int_equal(address.address.ipv4.sin_port, htons(SCHEME_HTTP));
    assert_string_equal(inet_ntoa(address.address.ipv4.sin_addr), "127.0.0.1");
    uriDetailsFree(&details);
}

/**
 * This test ensures that a uri with a port can be converted directly into a socket address if enough information is given to do so
 */
static void UriConvertToSocketAddressWithScheme(void **state) {
    SocketAddress address;
    UriDetails details = uriDetailsNewFrom("localhost");
    assert_null(uriDetailsCreateSocketAddress(&details, &address, SCHEME_HTTPS));
    assert_int_equal(address.scheme, SCHEME_HTTPS);
    assert_int_equal(address.address.sock.sa_family, AF_INET);
    assert_int_equal(address.address.ipv4.sin_port, htons(SCHEME_HTTPS));
    assert_string_equal(inet_ntoa(address.address.ipv4.sin_addr), "127.0.0.1");
    uriDetailsFree(&details);
}

/**
 * This test is like the UriSetScheme test except with a port in the uri
 */
static void UriConvertToSocketAddressWithPort(void **state) {
    SocketAddress address;
    UriDetails details = uriDetailsNewFrom("localhost:1");
    assert_string_equal(details.host, "localhost");
    assert_string_equal(details.port, "1");

    assert_null(uriDetailsCreateSocketAddress(&details, &address, SCHEME_FTPS));
    assert_int_equal(address.scheme, SCHEME_FTPS);
    assert_int_equal(address.address.sock.sa_family, AF_INET);
    assert_int_equal(address.address.ipv4.sin_port, htons(1));
    assert_string_equal(inet_ntoa(address.address.ipv4.sin_addr), "127.0.0.1");
    uriDetailsFree(&details);
}

/**
 * This test checks that two paths can be added together with neither having a path divider in it's string
 * @remark uriPathCombine() should not to be confused with platformPathCombine()
 */
static void UriPathCombineString(void **state) {
    const char *path1 = "foo", *path2 = "bar";
    char output[FILENAME_MAX];

    uriPathCombine(output, path1, path2);
    assert_string_equal(output, "foo/bar");
}

/**
 * This test checks that two paths can be added together, if they have path dividers but not on the joining end
 * @remark uriPathCombine() should not to be confused with platformPathCombine()
 */
static void UriPathCombineStringNoDivider(void **state) {
    const char *path1 = "/foo", *path2 = "bar/";
    char output[FILENAME_MAX];

    uriPathCombine(output, path1, path2);
    assert_string_equal(output, "/foo/bar/");
}

/**
 * This test checks that two paths can be added together, if the first has a path divider
 * @remark uriPathCombine() should not to be confused with platformPathCombine()
 */
static void UriPathCombineStringTrailingDivider(void **state) {
    const char *path1 = "/foo/", *path2 = "bar/";
    char output[FILENAME_MAX];

    uriPathCombine(output, path1, path2);
    assert_string_equal(output, "/foo/bar/");
}

/**
 * This test checks that two paths can be added together, if the second has a path divider
 * @remark uriPathCombine() should not to be confused with platformPathCombine()
 */
static void UriPathCombineStringLeadingDivider(void **state) {
    const char *path1 = "/foo", *path2 = "/bar/";
    char output[FILENAME_MAX];

    platformPathCombine(output, path1, path2);
    assert_string_equal(output, "/foo/bar/");
}

/**
 * This test checks that two paths can be added together, if both strings have the divider
 * @remark uriPathCombine() should not to be confused with platformPathCombine()
 */
static void UriPathCombineStringBothDividers(void **state) {
    const char *path1 = "/foo/", *path2 = "/bar/";
    char output[FILENAME_MAX];

    platformPathCombine(output, path1, path2);
    assert_string_equal(output, "/foo/bar/");
}

/**
 * This test checks that non-sense paths are not combined in a non-sense way
 * @remark uriPathCombine() should not to be confused with platformPathCombine()
 */
static void UriPathCombineStringJustDividers(void **state) {
    const char *path1 = "////", *path2 = path1;
    char output[FILENAME_MAX];

    uriPathCombine(output, path1, path2);
    assert_string_equal(output, "/");
}

/**
 * This test checks that two root paths are not combined together
 * @remark uriPathCombine() should not to be confused with platformPathCombine()
 */
static void UriPathCombineStringDumbInput(void **state) {
    const char *path1, *path2 = path1 = "/";
    char output[FILENAME_MAX];

    uriPathCombine(output, path1, path2);
    assert_string_equal(output, "/");
}

/**
 * This test ensures that uri paths can be appended to and a new string be returned
 */
static void UriPathAppend(void **state) {
    const char *path1 = "/animal/dog", *path2 = "bone";
    char *r;
    assert_non_null((r = uriPathAbsoluteAppend(path1, path2)));
    assert_string_equal(r, "/animal/dog/bone");
    free(r);
}

/**
 * This test ensures that when an absolute uri paths is attempting to be appended it is instead return verbatim
 */
static void UriPathAppendAbsolute(void **state) {
    const char *path1 = "/animal/dog", *path2 = "/bone";
    char *r;
    assert_non_null((r = uriPathAbsoluteAppend(path1, path2)));
    assert_string_equal(r, path2);
    free(r);
}

/**
 * This test checks that a relative path '.' does not get appended and that the same path as before is returned
 */
static void UriPathAppendGoNowhere(void **state) {
    const char *path1 = "/animal/dog", *path2 = ".";
    char *r;
    assert_non_null((r = uriPathAbsoluteAppend(path1, path2)));
    assert_string_equal(r, "/animal/dog");
    free(r);
}

/**
 * This test ensures that a relative path '.' gets omitted from appending
 */
static void UriPathAppendGoNowhereDouble(void **state) {
    const char *path1 = "/animal/dog", *path2 = "././";
    char *r;
    assert_non_null((r = uriPathAbsoluteAppend(path1, path2)));
    assert_string_equal(r, "/animal/dog");
    free(r);
}

/**
 * This test ensures that a current path '.' gets omitted from appending but valid paths aren't
 */
static void UriPathAppendGoNowhereYetSomewhere(void **state) {
    const char *path1 = "/animal/dog", *path2 = "./bone";
    char *r;
    assert_non_null((r = uriPathAbsoluteAppend(path1, path2)));
    assert_string_equal(r, "/animal/dog/bone");
    free(r);
}

/**
 * This test ensures that a current path '.' gets omitted from appending but valid paths aren't even when there's many
 */
static void UriPathAppendGoNowhereYetSomewhereDouble(void **state) {
    const char *path1 = "/animal/dog", *path2 = "././bone";
    char *r;
    assert_non_null((r = uriPathAbsoluteAppend(path1, path2)));
    assert_string_equal(r, "/animal/dog/bone");
    free(r);
}

/**
 * This test ensures that a up path works even if curret paths occurred before
 */
static void UriPathAppendGoNowhereAndUp(void **state) {
    const char *path1 = "/animal/dog", *path2 = "././../bone";
    char *r;
    assert_non_null((r = uriPathAbsoluteAppend(path1, path2)));
    assert_string_equal(r, "/animal/bone");
    free(r);
}

/**
 * This test ensures that a up path works even if current paths occur after
 */
static void UriPathAppendGoUpAndNowhere(void **state) {
    const char *path1 = "/animal/dog", *path2 = "../././bone";
    char *r;
    assert_non_null((r = uriPathAbsoluteAppend(path1, path2)));
    assert_string_equal(r, "/animal/bone");
    free(r);
}

/**
 * This test ensures that a up path works
 */
static void UriPathAppendGoUp(void **state) {
    const char *path1 = "/animal/dog", *path2 = "..";
    char *r;
    assert_non_null((r = uriPathAbsoluteAppend(path1, path2)));
    assert_string_equal(r, "/animal");
    free(r);
}

/**
 * This test ensures that a up path can work multiple times
 */
static void UriPathAppendGoUpDouble(void **state) {
    const char *path1 = "/animal/dog", *path2 = "../..";
    char *r;
    assert_non_null((r = uriPathAbsoluteAppend(path1, path2)));
    assert_string_equal(r, "/");
    free(r);
}

/**
 * This test ensures that going up too many paths will simply leave you at root
 */
static void UriPathAppendGoUpTooFar(void **state) {
    const char *path1 = "/animal/dog", *path2 = "../../..";
    char *r;
    assert_non_null((r = uriPathAbsoluteAppend(path1, path2)));
    assert_string_equal(r, "/");
    free(r);
}

/**
 * This test ensures that going up still allows you to go to a path afterward
 */
static void UriPathAppendGoSideways(void **state) {
    const char *path1 = "/animal/dog", *path2 = "../cat";
    char *r;
    assert_non_null((r = uriPathAbsoluteAppend(path1, path2)));
    assert_string_equal(r, "/animal/cat");
    free(r);
}

/**
 * This test ensures that going up still allows you to go to a path afterward
 */
static void UriPathAppendGoSidewaysDouble(void **state) {
    const char *path1 = "/animal/dog/bone", *path2 = "../../cat/fish";
    char *r;
    assert_non_null((r = uriPathAbsoluteAppend(path1, path2)));
    assert_string_equal(r, "/animal/cat/fish");
    free(r);
}

/**
 * This test checks that uriPathLast() return the last element of the path correctly as a sub-pointer
 */
static void UriPathLast(void **state) {
    const char *path = "/animal/dog.png";
    const char *last = uriPathLast(path);
    assert_non_null(last);
    assert_string_equal(last, "dog.png");
    assert_ptr_equal(last, &path[8]);
}

/**
 * This test checks that uriPathLast() return a new heap with the last element and any trailing divider removed
 */
static void UriPathLastHeap(void **state) {
    const char *path = "/animal/cat/";
    char *last = uriPathLast(path);
    assert_non_null(last);
    assert_ptr_not_equal(last, &path[8]);
    assert_string_equal(last, "cat"), free(last);
    assert_null(uriPathLast("/")); /* Always return NULL on root path */
}

#ifdef GETHOSTBYNAME_CANT_IPV4STR

/**
 * This test checks that isValidIpv4Str() returns correct results
 */
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
    cmocka_unit_test(UriConvertToSocketAddress),
    cmocka_unit_test(UriConvertToSocketAddressWithPort),
    cmocka_unit_test(UriConvertToSocketAddressWithScheme),
    cmocka_unit_test(UriGetAddressFromAddress),
    cmocka_unit_test(UriDetailsToString),
    cmocka_unit_test(UriDetailsToStringFile),
    cmocka_unit_test(UriDetailsToStringWithPort),
    cmocka_unit_test(UriDetailsToStringWithQuery),
    cmocka_unit_test(UriDetailsToStringBase),
    cmocka_unit_test(UriDetailsToStringBaseFile),
    cmocka_unit_test(UriDetailsToStringBaseWithPort),
    cmocka_unit_test(UriDetailsToStringBaseWithQuery),
    cmocka_unit_test(UriGetAddressFromHost),
    cmocka_unit_test(UriGetPort),
    cmocka_unit_test(UriGetPortInvalid),
    cmocka_unit_test(UriGetScheme),
    cmocka_unit_test(UriSetAddressAndPort),
    cmocka_unit_test(UriSetScheme),
    cmocka_unit_test(UriNewMinimum),
    cmocka_unit_test(UriNewNoString),
    cmocka_unit_test(UriNewPathless),
    cmocka_unit_test(UriNewVerbose),
    cmocka_unit_test(UriPathCombineString),
    cmocka_unit_test(UriPathAppend),
    cmocka_unit_test(UriPathAppendAbsolute),
    cmocka_unit_test(UriPathAppendGoNowhere),
    cmocka_unit_test(UriPathAppendGoNowhereAndUp),
    cmocka_unit_test(UriPathAppendGoNowhereDouble),
    cmocka_unit_test(UriPathAppendGoNowhereYetSomewhere),
    cmocka_unit_test(UriPathAppendGoNowhereYetSomewhereDouble),
    cmocka_unit_test(UriPathAppendGoSideways),
    cmocka_unit_test(UriPathAppendGoSidewaysDouble),
    cmocka_unit_test(UriPathAppendGoUp),
    cmocka_unit_test(UriPathAppendGoUpAndNowhere),
    cmocka_unit_test(UriPathAppendGoUpDouble),
    cmocka_unit_test(UriPathAppendGoUpTooFar),
    cmocka_unit_test(UriPathCombineStringBothDividers),
    cmocka_unit_test(UriPathCombineStringDumbInput),
    cmocka_unit_test(UriPathCombineStringJustDividers),
    cmocka_unit_test(UriPathCombineStringLeadingDivider),
    cmocka_unit_test(UriPathCombineStringNoDivider),
    cmocka_unit_test(UriPathCombineStringTrailingDivider),
    cmocka_unit_test(UriPathLast),
    cmocka_unit_test(UriPathLastHeap)
};

#endif /* NEW_DL_TEST_FETCH_H */
