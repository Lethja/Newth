<!-- TOC -->
* [What is Open Watcom](#what-is-open-watcom)
* [Setup Open Watcom](#setup-open-watcom)
  * [Requirements](#requirements)
  * [Installing Open Watcom](#installing-open-watcom)
    * [Installing SvarDOS package manager onto another DOS (optional)](#installing-svardos-package-manager-onto-another-dos-optional)
* [Building Newth](#building-newth)
  * [Setup Watcom build environment](#setup-watcom-build-environment)
  * [Adding Watt32 includes](#adding-watt32-includes)
  * [Configuring Watt32 library for linking with Newth](#configuring-watt32-library-for-linking-with-newth)
  * [Build Newth](#build-newth)
<!-- TOC -->

# What is Open Watcom

Watcom C/C++ was an integrated development environment for C, C++, and Fortran programming languages that
was a commercial product until it was discontinued, it has since been released under the
Sybase Open Watcom Public License as Open Watcom. Open Watcoms niche over other typical compiler choices is it's
build targets for legacy hardware and software. Open Watcom builds native code for 16-bit x86 processors and can compile
code from a modern Linux/Windows host for ancient versions of DOS, OS/2 & Windows. 

# Setup Open Watcom

## Requirements

To build Newth with Watcom you will need the following:

- A i386 machine
- A i386 compatible DOS operating system
- Open Watcom C 1.9
- [Watt32 library](https://github.com/gvanem/Watt-32) compiled with the same version of Watcom

> Tip: If hardware isn't available it's possible to proceed with a emulator such as [Dosbox-X](https://dosbox-x.com/)

## Installing Open Watcom

Installation instructions for Watcom differ depending on the build machines operating system.
Some common operating systems are listed in the table below:

| Operating System    | Installation Instructions                                                                                               |
|---------------------|-------------------------------------------------------------------------------------------------------------------------|
| FreeDOS             | Load the bonus CD/ISO then run `FDIMPLES`, navigate to the **Development** group and check **WATCOMC** for installation |
| SvarDOS             | Run `PKGNET pull ow` or [download manually](http://www.svardos.org/?p=repo&cat=devel) then run `PKG install ow.svp`     |
| Linux/MacOS/Windows | Download and run [Open Watcom installer](https://github.com/open-watcom/open-watcom-v2/releases)                        |

> Note: although Open Watcom can cross compile from Linux/Posix & NT systems 
> these instruction will assume Newth is being built by a DOS-like system.

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

## Setup Watcom build environment

The Open Watcom build environment needs to be setup before it can be used. A script is included to do this.
On a typical DOS installation of Open Watcom run the following to enable the build environment.

> Note: the following steps will need to be repeated each time DOS boots.
> If this system is using Open Watcom frequently consider making the following commands 
> part of your `AUTOEXEC.BAT`/`FDAUTO.BAT` script  

```
%DOSDIR%\DEVEL\OW\OWSETENV.BAT
```

> Tip: `wcc`, `wlink` & `wmake` should now be valid commands
> if this isn't the case check the directories in the script and modify them as necessary.

## Adding Watt32 includes

Watcom needs to see Watt32s headers as if they're from the system path when compiling Newth.
To achieve this append the full path to Watt32s include directory to the `INCLUDE` environment variable like so

```
set INCLUDE=%INCLUDE%;C:\NEWTH\EXT\WATT32S\INC
```

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
+#define USE_BSD_API
+#define USE_BSD_FATAL
 #undef USE_BOOTP
-#undef USE_DHCP
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

Run `wmake -f TH.MAK` to build the project.
A self contained 16-bit EXE binary called `TH.EXE` will be made
and can be run from any path including a floppy diskette.