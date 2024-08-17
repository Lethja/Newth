CC=wcc
WATT_32=watt32s

all: .SYMBOLIC dl.exe th.exe

dl.exe: flag.lnk dldef.lnk err.o doswatbd.o hex.o io.o plat_dl.o queue.o recvbufr.o signal.o site.o s_file.o s_http.o uri.o xml.o dl.o dllnk.lnk
	wlink NAME $@ @dllnk.lnk

th.exe: flag.lnk thdef.lnk doswatbt.o event.o hex.o http.o plat_th.o poll.o routine.o signal.o server.o sendbufr.o th.o thlnk.lnk
	wlink NAME $@ @thlnk.lnk

flag.lnk:
	%write flag.lnk -q -dWATT32=1 -bt=dos -ml -0 -i="$(WATT_32)/inc" -d0 -osr -zc -dNDEBUG=1

dldef.lnk:
	%write dldef.lnk -dPLATFORM_SYS_ARGV=1 -dPLATFORM_SYS_EXEC=1 -dPLATFORM_SYS_WRITE=1

thdef.lnk:
	%write thdef.lnk -dPLATFORM_NET_ADAPTER=1 -dPLATFORM_NET_LISTEN=1 -dPLATFORM_SYS_ARGV=1

dllnk.lnk:
	%write dllnk.lnk SYS  dos
	%write dllnk.lnk OPT  st=9216
	%write dllnk.lnk FILE err.o,doswatbd.o,hex.o,io.o,plat_dl.o,queue.o,recvbufr.o,signal.o,site.o,s_file.o,s_http.o,uri.o,xml.o,dl.o
	%write dllnk.lnk LIBP $(WATT_32)/lib/
	%write dllnk.lnk LIBF wattcpwl.lib

thlnk.lnk:
	%write thlnk.lnk SYS  dos
	%write thlnk.lnk OPT  st=4608
	%write thlnk.lnk FILE doswatbt.o,event.o,hex.o,http.o,plat_th.o,poll.o,routine.o,signal.o,server.o,sendbufr.o,th.o
	%write thlnk.lnk LIBP $(WATT_32)/lib/
	%write thlnk.lnk LIBF wattcpwl.lib

err.o: ../../src/client/err.c
	$(CC) @flag.lnk -fo="$@" $<
dl.o: ../../src/cli/dl.c
	$(CC) @flag.lnk @dldef.lnk -fo="$@" $<
doswatbd.o: ../../src/platform/doswatb.c
	$(CC) @flag.lnk @dldef.lnk -fo="$@" $<
doswatbt.o: ../../src/platform/doswatb.c
	$(CC) @flag.lnk @thdef.lnk -fo="$@" $<
event.o: ../../src/server/event.c
	$(CC) @flag.lnk -fo="$@" $<
hex.o: ../../src/common/hex.c
	$(CC) @flag.lnk -fo="$@" $<
http.o: ../../src/server/http.c
	$(CC) @flag.lnk -fo="$@" $<
io.o: ../../src/client/io.c
	$(CC) @flag.lnk -fo="$@" $<
queue.o: ../../src/client/queue.c
	$(CC) @flag.lnk -fo="$@" $<
recvbufr.o: ../../src/client/recvbufr.c
	$(CC) @flag.lnk -fo="$@" $<
site.o: ../../src/client/site.c
	$(CC) @flag.lnk -fo="$@" $<
s_file.o: ../../src/client/site/file.c
	$(CC) @flag.lnk @dldef.lnk -fo="$@" $<
s_http.o: ../../src/client/site/http.c
	$(CC) @flag.lnk @dldef.lnk -fo="$@" $<
routine.o: ../../src/server/routine.c
	$(CC) @flag.lnk @thdef.lnk -fo="$@" $<
server.o: ../../src/server/server.c
	$(CC) @flag.lnk -fo="$@" $<
sendbufr.o: ../../src/server/sendbufr.c
	$(CC) @flag.lnk -fo="$@" $<
signal.o: ../../src/common/signal.c
	$(CC) @flag.lnk -fo="$@" $<
plat_dl.o: ../../src/platform/platform.c
	$(CC) @flag.lnk @dldef.lnk -fo="$@" $<
plat_th.o: ../../src/platform/platform.c
	$(CC) @flag.lnk @thdef.lnk -fo="$@" $<
poll.o: ../../src/server/mltiplex/poll.c
	$(CC) @flag.lnk @thdef.lnk -fo="$@" $<
th.o: ../../src/cli/th.c
	$(CC) @flag.lnk @thdef.lnk -fo="$@" $<
uri.o: ../../src/client/uri.c
	$(CC) @flag.lnk -fo="$@" $<
xml.o: ../../src/client/xml.c
	$(CC) @flag.lnk -fo="$@" $<