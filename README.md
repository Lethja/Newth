# Newth HTTP File Server

Newth is a simple HTTP server intended to share files across a local area network temporarily. It is simple, portable
and light on resources. It can run on almost every personal computer ever made.

## What is Newth

Newth is a multiplexing server, multiple clients can connect to an instance and be served at the same time.
Clients only need a HTTP 1.1 compliant web browser to download files from Newth
which is included in most operating systems with a TCP/IP stack.

### What is it designed to do

Newth is perfect for when users want to temporarily host files or even entire directories over a trusted local network,
simply point it to a local path and all subdirectories are available in a read only mode.
Once the files have been transferred simply close the server like any other program.
If you want to host files in two different directories at the same time simply start another instance of Newth.

### What it isn't designed to do

Newth is not a web server, it is not designed to serve thousands of requests at a time nor is there's any support for
scripting, uploading or hostnames which is all by design.
While forwarding Newth to the open Internet should be relatively safe
it should be done with the upmost caution like any other server exposed to the Internet.

## Project Structure

### Short names

Great care has been taken to make sure all folder and file names in this repository comply with
the [8.3 name limitation](https://en.wikipedia.org/wiki/8.3_filename).
This mean the repository can be on such file systems without issue. 
Unfortunately some tools are hardcoded to look for names that don't comply with this specification. When this is the
case instructions on how to rename or symlink the effects files have been given the build systems instructions.

### Build Systems

Newth supports being built for several different types of operating systems as such there are several build systems of
which many are ideal for their own environment.
Each build environment gets its own folder and references the same source code with minimal changes.

| Folder                         | Build System                  | Intended Build Target       |
|--------------------------------|-------------------------------|-----------------------------|
| [Autotool](Autotool/README.md) | GNU Autotools                 | POSIX.1-2001 compliant UNIX |
| [CMake](CMake/README.md)       | CMake                         | POSIX.1-2001 compliant UNIX |
| [CodeBlks](CodeBlks/README.md) | Code::Blocks                  | Linux, MacOS, Windows NT 5+ |
| [DJGPP](DJGPP/README.md)       | DJGPP Makefile                | DOS 4 (80386)               |
| [VC6](VC6/README.md)           | Microsoft Visual Studio C++ 6 | Windows 95 (80386)          |
| [Watcom](Watcom/README.md)     | Open Watcom 1.9               | IBM 5150 (8086)             |

### Code layout

Newths codebase is split into 3 folders

| Folder | Description                                                                                                     |
|--------|-----------------------------------------------------------------------------------------------------------------|
| ext    | Where build systems should look for any local yet external headers/libraries they might require (usually empty) |
| src    | The Newth source code itself                                                                                    |
| test   | CMocka unit testing modules for automatically testing Newths source code                                        |
