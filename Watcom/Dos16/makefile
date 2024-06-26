CC=wcc

# Default building process
all: .SYMBOLIC dl.exe th.exe

dl.exe: release_dl err.o doswatbd.o hex.o io.o plat_dl.o queue.o recvbufr.o signal.o site.o s_file.o s_http.o uri.o xml.o dl.o
    %write link.lnk NAME $@
    %write link.lnk SYS  dos
    %write link.lnk OPT  st=4608
    %write link.lnk FILE err.o,doswatbd.o,hex.o,io.o,plat_dl.o,queue.o,recvbufr.o,signal.o,site.o,s_file.o,s_http.o,uri.o,xml.o,dl.o
    %write link.lnk LIBP watt32s/lib/
    %write link.lnk LIBF wattcpwl.lib
    wlink @link.lnk

dldbg.exe: debug_dl err.o doswatbd.o hex.o io.o plat_dl.o queue.o recvbufr.o signal.o site.o s_file.o s_http.o uri.o xml.o dl.o
    %write link.lnk NAME $@
    %write link.lnk SYS  dos
    %write link.lnk OPT  st=4608
    %write link.lnk FILE err.o,doswatbd.o,hex.o,io.o,plat_dl.o,queue.o,recvbufr.o,signal.o,site.o,s_file.o,s_http.o,uri.o,xml.o,dl.o
    %write link.lnk LIBP watt32s/lib/
    %write link.lnk LIBF wattcpwl.lib
    wlink @link.lnk

dlwlk.exe: walk_dl err.o doswatbd.o hex.o io.o plat_dl.o queue.o recvbufr.o signal.o site.o s_file.o s_http.o uri.o xml.o dl.o
    %write link.lnk NAME $@
    %write link.lnk SYS  dos
    %write link.lnk OPT  st=4608
    %write link.lnk FILE err.o,doswatbd.o,hex.o,io.o,plat_dl.o,queue.o,recvbufr.o,site.o,s_file.o,s_http.o,uri.o,xml.o,dl.o
    %write link.lnk LIBP watt32s/lib/
    %write link.lnk LIBF wattcpwl.lib
    wlink @link.lnk

th.exe: release_th doswatbt.o event.o hex.o http.o plat_th.o poll.o routine.o signal.o server.o sendbufr.o th.o
    %write link.lnk NAME $@
    %write link.lnk SYS  dos
    %write link.lnk OPT  st=4608
    %write link.lnk FILE doswatbt.o,event.o,hex.o,http.o,plat_th.o,poll.o,routine.o,signal.o,server.o,sendbufr.o,th.o
    %write link.lnk LIBP watt32s/lib/
    %write link.lnk LIBF wattcpwl.lib
    wlink @link.lnk

thdbg.exe: debug_th doswatbt.o event.o hex.o http.o plat_th.o poll.o routine.o signal.o server.o sendbufr.o th.o
    %write link.lnk NAME $@
    %write link.lnk SYS  dos
    %write link.lnk OPT  st=4608
    %write link.lnk FILE doswatbt.o,event.o,hex.o,http.o,plat_th.o,poll.o,routine.o,signal.o,server.o,sendbufr.o,th.o
    %write link.lnk LIBP watt32s/lib/
    %write link.lnk LIBF wattcpwl.lib
    wlink @link.lnk

thwlk.exe: walk_th doswatbt.o event.o hex.o http.o plat_th.o poll.o routine.o signal.o server.o sendbufr.o th.o
    %write link.lnk NAME $@
    %write link.lnk SYS  dos
    %write link.lnk OPT  st=4608
    %write link.lnk FILE doswatbt.o,event.o,hex.o,http.o,plat_th.o,poll.o,routine.o,signal.o,server.o,sendbufr.o,th.o
    %write link.lnk LIBP watt32s/lib/
    %write link.lnk LIBF wattcpwl.lib
    wlink @link.lnk

dbg: .SYMBOLIC dldbg.exe thdbg.exe

wlk: .SYMBOLIC dlwlk.exe thwlk.exe

err.o:
	$(CC) @flag.lnk -fo="$@" "../../src/client/err.c"
dl.o:
	$(CC) @flag.lnk -fo="$@" "../../src/cli/dl.c"
doswatbd.o: ../../src/platform/doswatb.c
    $(CC) @flag.lnk -fo="$@" "../../src/platform/doswatb.c"
doswatbt.o: ../../src/platform/doswatb.c
    $(CC) @flag.lnk -fo="$@" "../../src/platform/doswatb.c"
