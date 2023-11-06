#!/bin/bash

# Check for the optional executable compressor
# Allows the build to fit on a 5¼-inch QD diskette if present
UPX=`which upx`

# Build
make

# Compress
if [ -n "$UPX" ]; then
	upx thdj.exe -o th.exe
else
	cp thdj.exe th.exe
fi

# Create disk image(s)
mkdir -p diskette
if [ -n "$UPX" ]; then
	mformat -C -i diskette/thdj_720.ima -v "THDJ" -f 720
fi
mformat -C -i diskette/thdj_1.4.ima -v "THDJ" -f 1440

# Move binary onto disk image(s)
if [ -n "$UPX" ]; then
	mcopy -i diskette/thdj_720.ima th.exe ::
	echo "5¼-inch QD Diskette Image"
	mdir -i diskette/thdj_720.ima ::
fi
mcopy -i diskette/thdj_1.4.ima th.exe ::
echo "3½-inch HD Diskette Image"
mdir -i diskette/thdj_1.4.ima ::

rm th.exe
