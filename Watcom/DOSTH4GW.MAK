CC=wcc386

# Default building process
th4gw.exe: release d32/th.o d32/server.o d32/platform.o d32/event.o d32/http.o d32/doswatb.o d32/routine.o d32/sockbufr.o
    %write link.lnk NAME $@
    %write link.lnk SYS  dos4g
    %write link.lnk OPT  st=8192
    %write link.lnk FILE d32/th.o,d32/server.o,d32/platform.o,d32/event.o,d32/http.o,d32/doswatb.o,d32/routine.o,d32/sockbufr.o
    %write link.lnk LIBP ../ext/watt32s/lib/
    %write link.lnk LIBF wattcpwf.lib
    wlink @link.lnk
    
th4gwdbg.exe: debug d32/th.o d32/server.o d32/platform.o d32/event.o d32/http.o d32/doswatb.o d32/routine.o d32/sockbufr.o
    %write link.lnk NAME $@
    %write link.lnk SYS  dos4g
    %write link.lnk OPT  st=8192
    %write link.lnk FILE d32/th.o,d32/server.o,d32/platform.o,d32/event.o,d32/http.o,d32/doswatb.o,d32/routine.o,d32/sockbufr.o
    %write link.lnk LIBP ../ext/watt32s/lib/
    %write link.lnk LIBF wattcpwf.lib
    wlink @link.lnk

dbg: .SYMBOLIC th4gdbg.exe

d32/th.o: objfolder
    $(CC) @flag.lnk -fo="$@" "../src/cli/th.c"
d32/server.o: objfolder
    $(CC) @flag.lnk -fo="$@" "../src/common/server.c"
d32/doswatb.o: objfolder
    $(CC) @flag.lnk -fo="$@" "../src/platform/doswatb.c"
d32/platform.o: objfolder
    $(CC) @flag.lnk -fo="$@" "../src/platform/platform.c"
d32/event.o: objfolder
    $(CC) @flag.lnk -fo="$@" "../src/server/event.c"
d32/http.o: objfolder
    $(CC) @flag.lnk -fo="$@" "../src/server/http.c"
d32/routine.o: objfolder
    $(CC) @flag.lnk -fo="$@" "../src/server/routine.c"
d32/sockbufr.o: objfolder
    $(CC) @flag.lnk -fo="$@" "../src/server/sockbufr.c"

debug: .SYMBOLIC
    @del flag.lnk
    %write flag.lnk -dWATT32=1 -dHTTP_CONNECTION_NEVER_REUSE=1 -bt=dos -mf -3 -d3 -db -dDBGHALT=1

release: .SYMBOLIC
    @del flag.lnk
    %write flag.lnk -dWATT32=1 -dHTTP_CONNECTION_NEVER_REUSE=1 -bt=dos -mf -3 -d0 -osr -zc -s -dNDEBUG=1

objfolder: .SYMBOLIC
    IF NOT EXIST D32 MD D32

clean: .SYMBOLIC
    IF EXIST D32 DELTREE /Y D32
    IF EXIST TH32.EXE DEL TH32.EXE
    IF EXIST TH32DBG.EXE DEL TH32DBG.EXE