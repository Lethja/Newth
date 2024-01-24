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
    assert_null(siteArrayActiveSetNth(0));
    assert_non_null(siteArrayActiveSetNth(1));
    assert_null(siteArrayAdd(&site1));
    assert_null(siteArrayActiveSetNth(1));
    assert_non_null(siteArrayActiveSetNth(2));
    assert_null(siteArrayAdd(&site2));
    assert_null(siteArrayActiveSetNth(2));
    assert_non_null(siteArrayActiveSetNth(3));
    assert_null(siteArrayActiveSetNth(2));
    assert_memory_equal(siteArrayActiveGet(), &site2, sizeof(Site));
    assert_null(siteArrayActiveSetNth(0));
    assert_null(siteArrayActiveSetNth(1));
    assert_memory_equal(siteArrayActiveGet(), &site1, sizeof(Site));
    assert_null(siteArrayActiveSetNth(2));
    siteArrayRemove(&site1);
    assert_null(siteArrayActiveGet());
    assert_int_equal(siteArrayActiveGetNth(), -1);
    assert_null(siteArrayActiveSetNth(1));
    assert_memory_equal(siteArrayActiveGet(), &site2, sizeof(Site));
    assert_null(siteArrayAdd(&site1));
    assert_null(siteArrayActiveSetNth(2));
    assert_non_null(siteArrayActiveGet());
    assert_int_equal(siteArrayActiveGetNth(), 2);
    siteArrayRemoveNth(2);
    assert_null(siteArrayActiveGet());
    assert_int_equal(siteArrayActiveGetNth(), -1);
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

    assert_non_null(strstr(siteWorkingDirectoryGet(&site), site.site.file.fullUri));
    free(wd), siteFree(&site);
}

static void SiteFileSetDirectory(void **state) {
    Site site;
    char *wd = malloc(FILENAME_MAX + 1);

    assert_null(siteNew(&site, SITE_FILE, NULL));
    assert_non_null(platformGetWorkingDirectory(wd, FILENAME_MAX));
    assert_non_null(site.site.file.fullUri);

    siteWorkingDirectorySet(&site, "..");
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

    platformDirClose(d), d = siteDirectoryListingOpen(&site, ".");
    assert_non_null(d);

    while ((e = siteDirectoryListingRead(&site, d)))
        ++j, siteDirectoryEntryFree(e);

    assert_int_equal(i, j);

    siteDirectoryListingClose(&site, d), siteFree(&site);
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

    wd = siteWorkingDirectoryGet(&site);
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
    assert_string_equal(siteWorkingDirectoryGet(&site), wd);

    siteWorkingDirectorySet(&site, ".."), rewind(mockReceiveStream);
    assert_string_equal(siteWorkingDirectoryGet(&site), "http://127.0.0.1/");

    siteWorkingDirectorySet(&site, "foo"), rewind(mockReceiveStream);
    assert_string_equal(siteWorkingDirectoryGet(&site), wd);

    siteWorkingDirectorySet(&site, "bar"), rewind(mockReceiveStream);
    assert_string_equal(siteWorkingDirectoryGet(&site), "http://127.0.0.1/foo/bar");

    siteWorkingDirectorySet(&site, "/bar");
    assert_string_equal(siteWorkingDirectoryGet(&site), "http://127.0.0.1/bar");

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
    assert_string_equal(siteWorkingDirectoryGet(&site), wd);
    fwrite(HttpHeaderResponseFile, 1, strlen(HttpHeaderResponseFile), mockReceiveStream), rewind(mockReceiveStream);

    assert_int_equal(siteWorkingDirectorySet(&site, "bar"), 1);
    assert_string_equal(siteWorkingDirectoryGet(&site), wd);

    siteFree(&site);
}

