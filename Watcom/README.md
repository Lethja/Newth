<!-- TOC -->
* [What is Open Watcom](#what-is-open-watcom)
  * [Watcom compiler flags](#watcom-compiler-flags)
* [Setup Open Watcom](#setup-open-watcom)
  * [Requirements](#requirements)
    * [Choose processor architecture](#choose-processor-architecture)
      * [Processor compatibility matrix](#processor-compatibility-matrix)
      * [Build time](#build-time)
  * [Installing Open Watcom](#installing-open-watcom)
    * [Installing SvarDOS package manager onto another DOS (optional)](#installing-svardos-package-manager-onto-another-dos-optional)
* [Building Newth](#building-newth)
  * [Setup Watcom build environment](#setup-watcom-build-environment)
  * [Build Newth](#build-newth)
    * [Build for DOS](#build-for-dos)
      * [Configuring Watt32 for linking with Newth](#configuring-watt32-for-linking-with-newth)
      * [Build for 16-bit (Real Mode)](#build-for-16-bit-real-mode)
      * [Build for 32-bit (DOS4GW)](#build-for-32-bit-dos4gw)
    * [Build for Windows](#build-for-windows)
      * [Configuring Open Watcom for linking to Win32](#configuring-open-watcom-for-linking-to-win32)
      * [Build for Win32](#build-for-win32)
* [After building](#after-building)
  * [Compress binary (optional)](#compress-binary-optional)
  * [Create diskette image (optional)](#create-diskette-image-optional)
      * [On Compressed Binaries](#on-compressed-binaries)
        * [Real Mode 5¼-inch DD Diskette](#real-mode-5-inch-dd-diskette)
        * [DOS4GW 5¼-inch QD Diskettes](#dos4gw-5-inch-qd-diskettes)
        * [Multi-arch 3½-inch HD Diskettes](#multi-arch-3-inch-hd-diskettes)
        * [Multi-arch 3½-inch ED Diskette](#multi-arch-3-inch-ed-diskette)
      * [On Uncompressed Binaries](#on-uncompressed-binaries)
        * [Real Mode 5¼-inch QD Diskettes](#real-mode-5-inch-qd-diskettes)
        * [DOS4GW 3½-inch HD Diskettes](#dos4gw-3-inch-hd-diskettes)
        * [DOS4GW 3½-inch ED Diskette](#dos4gw-3-inch-ed-diskette)
<!-- TOC -->

# What is Open Watcom

Watcom C/C++ was an integrated development environment for C, C++, and Fortran programming languages that
was a commercial product until it was discontinued, it has since been released under the
Sybase Open Watcom Public License as Open Watcom.
Open Watcoms niche over other typical compiler choices is its 
build targets for legacy hardware and software.
Open Watcom builds native code for 16-bit x86 processors and can compile
code from a modern Linux/Windows host for ancient versions of DOS, OS/2 & Windows.

> Note: Building Newth with Watcom is considered experimental due to the Watt32 library requirement

## Watcom compiler flags

Open Watcoms compiler flags (`wcc` & `wcc386`) are quite different compared to a typical modern Clang, or GCC.
Here are some of the more common ones:

```
-bt=      : Set build target...
    com   : ... 16-bit DOS COM file
    dos   : ... 16-bit DOS EXE file
    dos4g : ... 32-bit DOS EXE file
-d        : Debug level/define variable...
  0       : ... No debugging information (release build)
  1       : ... Some debugging information (debug build)
  2       : ... Full debugging information (developer build)
  FOO=bar : ... Set define 'FOO' to value 'bar'
-m        : Memory model...
  f       : ... flat  (32-bit only)
  s       : ... small (16-bit only)
  l       : ... large (16-bit only)
-0        : Optimize for 8086
-3        : Optimize for 386
-o        : Optimize...
  r       : ... instructions to make the most effective use of the CPU pipeline
  s       : ... for space over performance
-q        : Be quiet!
-s        : Disable stack overflow checks
-zc       : Place const data into code segment
```

# Setup Open Watcom

## Requirements

To build Newth with Watcom, you will need the following:

- 80386 compatible machine or emulator
- DOS 4.0+ operating system or emulator
- [GNU Mtools](https://www.gnu.org/software/mtools/) (optional)
- [Open Watcom 1.9](http://openwatcom.org/)
- [UPX binary compression](https://upx.github.io/) (optional)
- [Watt32 library](https://github.com/gvanem/Watt-32) compiled with the same version of Open Watcom

> Tip: If hardware isn't available, it's possible to proceed with an emulator such as [Dosbox-X](https://dosbox-x.com/)

### Choose processor architecture

Open Watcom can build DOS executables for both 16-bit real mode and 32-bit protected mode.
It is important to decide what version to build (if not both) before proceeding.
The 16-bit version can run on 32-bit processors.
This might be desired for portability,
but typically the 32-bit version should be used
whenever a processor can run it as it will give the program more memory to use
and take advantage of the ISA.

#### Processor compatibility matrix

| Compatibility | 8086 | 8088   | 80186 | 80286 | 80386  | 80486 | i586 | i686 | x86_64 |
|---------------|------|--------|-------|-------|--------|-------|------|------|--------|
| Real Mode     | Yes  | Native | Yes   | Yes   | Yes    | Yes   | Yes  | Yes  | Yes    |
| DOS4GW        | No   | No     | No    | No    | Native | Yes   | Yes  | Yes  | Yes    |

> Note: Open Watcom is a DOS4GW binary

#### Build time

Depending on the hardware being used to build Newth long periods may pass without any output printed on the screen.
Expected build times are listed below:

| Market Name       | ISA      | Clock  | Build time (up to) |
|-------------------|----------|--------|--------------------|
| Intel i386        | 80386    | 25Mhz  | ~14 minutes        |
| Intel i486        | 80486    | 33Mhz  | ~6 minutes         |
| Intel i486 DX2    | 80486    | 66Mhz  | ~3 minutes         |
| Intel Pentium     | i586     | 60Mhz  | ~3 minutes         |
| Intel Pentium MMX | i586+MMX | 133Mhz | ~1 minute          |
| AMD K6            | K6       | 133Mhz | ~50 seconds        |
| AMD Athlon        | K7       | 600Mhz | ~30 seconds        |

## Installing Open Watcom

Installation instructions for Watcom differ depending on the build machines operating system.
Some common operating systems are listed in the table below:

| Operating System | Installation Instructions                                                                                                                                                                                                                                                                 |
|------------------|-------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------|
| FreeDOS          | Load the bonus CD/ISO then run `FDIMPLES`, navigate to the **Development** group and check **WATCOMC** for installation                                                                                                                                                                   |
| SvarDOS          | Run `PKGNET pull ow` or [download manually](http://www.svardos.org/?p=repo&cat=devel) then run `PKG install ow.svp`                                                                                                                                                                       |
| Other            | Download and run the [Open Watcom](https://github.com/open-watcom/open-watcom-v2/releases) installation binary. If choosing a selective installation then make sure the **16-bit compiler** with **Large memory model** and **DOS Target operating system** are selected for installation |

> Note: although Open Watcom can cross compile from Linux/Posix & NT systems
> these instructions will assume Newth is being built by a DOS-like system.

### Installing SvarDOS package manager onto another DOS (optional)

If you already have a packet driver setup on your DOS system,
then it is relatively straight forward to set up the SvarDOS package manager.
`.svp` files are normal `.zip` files, so it should be possible to bootstrap the SvarDOS repository by
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

## Setup Watcom build environment

The Open Watcom build environment needs to be set up before it can be used.
A script is included to do this.
On a typical DOS installation of Open Watcom run the following to enable the build environment.

> Note: the following steps will need to be repeated each time DOS boots.
> If this system is using Open Watcom frequently consider making the following commands
> part of your `AUTOEXEC.BAT`/`FDAUTO.BAT` script

```
%DOSDIR%\DEVEL\OW\OWSETENV.BAT
```

> Tip: `wcc`, `wlink` & `wmake` should now be valid commands
> if this isn't the case check the directories in the script and modify them as necessary.

> Tip: Open Watcom for DOS is built with DOS4GW extender which prints its copyright notice every time a program starts.
> This can get particular nuisance in where the copyright notice will print continuously. 
> Run `SET DOS4G=QUIET` to prevent the DOS4GW copyright notice from being displayed every time a command runs.

## Build Newth

### Build for DOS

#### Configuring Watt32 for linking with Newth

Newth on DOS depends on Watt32 for its networking backend.
The `Watt32s` folder is a Git submodule to this library and can be built in place.
If your development machine doesn't have access to Git, then the Watt32 sources should be extracted on symlinked
into the `Watt32s` folder so that it looks like so.

```
Watt32s\inc
Watt32s\lib
Watcom\Dos16\makefile
Watcom\Dos4g\makefile
```

Newth depends on BSD-like networking API, and compiling for DOS is no exception.
For Newth to link to Watt32 correctly `USE_BSD_API` must be defined when building Watt32 library.
It's optional but a good idea to also define `USE_BOOTP` and/or `USE_DHCP`
so that Newth can configure its IP address automatically.

To do this, apply the [Watt32s.pat](../Watt32s.pat) patch file:

| GNU Patch                                      | DifPat                                       |
|------------------------------------------------|----------------------------------------------|
| `patch ../Watt32s/src/config.h ../Watt32s.pat` | `pat ..\Watt32s.pat ..\Watt32s\src\config.h` |

Alternatively, edit `Watt32s\src\config.h` manually so it reads like:

```c
#undef USE_DEBUG
#undef USE_MULTICAST
#undef USE_BIND
#define USE_BSD_API
#undef USE_BSD_FATAL
#define USE_BOOTP
#define USE_DHCP
#undef USE_RARP
#undef USE_GEOIP
#undef USE_IPV6
#undef USE_LANGUAGE
#undef USE_FRAGMENTS
#undef USE_STATISTICS
#undef USE_STACKWALKER
#undef USE_FSEXT
```

> Caution: some versions of Watt-32 have a broken implementation of DHCP that can cause an infinite loop.
> On a real DOS this means it could very well lock up the computer with no option but to hard reset.
> If not using the Git submodule and in doubt leave `USE_DHCP` undefined.
> Most DHCP servers are backwards compatible with BOOTP.

#### Build for 16-bit (Real Mode)

From the `Dos16` directory run `wmake` to build the project.
Two self-contained 16-bit binaries called `DL.EXE` and `TH.EXE` will be made and can be run from any path
(including a floppy diskette) on any DOS 2.0 or later computer.

#### Build for 32-bit (DOS4GW)

From the `Dos4g` directory run `wmake` to build the project.
Two 32-bit binaries called `DL.EXE` and `TH.EXE` will be made and can be run from any path (including a floppy diskette)
on any DOS 4.0 or later computer with an i386 compatible CPU.

`DOS4GW.EXE` will need to either exist in a `%PATH%` directory
or the same directory as the binaries for the programs to function.
To put a copy of `DOS4GW.EXE` in the same directory as `DL.EXE` and `TH.EXE` run `COPY %WATCOM%\BINW\DOS4GW.EXE .`.

### Build for Windows

#### Configuring Open Watcom for linking to Win32
Newth on Windows depends on a small number of Win32 libraries as well as Winsock 1.1 or later for its networking backend.
Even though Open Watcom can dynamically link to these dependencies 
without any third party libraries, it still needs to be told where to look.
This can be achieved by appending the platform-specific header files to the `INCLUDE` enviroment variable.

| Build Platform | Command                            |
|----------------|------------------------------------|
| POSIX          | `INCLUDE=$WATCOM/h:$WATCOM/h/nt`   |
| DOS/NT         | `INCLUDE=%WATCOM%\h;%WATCOM%\h\nt` |

#### Build for Win32
With the enviroment variable setup: 
from the `WinNT` directory run `wmake` to build the project.
Two 32-bit PE binaries call `DL.EXE` and `TH.EXE` will be made 
that can be run from any path on Windows 95 or later
on any PC with a 80386-compatible processor. 

# After building

## Compress binary (optional)

On DOS machines, disk space is usually at a premium.
Even though the release builds are stripped of all debugging symbols, it is possible to make the binary take
substantially less disk space with UPX compression so that it fits comfortably on a smaller diskette standard.

| Build     | UPX Command                       | Fits on            |
|-----------|-----------------------------------|--------------------|
| Real Mode | `UPX DL.EXE TH.EXE --best --8086` | 5¼-inch DD (360k)  |
| DOS4GW    | `UPX DL.EXE --best`               | 5¼-inch QD (720k)  |
| DOS4GW    | `UPX DL.EXE TH.EXE --best`        | 5¼-inch HD (1200k) |

> Note: While Windows 95 binaries can also be compressed with UPX, 
> they tend to be so small in their uncompressed state that it is not deemed necessary.

## Create diskette image (optional)

On a real DOS machines it makes sense to directly copy the new binaries onto a newly formatted diskette.
Conversely, when cross-compiling or distributing over the Internet,
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

#### On Compressed Binaries

##### Real Mode 5¼-inch DD Diskette

```bash
mformat -C -i newth360.ima -v "NEWTH" -f 360
mcopy -i newth360.ima DL.EXE TH.EXE ::
```

##### DOS4GW 5¼-inch QD Diskettes

```bash
mformat -C -i dl4g_720.ima -v "DL4GW" -f 720
mcopy -i dl4g_720.ima DL.EXE DOS4GW.EXE ::
```

```bash
mformat -C -i th4g_720.ima -v "TH4GW" -f 720
mcopy -i th4g_720.ima TH.EXE DOS4GW.EXE ::
```

##### Multi-arch 3½-inch HD Diskettes

```bash
mformat -C -i dlma_1.4.ima -v "DLMULTI" -f 1440
mmd -i dlma_1.4.ima ::\16 \4G
mcopy -i dlma_1.4.ima Dos16/DL.EXE ::\16
mcopy -i dlma_1.4.ima Dos4g/DL.EXE Dos4g/DOS4GW.EXE ::\4G
```

```bash
mformat -C -i thma_1.4.ima -v "THMULTI" -f 1440
mmd -i thma_1.4.ima ::\16 \4G
mcopy -i thma_1.4.ima Dos16/TH.EXE ::\16
mcopy -i thma_1.4.ima Dos4g/TH.EXE Dos4g/DOS4GW.EXE ::\4G
```

##### Multi-arch 3½-inch ED Diskette

```bash
mformat -C -i newth2.8.ima -v "NEWTH MA" -f 2880
mmd -i newth2.8.ima ::\16 \4G
mcopy -i newth2.8.ima Dos16/DL.EXE Dos16/TH.EXE ::\16
mcopy -i newth2.8.ima Dos4g/DL.EXE Dos4g/TH.EXE Dos4g/DOS4GW.EXE ::\4G
```

#### On Uncompressed Binaries

##### Real Mode 5¼-inch QD Diskettes

```bash
mformat -C -i dl_720.ima -v "DL" -f 720
mcopy -i dl_720.ima DL.EXE ::
```

```bash
mformat -C -i th_720.ima -v "TH" -f 720
mcopy -i th_720.ima TH.EXE ::
```

##### DOS4GW 3½-inch HD Diskettes

```bash
mformat -C -i dl4g_1.4.ima -v "DL4GW" -f 1440
mcopy -i dl4g_1.4.ima DL.EXE DOS4GW.EXE ::
```

```bash
mformat -C -i th4g_1.4.ima -v "TH4GW" -f 1440
mcopy -i th4g_1.4.ima TH.EXE DOS4GW.EXE ::
```

##### DOS4GW 3½-inch ED Diskette

```bash
mformat -C -i new4g2.8.ima -v "NEWTH 4G" -f 2880
mcopy -i new4g2.8.ima DL.EXE TH.EXE DOS4GW.EXE ::
```