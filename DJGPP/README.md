<!-- TOC -->
* [What is DJGPP](#what-is-djgpp)
* [Setup DJGPP](#setup-djgpp)
  * [Requirements](#requirements)
  * [Installing DJGPP](#installing-djgpp)
    * [Installing SvarDOS package manager onto another DOS (optional)](#installing-svardos-package-manager-onto-another-dos-optional)
* [Building Newth](#building-newth)
  * [Setup DJGPP build environment](#setup-djgpp-build-environment)
    * [DOS-like](#dos-like)
    * [nix-like](#nix-like)
  * [Configuring Watt32 for linking with Newth](#configuring-watt32-for-linking-with-newth)
  * [Build](#build)
* [After building](#after-building)
  * [DPMI server](#dpmi-server)
    * [Acquire CWSDPMI](#acquire-cwsdpmi)
    * [Using CWSDPMI](#using-cwsdpmi)
      * [Method 1: Copy CWSDPMI.EXE to the same directory as the program](#method-1-copy-cwsdpmiexe-to-the-same-directory-as-the-program)
      * [Method 2: Cwsdpmi as part of the binaries](#method-2-cwsdpmi-as-part-of-the-binaries)
  * [Compress (optional)](#compress-optional)
  * [Create diskette image (optional)](#create-diskette-image-optional)
    * [On a compressed binary](#on-a-compressed-binary)
      * [5¼-inch QD Diskettes](#5-inch-qd-diskettes)
      * [5¼-inch HD Diskette](#5-inch-hd-diskette)
    * [On an uncompressed binary](#on-an-uncompressed-binary)
      * [3½-inch HD Diskettes](#3-inch-hd-diskettes)
      * [3½-inch ED Diskette](#3-inch-ed-diskette)
<!-- TOC -->

# What is DJGPP

[DJGPP](https://www.delorie.com/djgpp/) is a port of GNUs C/C++/Fortran compilers to DOS systems.
It can be used to build i386 compatible binaries on DOS, for DOS.
DJGPP cross-compilers to build from typical modern systems are also available.

> Note: Building Newth with DJGPP is considered experimental due to the Watt32 library requirement

# Setup DJGPP

## Requirements

To build Newth with DJGPP, you will need the following:

- 80386 compatible machine or emulator
- DOS 4.0+ operating system or emulator
- [DJGPP v2.03](https://www.delorie.com/djgpp/) or later
- [GNU Mtools](https://www.gnu.org/software/mtools/) (optional)
- [UPX binary compression](https://upx.github.io/) (optional)
- [Watt-32 library](https://github.com/gvanem/Watt-32) compiled with the same version of DJGPP

> Tip: If hardware isn't available, it's possible to proceed with an emulator such as [Dosbox-X](https://dosbox-x.com/)

## Installing DJGPP

Installation instructions for DJGPP differ depending on the build machines operating system.
Some common operating systems are listed in the table below:

| Operating System | Installation Instructions                                                                                                                                                                                                                                                                   |
|------------------|---------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------|
| FreeDOS          | Load the bonus CD/ISO then run `FDIMPLES`, navigate to the **Development** group and check **DJGPP**, **DJGPP_BN**, **DJGPP_GC** & **DJGPP_MK** for installation                                                                                                                            |
| Linux            | Install and configure a [DJGPP cross compiler](https://github.com/andrewwutw/build-djgpp)                                                                                                                                                                                                   |
| MacOS            | Install and configure a [DJGPP cross compiler](https://github.com/andrewwutw/build-djgpp)                                                                                                                                                                                                   |
| SvarDOS          | Run `PKGNET pull djgpp`, `PKGNET pull djgpp_bn`, `PKGNET pull djgpp_gc` and `PKGNET pull djgpp_mk` or [download manually](http://www.svardos.org/?p=repo&cat=devel) then run `PKG install djgpp.svp`, `PKG install djgpp_bn.svp`, `PKG install djgpp_gc.svp` and `PKG install djgpp_mk.svp` |
| Windows          | Install and configure a [DJGPP cross compiler](https://github.com/andrewwutw/build-djgpp)                                                                                                                                                                                                   |

### Installing SvarDOS package manager onto another DOS (optional)

If you already have a packet driver setup on your DOS system,
then it is relatively straight forward to set up the SvarDOS
package manager.
`.svp` files are normal `.zip` files so it should be possible to bootstrap the SvarDOS repository by
extracting `PKG.EXE` and moving it and `PKGNET.SVP` onto the system.
After which you can use `PKG.EXE` to install
`PKGNET.SVP` to download `PKG.SVP` to install.

Make sure the network packet driver is loaded and both `%DOSDIR%` & `%PATH%` are set correctly then run the following:

```
PKG.EXE install PKGNET.SVP
PKGNET pull pkg
PKG.EXE install PKG.SVP
```

Optionally, remove the extracted `PKG.EXE` and `.SVP` files to free disk space:

```
DEL PKG.EXE
DEL *.SVP
```

# Building Newth

## Setup DJGPP build environment

The DJGPP build environment needs to be set up before it can be used.

### DOS-like

On a typical DOS installation of DJGPP run the following to enable the build environment.

> Note: the following steps will need to be repeated each time DOS boots.
> If this system is using DJGPP frequently consider making the following commands
> part of your `AUTOEXEC.BAT`/`FDAUTO.BAT` script

```
SET PATH=%PATH%;%DOSDIR%\DEVEL\DJGPP\BIN
SET DJGPP=%DOSDIR%\DEVEL\DJGPP\DJGPP.ENV
```

> Tip: `gcc` should now be valid commands
> if this isn't the case check the directories in the script and modify them as necessary.

### nix-like

Assuming the DJGPP cross-compiler tarball was extracted to `/opt` run the following to set up DJGPP in your shell

```
export PATH=/opt/djgpp/i586-pc-msdosdjgpp/bin/:/opt/djgpp/bin/:$PATH
export GCC_EXEC_PREFIX=/opt/djgpp/lib/gcc/
export DJDIR=/opt/djgpp/i586-pc-msdosdjgpp
```

## Configuring Watt32 for linking with Newth

Newth on DOS depends on Watt32.
The `Watt32s` folder is a Git submodule to this library and can be built in place.
If your development machine doesn't have access to Git, then the Watt32 sources should be extracted on symlinked
into the `Watt32s` folder so that it looks like so.

```
DJGPP\makefile
Watt32s\inc
Watt32s\lib
```

Newth depends on BSD-like networking API, and compiling for DOS is no exception.
For Newth to link to Watt32 correctly `USE_BSD_API` must be defined when building Watt32 library.
It's optional but a good idea to define `USE_BOOTP` and/or `USE_DHCP` 
so that Newth can configure its IP address automatically.

To do this `Watt32s\src\config.h` has to be manually modified like so:

```diff
 #undef USE_DEBUG
 #undef USE_MULTICAST
 #undef USE_BIND
-#undef USE_BSD_API
+#define USE_BSD_API
 #undef USE_BSD_FATAL
-#undef USE_BOOTP
-#undef USE_DHCP
+#define USE_BOOTP
+#define USE_DHCP
 #undef USE_RARP
 #undef USE_GEOIP
 #undef USE_IPV6
 #undef USE_LANGUAGE
 #undef USE_FRAGMENTS
 #undef USE_STATISTICS
 #undef USE_STACKWALKER
 #undef USE_FSEXT
```

> Caution: some versions of watt-32 have a broken implementation of DHCP that can cause an infinite loop.
> On a real DOS this means it could very well lock up the computer with no option but to hard reset.
> If in doubt leave `USE_DHCP` undefined. Most DHCP servers are backwards compatible with BOOTP.

## Build

Run `make` to build the project.
Two 32-bit binaries called `dl.exe` and `th.exe` will be made that can be run from any path (including a floppy
diskette)
on any DOS computer with a DPMI server running on an i386 compatible CPU with a 80387 compatible FPU.

# After building

## DPMI server

A DOS Protected Mode Interface (DPMI) server is a utility that allows real mode DOS to extend itself with protected mode
features of the i386 and later processors.

Like all 32-bit DOS executables, DJGPP binaries need a DPMI server to run.
FreeDOS and Windows 95 setup their own DPMI
server by default, and no further files are necessary on these systems.
If your system doesn't include its own DPMI server
then `CWSDPMI` can be used which is the DJGPP equivalent to Watcoms `DOS4GW.EXE`.

### Acquire CWSDPMI

| Operating System | Installation Instructions                                                                                                 |
|------------------|---------------------------------------------------------------------------------------------------------------------------|
| SvarDOS          | Run `PKGNET pull cwsdpmi` or [download manually](http://svardos.org/?p=repo&cat=progs) then run `PKG install cwsdpmi.svp` |

### Using CWSDPMI

There are two ways to use Cwsdpmi. Which way is better depends on the circumstances of the user and system.
Both methods assume that you're in the same directory as `DL.EXE` and `TH.EXE` and that `cwsdpmi.svp` has been
installed in its default directory.

#### Method 1: Copy CWSDPMI.EXE to the same directory as the program

Similar to `DOS4GW.EXE` for Watcom built applications.
`CWSDPMI.EXE` can be placed in the same directory as an EXE to make it start.
Several DJGPP binaries in the same directory can make use of the same `CWSDPMI.EXE` which can save some disk space.

```
COPY %DOSDIR%\PROGS\CWSDPMI\CWSDPMI.EXE .
```

#### Method 2: Cwsdpmi as part of the binaries

`CWSDPMI.EXE` can be baked into `DL.EXE` and `TH.EXE` so that no external files are necessary to start the program.
This is much more ideal for executables that are intended for portable use.

```
COPY %DOSDIR%\PROGS\CWSDPMI\CWSDSTUB.EXE .

MOVE dl.exe dlnostub.exe
exe2coff dlnostub.exe
COPY /B CWSDSTUB.EXE+dlnostub dl.exe

MOVE th.exe thnostub.exe
exe2coff thnostub.exe
COPY /B CWSDSTUB.EXE+thnostub th.exe
```

## Compress (optional)

On DOS machines, disk space is usually at a premium.
Even though the release builds are stripped of all debugging symbols, it is possible to make the binary take
substantially less disk space with UPX compression so that it fits comfortably on a smaller diskette standard.

| Build | UPX command                | Fits on            |
|-------|----------------------------|--------------------|
| DJGPP | `UPX dl.exe --best`        | 5¼-inch QD (720k)  |
| DJGPP | `UPX dl.exe th.exe --best` | 5¼-inch HD (1200k) |

## Create diskette image (optional)

On a real DOS machines it makes sense to directly copy the new binaries onto a newly formatted diskette.
Conversely, when cross compiling or distributing over the Internet,
it may make more sense to distribute as a floppy disk
image so that users can make their own disks locally.
This can be achieved with GNU Mtools.

> Note: At the time of writing, there's no DOS port of GNU Mtools.
> The newly created binaries will need
> to be transferred to a more modern Linux or Windows machine to use Mtools on them.

With compression, the binaries will fit much better into diskette image than they otherwise would,
in some cases becoming compatible with a lower standard of diskette.
There might be a lot of free space after copying the files to the image.
However, users may want to put other files on the disk
(such as WatTCP configuration and/or a network packet driver)
and a real diskette may contain bad sectors.

With Mtools installed, create a diskette image with `mformat` then copy the binaries to the new image with `mcopy`.
Below are some example configurations.

> Tip: If you can only spare one diskette which can't fit both programs on it.
> You could write `DL.EXE` and a packet driver to the disk
> then use these to download `TH.EXE` over the network.

### On a compressed binary

#### 5¼-inch QD Diskettes

```bash
mformat -C -i dldj_720.ima -v "DLDJ" -f 720
mcopy -i dldj_720.ima DL.EXE ::
```

```bash
mformat -C -i thdj_720.ima -v "THDJ" -f 720
mcopy -i thdj_720.ima TH.EXE ::
```

#### 5¼-inch HD Diskette

```bash
mformat -C -i newth1.2.ima -v "NEWTH DJ" -f 1200
mcopy -i newth1.2.ima DL.EXE TH.EXE ::
```

### On an uncompressed binary

#### 3½-inch HD Diskettes

```bash
mformat -C -i dldj_1.4.ima -v "DLDJ" -f 1440
mcopy -i dldj_1.4.ima DL.EXE ::
```

```bash
mformat -C -i thdj_1.4.ima -v "THDJ" -f 1440
mcopy -i thdj_1.4.ima TH.EXE ::
```

#### 3½-inch ED Diskette

```bash
mformat -C -i newth2.8.ima -v "NEWTH DJ" -f 2880
mcopy -i newth2.8.ima DL.EXE TH.EXE ::
```