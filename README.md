# Newth HTTP tools

Newth is a straightforward set of HTTP tools
designed for temporarily sharing files and folders across a local area network.
These tools are highly versatile and can be built and run on nearly any personal computer ever made.

## Tools

### DL

HTTP 1.1 file downloading client. At the time of writing, it is still under development.

### TH

HTTP 1.1 multiplexing file server, multiple clients can connect to an instance and be served at the same time.
Clients only need to be HTTP 1.1 compliant to download files from `th` which is included in most operating
systems with a TCP/IP stack or [DL](#dl) can be used.

## Project Structure

### Short names

All folder and file names in this repository are designed 
to comply with the [8.3 file name limitation](https://en.wikipedia.org/wiki/8.3_filename)
for compatibility with vintage operating systems.
This ensures the source code can be stored on these systems.
However, some modern tools may require longer file names.
In such cases, instructions for renaming or creating a symlink 
for the affected files are provided in the build system documentation.

### Build Systems

Newth can be built for multiple operating systems, each with its own build system.
Depending on the system environment, certain build systems may be more suitable than others.
Each build environment has its own folder and references the same source code with minimal modifications.

| Folder                         | Build System                  | Intended Build Target               |
|--------------------------------|-------------------------------|-------------------------------------|
| [Autotool](Autotool/README.md) | GNU Autotools                 | POSIX.1-2001 compliant UNIX system  |
| [CMake](CMake/README.md)       | CMake                         | POSIX.1-2001 compliant system       |
| [CodeBlks](CodeBlks/README.md) | Code::Blocks                  | Linux, macOS, Windows NT 5+         |
| [DJGPP](DJGPP/README.md)       | DJGPP Makefile                | DOS 4 (80386)                       |
| [VC6](VC6/README.md)           | Microsoft Visual Studio C++ 6 | Windows 95 (80386)                  |
| [Watcom](Watcom/README.md)     | Open Watcom 1.9               | DOS 2 (8086)                        |
| [Zig](Zig/README.md)           | Zig                           | Anything `zig cc` can cross-compile |

# See also

Retro computer enthusiasts might be interested in [FujiNet](https://fujinet.online/) 
for hardware-accelerated networking solutions for several retro computer systems.

Projects that do not require supporting older operating systems 
may find [libmicrohttpd](https://www.gnu.org/software/libmicrohttpd/) to be a useful HTTP server library.