event.o: ../../src/server/event.c
    $(CC) @flag.lnk -fo="$@" "../../src/server/event.c"
hex.o: ../../src/common/hex.c
    $(CC) @flag.lnk -fo="$@" "../../src/common/hex.c"
http.o: ../../src/server/http.c
    $(CC) @flag.lnk -fo="$@" "../../src/server/http.c"
io.o: ../../src/client/io.c
    $(CC) @flag.lnk -fo="$@" "../../src/client/io.c"
queue.o: ../../src/client/queue.c
    $(CC) @flag.lnk -fo="$@" "../../src/client/queue.c"
recvbufr.o: ../../src/client/recvbufr.c
    $(CC) @flag.lnk -fo="$@" "../../src/client/recvbufr.c"
site.o: ../../src/client/site.c
    $(CC) @flag.lnk -fo="$@" "../../src/client/site.c"
s_file.o: ../../src/client/site/file.c
    $(CC) @flag.lnk -fo="$@" "../../src/client/site/file.c"
s_http.o: ../../src/client/site/http.c
    $(CC) @flag.lnk -fo="$@" "../../src/client/site/http.c"
routine.o: ../../src/server/routine.c
    $(CC) @flag.lnk -fo="$@" "../../src/server/routine.c"
server.o: ../../src/server/server.c
    $(CC) @flag.lnk -fo="$@" "../../src/server/server.c"
sendbufr.o: ../../src/server/sendbufr.c
    $(CC) @flag.lnk -fo="$@" "../../src/server/sendbufr.c"
signal.o: ../../src/common/signal.c
    $(CC) @flag.lnk -fo="$@" "../../src/common/signal.c"
plat_dl.o: ../../src/platform/platform.c
    $(CC) @flag.lnk -fo="$@" "../../src/platform/platform.c"
plat_th.o: ../../src/platform/platform.c
    $(CC) @flag.lnk -fo="$@" "../../src/platform/platform.c"
poll.o: ../../src/server/mltiplex/poll.c
    $(CC) @flag.lnk -fo="$@" "../../src/server/mltiplex/poll.c"
th.o: ../../src/cli/th.c
    $(CC) @flag.lnk -fo="$@" "../../src/cli/th.c"
uri.o:
	$(CC) @flag.lnk -fo="$@" "../../src/client/uri.c"
xml.o:
	$(CC) @flag.lnk -fo="$@" "../../src/client/xml.c"

walk_dl: .SYMBOLIC
    %write flag.lnk -q -dWATT32=1 -bt=dos -ml -0 -i="watt32s/inc/" -d3 -db -dDBGHALT=1 -dPLATFORM_SYS_ARGV=1 -dPLATFORM_SYS_EXEC=1 -dPLATFORM_SYS_WRITE=1

debug_dl: .SYMBOLIC
    %write flag.lnk -q -dWATT32=1 -bt=dos -ml -0 -i="watt32s/inc/" -d3 -db -dPLATFORM_SYS_ARGV=1 -dPLATFORM_SYS_EXEC=1 -dPLATFORM_SYS_WRITE=1

release_dl: .SYMBOLIC
    %write flag.lnk -q -dWATT32=1 -bt=dos -ml -0 -i="watt32s/inc/" -d0 -osr -zc -dNDEBUG=1 -dPLATFORM_SYS_ARGV=1 -dPLATFORM_SYS_EXEC=1 -dPLATFORM_SYS_WRITE=1

walk_th: .SYMBOLIC
    %write flag.lnk -q -dWATT32=1 -bt=dos -ml -0 -i="watt32s/inc/" -d3 -db -dDBGHALT=1 -dPLATFORM_NET_ADAPTER=1 -dPLATFORM_NET_LISTEN=1 -dPLATFORM_SYS_ARGV=1

debug_th: .SYMBOLIC
    %write flag.lnk -q -dWATT32=1 -bt=dos -ml -0 -i="watt32s/inc/" -d3 -db -dPLATFORM_NET_ADAPTER=1 -dPLATFORM_NET_LISTEN=1 -dPLATFORM_SYS_ARGV=1

release_th: .SYMBOLIC
    %write flag.lnk -q -dWATT32=1 -bt=dos -ml -0 -i="watt32s/inc/" -d0 -osr -zc -dNDEBUG=1 -dPLATFORM_NET_ADAPTER=1 -dPLATFORM_NET_LISTEN=1 -dPLATFORM_SYS_ARGV=1
