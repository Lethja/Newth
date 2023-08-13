<!-- TOC -->
* [What is Code::Blocks](#what-is-codeblocks)
* [Setup Code::Blocks](#setup-codeblocks)
  * [Requirements](#requirements)
  * [Installing Code::Blocks](#installing-codeblocks)
* [Building Newth](#building-newth)
  * [Create symbolic links](#create-symbolic-links)
    * [Long names](#long-names)
      * [Posix](#posix)
      * [Windows](#windows)
  * [Build Newth](#build-newth)
<!-- TOC -->

# What is Code::Blocks

[Code::Blocks](http://www.codeblocks.org/) is a [GPLv3](https://www.gnu.org/licenses/gpl-3.0.en.html) licenced
Integrated Development Environment (IDE) for Linux, macOS and Windows.
Newth supports being built within the Code::Blocks IDE on Windows.

> Tip: while Code::Blocks is multi-platform `Newth.cpb` was created with older versions of Windows in mind.
> BSD, Linux & macOS builders should build with [Autotools](../Autotool/README.md)
> or [CMake](../CMake/README.md) as they are more elegant building solutions for those operating systems.

# Setup Code::Blocks

## Requirements

To build Newth with Code::Blocks, you will need the following:

- Code::Blocks 16.01 or later
- A C89 compliant compiler

> Tip: Code::Blocks Windows installers that end with `mingw-setup.exe` include the 
> [TDM-GCC compiler](https://jmeubank.github.io/tdm-gcc/) which is a more than adequate compiler for building Newth

## Installing Code::Blocks

1. Visit the [Code::Blocks website](http://www.codeblocks.org/) and download an installer
2. Run the Code::Blocks installer
3. Open Code::Blocks and select your default compiler if asked (included GCC is recommended)

> Tip: after the Code::Blocks installation is complete `.cpb` and `.workspace` files should be associated with Code::Blocks

# Building Newth

## Create symbolic links

### Long names
Code::Blocks `.workspace` file extension has more characters then can be stored on a DOS file system which is limited to
a maximum of 8 characters and an additional 3 characters for an extension. These files have been given a shorter name.
To fix this: a symbolic link file with the correct name `Newth.workspace` can be made pointing to `Newth.ws`

Open a terminal/command prompt in the directory containing the `Newth.ws` and run the following:
#### Posix
```
ln -s Newth.ws Newth.workspace
```
#### Windows
```
MKLINK Newth.workspace Newth.ws 
```

## Build Newth

1. Open `Newth.workspace` in Code::Blocks
2. From the **Management** sidebar select the **Projects** tab
3. Expand the workspace tree item if it isn't already. The activated project will appear in bold text. To change the active another project, right click the other project and select **Activate Project** 
4. Select **Build** from the menu bar then point to **Select Target** then check the profile you want to build with
> If in doubt, select the **GCC Release** or **MingW32 Release** profile 
5. Select **Build** from the menu bar then select **Build**
6. The binaries of each project will be located in `bin` folder relative to `Newth.cpb`