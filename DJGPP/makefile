CC=gcc

all:
	$(CC) -O3 -std=c89 -DWATT32=1 -I../ext/watt32s/inc ../src/cli/th.c -o th.o
	$(CC) -O3 -std=c89 -DWATT32=1 -I../ext/watt32s/inc ../src/common/server.c -o server.o
	$(CC) -O3 -std=c89 -DWATT32=1 -I../ext/watt32s/inc ../src/platform/doswatb.c -o doswatb.o
	$(CC) -O3 -std=c89 -DWATT32=1 -I../ext/watt32s/inc ../src/platform/platform.c -o platform.o
	$(CC) -O3 -std=c89 -DWATT32=1 -I../ext/watt32s/inc ../src/server/event.c -o event.o
	$(CC) -O3 -std=c89 -DWATT32=1 -I../ext/watt32s/inc ../src/server/http.c -o http.o
	$(CC) -O3 -std=c89 -DWATT32=1 -I../ext/watt32s/inc ../src/server/routine.c -o routine.o
	$(CC) -O3 -std=c89 -DWATT32=1 -I../ext/watt32s/inc ../src/server/sockbufr.c -o sockbufr.o
	$(CC) -O3 -std=c89 -s *.o ../ext/watt32s/lib/libwatt.a -o th32.exe

clean:
	del *.o
	del *.exe