bin_PROGRAMS = th dl

dl_SOURCES = src/common/client.c \
    src/server/event.c src/server/event.h \
    src/platform/platform.c src/platform/platform.h \
    src/platform/posix01.c src/platform/posix01.h

th_SOURCES = src/cli/th.c \
    src/common/server.h src/common/server.c \
    src/platform/platform.c src/platform/platform.h \
    src/platform/posix01.c src/platform/posix01.h \
    src/server/event.c src/server/event.h \
    src/server/http.c src/server/http.h \
    src/server/routine.c src/server/routine.h \
    src/server/sockbufr.c src/server/sockbufr.h

if CMOCKA
    TESTS = unittest
    check_PROGRAMS = unittest
    unittest_SOURCES = test/test.c test/mock.h test/common.h test/sockbufr.h \
        src/server/sockbufr.c src/platform/mockput.c
    unittest_CFLAGS = ${CMOCKA_CFLAGS} -DMOCK=1
    unittest_LDFLAGS = ${CMOCKA_LIBS}
endif