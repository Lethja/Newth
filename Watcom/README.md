<!-- TOC -->
* [What is Watcom](#what-is-watcom)
* [Setup Autotools](#setup-autotools)
  * [Requirements](#requirements)
  * [Installing Watcom](#installing-watcom)
* [Building Newth](#building-newth)
  * [Setup the build environment](#setup-the-build-environment)
  * [Adding Watt32 includes](#adding-watt32-includes)
  * [Build Newth](#build-newth)
<!-- TOC -->

# What is Watcom
Watcom C/C++ was an integrated development environment (IDE) product 
from Watcom International Corporation for C, C++, and Fortran programming languages.
Watcom C/C++ was a commercial product until it was discontinued, 
it has since been released under the Sybase Open Watcom Public License as Open Watcom C/C++.

# Setup Autotools

## Requirements
To build Newth with Watcom you will need the following:
- A i386 machine or softcore
- A i386 compatible DOS operating system or emulator
- Open Watcom C 1.9
- [Watt32 library](https://watt-32.net/) compiled with the same version of Watcom

> Tip: Don't have a real DOS machine? Try [Dosbox-X](https://dosbox-x.com/)

## Installing Watcom
Installation instructions for Watcom differ depending on the build machines operating system.
Some common operating systems are listed in the table below:

| Operating System | Installation Instructions                                                                                               |
|------------------|-------------------------------------------------------------------------------------------------------------------------|
| FreeDOS          | Load the bonus CD/ISO then run `FDIMPLES`, navigate to the **Development** group and check **WATCOMC** for installation |
| SvarDOS          | Run `PKGNET pull ow` or [download manually](http://www.svardos.org/?p=repo&cat=devel) then run `PKG install ow.svp`     |

> Tip: `.SVP` files are typical zip files. 
> If the build system is a DOS emulator or other type of DOS system it should be possible to bootstrap
> the SvarDOS repository by extracting `PKG.EXE` and moving it and `PKGNET.SVP` into the DOS environment.
> 
> Make sure the network packet driver is loaded and both `%DOSDIR%` & `%PATH%` are set correctly then run the following:
> ```
> PKG.EXE install PKGNET.SVP
> PKGNET pull pkg
> PKG.EXE install PKG.SVP
> ```
> Optionally, remove the extracted `PKG.EXE` and `.SVP` files to free disk space:
> ```
> DEL PKG.EXE
> DEL *.SVP
> ```

# Building Newth

## Setup the build environment
The Watcom build environment needs to be setup before it can be used. A script is included to do this.
On a typical DOS installation of Open Watcom run the following to enable the build environment.
```
%DOSDIR%\DEVEL\OW\OWSETENV.BAT
```
> Tip: `wcc`, `wlink` & `wmake` should now be valid commands
if this isn't the case check the directories in the script and modify them as necessary.

## Adding Watt32 includes
Watcom needs to see Watt32s headers as if they're from the system path when compiling Newth.
To achieve this append the full path to Watt32s include directory to the `INCLUDE` environment variable like so
```
set INCLUDE=%INCLUDE%;C:\NEWTH\EXT\WATT32S\INC
```

## Build Newth
Run `wmake -f TH.MAK` to build the project.
A self contained 16-bit EXE binary called `TH.EXE` will be made
and can be run from any path including a floppy diskette.