static void SiteHttpDirEntry(void **state) {
    const char *HttpBody = "<!DOCTYPE html>\n"
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
    Site site;
    SiteDirectoryEntry *e;
    char length[255] = "";
    void *d;

    mockReset(), mockOptions = MOCK_CONNECT | MOCK_SEND | MOCK_RECEIVE, mockSendMaxBuf = mockReceiveMaxBuf = 1024;
    mockReceiveStream = tmpfile();

    /* Setup mocked response */
    sprintf(length, "content-length: %lu" HTTP_EOL HTTP_EOL, strlen(HttpBody)); /* Deliberately lowercase */
    fwrite(HttpHeaderResponseDir, 1, strlen(HttpHeaderResponseDir) - 2, mockReceiveStream);
    fwrite(length, 1, strlen(length), mockReceiveStream);
    fwrite(HttpBody, 1, strlen(HttpBody), mockReceiveStream);
    rewind(mockReceiveStream);

    assert_null(siteNew(&site, SITE_HTTP, "http://127.0.0.1/foo")), rewind(mockReceiveStream);

    /* Listing of http://127.0.0.1/foo */
    assert_non_null(d = siteDirectoryListingOpen(&site, "."));
    assert_non_null(e = siteDirectoryListingRead(&site, d));
    assert_string_equal(e->name, "file1.txt"), siteDirectoryEntryFree(e);
    assert_non_null(e = siteDirectoryListingRead(&site, d));
    assert_string_equal(e->name, "file2.txt"), siteDirectoryEntryFree(e);
    assert_non_null(e = siteDirectoryListingRead(&site, d));
    assert_string_equal(e->name, "file5.txt"), siteDirectoryEntryFree(e);
    siteDirectoryListingClose(&site, d), rewind(mockReceiveStream);

    /* Listing of http://127.0.0.1/ (absolute) */
    assert_non_null(d = siteDirectoryListingOpen(&site, "/"));
    assert_non_null(e = siteDirectoryListingRead(&site, d));
    assert_string_equal(e->name, "file1.txt"), siteDirectoryEntryFree(e);
    assert_non_null(e = siteDirectoryListingRead(&site, d));
    assert_string_equal(e->name, "file2.txt"), siteDirectoryEntryFree(e);
    assert_non_null(e = siteDirectoryListingRead(&site, d));
    assert_string_equal(e->name, "file3.txt"), siteDirectoryEntryFree(e);
    siteDirectoryListingClose(&site, d), rewind(mockReceiveStream);

    /* Listing of http://127.0.0.1/ (up) */
    assert_non_null(d = siteDirectoryListingOpen(&site, ".."));
    assert_non_null(e = siteDirectoryListingRead(&site, d));
    assert_string_equal(e->name, "file1.txt"), siteDirectoryEntryFree(e);
    assert_non_null(e = siteDirectoryListingRead(&site, d));
    assert_string_equal(e->name, "file2.txt"), siteDirectoryEntryFree(e);
    assert_non_null(e = siteDirectoryListingRead(&site, d));
    assert_string_equal(e->name, "file3.txt"), siteDirectoryEntryFree(e);
    siteDirectoryListingClose(&site, d), rewind(mockReceiveStream);

    siteFree(&site);
}

static void SiteHttpDirEntryApache(void **state) {
    const char *ApacheHtml = "<!DOCTYPE HTML PUBLIC \"-//W3C//DTD HTML 3.2 Final//EN\">\n"
                             "<html>\n"
                             "<body>\n"
                             "<pre><img src=\"/icons/blank.gif\" alt=\"Icon \">"
                             "<a href=\"?C=N;O=D\">Name</a> "
                             "<a href=\"?C=M;O=A\">Last</a>"
                             "<a href=\"?C=S;O=A\">Size</a> "
                             "<a href=\"?C=D;O=A\">Description</a><hr>"
                             "<a href=\"cloud/\">cloud/</a> 1970-01-01 00:00 - \n"
                             "<a href=\"cloud2/\">cloud2/</a> 1970-01-01 00:00 - \n"
                             "<a href=\"foo\">foo</a> 1970-01-01 00:00 0 \n"
                             "<hr></pre>\n"
                             "</body></html>";
    Site site;
    SiteDirectoryEntry *e;
    char length[255] = "";
    void *d;

    mockReset(), mockOptions = MOCK_CONNECT | MOCK_SEND | MOCK_RECEIVE, mockSendMaxBuf = mockReceiveMaxBuf = 1024;
    mockReceiveStream = tmpfile();

    /* Setup mocked response */
    sprintf(length, "content-length: %lu" HTTP_EOL HTTP_EOL, strlen(ApacheHtml)); /* Deliberately lowercase */
    fwrite(HttpHeaderResponseDir, 1, strlen(HttpHeaderResponseDir) - 2, mockReceiveStream);
    fwrite(length, 1, strlen(length), mockReceiveStream);
    fwrite(ApacheHtml, 1, strlen(ApacheHtml), mockReceiveStream);
    rewind(mockReceiveStream);

    assert_null(siteNew(&site, SITE_HTTP, "http://127.0.0.1/")), rewind(mockReceiveStream);

    /* Listing of http://127.0.0.1/ */
    assert_non_null(d = siteDirectoryListingOpen(&site, "."));
    assert_non_null(e = siteDirectoryListingRead(&site, d));
    assert_string_equal(e->name, "cloud"), siteDirectoryEntryFree(e);
    assert_non_null(e = siteDirectoryListingRead(&site, d));
    assert_string_equal(e->name, "cloud2"), siteDirectoryEntryFree(e);
    assert_non_null(e = siteDirectoryListingRead(&site, d));
    assert_string_equal(e->name, "foo"), siteDirectoryEntryFree(e);
    assert_null(siteDirectoryListingRead(&site, d));

    siteDirectoryListingClose(&site, d), rewind(mockReceiveStream);

    siteFree(&site);
}

