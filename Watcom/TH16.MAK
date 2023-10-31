CC=wcc

# Default building process
th16.exe: release d16/th.o d16/server.o d16/platform.o d16/event.o d16/http.o d16/doswatb.o d16/routine.o d16/sockbufr.o
    %write link.lnk NAME $@
    %write link.lnk SYS  dos
    %write link.lnk OPT  st=8192
    %write link.lnk FILE d16/th.o,d16/server.o,d16/platform.o,d16/event.o,d16/http.o,d16/doswatb.o,d16/routine.o,d16/sockbufr.o
    %write link.lnk LIBP ../ext/watt32s/lib/
    %write link.lnk LIBF wattcpwl.lib
    wlink @link.lnk

th16dbg.exe: debug d16/th.o d16/server.o d16/platform.o d16/event.o d16/http.o d16/doswatb.o d16/routine.o d16/sockbufr.o
    %write link.lnk NAME $@
    %write link.lnk SYS  dos
    %write link.lnk OPT  st=8192
    %write link.lnk FILE d16/th.o,d16/server.o,d16/platform.o,d16/event.o,d16/http.o,d16/doswatb.o,d16/routine.o,d16/sockbufr.o
    %write link.lnk LIBP ../ext/watt32s/lib/
    %write link.lnk LIBF wattcpwl.lib
    wlink @link.lnk

dbg: .SYMBOLIC th16dbg.exe

d16/th.o: objfolder
    $(CC) @flag.lnk -fo="$@" "../src/cli/th.c"
d16/server.o: objfolder
    $(CC) @flag.lnk -fo="$@" "../src/common/server.c"
d16/doswatb.o: objfolder
    $(CC) @flag.lnk -fo="$@" "../src/platform/doswatb.c"
d16/platform.o: objfolder
    $(CC) @flag.lnk -fo="$@" "../src/platform/platform.c"
d16/event.o: objfolder
    $(CC) @flag.lnk -fo="$@" "../src/server/event.c"
d16/http.o: objfolder
    $(CC) @flag.lnk -fo="$@" "../src/server/http.c"
d16/routine.o: objfolder
    $(CC) @flag.lnk -fo="$@" "../src/server/routine.c"
d16/sockbufr.o: objfolder
    $(CC) @flag.lnk -fo="$@" "../src/server/sockbufr.c"

debug: .SYMBOLIC
    @del flag.lnk
    %write flag.lnk -dWATT32=1 -bt=dos -ml -0 -d3 -db -dDBGHALT=1

release: .SYMBOLIC
    @del flag.lnk
    %write flag.lnk -dWATT32=1 -bt=dos -ml -0 -d0 -osr -zc -dNDEBUG=1

objfolder: .SYMBOLIC
    IF NOT EXIST D16 MD D16

clean: .SYMBOLIC
    IF EXIST D16 DELTREE /Y D16
    IF EXIST TH16.EXE DEL TH16.EXE
    IF EXIST TH16DBG.EXE DEL TH16DBG.EXE
