CC=gcc

all: dl th

dl:
	$(CC) -Wall -Wno-unknown-pragmas -O3 -s -DWATT32=1 -DNDEBUG=1 -Iwatt32s/inc \
		../src/cli/dl.c \
		../src/client/io.c ../src/client/io.h \
		../src/client/queue.c ../src/client/queue.h \
		../src/client/uri.c ../src/client/uri.h \
		../src/platform/doswatb.c \
		../src/platform/platform.c \
		../src/server/event.c \
		watt32s/lib/libwatt.a -o dl.exe

th:
	$(CC) -Wall -Wno-unknown-pragmas -O3 -s -DWATT32=1 -DNDEBUG=1 -Iwatt32s/inc \
		../src/cli/th.c \
		../src/platform/doswatb.c \
		../src/platform/platform.c \
		../src/server/event.c \
		../src/server/http.c \
		../src/server/routine.c \
		../src/server/server.c \
		../src/server/sockbufr.c \
		../src/server/mltiplex/select.c \
		watt32s/lib/libwatt.a -o th.exe

debug:
	$(CC) -Wall -Wno-unknown-pragmas -O3 -DWATT32=1 -Iwatt32s/inc \
		../src/cli/th.c \
		../src/platform/doswatb.c \
		../src/platform/platform.c \
		../src/server/event.c \
		../src/server/http.c \
		../src/server/routine.c \
		../src/server/server.c \
		../src/server/sockbufr.c \
		../src/server/mltiplex/select.c \
		watt32s/lib/libwatt.a -o thdbg.exe
