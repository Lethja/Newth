# Newth HTTP tools

Newth is a simple set of HTTP tools intended to share files and folders across a local area network temporarily.
The tools can be built for and run on almost every personal computer ever made.

## Tools

### DL

`dl` will be a HTTP 1.1 file downloading client. At the time of writing it is still under development.

### TH

`th` is a HTTP 1.1 multiplexing file server, multiple clients can connect to an instance and be served at the same time.
Clients only need a HTTP 1.1 compliant web browser to download files from `th` which is included in most operating
systems with a TCP/IP stack.

## Project Structure

### Short names

Care has been taken to make sure all folder and file names in this repository are compatible with
the [8.3 file name limitation](https://en.wikipedia.org/wiki/8.3_filename) present in some vintage operating systems.
Although this allows the source code to be stored on such systems some more modern tools are unfortunately hardcoded
to look for longer file names. When this is the case instructions on how to rename or symlink the effected files
have been given the build systems documentation.

### Build Systems

Newth supports being built for several different types of operating systems and as such there are several build systems.
Some build systems are more ideal then others depending on the systems environment.
Each build environment gets its own folder and references the same source code with minimal changes.

| Folder                         | Build System                  | Intended Build Target              |
|--------------------------------|-------------------------------|------------------------------------|
| [Autotool](Autotool/README.md) | GNU Autotools                 | POSIX.1-2001 compliant UNIX system |
| [CMake](CMake/README.md)       | CMake                         | POSIX.1-2001 compliant system      |
| [CodeBlks](CodeBlks/README.md) | Code::Blocks                  | Linux, MacOS, Windows NT 5+        |
| [DJGPP](DJGPP/README.md)       | DJGPP Makefile                | DOS 4 (80386)                      |
| [VC6](VC6/README.md)           | Microsoft Visual Studio C++ 6 | Windows 95 (80386)                 |
| [Watcom](Watcom/README.md)     | Open Watcom 1.9               | DOS 2 (8086)                       |
| [Zig](Zig/README.md)           | Zig                           | Cross Compiler                     |
