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

/**
 * This test ensures several sites can be added and removed from a site array
 */
static void SiteArrayFunctions(void **state) {
    SiteArray array;
    Site site1, site2;
    char *uri = platformPathSystemToFileScheme((char*) platformTempDirectoryGet());
    memset(&site1, 0, sizeof(Site)), memset(&site2, 0, sizeof(Site));
    assert_null(siteNew(&site1, SITE_FILE, uri));
    assert_null(siteNew(&site2, SITE_FILE, uri)), free(uri);
    assert_null(siteArrayInit(&array));
    assert_null(siteArrayActiveSetNth(&array, 0));
    assert_non_null(siteArrayActiveSetNth(&array, 1));
    assert_null(siteArrayAdd(&array, &site1));
    assert_null(siteArrayActiveSetNth(&array, 1));
    assert_non_null(siteArrayActiveSetNth(&array, 2));
    assert_null(siteArrayAdd(&array, &site2));
    assert_null(siteArrayActiveSetNth(&array, 2));
    assert_non_null(siteArrayActiveSetNth(&array, 3));
    assert_null(siteArrayActiveSetNth(&array, 2));
    assert_memory_equal(siteArrayActiveGet(&array), &site2, sizeof(Site));
    assert_null(siteArrayActiveSetNth(&array, 0));
    assert_null(siteArrayActiveSetNth(&array, 1));
    assert_memory_equal(siteArrayActiveGet(&array), &site1, sizeof(Site));
    assert_null(siteArrayActiveSetNth(&array, 2));
    siteArrayRemove(&array, &site1);
    assert_null(siteArrayActiveGet(&array));
    assert_int_equal(siteArrayActiveGetNth(&array), -1);
    assert_null(siteArrayActiveSetNth(&array, 1));
    assert_memory_equal(siteArrayActiveGet(&array), &site2, sizeof(Site));
    assert_null(siteArrayAdd(&array, &site1));
    assert_null(siteArrayActiveSetNth(&array, 2));
    assert_non_null(siteArrayActiveGet(&array));
    assert_int_equal(siteArrayActiveGetNth(&array), 2);
    siteArrayRemoveNth(&array, 2);
    assert_null(siteArrayActiveGet(&array));
    assert_int_equal(siteArrayActiveGetNth(&array), -1);
    siteArrayFree(&array), siteFree(&site1);
}

#pragma endregion

#pragma region File Site Tests

/**
 * This test ensures that siteNew() creates a file site without any junk data when no path is specified
 */
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

/**
 * This test ensures that siteNew() creates a file site with a path & without any junk data
 */
static void SiteFileNewWithPath(void **state) {
    char *testPath = platformPathSystemToFileScheme((char *) platformTempDirectoryGet());
    Site site;
    assert_null(siteNew(&site, SITE_FILE, testPath));
    assert_int_equal(site.type, SITE_FILE);
    assert_non_null(site.site.file.fullUri);
    assert_non_null(strstr(site.site.file.fullUri, testPath));
    free(testPath), siteFree(&site);
}

/**
 * This test checks that siteWorkingDirectoryGet() on a file site gets the correct path for a site
 */
static void SiteFileGetDirectory(void **state) {
    Site site;
    char *wd = malloc(FILENAME_MAX + 1);

    assert_null(siteNew(&site, SITE_FILE, NULL));
    assert_non_null(platformGetWorkingDirectory(wd, FILENAME_MAX));
    assert_non_null(site.site.file.fullUri);

    assert_non_null(strstr(siteWorkingDirectoryGet(&site), site.site.file.fullUri));
    free(wd), siteFree(&site);
}

/**
 * This test checks that siteWorkingDirectorySet() on a file site will set the correct path
 */
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

/**
 * This test checks that directory on a file site can be opened and listed
 */
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

/**
 * This test checks that a file on a file site can be opened and read
 */
static void SiteFileOpenFile(void **state) {
    FILE *f;
    const char *data = "The quick brown fox jumps over the lazy dog";
    Site site;
    SiteFileMeta *meta;
    PlatformFileStat st;
    char *p0 = platformPathSystemToFileScheme((char *) platformTempDirectoryGet()), *p1 = platformTempFilePath("nt_f1");

    assert_non_null(f = fopen(p1, "wb"));
    assert_int_equal(fwrite(data, 1, strlen(data), f), strlen(data)), fclose(f);
    assert_false(platformFileStat(p1, &st)), free(p1);

    assert_null(siteNew(&site, SITE_FILE, p0)), free(p0);

    assert_null(siteFileOpenRead(&site, "nt_f1", -1, -1));
    assert_non_null(meta = siteFileOpenMeta(&site));
    assert_non_null(meta->path);
    assert_non_null(meta->name);
    assert_string_equal(meta->name, "nt_f1");
    assert_int_equal(meta->length, st.st_size);
    assert_int_equal(meta->type, SITE_FILE_TYPE_FILE);
    assert_non_null(meta->modifiedDate);

    siteFree(&site); /* siteFree() should close the files if required */
}

/**
 * This test ensures that a file site can copy an open to read file to another file site open to write file
 */
