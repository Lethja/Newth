CC=gcc

release:
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
