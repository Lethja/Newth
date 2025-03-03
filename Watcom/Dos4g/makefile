CC = *wcc386
LD = *wlink

WATT_32_DIR = watt32s
WATT_32_LIB = wattcpwf.lib

COMMON_CFLAGS = -q -dWATT32=1 -bt=dos -mf -3 -i="$(WATT_32_DIR)/inc" -d0 -osr -zc -dNDEBUG=1
DL_CFLAGS = $(COMMON_CFLAGS) -dPLATFORM_SYS_ARGV=1 -dPLATFORM_SYS_EXEC=1 -dPLATFORM_SYS_WRITE=1
TH_CFLAGS = $(COMMON_CFLAGS) -dPLATFORM_NET_ADAPTER=1 -dPLATFORM_NET_LISTEN=1 -dPLATFORM_SYS_ARGV=1

DL_LFLAGS = OPT st=9216 FILE {$(DL_OBJ)} LIBP $(WATT_32_DIR)/lib LIBF $(WATT_32_LIB) SYS dos4g
TH_LFLAGS = OPT st=4608 FILE {$(TH_OBJ)} LIBP $(WATT_32_DIR)/lib LIBF $(WATT_32_LIB) SYS dos4g

DL_OBJ = o/hex.o o/signal.o dlo/err.o dlo/doswatb.o dlo/io.o dlo/platform.o dlo/queue.o dlo/recvbufr.o dlo/site.o dlo/file.o dlo/http.o dlo/uri.o dlo/xml.o dlo/dl.o
TH_OBJ = o/hex.o o/signal.o tho/doswatb.o tho/event.o tho/http.o tho/platform.o tho/poll.o tho/routine.o tho/server.o tho/sendbufr.o tho/th.o

all: .SYMBOLIC dl.exe th.exe

dl.exe: $(WATT_32_DIR)/lib/$(WATT_32_LIB) o dlo $(DL_OBJ)
	$(LD) NAME $@ $(DL_LFLAGS)

th.exe: $(WATT_32_DIR)/lib/$(WATT_32_LIB) o tho $(TH_OBJ)
	$(LD) NAME $@ $(TH_LFLAGS)

o/hex.o: ../../src/common/hex.c
	$(CC) $(COMMON_CFLAGS) -fo="$@" $<
o/signal.o: ../../src/common/signal.c
	$(CC) $(COMMON_CFLAGS) -fo="$@" $<

dlo/dl.o: ../../src/cli/dl.c
	$(CC) $(DL_CFLAGS) -fo="$@" $<
dlo/doswatb.o: ../../src/platform/doswatb.c
	$(CC) $(DL_CFLAGS) -fo="$@" $<
dlo/file.o: ../../src/client/site/file.c
	$(CC) $(DL_CFLAGS) -fo="$@" $<
dlo/http.o: ../../src/client/site/http.c
	$(CC) $(DL_CFLAGS) -fo="$@" $<
dlo/platform.o: ../../src/platform/platform.c
	$(CC) $(DL_CFLAGS) -fo="$@" $<
dlo/err.o: ../../src/client/err.c
	$(CC) $(COMMON_CFLAGS) -fo="$@" $<
dlo/io.o: ../../src/client/io.c
	$(CC) $(COMMON_CFLAGS) -fo="$@" $<
dlo/queue.o: ../../src/client/queue.c
	$(CC) $(COMMON_CFLAGS) -fo="$@" $<
dlo/recvbufr.o: ../../src/client/recvbufr.c
	$(CC) $(COMMON_CFLAGS) -fo="$@" $<
dlo/site.o: ../../src/client/site.c
	$(CC) $(COMMON_CFLAGS) -fo="$@" $<
dlo/uri.o: ../../src/client/uri.c
	$(CC) $(COMMON_CFLAGS) -fo="$@" $<
dlo/xml.o: ../../src/client/xml.c
	$(CC) $(COMMON_CFLAGS) -fo="$@" $<

tho/routine.o: ../../src/server/routine.c
	$(CC) $(TH_CFLAGS) -fo="$@" $<
tho/doswatb.o: ../../src/platform/doswatb.c
	$(CC) $(TH_CFLAGS) -fo="$@" $<
tho/platform.o: ../../src/platform/platform.c
	$(CC) $(TH_CFLAGS) -fo="$@" $<
tho/poll.o: ../../src/server/mltiplex/poll.c
	$(CC) $(TH_CFLAGS) -fo="$@" $<
tho/th.o: ../../src/cli/th.c
	$(CC) $(TH_CFLAGS) -fo="$@" $<
tho/server.o: ../../src/server/server.c
	$(CC)  $(COMMON_CFLAGS) -fo="$@" $<
tho/sendbufr.o: ../../src/server/sendbufr.c
	$(CC)  $(COMMON_CFLAGS) -fo="$@" $<
tho/event.o: ../../src/server/event.c
	$(CC)  $(COMMON_CFLAGS) -fo="$@" $<
tho/http.o: ../../src/server/http.c
	$(CC)  $(COMMON_CFLAGS) -fo="$@" $<

tho:
	mkdir tho
dlo:
	mkdir dlo
o:
	mkdir o
