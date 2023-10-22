CC=wcc
CFLAGS=-q -ms -0 -os -or -zc -s -bt=com

## Watcom CFLAGS of interest
# -q        : Be quiet!
# -ms       : Memory model small
# -ml       : Memory model large
# -0        : Target 8086 instruction set
# -os       : Optimize for space over performance
# -or       : Reorder instructions to make best use of the pipeline
# -zc       : Place const data into code segment
# -s        : Ignore stack overflow checks
# -bt=com   : Set build target to a DOS COM file
# -bt=dos   : Set build target to a DOS EXE file

.c.obj
    $(CC) $(CFLAGS) $<

test.exe: test.obj
    %write test.lnk NAME    $@
    %write test.lnk SYSTEM  DOS
    %write test.lnk FILE    test.obj
    wlink  @test.lnk

clean: .SYMBOLIC
    DEL *.OBJ
#   DEL *.COM
    DEL *.EXE