static void SiteFileTransferToFile(void **state) {
    void *f;
    const char *data = "The quick brown fox jumps over the lazy dog", buf[44] = {0};
    Site site1, site2;
    char *p0, *p1 = platformTempFilePath("nt_f1"), *p2 = platformTempFilePath("nt_f2");

    assert_non_null(f = fopen(p1, "wb")), free(p1);
    assert_int_equal(fwrite(data, 1, strlen(data), f), strlen(data));
    fclose(f), f = fopen(p2, "wb"), fclose(f);

    p0 = platformPathSystemToFileScheme((char *) platformTempDirectoryGet());
    assert_null(siteNew(&site1, SITE_FILE, p0));
    assert_null(siteNew(&site2, SITE_FILE, p0)), free(p0);

    assert_null(siteFileOpenRead(&site1, "nt_f1", -1, -1));
    assert_null(siteFileOpenWrite(&site2, "nt_f2"));

    assert_int_equal(siteFileRead(&site1, (char *) buf, 22), 22);
    assert_int_equal(siteFileWrite(&site2, (char *) buf, 22), 22);

    assert_int_equal(siteFileRead(&site1, (char *) buf, 22), 21);
    assert_int_equal(siteFileWrite(&site2, (char *) buf, 21), 21);

    siteFree(&site1), siteFree(&site2); /* siteFree() should close the files if required */

    assert_non_null(f = fopen(p2, "rb")), free(p2);
    assert_int_equal(fread((char*) buf, 1, strlen(data), f), strlen(data));
    assert_string_equal(data, buf);
    fclose(f);
}

/**
 * This test ensures that a incomplete site file copy can be resumed between two file sites
 */
static void SiteFileTransferToFileContinue(void **state) {
    FILE *f;
    const char *data = "The quick brown fox jumps over the lazy dog", buf[44] = {0};
    Site site1, site2;
    char *p0, *p1 = platformTempFilePath("nt_f1"), *p2 = platformTempFilePath("nt_f2");

    assert_non_null(f = fopen(p1, "wb")), free(p1);
    assert_int_equal(fwrite(data, 1, strlen(data), f), strlen(data));
    fclose(f), f = fopen(p2, "wb"), fclose(f);

    p0 = platformPathSystemToFileScheme((char *) platformTempDirectoryGet());
    assert_null(siteNew(&site1, SITE_FILE, p0));
    assert_null(siteNew(&site2, SITE_FILE, p0)), free(p0);

    assert_null(siteFileOpenRead(&site1, "nt_f1", 35, -1));
    assert_null(siteFileOpenWrite(&site2, "nt_f2"));

    assert_int_equal(siteFileRead(&site1, (char *) buf, 22), 8);
    assert_int_equal(siteFileWrite(&site2, (char *) buf, 8), 8);

    siteFree(&site1), siteFree(&site2); /* siteFree() should close the files if required */

    assert_non_null(f = fopen(p2, "rb")), free(p2);
    assert_int_equal(fread((char*) buf, 1, strlen(data), f), strlen(&data[35]));
    assert_string_equal(&data[35], buf);
    fclose(f);
}

#pragma endregion

#pragma region Http Site Tests

#ifdef MOCK

const char *HttpHeaderResponseDir = "HTTP/1.1 200 OK" HTTP_EOL
                                    "Content-Type: text/html; charset=ISO-8859-1" HTTP_EOL
                                    "Date: Thu, 1 Jan 1970 00:00:00 GMT" HTTP_EOL HTTP_EOL;

const char *HttpHeaderResponseDirChunk = "HTTP/1.1 200 OK" HTTP_EOL
        "Content-Type: text/html; charset=ISO-8859-1" HTTP_EOL
        "Date: Thu, 1 Jan 1970 00:00:00 GMT" HTTP_EOL
        "Transfer-Encoding: chunked" HTTP_EOL HTTP_EOL;

const char *HttpHeaderResponseFile = "HTTP/1.1 200 OK" HTTP_EOL
                                     "Content-Type: text/html; charset=ISO-8859-1" HTTP_EOL
                                     "Content-Disposition: attachment" HTTP_EOL
                                     "Date: Thu, 1 Jan 1970 00:00:00 GMT" HTTP_EOL HTTP_EOL;

/**
 * This test ensures that siteNew() will error out gracefully on http site without a path
 */
static void SiteHttpNew(void **state) {
    Site site;
    assert_ptr_equal(siteNew(&site, SITE_HTTP, NULL), ErrNoUriSpecified);
    assert_int_equal(site.type, SITE_HTTP);
    assert_int_equal(site.site.http.socket.serverSocket, -1);
    assert_null(site.site.http.fullUri);
    siteFree(&site);
}

/**
 * This test ensures that siteNew() creates a http site without any junk data
 */
static void SiteHttpNewWithPath(void **state) {
    Site site;

    mockReset(), mockOptions = MOCK_CONNECT | MOCK_SEND | MOCK_RECEIVE, mockSendMaxBuf = mockReceiveMaxBuf = 1024;
    assert_non_null((mockReceiveStream = tmpfile()));
    fwrite(HttpHeaderResponseDir, 1, strlen(HttpHeaderResponseDir), mockReceiveStream), rewind(mockReceiveStream);

    assert_null(siteNew(&site, SITE_HTTP, "http://127.0.0.1"));
    assert_int_equal(site.type, SITE_HTTP);
    assert_non_null(site.site.http.fullUri);
    assert_string_equal(site.site.http.fullUri, "http://127.0.0.1/"); /* Note that '/' has been appended at the end */
    siteFree(&site);
}

