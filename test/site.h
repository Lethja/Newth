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

#pragma region Site Array Tests

static void SiteArrayFunctions(void **state) {
    Site site1, site2;
    memset(&site1, 0, sizeof(Site)), memset(&site2, 0, sizeof(Site));
    assert_null(siteNew(&site1, SITE_FILE, "/"));
    assert_null(siteNew(&site2, SITE_FILE, "/"));
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
    siteArrayFree(), siteFree(&site1);
}

#pragma endregion

#pragma region File Site Tests

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

#pragma endregion

#pragma region Http Site Tests

#ifdef MOCK

const char *HttpHeaderResponseDir = "HTTP/1.1 200 OK" HTTP_EOL
                                    "Content-Type: text/html; charset=ISO-8859-1" HTTP_EOL
                                    "Date: Thu, 1 Jan 1970 00:00:00 GMT" HTTP_EOL HTTP_EOL;

const char *HttpHeaderResponseFile = "HTTP/1.1 200 OK" HTTP_EOL
                                     "Content-Type: text/html; charset=ISO-8859-1" HTTP_EOL
                                     "Content-Disposition: attachment" HTTP_EOL
                                     "Date: Thu, 1 Jan 1970 00:00:00 GMT" HTTP_EOL HTTP_EOL;

const char *HtmlBody = "<!DOCTYPE html>\n"
                       "<html>\n"
                       "<head>\n"
                       "\t<title>Directory Listing</title>\n"
                       "</head>\n"
                       "<body>\n"
                       "\t<h1>Directory Listing</h1>\n"
                       "\t<img href=\"Logo.png\" alt=\"Logo\">\n"
                       "\t<ul>\n"
                       "\t\t<li><a href=\"file1.txt\">file1.txt</a></li>\n"
                       "\t\t<li><a href=\"file2%2Etxt\">file2.txt</a></li>\n"
                       "\t\t<li><a href=\"/file3.txt\">file3.txt</a></li>\n"
                       "\t\t<li><a href=\"../file4.txt\">file4.txt</a></li>\n"
                       "\t\t<li><a href=\"/foo/file5.txt\">file5.txt</a></li>\n"
                       "\t\t<li><a href=\"http://example.com\">file6.txt</a></li>\n"
                       "\t</ul>\n"
                       "</body>\n"
                       "</html>";

static void SiteHttpNew(void **state) {
    Site site;
    assert_string_equal(siteNew(&site, SITE_HTTP, NULL), "No uri specified");
    assert_int_equal(site.type, SITE_HTTP);
    assert_int_equal(site.site.http.socket, -1);
    assert_null(site.site.http.fullUri);
    siteFree(&site);
}

static void SiteHttpNewWithPath(void **state) {
    Site site;

    mockReset(), mockOptions = MOCK_CONNECT | MOCK_SEND | MOCK_RECEIVE, mockSendMaxBuf = mockReceiveMaxBuf = 1024;
    mockReceiveStream = tmpfile(), fwrite(HttpHeaderResponseDir, 1, strlen(HttpHeaderResponseDir),
                                          mockReceiveStream), rewind(mockReceiveStream);

    assert_null(siteNew(&site, SITE_HTTP, "http://127.0.0.1"));
    assert_int_equal(site.type, SITE_HTTP);
    assert_non_null(site.site.http.fullUri);
    assert_string_equal(site.site.http.fullUri, "http://127.0.0.1/"); /* Note that '/' has been appended at the end */
    siteFree(&site);
}

static void SiteHttpGetDirectory(void **state) {
    Site site;
    UriDetails details;
    char *wd;

    mockReset(), mockOptions = MOCK_CONNECT | MOCK_SEND | MOCK_RECEIVE, mockSendMaxBuf = mockReceiveMaxBuf = 1024;
    mockReceiveStream = tmpfile(), fwrite(HttpHeaderResponseDir, 1, strlen(HttpHeaderResponseDir),
                                          mockReceiveStream), rewind(mockReceiveStream);

    assert_null(siteNew(&site, SITE_HTTP, "http://127.0.0.1"));
    assert_non_null(site.site.http.fullUri);

    wd = siteGetWorkingDirectory(&site);
    assert_string_equal(wd, "http://127.0.0.1/");

    details = uriDetailsNewFrom(wd);
    assert_string_equal(details.host, "127.0.0.1");
    assert_string_equal(details.path, "/");

    uriDetailsFree(&details), siteFree(&site);
}

static void SiteHttpSetDirectory(void **state) {
    Site site;
    char *wd = "http://127.0.0.1/foo";

    mockReset(), mockOptions = MOCK_CONNECT | MOCK_SEND | MOCK_RECEIVE, mockSendMaxBuf = mockReceiveMaxBuf = 1024;
    mockReceiveStream = tmpfile(), fwrite(HttpHeaderResponseDir, 1, strlen(HttpHeaderResponseDir),
                                          mockReceiveStream), rewind(mockReceiveStream);

    assert_null(siteNew(&site, SITE_HTTP, wd)), rewind(mockReceiveStream);
    assert_non_null(site.site.file.fullUri);
    assert_string_equal(siteGetWorkingDirectory(&site), wd);

    siteSetWorkingDirectory(&site, ".."), rewind(mockReceiveStream);
    assert_string_equal(siteGetWorkingDirectory(&site), "http://127.0.0.1/");

    siteSetWorkingDirectory(&site, "foo"), rewind(mockReceiveStream);
    assert_string_equal(siteGetWorkingDirectory(&site), wd);

    siteSetWorkingDirectory(&site, "bar"), rewind(mockReceiveStream);
    assert_string_equal(siteGetWorkingDirectory(&site), "http://127.0.0.1/foo/bar");

    siteSetWorkingDirectory(&site, "/bar");
    assert_string_equal(siteGetWorkingDirectory(&site), "http://127.0.0.1/bar");

    siteFree(&site);
}

