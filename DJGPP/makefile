CC=gcc
CFLAGS = -Wall -Wno-unknown-pragmas -O3 -s -Iwatt32s/inc

LIB_WATT = watt32s/lib/libwatt.a

DEF = -DWATT32=1 -DNDEBUG=1
SRC = ../src/common/hex.c ../src/common/signal.c ../src/platform/doswatb.c ../src/platform/platform.c

DL_DEF = $(DEF) -DPLATFORM_SYS_ARGV=1 -DPLATFORM_SYS_EXEC=1 -DPLATFORM_SYS_WRITE=1
DL_SRC = ../src/cli/dl.c $(SRC) ../src/client/err.c ../src/client/io.c ../src/client/queue.c ../src/client/recvbufr.c ../src/client/site.c ../src/client/site/file.c ../src/client/site/http.c ../src/client/uri.c ../src/client/xml.c

TH_DEF = $(DEF) -DPLATFORM_NET_ADAPTER=1 -DPLATFORM_NET_LISTEN=1 -DPLATFORM_SYS_ARGV=1
TH_SRC = ../src/cli/th.c $(SRC) ../src/server/event.c ../src/server/http.c ../src/server/routine.c ../src/server/server.c ../src/server/sendbufr.c ../src/server/mltiplex/select.c

DL_BIN = dl
TH_BIN = th

ifneq ($(origin COMSPEC), undefined)
DL_BIN := $(DL_BIN).exe
TH_BIN := $(TH_BIN).exe
endif

all: $(DL_BIN) $(TH_BIN)

$(DL_BIN):
	$(CC) $(CFLAGS) $(DL_DEF) $(DL_SRC) $(LIB_WATT) -o $@

$(TH_BIN):
	$(CC) $(CFLAGS) $(TH_DEF) $(TH_SRC) $(LIB_WATT) -o $@
