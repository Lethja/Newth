#ifndef NEW_DL_TEST_SITE_H
#define NEW_DL_TEST_SITE_H

#include "../src/client/site.h"

#pragma region CMocka headers

#include <stdarg.h>
#include <stddef.h>
#include <stdint.h>
#include <setjmp.h>

#include <cmocka.h>

#pragma endregion

static void SiteFileNewDefault(void **state) {
    Site site = siteNew(SITE_FILE, NULL);
    char *wd = malloc(FILENAME_MAX + 1);
    assert_non_null(platformGetWorkingDirectory(wd, FILENAME_MAX));

    assert_int_equal(site.type, SITE_FILE);
    assert_non_null(site.site.file.workingDirectory);
    assert_non_null(strstr(site.site.file.workingDirectory, wd));
    free(wd), siteFree(&site);
}

static void SiteFileNewWithPath(void **state) {
    /* TODO: Implement */
    const char *testPath = "/foo/bar";
    Site site = siteNew(SITE_FILE, testPath);

    assert_int_equal(site.type, SITE_FILE);
    assert_non_null(site.site.file.workingDirectory);
    assert_non_null(strstr(site.site.file.workingDirectory, testPath));
    siteFree(&site);
}

static void SiteFileGetDirectory(void **state) {
    Site site = siteNew(SITE_FILE, NULL);
    char *wd = malloc(FILENAME_MAX + 1);
    assert_non_null(platformGetWorkingDirectory(wd, FILENAME_MAX));
    assert_non_null(site.site.file.workingDirectory);

    assert_non_null(strstr(siteGetWorkingDirectory(&site), site.site.file.workingDirectory));
    free(wd), siteFree(&site);
}

static void SiteFileSetDirectory(void **state) {
    Site site = siteNew(SITE_FILE, NULL);
    char *wd = malloc(FILENAME_MAX + 1);
    assert_non_null(platformGetWorkingDirectory(wd, FILENAME_MAX));
    assert_non_null(site.site.file.workingDirectory);

    siteSetWorkingDirectory(&site, "..");
    assert_non_null(strstr(wd, &site.site.file.workingDirectory[7]));
    free(wd), siteFree(&site);
}

static void SiteFileDirEntry(void **state) {
    Site site = siteNew(SITE_FILE, NULL);
    size_t i, j = i = 0;
    void *d, *e;

    d = platformDirOpen(".");
    while ((e = platformDirRead(d)))
        ++i;

    platformDirClose(d);

    d = siteOpenDirectoryListing(&site, ".");
    while ((e = siteReadDirectoryListing(&site, d)))
        ++j, siteDirectoryEntryFree(e);

    siteCloseDirectoryListing(&site, d), siteFree(&site);
}

const struct CMUnitTest siteTest[] = {cmocka_unit_test(SiteFileNewDefault), cmocka_unit_test(SiteFileGetDirectory),
                                      cmocka_unit_test(SiteFileSetDirectory), cmocka_unit_test(SiteFileDirEntry)};

#endif /* NEW_DL_TEST_SITE_H */
