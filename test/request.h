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

static FILE *buildSampleFile(size_t length, char *path, int *descriptor) {
    const char *content = "123456789\n";
    size_t i, max = strlen(content);
    int fd;

    fd = mkstemp(path);
    for (i = 0; i < length; ++i)
        write(fd, content, (length - i) < max ? length - i : max);

    if (*descriptor) *descriptor = fd;

    return fdopen(fd, "r+b");
}

static long findHttpBodyStart(FILE *stream) {
    long r = 0, match = 0;

    rewind(stream);
    do {
        int ch = fgetc(stream);
        switch (ch) {
            case '\n':
                if (match) {
                    case '\r':
                        ++match;
                    break;
                } else
                    default:
                        match = 0;
                break;
            case EOF:
                return -1;
        }
        ++r;
    } while (match < 4);

    return r;
}

static void CompareStreams(void **state, FILE *stream1, FILE *stream2) {
    int a, b;
    unsigned long pos = 0;

    do {
        a = fgetc(stream1), b = fgetc(stream2);
        if (a != b) {
            char buffer1[BUFSIZ] = "", buffer2[BUFSIZ] = "";
            fseek(stream1, -1, SEEK_CUR), fseek(stream2, -1, SEEK_CUR);
            fgets(buffer1, BUFSIZ, stream1), fgets(buffer2, BUFSIZ, stream2);

            print_error("Streams differ after %lu iterations: a[%ld] = %d b[%ld] = %d\n", pos, ftell(stream1), a,
                        ftell(stream2), b);
            assert_string_equal(buffer1, buffer2);
        }
        ++pos;
    } while (a != EOF || b != EOF);
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

    FILE *tmpFile1 = tmpfile(), *tmpFile2;
    long correctPosition, testPosition;

    fwrite(http, strlen(http), 1, tmpFile1), correctPosition = ftell(tmpFile1);

    fwrite(body, strlen(body), 1, tmpFile1);
    testPosition = findHttpBodyStart(tmpFile1);
    assert_int_equal(correctPosition, testPosition);

    tmpFile2 = tmpfile(), fwrite(body, strlen(body), 1, tmpFile2), rewind(tmpFile2);
    CompareStreams(state, tmpFile1, tmpFile2);

    fclose(tmpFile1), fclose(tmpFile2);
}

static void RequestSmallFile(void **state) {
    char path[FILENAME_MAX] = "/tmp/XXXXXX", *fdStr, *header = "GET / HTTP/1.1" HTTP_EOL HTTP_EOL, buffer[BUFSIZ];
    int fd;
    FILE *read = buildSampleFile(SB_DATA_SIZE, path, &fd);

    mockOptions = MOCK_SEND, mockSendStream = tmpfile(), mockSendMaxBuf = BUFSIZ;

    globalRootPath = malloc(FILENAME_MAX);
    fdStr = strrchr(path, '/'), strncpy(globalRootPath, path, FILENAME_MAX), globalRootPath[fdStr - path] = '\0';
    ++fdStr;

    handlePath(0, header, fdStr);

#ifndef NDEBUG
    mockDumpFile(mockSendStream);
#endif

    rewind(read), findHttpBodyStart(mockSendStream);

    CompareStreams(state, read, mockSendStream);

    fclose(read), free(globalRootPath), globalRootPath = NULL, mockReset();
}

const struct CMUnitTest requestTest[] = {cmocka_unit_test(SelfTestHttpBodyStart), cmocka_unit_test(RequestSmallFile)};

#endif /* NEW_TH_TEST_REQUEST_H */
