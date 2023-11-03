#!/bin/bash

# Check for the optional executable compressor
# Allows the build to fit on a 5¼-inch QD diskette if present
UPX=`which upx`

# Build (and compress)
make

if [ -n "$UPX" ]; then
	upx th32.exe -o th.exe
else
	cp th32.exe th.exe
fi

# Create disk image(s)
mkdir -p diskette
if [ -n "$UPX" ]; then
	mformat -C -i diskette/th32_720.ima -v "THDJ32" -f 720
fi
mformat -C -i diskette/th32_1.4.ima -v "THDJ32" -f 1440

# Move binary onto disk image(s)
if [ -n "$UPX" ]; then
	mcopy -i diskette/th32_720.ima th.exe ::
	echo "5¼-inch QD Diskette Image"
	mdir -i diskette/th32_720.ima ::
fi
mcopy -i diskette/th32_1.4.ima th.exe ::
echo "3½-inch HD Diskette Image"
mdir -i diskette/th32_1.4.ima ::

rm th.exe
