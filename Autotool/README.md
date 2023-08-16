<!-- TOC -->
* [What is Autotools](#what-is-autotools)
* [Setup Autotools](#setup-autotools)
  * [Requirements](#requirements)
  * [Installing Autotools](#installing-autotools)
* [Building Newth](#building-newth)
  * [Create symbolic links](#create-symbolic-links)
    * [Long names](#long-names)
      * [Posix](#posix)
      * [Windows](#windows)
    * [Source Folders](#source-folders)
      * [Posix](#posix-1)
      * [Windows](#windows-1)
  * [Setup the build system](#setup-the-build-system)
  * [Configure build system](#configure-build-system)
  * [Build Newth](#build-newth)
  * [Running the test (optional)](#running-the-test-optional)
<!-- TOC -->

# What is Autotools

GNU Autotools is a combination of
[autoconf](https://www.gnu.org/software/autoconf/), [automake](https://www.gnu.org/software/automake/)
& [libtool](https://www.gnu.org/software/libtool/) that when used together form a fully functional build system.
The advantage Autotools has over other build systems such as [CMake](https://cmake.org/) is the level of transparency
and flexibility from its POSIX shell script output.
It is worth noting that each component that makes up Autoconf can be used independent of one another.
Newth can be built using AutoTools on most POSIX compliant systems.

# Setup Autotools

## Requirements

To build Newth with Autotools you will need the following:

- Autoconf 2.71 or newer installed on your system
- Automake installed on your system
- A C89 compliant compiler

## Installing Autotools

Installation instructions for Autotools differ depending on the build machines operating system.
Some common operating systems are listed in the table below:

| Operating System                                                | Installation Instructions                                                                                                |
|-----------------------------------------------------------------|--------------------------------------------------------------------------------------------------------------------------|
| Arch Linux (and other `.pkg.tar.xz` based distributions)        | Run `pacman -Syu autoconf autoconf-archive automake libtool`                                                             |
| Debian (and other `.deb` based distributions)                   | Run `apt-get install autoconf autoconf-archive automake libtool`                                                         |
| MacOS                                                           | Install [Homebrew](https://brew.sh/) then run `brew install autoconf autoconf-archive automake libtool`                  |
| Gentoo Linux (and other `.ebuild` based distributions)          | Run `emerge -au autoconf autoconf-archive automake libtool`                                                              |
| Red Hat Enterprise Linux (and other `.rpm` based distributions) | Run `yum install autoconf autoconf-archive automake libtool` or `dnf install autoconf autoconf-archive automake libtool` |

> Tip: Windows can technically run Autotools and the generated configuration script with the bash shell included in
> [Cygwin](https://www.cygwin.com/), [Git for Windows](https://gitforwindows.org/) or [MSYS2](https://www.msys2.org/)
> however the results are not worth the effort for standalone Windows application development.
> Windows builders should use [Visual Studio C++ 6](../VC6/README.md) or [Code::Blocks](../CodeBlks/README.md) to build instead

> Tip: Homebrew is also available for x86_64 Linux as well as ARM and x86_64 macOS support

# Building Newth

## Create symbolic links

### Long names
Autotools depends on some file names having more characters then can be stored on a DOS file system which is limited to
a maximum of 8 characters and an additional 3 characters for an extension. These files have been given a shorter name
to be store-able on these older systems but are useless without the correct name.
To fix this: a symbolic link file with the correct name `configure.ac` can be made pointing to `autoconf.ac`

Open a terminal/command prompt in the directory containing the `autoconf.ac` and `Makefile.am` files and run the following:
#### Posix
```
ln -s autoconf.ac configure.ac
```
#### Windows
```
MKLINK configure.ac autoconf.ac 
```

### Source Folders
Autotools expects the source code to be in subdirectories relative to itself but the Newth folder structure is
setup to support several build systems in different subdirectories away from the source code instead. 
To fix this: the `src` and `test` directories need to be symbolically linked into the `Autotools` folder.  

Open a terminal/command prompt in the directory containing the `autoconf.ac` and `Makefile.am` files and run the following:
#### Posix
```
ln -s ../src & ln -s ../test
```
#### Windows
```
MKLINK /d src ..\src & MKLINK /d test ..\test 
```

## Setup the build system

Open a terminal/command prompt in the directory containing the `autoconf.ac` and `Makefile.am` files and run the following:
```
autoreconf -fi
```
This should generate the `configure` and `makefile` among other files required for the build system

## Configure build system

Run the configuration with the `./configure` script. the configuration script has several options specific to Newth:

| Parameter         | Comment                                                                                                                                                                                        |
|-------------------|------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------|
| `--enable-iiface` | Manually bind a socket to every network adapter at the application level. This option should only be enabled when targeting operating systems that can't do this internally; possible but rare |

> Tip: Autoconf configurations are persistent (even after shutdowns).
> You can run `./config.status --config` to see the current setup configuration
> and `./configure --help` to see all configuration options.
> To reconfigure, run `./configure` with all the desired parameters and environment variables again

## Build Newth

Run `make` to build the project.
Binaries called `th` and `dl` will be made

Run `make install` to install the built binaries into your operating system (optional)

Run `make dist` or `make distcheck` to put the project into a tarball archive ready for distribution (optional)
> Tip: The tarball made from `make dist` will not require Autotools to be installed on systems that extract it.
> The instructions to build and install Newth are simplified to the following when building from a `dist` tarball:
> ```
> tar -xf newth-*.tar.gz
> cd newth-*/
> ./configure
> make
> make install
> ```

## Running the test (optional)

If CMocka was detected in the configure stage `make check` will to build and run unit tests. 
The binary will be called `unittest`

The test executable will run a bunch of tests with functions throughout the project
and compare results to known good values. All tests should pass.

Example usage: `make check` to build and run or `./unittest` to run after being built

> The tests contain mocking functions that replace system calls.
This mocking functionality might not work correctly on some systems. If this is the case the mockTest group will not
pass and no further tests will run.