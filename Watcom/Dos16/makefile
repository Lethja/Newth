CC=wcc

# Default building process
all: .SYMBOLIC dl.exe th.exe

dl.exe: release doswatb.o event.o hex.o io.o platform.o queue.o site.o s_file.o s_http.o uri.o xml.o dl.o
    %write link.lnk NAME $@
    %write link.lnk SYS  dos
    %write link.lnk OPT  st=4608
    %write link.lnk FILE doswatb.o,event.o,hex.o,io.o,platform.o,queue.o,site.o,s_file.o,s_http.o,uri.o,xml.o,dl.o
    %write link.lnk LIBP watt32s/lib/
    %write link.lnk LIBF wattcpwl.lib
    wlink @link.lnk

dldbg.exe: debug doswatb.o event.o hex.o io.o platform.o queue.o site.o s_file.o s_http.o uri.o xml.o dl.o
    %write link.lnk NAME $@
    %write link.lnk SYS  dos
    %write link.lnk OPT  st=4608
    %write link.lnk FILE doswatb.o,event.o,hex.o,io.o,platform.o,queue.o,site.o,s_file.o,s_http.o,uri.o,xml.o,dl.o
    %write link.lnk LIBP watt32s/lib/
    %write link.lnk LIBF wattcpwl.lib
    wlink @link.lnk

dlwlk.exe: walk doswatb.o event.o hex.o io.o platform.o queue.o site.o s_file.o s_http.o uri.o xml.o dl.o
    %write link.lnk NAME $@
    %write link.lnk SYS  dos
    %write link.lnk OPT  st=4608
    %write link.lnk FILE doswatb.o,event.o,hex.o,io.o,platform.o,queue.o,site.o,s_file.o,s_http.o,uri.o,xml.o,dl.o
    %write link.lnk LIBP watt32s/lib/
    %write link.lnk LIBF wattcpwl.lib
    wlink @link.lnk

th.exe: release doswatb.o event.o hex.o http.o platform.o poll.o routine.o server.o sockbufr.o th.o
    %write link.lnk NAME $@
    %write link.lnk SYS  dos
    %write link.lnk OPT  st=4608
    %write link.lnk FILE doswatb.o,event.o,hex.o,http.o,platform.o,poll.o,routine.o,server.o,sockbufr.o,th.o
    %write link.lnk LIBP watt32s/lib/
    %write link.lnk LIBF wattcpwl.lib
    wlink @link.lnk

thdbg.exe: debug doswatb.o event.o hex.o http.o platform.o poll.o routine.o server.o sockbufr.o th.o
    %write link.lnk NAME $@
    %write link.lnk SYS  dos
    %write link.lnk OPT  st=4608
    %write link.lnk FILE doswatb.o,event.o,hex.o,http.o,platform.o,poll.o,routine.o,server.o,sockbufr.o,th.o
    %write link.lnk LIBP watt32s/lib/
    %write link.lnk LIBF wattcpwl.lib
    wlink @link.lnk

thwlk.exe: walk doswatb.o event.o hex.o http.o platform.o poll.o routine.o server.o sockbufr.o th.o
    %write link.lnk NAME $@
    %write link.lnk SYS  dos
    %write link.lnk OPT  st=4608
    %write link.lnk FILE doswatb.o,event.o,hex.o,http.o,platform.o,poll.o,routine.o,server.o,sockbufr.o,th.o
    %write link.lnk LIBP watt32s/lib/
    %write link.lnk LIBF wattcpwl.lib
    wlink @link.lnk

dbg: .SYMBOLIC dldbg.exe thdbg.exe

wlk: .SYMBOLIC dlwlk.exe thwlk.exe

dl.o:
	$(CC) @flag.lnk -fo="$@" "../../src/cli/dl.c"
doswatb.o: ../../src/platform/doswatb.c
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
sockbufr.o: ../../src/server/sockbufr.c
    $(CC) @flag.lnk -fo="$@" "../../src/server/sockbufr.c"
platform.o: ../../src/platform/platform.c
    $(CC) @flag.lnk -fo="$@" "../../src/platform/platform.c"
poll.o: ../../src/server/mltiplex/poll.c
    $(CC) @flag.lnk -fo="$@" "../../src/server/mltiplex/poll.c"
th.o: ../../src/cli/th.c
    $(CC) @flag.lnk -fo="$@" "../../src/cli/th.c"
uri.o:
	$(CC) @flag.lnk -fo="$@" "../../src/client/uri.c"
xml.o:
	$(CC) @flag.lnk -fo="$@" "../../src/client/xml.c"

#TODO: Omit PLATFORM_NET from DL build to save disk space
walk: .SYMBOLIC
    %write flag.lnk -q -dWATT32=1 -bt=dos -ml -0 -i="watt32s/inc/" -d3 -db -dDBGHALT=1 -dPLATFORM_NET_ADAPTER=1 -dPLATFORM_NET_LISTEN=1

debug: .SYMBOLIC
    %write flag.lnk -q -dWATT32=1 -bt=dos -ml -0 -i="watt32s/inc/" -d3 -db -dPLATFORM_NET_ADAPTER=1 -dPLATFORM_NET_LISTEN=1

release: .SYMBOLIC
    %write flag.lnk -q -dWATT32=1 -bt=dos -ml -0 -i="watt32s/inc/" -d0 -osr -zc -dNDEBUG=1 -dPLATFORM_NET_ADAPTER=1 -dPLATFORM_NET_LISTEN=1
