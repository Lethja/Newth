#ifndef NEW_TH_TEST_PLATFORM_H
#define NEW_TH_TEST_PLATFORM_H

#include "common.h"
#include "../src/server/sockbufr.h"
#include <errno.h>
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

static void PathCombineString(void **state) {
    const char *path1 = "foo", *path2 = "bar";
    char output[FILENAME_MAX];

    platformPathCombine(output, path1, path2);
    assert_string_equal(output, "foo/bar");
}

static void PathCombineStringNoDivider(void **state) {
    const char *path1 = "/foo", *path2 = "bar/";
    char output[FILENAME_MAX];

    platformPathCombine(output, path1, path2);
    assert_string_equal(output, "/foo/bar/");
}

static void PathCombineStringTrailingDivider(void **state) {
    const char *path1 = "/foo/", *path2 = "bar/";
    char output[FILENAME_MAX];

    platformPathCombine(output, path1, path2);
    assert_string_equal(output, "/foo/bar/");
}

static void PathCombineStringLeadingDivider(void **state) {
    const char *path1 = "/foo", *path2 = "/bar/";
    char output[FILENAME_MAX];

    platformPathCombine(output, path1, path2);
    assert_string_equal(output, "/foo/bar/");
}

static void PathCombineStringBothDividers(void **state) {
    const char *path1 = "/foo/", *path2 = "/bar/";
    char output[FILENAME_MAX];

    platformPathCombine(output, path1, path2);
    assert_string_equal(output, "/foo/bar/");
}

static void PathCombineStringDumbInput(void **state) {
    const char *path1, *path2 = path1 = "/";
    char output[FILENAME_MAX];

    platformPathCombine(output, path1, path2);
    assert_string_equal(output, "/");
}

#pragma clang diagnostic pop

const struct CMUnitTest platformTest[] = {cmocka_unit_test(PathCombineString),
                                          cmocka_unit_test(PathCombineStringBothDividers),
                                          cmocka_unit_test(PathCombineStringDumbInput),
                                          cmocka_unit_test(PathCombineStringLeadingDivider),
                                          cmocka_unit_test(PathCombineStringNoDivider),
                                          cmocka_unit_test(PathCombineStringTrailingDivider)};

#endif /* NEW_TH_TEST_PLATFORM_H */
