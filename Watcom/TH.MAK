CC16=wcc
CFLAGS16=-dWATT32=1 -bt=dos -d0 -ml -0 -osr -zc -s

CC32=wcc386
CFLAGS32=-dWATT32=1 -bt=dos -d0 -mf -3 -osr -zc -s

## Watcom CFLAGS of interest
# -bt=      : Set build target...
#     com   : 16-bit DOS COM file
#     dos   : 16-bit DOS EXE file
#     dos4g : 32-bit DOS EXE file
# -d0       : No debugging information
# -m        : Memory model...
#   f       : Flat  (32-bit only)
#   s       : Small (16-bit only)
#   l       : Large (16-bit only)
# -0        : Optimize for 8086
# -3        : Optimize for 386
# -o        : Optimize...
#   r       : Reorder instructions for most effective use of the CPU pipeline
#   s       : For space over performance
# -q        : Be quiet!
# -s        : Ignore stack overflow checks
# -zc       : Place const data into code segment

# Default building process
th16.exe: d16/th.obj
    %write link.lnk NAME $@
    %write link.lnk SYS  dos
    %write link.lnk FILE d16/th.obj,d16/server.obj,d16/platform.obj,d16/event.obj,d16/http.obj,d16/owatcom.obj,d16/routine.obj,d16/sockbufr.obj
    %write link.lnk LIBP ../ext/watt32s/lib/
    %write link.lnk LIBF wattcpwl.lib
    wlink @link.lnk

d16/th.obj:
    IF NOT EXIST D16 MD D16
    $(CC16) $(CFLAGS16) -fo="d16/th.obj" "../src/cli/th.c"
    $(CC16) $(CFLAGS16) -fo="d16/server.obj" "../src/common/server.c"
    $(CC16) $(CFLAGS16) -fo="d16/platform.obj" "../src/platform/platform.c"
    $(CC16) $(CFLAGS16) -fo="d16/event.obj" "../src/server/event.c"
    $(CC16) $(CFLAGS16) -fo="d16/http.obj" "../src/server/http.c"
    $(CC16) $(CFLAGS16) -fo="d16/owatcom.obj" "../src/platform/owatcom.c"
    $(CC16) $(CFLAGS16) -fo="d16/routine.obj" "../src/server/routine.c"
    $(CC16) $(CFLAGS16) -fo="d16/sockbufr.obj" "../src/server/sockbufr.c"

th32.exe: d32/th.obj
    %write link.lnk NAME $@
    %write link.lnk SYS  dos4g
    %write link.lnk FILE d32/th.obj,d32/server.obj,d32/platform.obj,d32/event.obj,d32/http.obj,d32/owatcom.obj,d32/routine.obj,d32/sockbufr.obj
    %write link.lnk LIBP ../ext/watt32s/lib/
    %write link.lnk LIBF wattcpwf.lib
    wlink @link.lnk

d32/th.obj:
    IF NOT EXIST D32 MD D32
    $(CC32) $(CFLAGS32) -fo="d32/th.obj" "../src/cli/th.c"
    $(CC32) $(CFLAGS32) -fo="d32/server.obj" "../src/common/server.c"
    $(CC32) $(CFLAGS32) -fo="d32/platform.obj" "../src/platform/platform.c"
    $(CC32) $(CFLAGS32) -fo="d32/event.obj" "../src/server/event.c"
    $(CC32) $(CFLAGS32) -fo="d32/http.obj" "../src/server/http.c"
    $(CC32) $(CFLAGS32) -fo="d32/owatcom.obj" "../src/platform/owatcom.c"
    $(CC32) $(CFLAGS32) -fo="d32/routine.obj" "../src/server/routine.c"
    $(CC32) $(CFLAGS32) -fo="d32/sockbufr.obj" "../src/server/sockbufr.c"

clean: .SYMBOLIC
    IF EXIST D16 DELTREE /Y D16
    IF EXIST D32 DELTREE /Y D32
    IF EXIST TH16.EXE DEL TH16.EXE
    IF EXIST TH32.EXE DEL TH32.EXE
