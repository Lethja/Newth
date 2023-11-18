<!-- TOC -->
* [What is CMake](#what-is-cmake)
* [Setup CMake](#setup-cmake)
  * [Requirements](#requirements)
  * [Installing CMake](#installing-cmake)
* [Building Newth](#building-newth)
  * [Create symbolic links](#create-symbolic-links)
    * [Long names](#long-names)
      * [Posix](#posix)
      * [Windows](#windows)
  * [Generate build system](#generate-build-system)
  * [Build Newth](#build-newth)
  * [Running the test (optional)](#running-the-test-optional)
<!-- TOC -->

# What is CMake

[CMake](https://cmake.org/) is a multi-platform build system that can generate code for other build systems.
CMake has a some key features over other build systems like [GNU Autoconf](https://www.gnu.org/software/autoconf/)
such as native Windows support, an optional yet official graphical configuration tools, integration with
many popular IDEs and a relatively easy to write scripting language.

Newth can be built using CMake on many modern systems.

# Setup CMake

## Requirements

To build Newth with CMake you will need the following:

- CMake 3.22 or newer installed on your system
- A C89 compliant compiler

> Tip: If you're not sure what compiler to use, [Clang](https://clang.llvm.org/) and [GCC](https://gcc.gnu.org/) are
> both highly recommended and freely available C/C++ compilers.

## Installing CMake

Installation instructions for CMake differ depending on the build machines operating system.
Some common operating systems are listed in the table below.

| Operating System                                                | Installation Instructions                      |
|-----------------------------------------------------------------|------------------------------------------------|
| Arch Linux (and other `.pkg.tar.xz` based distributions)        | Run `pacman -Syu cmake`                        |
| Debian (and other `.deb` based distributions)                   | Run `apt-get install cmake`                    |
| Gentoo Linux (and other `.ebuild` based distributions)          | Run `emerge -au cmake`                         |
| MacOS                                                           | [Visit CMake website](https://cmake.org/)      |
| Red Hat Enterprise Linux (and other `.rpm` based distributions) | Run `yum install cmake` or `dnf install cmake` |
| Windows                                                         | [Visit CMake website](https://cmake.org/)      |

# Building Newth

## Create symbolic links

### Long names

CMake depends on some file names having more characters then can be stored on a DOS file system which is limited to
a maximum of 8 characters and an additional 3 characters for an extension. These files have been given a shorter name
to be store-able on these older systems but are useless without the correct name.
To fix this: a symbolic link file with the correct name `CMakeLists.txt` can be made pointing to `CML.txt`

Open a terminal/command prompt in the directory containing the `CML.txt` and run the following:

#### Posix

```
ln -s CML.txt CMakeLists.txt
```

#### Windows

```
MKLINK CMakeLists.txt CML.txt 
```

## Generate build system

Before compiling the code itself CMake needs to generate the build system target
from instructions in the `CMakeLists.txt` file

Configure to build the library: run `cmake .`. You can append the following parameters to change build configuration:

| Parameter                        | Default Value | Comment                                                                                                                                                              |
|----------------------------------|---------------|----------------------------------------------------------------------------------------------------------------------------------------------------------------------|
| `-DENABLE_POLL=ON`               | `OFF`         | Use `poll()` network API instead of `select()`                                                                                                                       |
| `-DENABLE_WS1=OFF`               | `ON`          | Windows only - Enable Windows Socket v1 support (Windows Socket v2 will always be preferred when available)                                                          |
| `-DENABLE_W32_VISUAL_STYLES=OFF` | `ON`          | Windows only - Enable visual styling in the graphic user interface version of the program. <br/>Disable this if your build environment doesn't work with `.rc` files |

> Tip: it is possible to force CMake to generate a specific build system using the `-G` flag.
> For more information run `cmake --help`

## Build Newth

The command to compile and link the project into a library differs
depending on what build system CMake has generated for.

| Generated      | Build Action |
|----------------|--------------|
| Graphical IDEs | Open project |
| Ninja          | Run `ninja`  |
| Unix Makefile  | Run `make`   |

Binaries called `th` and `dl` will be made

> Tip: if in doubt, try run `cmake --build .` to have CMake run the build process for you

## Running the test (optional)

The test executable will run a bunch of tests with functions throughout the project
and compare results to known good values. All tests should pass.

Example usage: `./test`

> The tests contain mocking functions that replace system calls.
> This mocking functionality might not work correctly on some systems. If this is the case the mockTest group will not
> pass and no further tests will run.