static void SiteHttpSetDirectoryFailFile(void **state) {
    Site site;
    char *wd = "http://127.0.0.1/foo";

    mockReset(), mockOptions = MOCK_CONNECT | MOCK_SEND | MOCK_RECEIVE, mockSendMaxBuf = mockReceiveMaxBuf = 1024;
    mockReceiveStream = tmpfile(), fwrite(HttpHeaderResponseDir, 1, strlen(HttpHeaderResponseDir),
                                          mockReceiveStream), rewind(mockReceiveStream);

    assert_null(siteNew(&site, SITE_HTTP, wd)), rewind(mockReceiveStream);
    assert_non_null(site.site.file.fullUri);
    assert_string_equal(siteGetWorkingDirectory(&site), wd);
    fwrite(HttpHeaderResponseFile, 1, strlen(HttpHeaderResponseFile), mockReceiveStream), rewind(mockReceiveStream);

    assert_int_equal(siteSetWorkingDirectory(&site, "bar"), 1);
    assert_string_equal(siteGetWorkingDirectory(&site), wd);

    siteFree(&site);
}

static void SiteHttpDirEntry(void **state) {
    Site site;
    SiteDirectoryEntry *e;
    char length[255] = "";
    void *d;

    mockReset(), mockOptions = MOCK_CONNECT | MOCK_SEND | MOCK_RECEIVE, mockSendMaxBuf = mockReceiveMaxBuf = 1024;
    mockReceiveStream = tmpfile();

    /* Setup mocked response */
    sprintf(length, "content-length: %lu" HTTP_EOL HTTP_EOL, strlen(HtmlBody)); /* Deliberately lowercase */
    fwrite(HttpHeaderResponseDir, 1, strlen(HttpHeaderResponseDir) - 2, mockReceiveStream);
    fwrite(length, 1, strlen(length), mockReceiveStream);
    fwrite(HtmlBody, 1, strlen(HtmlBody), mockReceiveStream);
    rewind(mockReceiveStream);

    assert_null(siteNew(&site, SITE_HTTP, "http://127.0.0.1/foo"));

    /* Listing of http://127.0.0.1/foo */
    assert_non_null(d = siteOpenDirectoryListing(&site, "."));
    assert_non_null(e = siteReadDirectoryListing(&site, d));
    assert_string_equal(e->name, "file1.txt"), siteDirectoryEntryFree(e);
    assert_non_null(e = siteReadDirectoryListing(&site, d));
    assert_string_equal(e->name, "file2.txt"), siteDirectoryEntryFree(e);
    assert_non_null(e = siteReadDirectoryListing(&site, d));
    assert_string_equal(e->name, "file5.txt"), siteDirectoryEntryFree(e);
    siteCloseDirectoryListing(&site, d);

    /* Listing of http://127.0.0.1/ (absolute) */
    assert_non_null(d = siteOpenDirectoryListing(&site, "/"));
    assert_non_null(e = siteReadDirectoryListing(&site, d));
    assert_string_equal(e->name, "file1.txt"), siteDirectoryEntryFree(e);
    assert_non_null(e = siteReadDirectoryListing(&site, d));
    assert_string_equal(e->name, "file2.txt"), siteDirectoryEntryFree(e);
    assert_non_null(e = siteReadDirectoryListing(&site, d));
    assert_string_equal(e->name, "file3.txt"), siteDirectoryEntryFree(e);
    siteCloseDirectoryListing(&site, d);

    /* Listing of http://127.0.0.1/ (up) */
    assert_non_null(d = siteOpenDirectoryListing(&site, ".."));
    assert_non_null(e = siteReadDirectoryListing(&site, d));
    assert_string_equal(e->name, "file1.txt"), siteDirectoryEntryFree(e);
    assert_non_null(e = siteReadDirectoryListing(&site, d));
    assert_string_equal(e->name, "file2.txt"), siteDirectoryEntryFree(e);
    assert_non_null(e = siteReadDirectoryListing(&site, d));
    assert_string_equal(e->name, "file3.txt"), siteDirectoryEntryFree(e);
    siteCloseDirectoryListing(&site, d);

    siteFree(&site);
}

#endif /* MOCK */

#pragma endregion

#pragma clang diagnostic pop

const struct CMUnitTest siteTest[] = {cmocka_unit_test(SiteArrayFunctions), cmocka_unit_test(SiteFileNew),
                                      cmocka_unit_test(SiteFileNewWithPath), cmocka_unit_test(SiteFileGetDirectory),
                                      cmocka_unit_test(SiteFileSetDirectory), cmocka_unit_test(SiteFileDirEntry)
#ifdef MOCK
        , cmocka_unit_test(SiteHttpNew), cmocka_unit_test(SiteHttpNewWithPath), cmocka_unit_test(SiteHttpGetDirectory),
                                      cmocka_unit_test(SiteHttpSetDirectory),
                                      cmocka_unit_test(SiteHttpSetDirectoryFailFile), cmocka_unit_test(SiteHttpDirEntry)
#endif
};

#endif /* NEW_DL_TEST_SITE_H */