static void SiteHttpDirEntryLighttpd(void **state) {
    const char *LighttpdHtml = "<!DOCTYPE html PUBLIC \"-//W3C//DTD XHTML 1.1//EN\" \"http://www.w3.org/TR/xhtml11/DTD/xhtml11.dtd\">\n"
                               "<html xmlns=\"http://www.w3.org/1999/xhtml\" xml:lang=\"en\">\n"
                               "<body>\n"
                               "<tr><td class=\"n\"><a href=\"../\">Parent Directory</a>/</td>"
                               "<tr><td class=\"n\"><a href=\"file.txt\">file.txt</a></td>"
                               "<tr><td class=\"n\"><a href=\"file.txt.sig\">file.txt.sig</a>"
                               "</td><td class=\"m\">1970-Jan-01 00:00:00</td>"
                               "<td class=\"s\">0.1K</td><td class=\"t\">application/pgp-signature</td></tr>\n";
    Site site;
    SiteDirectoryEntry *e;
    char length[255] = "";
    void *d;

    mockReset(), mockOptions = MOCK_CONNECT | MOCK_SEND | MOCK_RECEIVE, mockSendMaxBuf = mockReceiveMaxBuf = 1024;
    mockReceiveStream = tmpfile();

    /* Setup mocked response */
    sprintf(length, "content-length: %lu" HTTP_EOL HTTP_EOL, strlen(LighttpdHtml)); /* Deliberately lowercase */
    fwrite(HttpHeaderResponseDir, 1, strlen(HttpHeaderResponseDir) - 2, mockReceiveStream);
    fwrite(length, 1, strlen(length), mockReceiveStream);
    fwrite(LighttpdHtml, 1, strlen(LighttpdHtml), mockReceiveStream);
    rewind(mockReceiveStream);

    assert_null(siteNew(&site, SITE_HTTP, "http://127.0.0.1/")), rewind(mockReceiveStream);

    /* Listing of http://127.0.0.1/ */
    assert_non_null(d = siteDirectoryListingOpen(&site, "."));
    assert_non_null(e = siteDirectoryListingRead(&site, d));
    assert_string_equal(e->name, "file.txt"), siteDirectoryEntryFree(e);
    assert_non_null(e = siteDirectoryListingRead(&site, d));
    assert_string_equal(e->name, "file.txt.sig"), siteDirectoryEntryFree(e);

    siteDirectoryListingClose(&site, d), rewind(mockReceiveStream);

    siteFree(&site);
}

static void SiteHttpDirEntryNginx(void **state) {
    const char *NginxHttp = "<html>\n"
                            "<head><title>Index of /</title></head>\n"
                            "<body>\n"
                            "<h1>Index of /</h1><hr><pre><a href=\"../\">../</a>\n"
                            "<a href=\"cloud/\">cloud/</a>                                             01-Jan-1970 00:00                   -\n"
                            "<a href=\"cloud2/\">cloud2/</a>                                            01-Jan-1970 00:00                   -\n"
                            "<a href=\"foo\">foo</a>                                                01-Jan-1970 00:00                   0\n"
                            "</pre><hr></body>\n"
                            "</html>";
    Site site;
    SiteDirectoryEntry *e;
    char length[255] = "";
    void *d;

    mockReset(), mockOptions = MOCK_CONNECT | MOCK_SEND | MOCK_RECEIVE, mockSendMaxBuf = mockReceiveMaxBuf = 1024;
    mockReceiveStream = tmpfile();

    /* Setup mocked response */
    sprintf(length, "content-length: %lu" HTTP_EOL HTTP_EOL, strlen(NginxHttp)); /* Deliberately lowercase */
    fwrite(HttpHeaderResponseDir, 1, strlen(HttpHeaderResponseDir) - 2, mockReceiveStream);
    fwrite(length, 1, strlen(length), mockReceiveStream);
    fwrite(NginxHttp, 1, strlen(NginxHttp), mockReceiveStream);
    rewind(mockReceiveStream);

    assert_null(siteNew(&site, SITE_HTTP, "http://127.0.0.1/")), rewind(mockReceiveStream);

    /* Listing of http://127.0.0.1/ */
    assert_non_null(d = siteDirectoryListingOpen(&site, "."));
    assert_non_null(e = siteDirectoryListingRead(&site, d));
    assert_string_equal(e->name, "cloud"), siteDirectoryEntryFree(e);
    assert_non_null(e = siteDirectoryListingRead(&site, d));
    assert_string_equal(e->name, "cloud2"), siteDirectoryEntryFree(e);
    assert_non_null(e = siteDirectoryListingRead(&site, d));
    assert_string_equal(e->name, "foo"), siteDirectoryEntryFree(e);

    siteDirectoryListingClose(&site, d), rewind(mockReceiveStream);

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
                                      cmocka_unit_test(SiteHttpSetDirectoryFailFile),
                                      cmocka_unit_test(SiteHttpDirEntry), cmocka_unit_test(SiteHttpDirEntryApache),
                                      cmocka_unit_test(SiteHttpDirEntryLighttpd),
                                      cmocka_unit_test(SiteHttpDirEntryNginx)
#endif
};

#endif /* NEW_DL_TEST_SITE_H */
