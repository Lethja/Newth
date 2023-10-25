CC16=wcc
CFLAGS16=-dWATT32=1 -bt=dos -d0 -ml -0 -osr -zc -s

CC32=wcc386
CFLAGS32=-dWATT32=1 -bt=dos -d0 -mf -3 -osr -zc -s

## Watcom CFLAGS of interest
# -bt=com   : Set build target to a DOS COM file
# -bt=dos   : Set build target to a DOS EXE file
# -d0       : No debugging information
# -ms       : Memory model small
# -ml       : Memory model large
# -0        : Target 8086 instruction set
# -os       : Optimize for space over performance
# -or       : Reorder instructions to make best use of the pipeline
# -q        : Be quiet!
# -s        : Ignore stack overflow checks
# -zc       : Place const data into code segment

# Default building process
th16: th16build
    %write link.lnk N    $@
    %write link.lnk SYS  DOS
    %write link.lnk F    th.obj,server.obj,platform.obj,event.obj,http.obj,owatcom.obj,routine.obj,sockbufr.obj
    %write link.lnk LIBP "../ext/watt32s/lib/"
    %write link.lnk LIBF wattcpwl.lib
    wlink @link.lnk

th16build:
    $(CC16) $(CFLAGS16) -fo=th.obj "../src/cli/th.c"
    $(CC16) $(CFLAGS16) -fo=server.obj "../src/common/server.c"
    $(CC16) $(CFLAGS16) -fo=platform.obj "../src/platform/platform.c"
    $(CC16) $(CFLAGS16) -fo=event.obj "../src/server/event.c"
    $(CC16) $(CFLAGS16) -fo=http.obj "../src/server/http.c"
    $(CC16) $(CFLAGS16) -fo=owatcom.obj "../src/platform/owatcom.c"
    $(CC16) $(CFLAGS16) -fo=routine.obj "../src/server/routine.c"
    $(CC16) $(CFLAGS16) -fo=sockbufr.obj "../src/server/sockbufr.c"

th32: th32build
    %write link.lnk N    $@
    %write link.lnk SYS  DOS4G
    %write link.lnk F    d32/th.obj,d32/server.obj,d32/platform.obj,d32/event.obj,d32/http.obj,d32/owatcom.obj,d32/routine.obj,d32/sockbufr.obj
    %write link.lnk LIBP "../ext/watt32s/lib/"
    %write link.lnk LIBF wattcpwf.lib
    wlink @link.lnk

th32build:
    md d32
    $(CC32) $(CFLAGS32) -fo="d32/th.obj" "../src/cli/th.c"
    $(CC32) $(CFLAGS32) -fo="d32/server.obj" "../src/common/server.c"
    $(CC32) $(CFLAGS32) -fo="d32/platform.obj" "../src/platform/platform.c"
    $(CC32) $(CFLAGS32) -fo="d32/event.obj" "../src/server/event.c"
    $(CC32) $(CFLAGS32) -fo="d32/http.obj" "../src/server/http.c"
    $(CC32) $(CFLAGS32) -fo="d32/owatcom.obj" "../src/platform/owatcom.c"
    $(CC32) $(CFLAGS32) -fo="d32/routine.obj" "../src/server/routine.c"
    $(CC32) $(CFLAGS32) -fo="d32/sockbufr.obj" "../src/server/sockbufr.c"

clean: .SYMBOLIC
    DEL *.OBJ