bin_PROGRAMS = th dl

if WINDOWS_HOST

PLATFORM_SRC = src/platform/mscrtdl.c src/platform/winsock/wsock1.c src/platform/winsock/wsock2.c src/platform/winsock/wsipv6.c
dl_LDADD = -lwsock32
th_LDADD = -lwsock32
unittest_LDADD = -lwsock32 ${cmocka_LIBS}

else

PLATFORM_SRC = src/platform/posix01.c src/platform/posix01.h

endif

if MULTIPLEX_POLL

MULTIPLEX_SRC = src/server/mltiplex/poll.c

else

MULTIPLEX_SRC = src/server/mltiplex/select.c

endif

dl_SOURCES = src/cli/dl.c src/common/defines.h \
    src/client/io.c src/client/io.h \
    src/client/queue.c src/client/queue.h \
    src/client/recvbufr.c src/client/recvbufr.h \
    src/client/site.c src/client/site.h \
    src/client/site/file.c src/client/site/file.h \
    src/client/site/http.c src/client/site/http.h \
    src/client/uri.c src/client/uri.h \
    src/client/xml.c src/client/xml.h \
    src/common/hex.c src/common/hex.h \
    src/platform/platform.c src/platform/platform.h \
    $(PLATFORM_SRC)


th_SOURCES = src/cli/th.c src/common/defines.h \
    src/common/hex.c src/common/hex.h \
    src/platform/platform.c src/platform/platform.h \
    src/server/event.c src/server/event.h \
    src/server/http.c src/server/http.h \
    src/server/routine.c src/server/routine.h \
    src/server/server.h src/server/server.c \
    src/server/sendbufr.c src/server/sendbufr.h \
    $(MULTIPLEX_SRC) $(PLATFORM_SRC)

th_CFLAGS = -DPLATFORM_NET_ADAPTER=1 -DPLATFORM_NET_LISTEN=1

if CMOCKA
    TESTS = unittest
    check_PROGRAMS = unittest

    unittest_CFLAGS = ${cmocka_CFLAGS} -DMOCK=1 -DSB_DATA_SIZE=128 -DEWOULDBLOCK=41 -DPLATFORM_NET_LISTEN=1
    unittest_LDFLAGS = ${cmocka_LIBS} \
		-Wl,--wrap=malloc,--wrap=calloc,--wrap=realloc,--wrap=free,--wrap=connect,--wrap=recv,--wrap=send,--wrap=fclose

    unittest_SOURCES = test/test.c test/common.h test/fetch.h test/mock.h test/platform.h \
		test/queue.h test/recvbufr.h test/request.h test/site.h test/sendbufr.h test/xml.h

    unittest_SOURCES += src/platform/mockput.c src/platform/mockput.h \
        src/client/io.c src/client/io.h \
        src/client/queue.c src/client/queue.h \
        src/client/recvbufr.c src/client/recvbufr.h \
        src/client/site.c src/client/site.h \
        src/client/uri.c src/client/uri.h \
        src/client/xml.c src/client/xml.h \
        src/client/site/file.c src/client/site/file.h \
        src/client/site/http.c src/client/site/http.h \
        src/common/hex.c src/common/hex.h \
		src/platform/platform.c src/platform/platform.h \
		src/server/event.c src/server/event.h \
		src/server/http.c src/server/http.h \
		src/server/routine.c src/server/routine.h \
        src/server/server.h src/server/server.c \
		src/server/sendbufr.c src/server/sendbufr.h \
		$(MULTIPLEX_SRC) $(PLATFORM_SRC)
endif
