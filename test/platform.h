#ifndef NEW_TH_TEST_PLATFORM_H
#define NEW_TH_TEST_PLATFORM_H

#include "../src/server/sendbufr.h"
#include <errno.h>
#include <string.h>

#ifdef DOS_DIVIDER
#define DIVIDER "\\"
#else
#define DIVIDER "/"
#endif

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
 * This test checks that two paths can be added together with neither having a path divider in it's string
 * @remark platformPathCombine() should not to be confused with uriPathCombine()
 */
static void PathCombineString(void **state) {
    const char *path1 = "foo", *path2 = "bar";
    char output[FILENAME_MAX];

    platformPathCombine(output, path1, path2);
    assert_string_equal(output, "foo" DIVIDER "bar");
}

/**
 * This test checks that two paths can be added together, if they have path dividers but not on the joining end
 * @remark platformPathCombine() should not to be confused with uriPathCombine()
 */
static void PathCombineStringNoDivider(void **state) {
    const char *path1 = DIVIDER "foo", *path2 = "bar" DIVIDER;
    char output[FILENAME_MAX];

    platformPathCombine(output, path1, path2);
    assert_string_equal(output, DIVIDER "foo" DIVIDER "bar" DIVIDER);
}

/**
 * This test checks that two paths can be added together, if the first has a path divider
 * @remark platformPathCombine() should not to be confused with uriPathCombine()
 */
static void PathCombineStringTrailingDivider(void **state) {
    const char *path1 = DIVIDER "foo" DIVIDER, *path2 = "bar" DIVIDER;
    char output[FILENAME_MAX];

    platformPathCombine(output, path1, path2);
    assert_string_equal(output, DIVIDER "foo" DIVIDER "bar" DIVIDER);
}

/**
 * This test checks that two paths can be added together, if the second has a path divider
 * @remark platformPathCombine() should not to be confused with uriPathCombine()
 */
static void PathCombineStringLeadingDivider(void **state) {
    const char *path1 = DIVIDER "foo", *path2 = DIVIDER "bar" DIVIDER;
    char output[FILENAME_MAX];

    platformPathCombine(output, path1, path2);
    assert_string_equal(output, DIVIDER "foo" DIVIDER "bar" DIVIDER);
}

/**
 * This test checks that two paths can be added together, if both strings have the divider
 * @remark platformPathCombine() should not to be confused with uriPathCombine()
 */
static void PathCombineStringBothDividers(void **state) {
    const char *path1 = DIVIDER "foo" DIVIDER, *path2 = DIVIDER "bar" DIVIDER;
    char output[FILENAME_MAX];

    platformPathCombine(output, path1, path2);
    assert_string_equal(output, DIVIDER "foo" DIVIDER "bar" DIVIDER);
}

/**
 * This test checks that non-sense paths are not combined in a non-sense way
 * @remark platformPathCombine() should not to be confused with uriPathCombine()
 */
static void PathCombineStringJustDividers(void **state) {
    const char *path1 = DIVIDER DIVIDER DIVIDER DIVIDER, *path2 = path1;
    char output[FILENAME_MAX];

    platformPathCombine(output, path1, path2);
    assert_string_equal(output, DIVIDER);
}

/**
 * This test checks that two root paths are not combined together
 * @remark platformPathCombine() should not to be confused with uriPathCombine()
 */
static void PathCombineStringDumbInput(void **state) {
    const char *path1, *path2 = path1 = DIVIDER;
    char output[FILENAME_MAX];

    platformPathCombine(output, path1, path2);
    assert_string_equal(output, DIVIDER);
}

/**
 * This test ensures a relative path is appended correctly
 */
static void PathCombineStringRelativeInput(void **state) {
    const char *path1 = ".", *path2 = "foo";
    char output[FILENAME_MAX];
    platformPathCombine(output, path1, path2);
    assert_string_equal(output, "./foo");
}

/**
 * This test checks that platformStringFindNeedle() (a.k.a case-insensitive strstr()) returns expected results
 */
static void HaystackAndNeedle(void **state) {
    const char *msg1 = "foo", *msg2 = "bar", *msg3 = "foobar", *msg4 = "barfoo", *msg5 = "";
    const char *MSG1 = "FOO", *MSG2 = "BAR";

    /* Empty string shouldn't match, what is there to match? */
    assert_null(platformStringFindNeedle(msg5, msg5));

    /* 'barfoo' starts to match 'foobar' but doesn't */
    assert_null(platformStringFindNeedle(msg3, msg4));

    /* 'foo' and 'bar' both match inside 'foobar' */
    assert_ptr_equal(platformStringFindNeedle(msg3, msg1), msg3);
    assert_ptr_equal(platformStringFindNeedle(msg3, msg2), &msg3[3]);

    /* 'FOO' and 'BAR' both match inside 'foobar' since the function is case insensitive */
    assert_ptr_equal(platformStringFindNeedle(msg3, MSG1), msg3);
    assert_ptr_equal(platformStringFindNeedle(msg3, MSG2), &msg3[3]);
}

