CC=wcc386

# Default building process
all: .SYMBOLIC dl.exe th.exe

dl.exe: release_dl err.o mscrtdbd.o hex.o io.o plat_dl.o queue.o recvbufr.o signal.o site.o s_file.o s_http.o uri.o xml.o dl.o
    %write link.lnk NAME $@
    %write link.lnk SYS  nt
    %write link.lnk OPT  st=9216
    %write link.lnk FILE err.o,mscrtdbd.o,hex.o,io.o,plat_dl.o,queue.o,recvbufr.o,signal.o,site.o,s_file.o,s_http.o,uri.o,xml.o,dl.o
    %write link.lnk LIBF wsock32.lib
    wlink @link.lnk

dldbg.exe: debug_dl err.o mscrtdbd.o hex.o io.o plat_dl.o queue.o recvbufr.o signal.o site.o s_file.o s_http.o uri.o xml.o dl.o
    %write link.lnk NAME $@
    %write link.lnk SYS  nt
    %write link.lnk OPT  st=9216
    %write link.lnk FILE err.o,mscrtdbd.o,hex.o,io.o,plat_dl.o,queue.o,recvbufr.o,signal.o,site.o,s_file.o,s_http.o,uri.o,xml.o,dl.o
    %write link.lnk LIBF wsock32.lib
    wlink @link.lnk

dlwlk.exe: walk_dl err.o mscrtdbd.o hex.o io.o plat_dl.o queue.o recvbufr.o signal.o site.o s_file.o s_http.o uri.o xml.o dl.o
    %write link.lnk NAME $@
    %write link.lnk SYS  nt
    %write link.lnk OPT  st=9216
    %write link.lnk FILE err.o,mscrtdbd.o,hex.o,io.o,plat_dl.o,queue.o,recvbufr.o,site.o,s_file.o,s_http.o,uri.o,xml.o,dl.o
    %write link.lnk LIBF wsock32.lib
    wlink @link.lnk

th.exe: release_th mscrtdbt.o event.o hex.o http.o plat_th.o select.o routine.o signal.o server.o sendbufr.o th.o wsipv6.o wsock1.o wsock2.o
    %write link.lnk NAME $@
    %write link.lnk SYS  nt
    %write link.lnk OPT  st=4608
    %write link.lnk FILE mscrtdbt.o,event.o,hex.o,http.o,plat_th.o,select.o,routine.o,signal.o,server.o,sendbufr.o,th.o,wsipv6.o,wsock1.o,wsock2.o
    %write link.lnk LIBF wsock32.lib
    wlink @link.lnk

thdbg.exe: debug_th mscrtdbt.o event.o hex.o http.o plat_th.o select.o routine.o signal.o server.o sendbufr.o th.o wsipv6.o wsock1.o wsock2.o
    %write link.lnk NAME $@
    %write link.lnk SYS  nt
    %write link.lnk OPT  st=4608
    %write link.lnk FILE mscrtdbt.o,event.o,hex.o,http.o,plat_th.o,select.o,routine.o,signal.o,server.o,sendbufr.o,th.o,wsipv6.o,wsock1.o,wsock2.o
    %write link.lnk LIBF wsock32.lib
    wlink @link.lnk

thwlk.exe: walk_th mscrtdbt.o event.o hex.o http.o plat_th.o select.o routine.o signal.o server.o sendbufr.o th.o wsipv6.o wsock1.o wsock2.o
    %write link.lnk NAME $@
    %write link.lnk SYS  nt
    %write link.lnk OPT  st=4608
    %write link.lnk FILE mscrtdbt.o,event.o,hex.o,http.o,plat_th.o,select.o,routine.o,signal.o,server.o,sendbufr.o,th.o,wsipv6.o,wsock1.o,wsock2.o
    %write link.lnk LIBF wsock32.lib
    wlink @link.lnk

dbg: .SYMBOLIC dldbg.exe thdbg.exe

wlk: .SYMBOLIC dlwlk.exe thwlk.exe

err.o:
	$(CC) @flag.lnk -fo="$@" "../../src/client/err.c"
dl.o:
	$(CC) @flag.lnk -fo="$@" "../../src/cli/dl.c"
mscrtdbd.o: ../../src/platform/mscrtdl.c
    $(CC) @flag.lnk -fo="$@" "../../src/platform/mscrtdl.c"
mscrtdbt.o: ../../src/platform/mscrtdl.c
    $(CC) @flag.lnk -fo="$@" "../../src/platform/mscrtdl.c"
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
select.o: ../../src/server/mltiplex/select.c
    $(CC) @flag.lnk -fo="$@" "../../src/server/mltiplex/select.c"
th.o: ../../src/cli/th.c
    $(CC) @flag.lnk -fo="$@" "../../src/cli/th.c"
uri.o:
    $(CC) @flag.lnk -fo="$@" "../../src/client/uri.c"
xml.o:
    $(CC) @flag.lnk -fo="$@" "../../src/client/xml.c"
wsipv6.o: ../../src/platform/winsock/wsipv6.c
    $(CC) @flag.lnk -fo="$@" "../../src/platform/winsock/wsipv6.c"
wsock1.o: ../../src/platform/winsock/wsock1.c
    $(CC) @flag.lnk -fo="$@" "../../src/platform/winsock/wsock1.c"
wsock2.o: ../../src/platform/winsock/wsock2.c
    $(CC) @flag.lnk -fo="$@" "../../src/platform/winsock/wsock2.c"


walk_dl: .SYMBOLIC
    %write flag.lnk -q -bt=nt -3 -d3 -db -dDBGHALT=1 -dPLATFORM_SYS_ARGV=1 -dPLATFORM_SYS_EXEC=1 -dPLATFORM_SYS_WRITE=1

debug_dl: .SYMBOLIC
    %write flag.lnk -q -bt=nt -3 -d3 -db -dPLATFORM_SYS_ARGV=1 -dPLATFORM_SYS_EXEC=1 -dPLATFORM_SYS_WRITE=1

release_dl: .SYMBOLIC
    %write flag.lnk -q -bt=nt -3 -d0 -osr -zc -dNDEBUG=1 -dPLATFORM_SYS_ARGV=1 -dPLATFORM_SYS_EXEC=1 -dPLATFORM_SYS_WRITE=1

walk_th: .SYMBOLIC
    %write flag.lnk -q -bt=nt -3 -d3 -db -dDBGHALT=1 -dENABLE_WS1=1 -dPLATFORM_NET_ADAPTER=1 -dPLATFORM_NET_LISTEN=1 -dPLATFORM_SYS_ARGV=1

debug_th: .SYMBOLIC
    %write flag.lnk -q -bt=nt -3 -d3 -db -dENABLE_WS1=1 -dPLATFORM_NET_ADAPTER=1 -dPLATFORM_NET_LISTEN=1 -dPLATFORM_SYS_ARGV=1

release_th: .SYMBOLIC
    %write flag.lnk -q -bt=nt -3 -d0 -osr -zc -dNDEBUG=1 -dENABLE_WS1=1 -dPLATFORM_NET_ADAPTER=1 -dPLATFORM_NET_LISTEN=1 -dPLATFORM_SYS_ARGV=1
