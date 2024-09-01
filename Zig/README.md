<!-- TOC -->
* [What is Zig](#what-is-zig)
* [Setup Zig](#setup-zig)
  * [Requirements](#requirements)
  * [Installing Zig](#installing-zig)
* [Building Newth](#building-newth)
  * [Configure build system](#configure-build-system)
    * [Zig](#zig)
      * [Optimize](#optimize)
      * [Target](#target)
    * [Newth Options](#newth-options)
  * [Build Newth](#build-newth)
<!-- TOC -->

# What is Zig

Zig is a low level programming language with a focus on memory safety and low friction cross-compilation and is its own
build system also written in Zig and making use of it its "comptime" feature. 
Because Zigs toolchain also contains C compiler it can be used to build C projects.

# Setup Zig

## Requirements

To build Newth with Zig you will need the following:

- Zig v0.13

> Tip: Zig v0.13 is considered an unstable API meaning newer versions may break backwards compatibility

## Installing Zig

Installation instructions for Zig differ depending on the build machines operating system.
Some common operating systems are listed [here](https://github.com/ziglang/zig/wiki/Install-Zig-from-a-Package-Manager).
If the required operating system or package manager is not listed visit https://ziglang.org/download/
and look for a manual build.

# Building Newth

## Configure build system

### Zig

#### Optimize

Zig has several optimization levels. They are as follows:

| Optimization Level        | Description                                                                                  |
|---------------------------|-----------------------------------------------------------------------------------------------|
| `-Doptimize=Debug`        | Default. Gives as much debugging information as possible at the cost of binary size and speed |
| `-Doptimize=ReleaseSafe`  | A release build with runtime safety checks build in at the cost of binary size and speed      |
| `-Doptimize=ReleaseFast`  | A release build with no runtime safety checks and a emphasis on execution speed               |
| `-Doptimize=ReleaseSmall` | A release build with no runtime safety checks and a emphasis on smaller binary sizes          |

#### Target

As mentioned [earlier](#what-is-zig) Zig is a cross-compiler.
This means it's possible to build Newth for one system on another.
While this in not unique to Zig it's unique in its easy of setup and use.

When targeting a system other than native Zig needs to know which system that is with a triplet, for example:

| Triplet                       | Description                    |
|-------------------------------|--------------------------------|
| `-Dtarget=x86-linux-gnu`      | A 32-bit GNU/Linux PC          |
| `-Dtarget=x86_64-linux-gnu`   | A typical 64-bit GNU/Linux PC  |
| `-Dtarget=x86_64-windows-gnu` | A typical 64-bit Windows 10 PC |
| `-Dtarget=riscv64-linux-musl` | RISC-V 64-bit MUSL/Linux       |

> Tip: run `zig targets` to see all possible combinations

### Newth Options

Newth has several configuration options that change features and functionality of the built binary. They are as follows:

| Option            | Description                                                           |
|-------------------|-----------------------------------------------------------------------|
| `-Dpoll=true`     | Use `poll()` instead of `select()` in `th`                            |
| `-Dreadline=true` | Enable GNU Readline support in `dl`                                   |
| `-Dwsock1=true`   | Enable WinSock 1.1 support for a truly portable Windows 32-bit binary |

## Build Newth

Run `zig build` in the directory containing `build.zig` to build the project.
Binaries called `th` and `dl` will be made in the `zig-out` subdirectory
followed by another directory with the target triplets name.
Don't forget to append any desired [build options](#configure-build-system) to the command.

### Examples
```
zig build 
```