/**
 * This test checks that a string can be converted into an argv char** array split by each word
 */
static void StringToArgv(void **state) {
    const char *str = "This is a test";
    char **argv = platformArgvConvertString(str);

    assert_string_equal(argv[0], "This");
    assert_string_equal(argv[1], "is");
    assert_string_equal(argv[2], "a");
    assert_string_equal(argv[3], "test");
    assert_null(argv[4]);

    platformArgvFree(argv);
}

/**
 * This test ensures that a single word string still gets converted into a argv with 1 argument
 */
static void StringToArgvOneWord(void **state) {
    const char *str = "Supercalifragilisticexpialidocious";
    char **argv = platformArgvConvertString(str);

    assert_string_equal(argv[0], str);
    assert_null(argv[1]);

    platformArgvFree(argv);
}

/**
 * This test ensures that excessive whitespace before an argument does not become part of any argument
 */
static void StringToArgvPaddingStart(void **state) {
    const char *str = "                  This is a test";
    char **argv = platformArgvConvertString(str);

    assert_string_equal(argv[0], "This");
    assert_string_equal(argv[1], "is");
    assert_string_equal(argv[2], "a");
    assert_string_equal(argv[3], "test");
    assert_null(argv[4]);

    platformArgvFree(argv);
}

/**
 * This test ensures that excessive whitespace between two arguments does not become part of any argument
 */
static void StringToArgvPaddingMiddle(void **state) {
    const char *str = "This is                   a test";
    char **argv = platformArgvConvertString(str);

    assert_string_equal(argv[0], "This");
    assert_string_equal(argv[1], "is");
    assert_string_equal(argv[2], "a");
    assert_string_equal(argv[3], "test");
    assert_null(argv[4]);

    platformArgvFree(argv);
}

/**
 * This test ensures that excessive whitespace after an arguments does not become part of any argument
 */
static void StringToArgvPaddingEnd(void **state) {
    const char *str = "This is a test                  ";
    char **argv = platformArgvConvertString(str);

    assert_string_equal(argv[0], "This");
    assert_string_equal(argv[1], "is");
    assert_string_equal(argv[2], "a");
    assert_string_equal(argv[3], "test");
    assert_null(argv[4]);

    platformArgvFree(argv);
}

/**
 * This test ensures that whitespace between double quotes is not treated as an argument divider
 */
static void StringToArgvEsc(void **state) {
    const char *str = "\"This is\" \"a\"\" test\"";
    char **argv = platformArgvConvertString(str);

    assert_string_equal(argv[0], "This is");
    assert_string_equal(argv[1], "a test");
    assert_null(argv[2]);

    platformArgvFree(argv);
}

/**
 * This test ensures that a systems temporary path can be located
 */
static void TemporaryPath(void **state) {
    const char *dir = platformTempDirectoryGet();
    assert_non_null(dir);
    printf("[  NOTICE  ] Temporary files stored at '%s'\n", platformTempDirectoryGet());
}

#ifdef DOS_DIVIDER

/**
 * This test ensures that platformPathCombine() accepts forward and backward slashes as path dividers if the system does
 */
static void PathCombineStringUnixDividers(void **state) {
    const char *path1 = "/foo/", *path2 = "/bar/";
    char output[FILENAME_MAX];

    platformPathCombine(output, path1, path2);
    assert_string_equal(output, "/foo" DIVIDER "bar/");
}

#endif

#pragma clang diagnostic pop

const struct CMUnitTest platformTest[] = {
    cmocka_unit_test(HaystackAndNeedle),
    cmocka_unit_test(PathCombineString),
    cmocka_unit_test(PathCombineStringBothDividers),
    cmocka_unit_test(PathCombineStringDumbInput),
    cmocka_unit_test(PathCombineStringJustDividers),
    cmocka_unit_test(PathCombineStringLeadingDivider),
    cmocka_unit_test(PathCombineStringNoDivider),
    cmocka_unit_test(PathCombineStringRelativeInput),
    cmocka_unit_test(PathCombineStringTrailingDivider),
#ifdef BACKSLASH_PATH_DIVIDER
    cmocka_unit_test(PathCombineStringUnixDividers),
#endif
    cmocka_unit_test(StringToArgv),
    cmocka_unit_test(StringToArgvEsc),
    cmocka_unit_test(StringToArgvOneWord),
    cmocka_unit_test(StringToArgvPaddingEnd),
    cmocka_unit_test(StringToArgvPaddingMiddle),
    cmocka_unit_test(StringToArgvPaddingStart),
    cmocka_unit_test(TemporaryPath)
};

#endif /* NEW_TH_TEST_PLATFORM_H */