/**
 * This test checks that siteWorkingDirectoryGet() on a http site gets the correct path for a site
 */
static void SiteHttpGetDirectory(void **state) {
    Site site;
    UriDetails details;
    char *wd;

    mockReset(), mockOptions = MOCK_CONNECT | MOCK_SEND | MOCK_RECEIVE, mockSendMaxBuf = mockReceiveMaxBuf = 1024;
    assert_non_null((mockReceiveStream = tmpfile()));
    fwrite(HttpHeaderResponseDir, 1, strlen(HttpHeaderResponseDir), mockReceiveStream), rewind(mockReceiveStream);

    assert_null(siteNew(&site, SITE_HTTP, "http://127.0.0.1"));
    assert_non_null(site.site.http.fullUri);

    wd = siteWorkingDirectoryGet(&site);
    assert_string_equal(wd, "http://127.0.0.1/");

    details = uriDetailsNewFrom(wd);
    assert_string_equal(details.host, "127.0.0.1");
    assert_string_equal(details.path, "/");

    uriDetailsFree(&details), siteFree(&site);
}

/**
 * This test checks that siteWorkingDirectorySet() on a http site will set the correct path
 */
static void SiteHttpSetDirectory(void **state) {
    Site site;
    char *wd = "http://127.0.0.1/foo";

    mockReset(), mockOptions = MOCK_CONNECT | MOCK_SEND | MOCK_RECEIVE, mockSendMaxBuf = mockReceiveMaxBuf = 1024;
    assert_non_null((mockReceiveStream = tmpfile()));
    fwrite(HttpHeaderResponseDir, 1, strlen(HttpHeaderResponseDir), mockReceiveStream), rewind(mockReceiveStream);

    assert_null(siteNew(&site, SITE_HTTP, wd)), rewind(mockReceiveStream);
    assert_non_null(site.site.http.fullUri);
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

/**
 * This test ensures siteWorkingDirectorySet() gracefully rejects setting a path to a file on a http site
 */
static void SiteHttpSetDirectoryFailFile(void **state) {
    Site site;
    char *wd = "http://127.0.0.1/foo";

    mockReset(), mockOptions = MOCK_CONNECT | MOCK_SEND | MOCK_RECEIVE, mockSendMaxBuf = mockReceiveMaxBuf = 1024;
    assert_non_null((mockReceiveStream = tmpfile()));
    fwrite(HttpHeaderResponseDir, 1, strlen(HttpHeaderResponseDir), mockReceiveStream), rewind(mockReceiveStream);

    assert_null(siteNew(&site, SITE_HTTP, wd)), rewind(mockReceiveStream);
    assert_non_null(site.site.http.fullUri);
    assert_string_equal(siteWorkingDirectoryGet(&site), wd);
    fwrite(HttpHeaderResponseFile, 1, strlen(HttpHeaderResponseFile), mockReceiveStream), rewind(mockReceiveStream);

    assert_int_equal(siteWorkingDirectorySet(&site, "bar"), 1);
    assert_string_equal(siteWorkingDirectoryGet(&site), wd);

    siteFree(&site);
}

/**
 * This test parses a mockup of a generic html file listing with good links
 */
static void SiteHttpDirEntryGood(void **state) {
    const char *HttpBody = "<!DOCTYPE html>\n<html>\n<head>\n"
                           "\t<title>Directory Listing</title>\n"
                           "</head>\n<body>\n\t<h1>Directory Listing</h1>\n"
                           "\t<img href=\"Logo.png\" alt=\"Logo\">\n\t<ul>\n"
                           "\t\t<li><a href=\"file1.txt\">file1.txt</a></li>\n"
                           "\t\t<li><a href=\"file2%2Etxt\">file2.txt</a></li>\n"
                           "\t\t<li><a href=\"/file3.txt\">file3.txt</a></li>\n"
                           "\t\t<li><a href=\"/foo/file5.txt\">file5.txt</a></li>\n"
                           "\t\t<li><a href=\"/foo/bar\">bar</a></li>\n"
                           "\t\t<li><a href=\"/foo/bar/\">bar</a></li>\n"
                           "\t\t<li><a href=\"foo\">bar</a></li>\n"
                           "\t\t<li><a href=\"foo/\">bar/</a></li>\n"
                           "\t</ul>\n</body>\n</html>";
    Site site;
    SiteFileMeta *e;
    char length[255] = "";
    void *d;

    mockReset(), mockOptions = MOCK_CONNECT | MOCK_SEND | MOCK_RECEIVE, mockSendMaxBuf = mockReceiveMaxBuf = 1024;
    assert_non_null((mockReceiveStream = tmpfile()));

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
    assert_non_null(e = siteDirectoryListingRead(&site, d));
    assert_string_equal(e->name, "bar"), siteDirectoryEntryFree(e);
    assert_non_null(e = siteDirectoryListingRead(&site, d));
    assert_string_equal(e->name, "bar"), siteDirectoryEntryFree(e);
    assert_non_null(e = siteDirectoryListingRead(&site, d));
    assert_string_equal(e->name, "foo"), siteDirectoryEntryFree(e);
    assert_non_null(e = siteDirectoryListingRead(&site, d));
    assert_string_equal(e->name, "foo"), siteDirectoryEntryFree(e);
    siteDirectoryListingClose(&site, d), rewind(mockReceiveStream);

    /* Listing of http://127.0.0.1/ (absolute) */
    assert_non_null(d = siteDirectoryListingOpen(&site, "/"));
    assert_non_null(e = siteDirectoryListingRead(&site, d));
    assert_string_equal(e->name, "file1.txt"), siteDirectoryEntryFree(e);
    assert_non_null(e = siteDirectoryListingRead(&site, d));
    assert_string_equal(e->name, "file2.txt"), siteDirectoryEntryFree(e);
    assert_non_null(e = siteDirectoryListingRead(&site, d));
    assert_string_equal(e->name, "file3.txt"), siteDirectoryEntryFree(e);
    assert_non_null(e = siteDirectoryListingRead(&site, d));
    assert_string_equal(e->name, "foo"), siteDirectoryEntryFree(e);
    assert_non_null(e = siteDirectoryListingRead(&site, d));
    assert_string_equal(e->name, "foo"), siteDirectoryEntryFree(e);
    siteDirectoryListingClose(&site, d), rewind(mockReceiveStream);

    /* Listing of http://127.0.0.1/ (up) */
    assert_non_null(d = siteDirectoryListingOpen(&site, ".."));
    assert_non_null(e = siteDirectoryListingRead(&site, d));
    assert_string_equal(e->name, "file1.txt"), siteDirectoryEntryFree(e);
    assert_non_null(e = siteDirectoryListingRead(&site, d));
    assert_string_equal(e->name, "file2.txt"), siteDirectoryEntryFree(e);
    assert_non_null(e = siteDirectoryListingRead(&site, d));
    assert_string_equal(e->name, "file3.txt"), siteDirectoryEntryFree(e);
    assert_non_null(e = siteDirectoryListingRead(&site, d));
    assert_string_equal(e->name, "foo"), siteDirectoryEntryFree(e);
    assert_non_null(e = siteDirectoryListingRead(&site, d));
    assert_string_equal(e->name, "foo"), siteDirectoryEntryFree(e);
    siteDirectoryListingClose(&site, d), rewind(mockReceiveStream);

    siteFree(&site);
}

/**
 * This test parses a mockup of a generic html file listing with bad links
 */
static void SiteHttpDirEntryBad(void **state) {
    const char *HttpBody = "<!DOCTYPE html>\n<html>\n<head>\n"
                           "\t<title>Directory Listing</title>\n</head>\n<body>\n"
                           "\t<h1>Directory Listing</h1>\n\t<img href=\"Logo.png\" alt=\"Logo\">\n\t<ul>\n"
                           "\t\t<li><a href=\"../file1.txt\">file1.txt</a></li>\n"
                           "\t\t<li><a href=\"http://example.com\">file3.txt</a></li>\n"
                           "\t\t<li><a href=\"http://example.com/file4.txt\">file4.txt</a></li>\n"
                           "\t</ul>\n</body>\n</html>";
    Site site;
    char length[255] = "";

    mockReset(), mockOptions = MOCK_CONNECT | MOCK_SEND | MOCK_RECEIVE, mockSendMaxBuf = mockReceiveMaxBuf = 1024;
    assert_non_null((mockReceiveStream = tmpfile()));

    /* Setup mocked response */
    sprintf(length, "content-length: %lu" HTTP_EOL HTTP_EOL, strlen(HttpBody)); /* Deliberately lowercase */
    fwrite(HttpHeaderResponseDir, 1, strlen(HttpHeaderResponseDir) - 2, mockReceiveStream);
    fwrite(length, 1, strlen(length), mockReceiveStream);
    fwrite(HttpBody, 1, strlen(HttpBody), mockReceiveStream);
    rewind(mockReceiveStream);

    assert_null(siteNew(&site, SITE_HTTP, "http://127.0.0.1/foo")), rewind(mockReceiveStream);

    assert_null(siteDirectoryListingOpen(&site, ".")), rewind(mockReceiveStream);
    assert_null(siteDirectoryListingOpen(&site, "/")), rewind(mockReceiveStream);
    assert_null(siteDirectoryListingOpen(&site, "..")), rewind(mockReceiveStream);

    siteFree(&site);
}

/**
 * This test parses a mockup of Apaches file listings
 */
static void SiteHttpDirEntryApache(void **state) {
    const char *ApacheHtml = "<!DOCTYPE HTML PUBLIC \"-//W3C//DTD HTML 3.2 Final//EN\">\n"
                             "<html>\n<body>\n<pre><img src=\"/icons/blank.gif\" alt=\"Icon \">"
                             "<a href=\"?C=N;O=D\">Name</a> "
                             "<a href=\"?C=M;O=A\">Last</a>"
                             "<a href=\"?C=S;O=A\">Size</a> "
                             "<a href=\"?C=D;O=A\">Description</a><hr>"
                             "<a href=\"cloud/\">cloud/</a> 1970-01-01 00:00 - \n"
                             "<a href=\"cloud2/\">cloud2/</a> 1970-01-01 00:00 - \n"
                             "<a href=\"foo\">foo</a> 1970-01-01 00:00 0 \n"
                             "<hr></pre>\n</body></html>";
    Site site;
    SiteFileMeta *e;
    char length[255] = "";
    void *d;

    mockReset(), mockOptions = MOCK_CONNECT | MOCK_SEND | MOCK_RECEIVE, mockSendMaxBuf = mockReceiveMaxBuf = 1024;
    assert_non_null((mockReceiveStream = tmpfile()));

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

/**
 * This test parses a mockup of Lighttpd file listings
 */
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
    SiteFileMeta *e;
    char length[255] = "";
    void *d;

    mockReset(), mockOptions = MOCK_CONNECT | MOCK_SEND | MOCK_RECEIVE, mockSendMaxBuf = mockReceiveMaxBuf = 1024;
    assert_non_null((mockReceiveStream = tmpfile()));

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

/**
 * This test parses a mockup of Newths server file listings (including chunked HTTP)
 */
static void SiteHttpDirEntryNewth(void **state) {
    const char *NewthHtml = "10a" HTTP_EOL "<!DOCTYPE HTML PUBLIC \"-//W3C//DTD HTML 4.01//EN\"\n"
                            "\t\"http://www.w3.org/TR/html4/strict.dtd\">\n<HTML>\n\t<HEAD>\n"
                            "\t\t<TITLE>/</TITLE>\n\t\t<STYLE TYPE=\"text/css\">\n\t\t*{\n"
                            "\t\t\tfont-family: monospace;\n"
                            "\t\t}\n\t\t\n\t\ta:hover,tr:hover{\n\t\t\tfont-weight: bold;\n\t\t}\n"
                            "\t\t</STYLE>\n\t</HEAD>\n\n\t<BODY>\n"
                            HTTP_EOL "34" HTTP_EOL
                            "\t\t<DIV>\n\t\t\t<A HREF=\"/\">/</A>\n\t\t</DIV>\n\t\t<HR>\n\t\t<UL>\n"
                            HTTP_EOL "29" HTTP_EOL
                            "\t\t\t<LI><A HREF=\"/cloud/\">cloud/</A></LI>\n"
                            HTTP_EOL "2b" HTTP_EOL
                            "\t\t\t<LI><A HREF=\"/cloud2/\">cloud2/</A></LI>\n"
                            HTTP_EOL "23" HTTP_EOL
                            "\t\t\t<LI><A HREF=\"/foo\">foo</A></LI>\n"
                            HTTP_EOL "19" HTTP_EOL
                            "\t\t</UL>\n\t</BODY>\n</HTML>\n"
                            HTTP_EOL "0" HTTP_EOL HTTP_EOL;
    Site site;
    SiteFileMeta *e;
    char length[255] = "";
    void *d;

    mockReset(), mockOptions = MOCK_CONNECT | MOCK_SEND | MOCK_RECEIVE, mockSendMaxBuf = mockReceiveMaxBuf = 1024;
    assert_non_null((mockReceiveStream = tmpfile()));

    /* Setup mocked response */
    sprintf(length, "transfer-encoding: chunked" HTTP_EOL HTTP_EOL); /* Deliberately lowercase */
    fwrite(HttpHeaderResponseDir, 1, strlen(HttpHeaderResponseDir) - 2, mockReceiveStream);
    fwrite(length, 1, strlen(length), mockReceiveStream);
    fwrite(NewthHtml, 1, strlen(NewthHtml), mockReceiveStream);
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

/**
 * This test parses a mockup of Nginxs server file listings
 */
static void SiteHttpDirEntryNginx(void **state) {
    const char *NginxHttp = "<html>\n<head><title>Index of /</title></head>\n<body>\n"
                            "<h1>Index of /</h1><hr><pre><a href=\"../\">../</a>\n"
                            "<a href=\"cloud/\">cloud/</a>                                             01-Jan-1970 00:00                   -\n"
                            "<a href=\"cloud2/\">cloud2/</a>                                            01-Jan-1970 00:00                   -\n"
                            "<a href=\"foo\">foo</a>                                                01-Jan-1970 00:00                   0\n"
                            "</pre><hr></body>\n</html>";
    Site site;
    SiteFileMeta *e;
    char length[255] = "";
    void *d;

    mockReset(), mockOptions = MOCK_CONNECT | MOCK_SEND | MOCK_RECEIVE, mockSendMaxBuf = mockReceiveMaxBuf = 1024;
    assert_non_null((mockReceiveStream = tmpfile()));

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

/**
 * This test parses a mockup of a HTTP file
 */
static void SiteHttpOpenFile(void **state) {
    const char *head = "HTTP/1.1 200 OK" HTTP_EOL
                       "Content-Type: text/html; charset=ISO-8859-1" HTTP_EOL
                       "Date: Thu, 1 Jan 1970 00:00:00 GMT" HTTP_EOL
                       "Length: 0" HTTP_EOL HTTP_EOL;

    const char *headFile = "HTTP/1.1 200 OK" HTTP_EOL
                           "Content-Type: text/html; charset=ISO-8859-1" HTTP_EOL
                           "Last-Modified: Tue, 20 Jun 2023 10:22:33 GMT" HTTP_EOL
                           "Date: Thu, 1 Jan 1970 00:00:00 GMT" HTTP_EOL
                           "Content-Length: 43" HTTP_EOL HTTP_EOL;
    Site site;
    SiteFileMeta *meta;

    char *p1 = platformTempFilePath("nt_f1");

    mockReset(), mockOptions = MOCK_CONNECT | MOCK_SEND | MOCK_RECEIVE, mockSendMaxBuf = mockReceiveMaxBuf = 1024;

    assert_non_null(mockReceiveStream = fopen(p1, "wb+")), free(p1);
    assert_int_equal(fwrite(head, 1, strlen(head), mockReceiveStream), strlen(head)), rewind(mockReceiveStream);

    assert_null(siteNew(&site, SITE_HTTP, "http://127.0.0.1"));
    rewind(mockReceiveStream);
    assert_int_equal(fwrite(headFile, 1, strlen(headFile), mockReceiveStream), strlen(headFile));
    rewind(mockReceiveStream);

    assert_null(siteFileOpenRead(&site, "foo", -1, -1));
    assert_non_null(meta = siteFileOpenMeta(&site));
    assert_non_null(meta->name);
    assert_string_equal(meta->name, "foo");
    assert_non_null(meta->modifiedDate);
    assert_non_null(p1 = malloc(30));
    platformTimeStructToStr(meta->modifiedDate, p1);
    assert_string_equal(p1, "Tue, 20 Jun 2023 10:22:33 GMT"), free(p1);
    assert_int_equal(meta->length, 43);
    assert_int_equal(meta->type, SITE_FILE_TYPE_DIRECTORY);
    siteFree(&site);
}

/**
 * This test parses a mockup of a HTTP file with content-disposition specified
 */
static void SiteHttpOpenFileAttachment(void **state) {
    const char *head = "HTTP/1.1 200 OK" HTTP_EOL
                       "Content-Type: text/html; charset=ISO-8859-1" HTTP_EOL
                       "Date: Thu, 1 Jan 1970 00:00:00 GMT" HTTP_EOL
                       "Length: 0" HTTP_EOL HTTP_EOL;

    const char *headFile = "HTTP/1.1 200 OK" HTTP_EOL
                           "Content-Type: text/html; charset=ISO-8859-1" HTTP_EOL
                           "Content-Disposition: attachment" HTTP_EOL
                           "Last-Modified: Tue, 20 Jun 2023 10:22:33 GMT" HTTP_EOL
                           "Date: Thu, 1 Jan 1970 00:00:00 GMT" HTTP_EOL
                           "Content-Length: 43" HTTP_EOL HTTP_EOL;
    Site site;
    SiteFileMeta *meta;

    char *p1 = platformTempFilePath("nt_f1");

    mockReset(), mockOptions = MOCK_CONNECT | MOCK_SEND | MOCK_RECEIVE, mockSendMaxBuf = mockReceiveMaxBuf = 1024;

    assert_non_null(mockReceiveStream = fopen(p1, "wb+")), free(p1);
    assert_int_equal(fwrite(head, 1, strlen(head), mockReceiveStream), strlen(head)), rewind(mockReceiveStream);

    assert_null(siteNew(&site, SITE_HTTP, "http://127.0.0.1"));
    rewind(mockReceiveStream);
    assert_int_equal(fwrite(headFile, 1, strlen(headFile), mockReceiveStream), strlen(headFile));
    rewind(mockReceiveStream);

    assert_null(siteFileOpenRead(&site, "foo", -1, -1));
    assert_non_null(meta = siteFileOpenMeta(&site));
    assert_non_null(meta->name);
    assert_string_equal(meta->name, "foo");
    assert_non_null(meta->modifiedDate);
    assert_non_null(p1 = malloc(30));
    platformTimeStructToStr(meta->modifiedDate, p1);
    assert_string_equal(p1, "Tue, 20 Jun 2023 10:22:33 GMT"), free(p1);
    assert_int_equal(meta->length, 43);
    assert_int_equal(meta->type, SITE_FILE_TYPE_FILE);
    siteFree(&site);
}

/**
 * This test parses a mockup of a HTTP file with content-disposition specifying a preferred name
 */
static void SiteHttpOpenFileFileName(void **state) {
    const char *head = "HTTP/1.1 200 OK" HTTP_EOL
                       "Content-Type: text/html; charset=ISO-8859-1" HTTP_EOL
                       "Date: Thu, 1 Jan 1970 00:00:00 GMT" HTTP_EOL
                       "Length: 0" HTTP_EOL HTTP_EOL;

    const char *headFile = "HTTP/1.1 200 OK" HTTP_EOL
                           "Content-Type: text/html; charset=ISO-8859-1" HTTP_EOL
                           "Content-Disposition: attachment; filename=\"bar\"" HTTP_EOL
                           "Last-Modified: Tue, 20 Jun 2023 10:22:33 GMT" HTTP_EOL
                           "Date: Thu, 1 Jan 1970 00:00:00 GMT" HTTP_EOL
                           "Content-Length: 43" HTTP_EOL HTTP_EOL;
    Site site;
    SiteFileMeta *meta;

    char *p1 = platformTempFilePath("nt_f1");

    mockReset(), mockOptions = MOCK_CONNECT | MOCK_SEND | MOCK_RECEIVE, mockSendMaxBuf = mockReceiveMaxBuf = 1024;

    assert_non_null(mockReceiveStream = fopen(p1, "wb+")), free(p1);
    assert_int_equal(fwrite(head, 1, strlen(head), mockReceiveStream), strlen(head)), rewind(mockReceiveStream);

    assert_null(siteNew(&site, SITE_HTTP, "http://127.0.0.1"));
    rewind(mockReceiveStream);
    assert_int_equal(fwrite(headFile, 1, strlen(headFile), mockReceiveStream), strlen(headFile));
    rewind(mockReceiveStream);

    assert_null(siteFileOpenRead(&site, "foo", -1, -1));
    assert_non_null(meta = siteFileOpenMeta(&site));
    assert_non_null(meta->name);
    assert_string_equal(meta->name, "bar");
    assert_non_null(meta->modifiedDate);
    assert_non_null(p1 = malloc(30));
    platformTimeStructToStr(meta->modifiedDate, p1);
    assert_string_equal(p1, "Tue, 20 Jun 2023 10:22:33 GMT"), free(p1);
    assert_int_equal(meta->length, 43);
    assert_int_equal(meta->type, SITE_FILE_TYPE_FILE);
    siteFree(&site);
}

/**
 * This test ensures that a http site can copy an open to read file to a file site file open to write
 */
static void SiteHttpTransferToFile(void **state) {
    const char *data = "The quick brown fox jumps over the lazy dog", buf[49] = {0};
    const char *head = "HTTP/1.1 200 OK" HTTP_EOL
                       "Content-Type: text/html; charset=ISO-8859-1" HTTP_EOL
                       "Date: Thu, 1 Jan 1970 00:00:00 GMT" HTTP_EOL
                       "Length: 0" HTTP_EOL HTTP_EOL;

    const char *headFile = "HTTP/1.1 200 OK" HTTP_EOL
                           "Content-Type: text/html; charset=ISO-8859-1" HTTP_EOL
                           "Content-Disposition: attachment" HTTP_EOL
                           "Date: Thu, 1 Jan 1970 00:00:00 GMT" HTTP_EOL
                           "Length: 43" HTTP_EOL HTTP_EOL;
    Site site1, site2;

    char *p0, *p1 = platformTempFilePath("nt_f1"), *p2 = platformTempFilePath("nt_f2");

    mockReset(), mockOptions = MOCK_CONNECT | MOCK_SEND | MOCK_RECEIVE, mockSendMaxBuf = mockReceiveMaxBuf = 1024;
    assert_non_null(mockReceiveStream = fopen(p2, "wb+")), fclose(mockReceiveStream);
    assert_non_null(mockReceiveStream = fopen(p1, "wb+")), free(p1);
    assert_int_equal(fwrite(head, 1, strlen(head), mockReceiveStream), strlen(head)), rewind(mockReceiveStream);

    assert_null(siteNew(&site1, SITE_HTTP, "http://127.0.0.1"));
    p0 = platformPathSystemToFileScheme((char *) platformTempDirectoryGet());
    assert_null(siteNew(&site2, SITE_FILE, p0)), free(p0);

    rewind(mockReceiveStream);
    assert_int_equal(fwrite(headFile, 1, strlen(headFile), mockReceiveStream), strlen(headFile));
    assert_int_equal(fwrite(data, 1, strlen(data), mockReceiveStream), strlen(data));
    rewind(mockReceiveStream);

    assert_null(siteFileOpenRead(&site1, "foo", -1, -1));
    assert_null(siteFileOpenWrite(&site2, "nt_f2"));

    assert_int_equal(siteFileRead(&site1, (char *) buf, 22), 22);
    assert_int_equal(siteFileWrite(&site2, (char *) buf, 22), 22);

    assert_int_equal(siteFileRead(&site1, (char *) buf, 22), 21);
    assert_int_equal(siteFileWrite(&site2, (char *) buf, 21), 21);

    siteFree(&site1), siteFree(&site2); /* siteFree() should close the files if required */

    fclose(mockReceiveStream);
    assert_non_null(mockReceiveStream = fopen(p2, "rb")), free(p2);
    assert_int_equal(fread((char*) buf, 1, strlen(data) + 5, mockReceiveStream), strlen(data));
    assert_string_equal(data, buf);
    fclose(mockReceiveStream), mockReceiveStream = NULL;
}

/**
 * This test ensures that opening a new file on a site will "gracefully" close an already open file data & connection
 */
static void SiteHttpTransferClobberedRequests(void **state) {
    const char *data1 = "The quick brown fox jumps over the lazy dog", *data2 = "The lazy dog jumps over the quick brown fox";
    const char *head = "HTTP/1.1 200 OK" HTTP_EOL
                       "Content-Type: text/html; charset=ISO-8859-1" HTTP_EOL
                       "Date: Thu, 1 Jan 1970 00:00:00 GMT" HTTP_EOL
                       "Length: 0" HTTP_EOL HTTP_EOL;

    const char *headFile = "HTTP/1.1 200 OK" HTTP_EOL
                           "Content-Type: text/html; charset=ISO-8859-1" HTTP_EOL
                           "Content-Disposition: attachment" HTTP_EOL
                           "Date: Thu, 1 Jan 1970 00:00:00 GMT" HTTP_EOL
                           "Length: 43" HTTP_EOL HTTP_EOL;
    Site site1, site2;
    FILE *test;

    char *p0, *p1 = platformTempFilePath("nt_f1"), *p2 = platformTempFilePath("nt_f2"), buf[49] = {0};

    mockReset(), mockOptions = MOCK_CONNECT | MOCK_SEND | MOCK_RECEIVE, mockSendMaxBuf = mockReceiveMaxBuf = 1024;
    assert_non_null(mockReceiveStream = fopen(p2, "wb+")), fclose(mockReceiveStream);
    assert_non_null(mockReceiveStream = fopen(p1, "wb+")), free(p1);
    assert_int_equal(fwrite(head, 1, strlen(head), mockReceiveStream), strlen(head)), rewind(mockReceiveStream);

    assert_null(siteNew(&site1, SITE_HTTP, "http://127.0.0.1"));
    p0 = platformPathSystemToFileScheme((char *) platformTempDirectoryGet());
    assert_null(siteNew(&site2, SITE_FILE, p0)), free(p0);

    rewind(mockReceiveStream);
    assert_int_equal(fwrite(headFile, 1, strlen(headFile), mockReceiveStream), strlen(headFile));
    assert_int_equal(fwrite(data1, 1, strlen(data1), mockReceiveStream), strlen(data1));
    rewind(mockReceiveStream);

    assert_null(siteFileOpenRead(&site1, "foo", -1, -1));
    assert_null(siteFileOpenWrite(&site2, "nt_f2"));

    assert_int_equal(siteFileRead(&site1, (char *) buf, 20), 20);
    assert_int_equal(siteFileWrite(&site2, (char *) buf, 20), 20);

    memset(buf, 0, sizeof(buf)), fflush(site2.site.file.file);
    assert_non_null(test = platformFileOpen(p2, "rb"));
    assert_int_equal(platformFileRead(buf, 1, 68, test), 20);
    assert_memory_equal(data1, buf, 20), platformFileClose(test);

    /* Requests clobbered beyond this point. TCP stream should be reset and all data wiped from recvBuffer */

    fflush(mockReceiveStream), rewind(mockReceiveStream);
    assert_int_equal(fwrite(headFile, 1, strlen(headFile), mockReceiveStream), strlen(headFile));
    assert_int_equal(fwrite(data2, 1, strlen(data2), mockReceiveStream), strlen(data2));
    rewind(mockReceiveStream);

    assert_null(siteFileOpenRead(&site1, "foo", -1, -1));
    assert_null(siteFileOpenWrite(&site2, "nt_f2"));

    assert_int_equal(siteFileRead(&site1, (char *) buf, 20), 20);
    assert_int_equal(siteFileWrite(&site2, (char *) buf, 20), 20);

    memset(buf, 0, sizeof(buf)), fflush(site2.site.file.file);
    assert_non_null(test = platformFileOpen(p2, "rb"));
    assert_int_equal(platformFileRead(buf, 1, 68, test), 20);
    assert_memory_equal(data2, buf, 20), platformFileClose(test);

    free(p2), siteFree(&site1), siteFree(&site2); /* siteFree() should close the files if required */
    fclose(mockReceiveStream), mockReceiveStream = NULL;
}

#endif /* MOCK */

#pragma endregion

#pragma clang diagnostic pop

const struct CMUnitTest siteTest[] = {
#ifdef MOCK
    cmocka_unit_test(SiteHttpNew),
    cmocka_unit_test(SiteHttpNewWithPath),
    cmocka_unit_test(SiteHttpGetDirectory),
    cmocka_unit_test(SiteHttpSetDirectory),
    cmocka_unit_test(SiteHttpSetDirectoryFailFile),
    cmocka_unit_test(SiteHttpDirEntryBad),
    cmocka_unit_test(SiteHttpDirEntryGood),
    cmocka_unit_test(SiteHttpDirEntryApache),
    cmocka_unit_test(SiteHttpDirEntryLighttpd),
    cmocka_unit_test(SiteHttpDirEntryNewth),
    cmocka_unit_test(SiteHttpDirEntryNginx),
    cmocka_unit_test(SiteHttpOpenFile),
    cmocka_unit_test(SiteHttpOpenFileAttachment),
    cmocka_unit_test(SiteHttpOpenFileFileName),
    cmocka_unit_test(SiteHttpTransferClobberedRequests),
    cmocka_unit_test(SiteHttpTransferToFile),
#endif
    cmocka_unit_test(SiteArrayFunctions),
    cmocka_unit_test(SiteFileNew),
    cmocka_unit_test(SiteFileNewWithPath),
    cmocka_unit_test(SiteFileGetDirectory),
    cmocka_unit_test(SiteFileOpenFile),
    cmocka_unit_test(SiteFileSetDirectory),
    cmocka_unit_test(SiteFileDirEntry),
    cmocka_unit_test(SiteFileTransferToFile),
    cmocka_unit_test(SiteFileTransferToFileContinue)
};

#endif /* NEW_DL_TEST_SITE_H */
