#ifndef NEW_TH_TEST_REQUEST_H
#define NEW_TH_TEST_REQUEST_H

#include "../src/common/server.h"
#include "../src/platform/platform.h"
#include <string.h>

#pragma region CMocka headers

#include <stdarg.h>
#include <stddef.h>
#include <stdint.h>
#include <setjmp.h>

#include <cmocka.h>
#include <unistd.h>

#pragma endregion

static char *BuildFdPath(int fd) {
    char *path = malloc(FILENAME_MAX);
    if (!path)
        return path;

    snprintf(path, FILENAME_MAX, "/proc/self/fd/%d", fd);
    return path;
}

static FILE *buildSampleFile(size_t length, char **path) {
    const char *content = "123456789\n", *tmpDir = "/tmp/testXXXXXX";
    size_t i, max = strlen(content);
    int fd;

    fd = mkstemp((char *) tmpDir);
    for (i = 0; i < length; ++i)
        write(fd, content, (length - i) < max ? length - i : max);

    if (*path)
        *path = BuildFdPath(fd);

    return fdopen(fd, "r+b");
}

static void FindHttpBodyStart(FILE *stream) {
    size_t match = 0;

    rewind(stream);
    do {
        int ch = fgetc(stream);
        switch (ch) {
            case '\n':
                if (match)
                    case '\r':
                        ++match;
                break;
            case EOF:
                return;
            default:
                match = 0;
                break;
        }
    } while (match < 4);
}

static void SelfTestHttpBodyStart(void **state) {
    const char *http = "HTTP/1.1 200 OK\n"
                       "Date: Thu, 27 Jul 2023 05:57:33 GMT" HTTP_EOL
                       "Content-Type: text/html; charset=utf-8" HTTP_EOL
                       "Content-Length: 6434" HTTP_EOL HTTP_EOL, *body = "<!DOCTYPE HTML>\n"
                                                                         "<html>\n"
                                                                         "<head>\n"
                                                                         "<title>There are no easter eggs here</title>\n"
                                                                         "</head>\n"
                                                                         "</html>";

    char buffer1[BUFSIZ] = "", buffer2[BUFSIZ] = "";
    FILE *tmpFile1 = tmpfile(), *tmpFile2;
    long correctPosition, testPosition;

    fwrite(http, strlen(http), 1, tmpFile1), correctPosition = ftell(tmpFile1);

    fwrite(body, strlen(body), 1, tmpFile1);
    FindHttpBodyStart(tmpFile1), testPosition = ftell(tmpFile1);

    assert_int_equal(correctPosition, testPosition);

    tmpFile2 = tmpfile(), fwrite(body, strlen(body), 1, tmpFile2), rewind(tmpFile2);

    while (fgets(buffer1, BUFSIZ, tmpFile1)) {
        fgets(buffer2, BUFSIZ, tmpFile2);
        assert_string_equal(buffer1, buffer2);
    }
}

const struct CMUnitTest requestTest[] = {cmocka_unit_test(SelfTestHttpBodyStart)};

#endif /* NEW_TH_TEST_REQUEST_H */
