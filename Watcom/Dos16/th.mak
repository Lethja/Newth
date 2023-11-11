CC=wcc

# Default building process
th.exe: release th.o select.o server.o platform.o event.o http.o doswatb.o routine.o sockbufr.o
    %write link.lnk NAME $@
    %write link.lnk SYS  dos
    %write link.lnk OPT  st=4608
    %write link.lnk FILE th.o,server.o,platform.o,event.o,http.o,doswatb.o,routine.o,sockbufr.o
    %write link.lnk LIBP watt32s/lib/
    %write link.lnk LIBF wattcpwl.lib
    wlink @link.lnk

thdbg.exe: debug th.o select.o server.o platform.o event.o http.o doswatb.o routine.o sockbufr.o
    %write link.lnk NAME $@
    %write link.lnk SYS  dos
    %write link.lnk OPT  st=4608
    %write link.lnk FILE th.o,server.o,platform.o,event.o,http.o,doswatb.o,routine.o,sockbufr.o
    %write link.lnk LIBP watt32s/lib/
    %write link.lnk LIBF wattcpwl.lib
    wlink @link.lnk

thwlk.exe: walk th.o select.o server.o platform.o event.o http.o doswatb.o routine.o sockbufr.o
    %write link.lnk NAME $@
    %write link.lnk SYS  dos
    %write link.lnk OPT  st=4608
    %write link.lnk FILE th.o,server.o,platform.o,event.o,http.o,doswatb.o,routine.o,sockbufr.o
    %write link.lnk LIBP watt32s/lib/
    %write link.lnk LIBF wattcpwl.lib
    wlink @link.lnk

dbg: .SYMBOLIC thdbg.exe

wlk: .SYMBOLIC thwlk.exe

th.o: ../../src/cli/th.c
    $(CC) @flag.lnk -fo="$@" "../../src/cli/th.c"
select.o: ../../src/server/mltiplex/select.c
    $(CC) @flag.lnk -fo="$@" "../../src/server/mltiplex/select.c"
server.o: ../../src/server/server.c
    $(CC) @flag.lnk -fo="$@" "../../src/server/server.c"
doswatb.o: ../../src/platform/doswatb.c
    $(CC) @flag.lnk -fo="$@" "../../src/platform/doswatb.c"
platform.o: ../../src/platform/platform.c
    $(CC) @flag.lnk -fo="$@" "../../src/platform/platform.c"
event.o: ../../src/server/event.c
    $(CC) @flag.lnk -fo="$@" "../../src/server/event.c"
http.o: ../../src/server/http.c
    $(CC) @flag.lnk -fo="$@" "../../src/server/http.c"
routine.o: ../../src/server/routine.c
    $(CC) @flag.lnk -fo="$@" "../../src/server/routine.c"
sockbufr.o: ../../src/server/sockbufr.c
    $(CC) @flag.lnk -fo="$@" "../../src/server/sockbufr.c"

walk: .SYMBOLIC
    %write flag.lnk -q -dWATT32=1 -bt=dos -ml -0 -i="watt32s/inc/" -d3 -db -dDBGHALT=1

debug: .SYMBOLIC
    %write flag.lnk -q -dWATT32=1 -bt=dos -ml -0 -i="watt32s/inc/" -d3 -db

release: .SYMBOLIC
    %write flag.lnk -q -dWATT32=1 -bt=dos -ml -0 -i="watt32s/inc/" -d0 -osr -zc -dNDEBUG=1
