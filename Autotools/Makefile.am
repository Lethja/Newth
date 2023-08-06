bin_PROGRAMS = th dl

PLATFORM = src/platform/posix01.c src/platform/posix01.h

if WINDOWS_HOST
	PLATFORM = src/platform/mscrtdl.c \
		src/platform/winsock/wsock1.c \
		src/platform/winsock/wsock2.c src/platform/winsock/wsipv6.c
endif

dl_SOURCES = src/common/client.c \
    src/server/event.c src/server/event.h \
    src/platform/platform.c src/platform/platform.h \
    $(PLATFORM)


th_SOURCES = src/cli/th.c \
    src/common/server.h src/common/server.c \
    src/platform/platform.c src/platform/platform.h \
    src/server/event.c src/server/event.h \
    src/server/http.c src/server/http.h \
    src/server/routine.c src/server/routine.h \
    src/server/sockbufr.c src/server/sockbufr.h \
    $(PLATFORM)

if CMOCKA
    TESTS = unittest
    check_PROGRAMS = unittest

    unittest_CFLAGS = ${CMOCKA_CFLAGS} -DMOCK=1 -DSB_DATA_SIZE=128 -DEWOULDBLOCK=41
    unittest_LDFLAGS = ${CMOCKA_LIBS} \
		-Wl,--wrap=malloc,--wrap=calloc,--wrap=realloc,--wrap=free,--wrap=send,--wrap=fclose

    unittest_SOURCES = test/test.c test/mock.h test/common.h \
		test/request.h test/sockbufr.h

    unittest_SOURCES += src/platform/mockput.c src/platform/mockput.h \
		src/common/server.h src/common/server.c \
		src/platform/platform.c src/platform/platform.h \
		src/server/event.c src/server/event.h \
		src/server/http.c src/server/http.h \
		src/server/routine.c src/server/routine.h \
		src/server/sockbufr.c src/server/sockbufr.h \
		$(PLATFORM)
endif
