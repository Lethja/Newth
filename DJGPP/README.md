<!-- TOC -->
* [What is DJGPP](#what-is-djgpp)
* [Setup DJGPP](#setup-djgpp)
  * [Requirements](#requirements)
  * [Installing DJGPP](#installing-djgpp)
    * [Installing SvarDOS package manager onto another DOS (optional)](#installing-svardos-package-manager-onto-another-dos-optional)
* [Building Newth](#building-newth)
  * [Setup DJGPP build environment](#setup-djgpp-build-environment)
  * [Configuring Watt32 library for linking with Newth](#configuring-watt32-library-for-linking-with-newth)
  * [Build Newth](#build-newth)
    * [Build](#build)
    * [Compress (optional)](#compress-optional)
<!-- TOC -->

# What is DJGPP

[DJGPP](https://www.delorie.com/djgpp/) is a port of GNUs C/C++/Fortran compilers to DOS systems.
It can be used to build i386 compatible binaries on DOS, for DOS.

# Setup DJGPP

## Requirements

To build Newth with DJGPP you will need the following:

- A i386 machine
- A i386 compatible DOS operating system
- DJGPP v2
- [Watt32 library](https://github.com/gvanem/Watt-32) compiled with the same version of DJGPP

> Tip: If hardware isn't available it's possible to proceed with a emulator such as [Dosbox-X](https://dosbox-x.com/)

## Installing DJGPP

Installation instructions for DJGPP differ depending on the build machines operating system.
Some common operating systems are listed in the table below:

| Operating System | Installation Instructions                                                                                                                                                                                                                                                                   |
|------------------|---------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------|
| FreeDOS          | Load the bonus CD/ISO then run `FDIMPLES`, navigate to the **Development** group and check **DJGPP**, **DJGPP_BN**, **DJGPP_GC** & **DJGPP_MK** for installation                                                                                                                            |
| SvarDOS          | Run `PKGNET pull DJGPP`, `PKGNET pull DJGPP_BN`, `PKGNET pull DJGPP_GC` and `PKGNET pull DJGPP_MK` or [download manually](http://www.svardos.org/?p=repo&cat=devel) then run `PKG install djgpp.svp`, `PKG install djgpp_bn.svp`, `PKG install djgpp_gc.svp` and `PKG install djgpp_mk.svp` |

### Installing SvarDOS package manager onto another DOS (optional)

If you already have a packet driver setup on your DOS system then it is relatively straight forward to setup the SvarDOS
package manager. `.SVP` files are normal zip files so it should be possible to bootstrap the SvarDOS repository by
extracting `PKG.EXE` and moving it and `PKGNET.SVP` onto the system. After which you can use `PKG.EXE` to install
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

The DJGPP build environment needs to be setup before it can be used. A script is included to do this.
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

## Configuring Watt32 library for linking with Newth

Newth depends on BSD-like networking API and compiling for DOS is no exception.
For Newth to link to Watt32 correctly `USE_BSD_API` must be defined when building Watt32 library.

To do this `config.h` has to be manually modified like so.

```
EDIT C:\NEWTH\EXT\WATT32S\SRC\CONFIG.H
```

Recommended changes:

```diff
 #undef USE_DEBUG
 #undef USE_MULTICAST
 #undef USE_BIND
-#undef USE_BSD_API
-#undef USE_BSD_FATAL
-#undef USE_BOOTP
-#undef USE_DHCP
+#define USE_BSD_API
+#define USE_BSD_FATAL
+#define USE_BOOTP
+#define USE_DHCP
 #undef USE_RARP
 #undef USE_GEOIP
 #undef USE_IPV6
 #undef USE_LANGUAGE
-#undef USE_FRAGMENTS
+#define USE_FRAGMENTS
 #undef USE_STATISTICS
 #undef USE_STACKWALKER
 #undef USE_FSEXT
```

## Build Newth

### Build

Run `make` to build the project.
A self contained 32-bit EXE binary called `TH32.EXE` will be made
and can be run from any path (including a floppy diskette) on any DOS 4.0+ computer with a 80386 compatible CPU.

### Compress (optional)

On DOS machines disk space is usually at a premium.
Even though the release builds are stripped of all debugging symbols it is possible to make the binary take
substantially less disk space with UPX compression so that it fits comfortably on a smaller diskette standard.

| Build | UPX command           | Fits on           |
|-------|-----------------------|-------------------|
| DJGPP | `UPX TH32.EXE --best` | 5¼-inch QD (720k) |