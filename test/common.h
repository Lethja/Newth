#ifndef NEW_TH_TEST_COMMON_H
#define NEW_TH_TEST_COMMON_H

#include "../src/server/server.h"
#include "../src/platform/platform.h"
#include <string.h>

#pragma clang diagnostic push
#pragma ide diagnostic ignored "UnusedParameter"

#pragma region CMocka headers

#include <stdarg.h>
#include <stddef.h>
#include <stdint.h>
#include <setjmp.h>

#include <cmocka.h>
#include <unistd.h>

#pragma endregion

static char *sampleText = "Glum-Schwartzkopf,vex'd_by NJ&IQ"; /* 32 characters, each unique */

static void WriteSampleFile(FILE *file, size_t length) {
    const char *content = "123456789\n";
    size_t i, max = strlen(content);

    if (file) {
        for (i = 0; i < length; ++i)
            fwrite(content, 1, (length - i) < max ? length - i : max, file);

        fflush(file);
    }
}

static long FindHttpBodyStart(FILE *stream) {
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

static PlatformFileOffset GetStreamRemaining(FILE *file) {
    PlatformFileOffset pos = platformFileTell(file), total;
    platformFileSeek(file, 0, SEEK_END);
    total = platformFileTell(file);
    platformFileSeek(file, pos, SEEK_SET);
    return total - pos;
}

static void CompareStreams(void **state, FILE *stream1, FILE *stream2) {
    int a, b;
    unsigned long pos = 0;
    PlatformFileOffset lenA = GetStreamRemaining(stream1), lenB = GetStreamRemaining(stream2);
    assert_int_equal(lenA, lenB);

    do {
        a = fgetc(stream1), b = fgetc(stream2);
        if (a != b) {
            char buffer1[BUFSIZ] = "", buffer2[BUFSIZ] = "";
            fseek(stream1, -1, SEEK_CUR), fseek(stream2, -1, SEEK_CUR);
#pragma GCC diagnostic ignored "-Wunused-result"
            fgets(buffer1, BUFSIZ, stream1), fgets(buffer2, BUFSIZ, stream2);

            print_error("Streams differ after %lu iterations: a[%ld] = %d b[%ld] = %d\n", pos, ftell(stream1), a,
                        ftell(stream2), b);
            assert_string_equal(buffer1, buffer2);
        }
        ++pos;
    } while (a != EOF || b != EOF);
}

#pragma clang diagnostic pop

#endif /* NEW_TH_TEST_COMMON_H */
