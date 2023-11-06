#!/bin/bash

# Check for the optional executable compressor
# Allows the real mode build to fit on a 5¼-inch DD diskette if present
UPX=`which upx`

# Build
wmake -f DOSTH.MAK
wmake -f DOSTH4GW.MAK

# Compress
if [ -n "$UPX" ]; then
	upx TH.EXE TH4GW.EXE
fi

# Create disk image(s)
mkdir -p diskette
if [ -n "$UPX" ]; then
  mformat -C -i diskette/th_360.ima -v "TH" -f 360
  mformat -C -i diskette/th_720.ima -v "TH" -f 720
	mformat -C -i diskette/th4g_720.ima -v "TH4GW" -f 720
  mformat -C -i diskette/thma_1.4.ima -v "TH" -f 720
else
  mformat -C -i diskette/th_720.ima -v "TH" -f 720
  mformat -C -i diskette/th4g_1.4.ima -v "TH4GW" -f 1440
fi

# Move binary onto disk images
echo "Real Mode 5¼-inch QD Diskette Image"
mcopy -i diskette/th_720.ima TH.EXE ::
mdir -i diskette/th_720.ima ::
if [ -n "$UPX" ]; then
  echo "Real Mode 5¼-inch DD Diskette Image"
	mcopy -i diskette/th_360.ima TH.EXE ::
  mdir -i diskette/th_360.ima ::
  echo "DOS4GW 5¼-inch QD Diskette Image"
  mcopy -i diskette/th4g_720.ima TH4GW.EXE ::
  mdir -i diskette/th4g_720.ima ::
  echo "Multi-arch 3½-inch HD Diskette Image"
  mcopy -i diskette/thma_1.4.ima TH.EXE TH4GW.EXE ::
  mdir -i diskette/thma_1.4.ima ::
else
  echo "DOS4GW 3½-inch HD Diskette Image"
  mcopy -i diskette/th4g_1.4.ima TH4GW.EXE ::
  mdir -i diskette/th4g_1.4.ima ::
fi
