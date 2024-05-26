# Newth Source Code

All the C source code for Newth is contained in this directory.
Aside from some code in the `platform` directory, the source code should be fully ANSI C compatible.

## Code Style

The `astyleo` file contains formatting options to beautify the source code using
[Artistic Style](https://astyle.sourceforge.net/astyle.html).

Run `astyle --options=astyleo *.h,*.c` from this directory before staging your changes
to ensure any changed code is formatted consistently with the rest of the project.

## Directories

| Name       | Description                                                                                                       |
|------------|-------------------------------------------------------------------------------------------------------------------|
| `cli`      | Source code to build portable Command Line Interface (CLI) binaries of Newth                                      |
| `client`   | Source code only useful for downloader binaries                                                                   |
| `common`   | Platform portable source code needed by both the client and server                                                |
| `gui`      | Source code to build Graphical User Interface (GUI) binaries of Newth. Some implementations are platform specific |
| `platform` | Abstraction functions to perform tasks that are not portable between different operating systems                  |
| `server`   | Source code only useful for the server binaries                                                                   |
