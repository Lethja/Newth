CC=gcc

all: dl th

debug: dldbg thdbg

dl:
	$(CC) -Wall -Wno-unknown-pragmas -O3 -s -DWATT32=1 -DNDEBUG=1 -Iwatt32s/inc \
		../src/cli/dl.c \
		../src/client/io.c \
		../src/client/queue.c \
		../src/client/site.c \
		../src/client/site/file.c \
		../src/client/site/http.c \
		../src/client/uri.c \
		../src/client/xml.c \
		../src/common/hex.c \
		../src/platform/doswatb.c \
		../src/platform/platform.c \
		watt32s/lib/libwatt.a -o dl.exe

th:
	$(CC) -Wall -Wno-unknown-pragmas -O3 -s -DWATT32=1 -DNDEBUG=1 -DPLATFORM_NET_ADAPTER=1 -DPLATFORM_NET_LISTEN=1 -Iwatt32s/inc \
		../src/cli/th.c \
		../src/common/hex.c \
		../src/platform/doswatb.c \
		../src/platform/platform.c \
		../src/server/event.c \
		../src/server/http.c \
		../src/server/routine.c \
		../src/server/server.c \
		../src/server/sendbufr.c \
		../src/server/mltiplex/select.c \
		watt32s/lib/libwatt.a -o th.exe

dldbg:
	$(CC) -Wall -Wno-unknown-pragmas -O3 -s -DWATT32=1 -Iwatt32s/inc \
		../src/cli/dl.c \
		../src/client/io.c \
		../src/client/queue.c \
		../src/client/site.c \
		../src/client/site/file.c \
		../src/client/site/http.c \
		../src/client/uri.c \
		../src/client/xml.c \
		../src/common/hex.c \
		../src/platform/doswatb.c \
		../src/platform/platform.c \
		watt32s/lib/libwatt.a -o dldbg.exe

thdbg:
	$(CC) -Wall -Wno-unknown-pragmas -O3 -DWATT32=1 -DPLATFORM_NET_ADAPTER=1 -DPLATFORM_NET_LISTEN=1 -Iwatt32s/inc \
		../src/cli/th.c \
		../src/common/hex.c \
		../src/platform/doswatb.c \
		../src/platform/platform.c \
		../src/server/event.c \
		../src/server/http.c \
		../src/server/routine.c \
		../src/server/server.c \
		../src/server/sendbufr.c \
		../src/server/mltiplex/select.c \
		watt32s/lib/libwatt.a -o thdbg.exe
