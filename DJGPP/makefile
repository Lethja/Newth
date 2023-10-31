CC=gcc

all:
	$(CC) -O3 -s -DWATT32=1 -I../ext/watt32s/inc \
		../src/cli/th.c \
		../src/common/server.c \
		../src/platform/doswatb.c \
		../src/platform/platform.c \
		../src/server/event.c \
		../src/server/http.c \
		../src/server/routine.c \
		../src/server/sockbufr.c \
		../ext/watt32s/lib/libwatt.a -o th32.exe

clean:
	del *.o
	del *.exe
