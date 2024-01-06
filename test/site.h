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

static void SiteArrayFunctions(void **state) {
    Site site1, site2;
    memset(&site1, 0, sizeof(Site)), memset(&site2, 0, sizeof(Site));
    assert_null(siteNew(&site1, SITE_HTTP, "foo"));
    assert_null(siteNew(&site2, SITE_HTTP, "bar"));
    siteArrayInit();
    assert_null(siteArraySetActiveNth(0));
    assert_non_null(siteArraySetActiveNth(1));
    assert_null(siteArrayAdd(&site1));
    assert_null(siteArraySetActiveNth(1));
    assert_non_null(siteArraySetActiveNth(2));
    assert_null(siteArrayAdd(&site2));
    assert_null(siteArraySetActiveNth(2));
    assert_non_null(siteArraySetActiveNth(3));
    assert_null(siteArraySetActiveNth(2));
    assert_memory_equal(siteArrayGetActive(), &site2, sizeof(Site));
    assert_null(siteArraySetActiveNth(0));
    assert_null(siteArraySetActiveNth(1));
    assert_memory_equal(siteArrayGetActive(), &site1, sizeof(Site));
    assert_null(siteArraySetActiveNth(2));
    siteArrayRemove(&site1);
    assert_null(siteArrayGetActive());
    assert_int_equal(siteArrayGetActiveNth(), -1);
    assert_null(siteArraySetActiveNth(1));
    assert_memory_equal(siteArrayGetActive(), &site2, sizeof(Site));
    assert_null(siteArrayAdd(&site1));
    assert_null(siteArraySetActiveNth(2));
    assert_non_null(siteArrayGetActive());
    assert_int_equal(siteArrayGetActiveNth(), 2);
    siteArrayRemoveNth(2);
    assert_null(siteArrayGetActive());
    assert_int_equal(siteArrayGetActiveNth(), -1);
    siteArrayFree();
}

static void SiteFileNew(void **state) {
    Site site;
    char *wd = malloc(FILENAME_MAX + 1);
    assert_null(siteNew(&site, SITE_FILE, NULL));
    assert_non_null(platformGetWorkingDirectory(wd, FILENAME_MAX));

    assert_int_equal(site.type, SITE_FILE);
    assert_non_null(site.site.file.fullUri);
    assert_non_null(strstr(site.site.file.fullUri, wd));
    free(wd), siteFree(&site);
}

static void SiteFileNewWithPath(void **state) {
    const char *testPath = "/tmp";
    Site site;
    assert_null(siteNew(&site, SITE_FILE, testPath));
    assert_int_equal(site.type, SITE_FILE);
    assert_non_null(site.site.file.fullUri);
    assert_non_null(strstr(site.site.file.fullUri, testPath));
    siteFree(&site);
}

static void SiteFileGetDirectory(void **state) {
    Site site;
    char *wd = malloc(FILENAME_MAX + 1);

    assert_null(siteNew(&site, SITE_FILE, NULL));
    assert_non_null(platformGetWorkingDirectory(wd, FILENAME_MAX));
    assert_non_null(site.site.file.fullUri);

    assert_non_null(strstr(siteGetWorkingDirectory(&site), site.site.file.fullUri));
    free(wd), siteFree(&site);
}

static void SiteFileSetDirectory(void **state) {
    Site site;
    char *wd = malloc(FILENAME_MAX + 1);

    assert_null(siteNew(&site, SITE_FILE, NULL));
    assert_non_null(platformGetWorkingDirectory(wd, FILENAME_MAX));
    assert_non_null(site.site.file.fullUri);

    siteSetWorkingDirectory(&site, "..");
    assert_non_null(strstr(wd, &site.site.file.fullUri[7]));
    free(wd), siteFree(&site);
}

static void SiteFileDirEntry(void **state) {
    Site site;
    size_t i, j = i = 0;
    void *d, *e;

    assert_null(siteNew(&site, SITE_FILE, NULL));
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

const struct CMUnitTest siteTest[] = {cmocka_unit_test(SiteArrayFunctions), cmocka_unit_test(SiteFileNew),
                                      cmocka_unit_test(SiteFileNewWithPath), cmocka_unit_test(SiteFileGetDirectory),
                                      cmocka_unit_test(SiteFileSetDirectory), cmocka_unit_test(SiteFileDirEntry)};

#endif /* NEW_DL_TEST_SITE_H */
