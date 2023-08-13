<!-- TOC -->
* [What is Microsoft Visual Studio C++ 6.0](#what-is-microsoft-visual-studio-c-60)
* [Setup Microsoft Visual Studio C++ 6.0](#setup-microsoft-visual-studio-c-60)
  * [Requirements](#requirements)
  * [Installing Microsoft Visual Studio C++ 6.0](#installing-microsoft-visual-studio-c-60)
  * [Build Newth](#build-newth)
<!-- TOC -->

# What is Microsoft Visual Studio C++ 6.0

[Microsoft Visual Studio C++ 6.0](https://en.wikipedia.org/wiki/Visual_Studio#6.0_(1998)) was a 
Integrated Development Environment (IDE) from Microsoft for developing programs on Windows 9x and 32-bit NT systems. 
Newth supports being built with Microsoft Visual Studio C++ 6.0 on Windows.

Microsoft Visual Studio C++ 6.0 creates the ultimate portable version of Newth binaries for Microsoft Windows systems 
that do not link to a C runtime or any other dynamic link libraries files not already included in Windows 95 and later.
The graphical interface binaries support visual styling and have Windows Socket v1, v2 & IPV6 support at runtime and;
is unfortunately the only way build Newth to be compatible with 80386 processors (which Windows 95 supports) at this time.

# Setup Microsoft Visual Studio C++ 6.0

## Requirements

To build Newth with Microsoft Visual Studio C++ 6.0, you will need the following:

- Microsoft Windows 95 or later (Microsoft Windows XP SP3 32-bit known to work)
- Microsoft Visual Studio C++ 6.0
> Note: Microsoft Visual Studio 6 was sold in several editions and for several programming languages.
> Make sure you have the C++ version of Microsoft Visual Studio 6 (any edition is compatible)

## Installing Microsoft Visual Studio C++ 6.0

Microsoft Visual Studio C++ 6.0 is closed and proprietary software that is no longer sold or supported by the
Microsoft Corporation. Buying a used physical copy from a seller is the only means of acquiring the software.
Be sure that the licence key is included before purchasing from any seller. Good luck.

## Build Newth

1. Open `newth.dsw` in Microsoft Visual Studio C++ 6.0
> Note: if `.dsw` is not recognised as a file for Microsoft Visual Studio 6.0 by Microsoft Windows, start Microsoft Visual Studio C++ 6.0 then select **File** from the menu bar then **Open Workspace** and select `newth.dsw` in the **Open Workspace** file dialog
2. Select **Build** from the menu bar then **Set Active Configuration...**
3. Select the project and configuration you want to build then select **OK**
> If in doubt choose the *"Release"* version of the project you want to build
4. Press the **F7** key to build the project
5. The binaries of each project will be located in `Debug` or `Release` folders respectively relative to `newth.dsw`