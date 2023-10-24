CC=wcc
CFLAGS=-dWATT32=1 -bt=dos -q -d0 -ml -0 -osr -zc -s

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
th.exe: obj
    %write test.lnk NAME    $@
    %write test.lnk SYSTEM  DOS
    %write test.lnk LIB     ..\ext\watt32s\lib\wattcpwl.lib
    %write test.lnk FILE    th.obj
    %write test.lnk FILE    server.obj
    %write test.lnk FILE    platform.obj
    %write test.lnk FILE    event.obj
    %write test.lnk FILE    http.obj
    %write test.lnk FILE    routine.obj
    %write test.lnk FILE    sockbufr.obj
    wlink  @test.lnk

# Compile without doing the final link
obj: th.obj server.obj platform.obj event.obj http.obj routine.obj sockbufr.obj watt32.obj

th.obj:
    $(CC) $(CFLAGS) -fo=th.obj ..\src\cli\th.c
server.obj:
    $(CC) $(CFLAGS) -fo=server.obj ..\src\common\server.c
platform.obj:
    $(CC) $(CFLAGS) -fo=platform.obj ..\src\platform\platform.c
event.obj:
    $(CC) $(CFLAGS) -fo=event.obj ..\src\server\event.c
http.obj:
    $(CC) $(CFLAGS) -fo=http.obj ..\src\server\http.c
routine.obj:
    $(CC) $(CFLAGS) -fo=routine.obj ..\src\server\routine.c
sockbufr.obj:
    $(CC) $(CFLAGS) -fo=sockbufr.obj ..\src\server\sockbufr.c
watt32.obj:
    $(CC) $(CFLAGS) -fo=watt32.obj ..\src\platform\watt32.c

clean: .SYMBOLIC
    DEL *.OBJ
    DEL *.EXE