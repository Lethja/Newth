<!-- TOC -->
* [What is Code::Blocks](#what-is-codeblocks)
* [Setup Code::Blocks](#setup-codeblocks)
  * [Requirements](#requirements)
  * [Installing Code::Blocks](#installing-codeblocks)
    * [Linux](#linux)
      * [System Repository](#system-repository)
      * [Flatpak](#flatpak)
    * [ReactOS, Windows & Wine](#reactos-windows--wine)
* [Building Newth](#building-newth)
  * [Create symbolic links](#create-symbolic-links)
    * [Long names](#long-names)
      * [Posix](#posix)
      * [Windows](#windows)
  * [Build Newth](#build-newth)
<!-- TOC -->

# What is Code::Blocks

[Code::Blocks](http://www.codeblocks.org/) is a [GPLv3](https://www.gnu.org/licenses/gpl-3.0.en.html) licenced
Integrated Development Environment (IDE) for Linux, macOS, and Windows.

> Tip: while Code::Blocks is multi-platform `Newth.cpb` was created with older versions of Windows in mind.
> BSD, Linux & macOS builders should build with [Autotools](../Autotool/README.md)
> or [CMake](../CMake/README.md) as they are more elegant building solutions for those operating systems.

# Setup Code::Blocks

## Requirements

To build Newth with Code::Blocks, you will need the following:

- Code::Blocks 16.01 or later
- A C89 compliant compiler

> Tip: Code::Blocks Windows installers that end with `mingw-setup.exe` include the
> [TDM-GCC compiler](https://jmeubank.github.io/tdm-gcc/) which is an adequate compiler for building Newth

## Installing Code::Blocks

### Linux

#### System Repository

| Distribution                     | Repository       | File Extension | Install Command                                                   |
|----------------------------------|------------------|----------------|-------------------------------------------------------------------|
| Debian<br/>Linux Mint<br/>Ubuntu | Apt<br/>Aptitude | `.deb`         | `apt install codeblocks gcc`<br/>`apt-get install codeblocks gcc` |
| Funtoo<br/>Gentoo<br/>Sabayon    | Portage          | `.ebuild`      | `emerge -u dev-util/codeblocks`                                   |
| Arch Linux<br/>Artix Linux       | Pacman           | `.pkg.tar.xz`  | `pacman -S codeblocks gcc`                                        |
| Fedora<br/>OpenSUSE<br/>RHEL     | Dnf<br/>Yum      | `.rpm`         | `dnf install codeblocks gcc`<br/>`yum install codeblocks gcc`     |

#### Flatpak

Code::Blocks is available on [flathub.org](https://flathub.org) and can be installed with the following commands

```bash
flatpak remote-add --if-not-exists flathub https://flathub.org/repo/flathub.flatpakrepo
flatpak install flathub org.codeblocks.codeblocks
```

### ReactOS, Windows & Wine

1. Visit the [Code::Blocks website](http://www.codeblocks.org/) and download an installer
2. Run the Code::Blocks installer
3. Open Code::Blocks and select your default compiler if asked (included GCC is recommended)

> Tip: after the Code::Blocks installation is complete `.cpb` and `.workspace` files should be associated with Code::
> Blocks

# Building Newth

## Create symbolic links

### Long names

Code::Blocks `.workspace` file extension has more characters than can be stored on a DOS file system which is limited to
a maximum of eight characters and an additional three characters for an extension.
These files have been given a shorter name.
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

1. Open `Newth.workspace` with **Code::Blocks**
2. From the **Management** sidebar, select the **Projects** tab
3. Expand the workspace tree item if it isn't already. The activated project will appear in bold text.
    1. To change the active another project, right-click the other project and select **Activate Project**
4. Select **Build** from the menu bar then point to **Select Target** then check the profile you want to build with
    1. If in doubt, select the **GCC Release** or **MingW32 Release** profile
5. Select **Build** from the menu bar then select **Build**
6. The binaries of each project will be located in `bin` folder relative to `Newth.ws`