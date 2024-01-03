#ifndef NEW_DL_TEST_SITE_H
#define NEW_DL_TEST_SITE_H

#include "../src/client/site.h"

#pragma clang diagnostic push
#pragma ide diagnostic ignored "UnusedParameter"

#pragma region CMocka headers

#include <stdarg.h>
#include <stddef.h>
#include <stdint.h>
#include <setjmp.h>

#include <cmocka.h>

#pragma endregion

static void SiteFileNew(void **state) {
    Site site = siteNew(SITE_FILE, NULL);
    char *wd = malloc(FILENAME_MAX + 1);
    assert_non_null(platformGetWorkingDirectory(wd, FILENAME_MAX));

    assert_int_equal(site.type, SITE_FILE);
    assert_non_null(site.site.file.fullUri);
    assert_non_null(strstr(site.site.file.fullUri, wd));
    free(wd), siteFree(&site);
}

static void SiteFileNewWithPath(void **state) {
    const char *testPath = "/tmp";
    Site site = siteNew(SITE_FILE, testPath);

    assert_int_equal(site.type, SITE_FILE);
    assert_non_null(site.site.file.fullUri);
    assert_non_null(strstr(site.site.file.fullUri, testPath));
    siteFree(&site);
}

static void SiteFileGetDirectory(void **state) {
    Site site = siteNew(SITE_FILE, NULL);
    char *wd = malloc(FILENAME_MAX + 1);
    assert_non_null(platformGetWorkingDirectory(wd, FILENAME_MAX));
    assert_non_null(site.site.file.fullUri);

    assert_non_null(strstr(siteGetWorkingDirectory(&site), site.site.file.fullUri));
    free(wd), siteFree(&site);
}

static void SiteFileSetDirectory(void **state) {
    Site site = siteNew(SITE_FILE, NULL);
    char *wd = malloc(FILENAME_MAX + 1);
    assert_non_null(platformGetWorkingDirectory(wd, FILENAME_MAX));
    assert_non_null(site.site.file.fullUri);

    siteSetWorkingDirectory(&site, "..");
    assert_non_null(strstr(wd, &site.site.file.fullUri[7]));
    free(wd), siteFree(&site);
}

static void SiteFileDirEntry(void **state) {
    Site site = siteNew(SITE_FILE, NULL);
    size_t i, j = i = 0;
    void *d, *e;

    d = platformDirOpen(".");
    assert_non_null(d);

    while ((e = platformDirRead(d))) {
        if (platformDirEntryGetName(e, NULL)[0] != '.') /* Exclude all names starting with '.' */
            ++i;
    }

    platformDirClose(d), d = siteOpenDirectoryListing(&site, ".");
    assert_non_null(d);

    while ((e = siteReadDirectoryListing(&site, d)))
        ++j, siteDirectoryEntryFree(e);

    assert_int_equal(i, j);

    siteCloseDirectoryListing(&site, d), siteFree(&site);
}

#pragma clang diagnostic pop

const struct CMUnitTest siteTest[] = {cmocka_unit_test(SiteFileNew), cmocka_unit_test(SiteFileNewWithPath),
                                      cmocka_unit_test(SiteFileGetDirectory), cmocka_unit_test(SiteFileSetDirectory),
                                      cmocka_unit_test(SiteFileDirEntry)};

#endif /* NEW_DL_TEST_SITE_H */
