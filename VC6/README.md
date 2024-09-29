<!-- TOC -->
* [What is Microsoft Visual Studio C++ 6.0](#what-is-microsoft-visual-studio-c-60)
* [Setup Microsoft Visual Studio C++ 6.0](#setup-microsoft-visual-studio-c-60)
  * [Requirements](#requirements)
  * [Installing Microsoft Visual Studio C++ 6.0](#installing-microsoft-visual-studio-c-60)
  * [Build Newth](#build-newth)
<!-- TOC -->

# What is Microsoft Visual Studio C++ 6.0

[Microsoft Visual Studio C++ 6.0](https://en.wikipedia.org/wiki/Visual_Studio#6.0_(1998)) was an 
Integrated Development Environment (IDE)
from Microsoft used for developing software on and for Windows 9x and 32-bit NT systems. 
Newth supports being built with Microsoft Visual Studio C++ 6.0 on Windows.

Microsoft Visual Studio C++ 6.0 enables the creation of highly portable 32-bit,
Win32 Newth binaries for Microsoft Windows systems.
These builds do not rely on a C runtime or any dynamic link libraries except those
included in Windows 95 and later versions.
The graphical interface binaries support visual styling and all builds provide support for Windows Socket v1,
v2, and IPV6.
Support is determined at runtime, which allows the same binary to run on Windows 95 yet enables features for 
newer versions that have support. 

> Note: Microsoft Visual Studio C++ 6.0 is currently the only way 
> to build Newth binaries compatible with i386 processors which Windows 95 supports

# Setup Microsoft Visual Studio C++ 6.0

## Requirements

To build Newth with Microsoft Visual Studio C++ 6.0, you will need the following:

- Microsoft Windows 95 or later (Microsoft Windows XP SP3 32-bit known to work)
- Microsoft Visual Studio C++ 6.0
> Note: Microsoft Visual Studio 6 was sold in several editions and for several programming languages.
> Make sure you have the C++ version of Microsoft Visual Studio 6 (any edition is compatible)

## Installing Microsoft Visual Studio C++ 6.0

Microsoft Visual Studio C++ 6.0 is no longer sold or supported by Microsoft Corporation
as it is proprietary and discontinued software.
The only way to acquire it is to purchase a used physical copy from a seller.
Ensure that the license key is included with the purchase.
Best of luck.

## Build Newth

1. Open `newth.dsw` in Microsoft Visual Studio C++ 6.0
2. Select **Build** from the menu bar then **Set Active Configuration...**
3. Select the project and configuration you want to build then select **OK**
4. Press the **F7** key to build the project
5. The binaries of each project will be located in `Debug` or `Release` folders respectively relative to `newth.dsw`

> Note: if `.dsw` is not recognized as a file for Microsoft Visual Studio 6.0 by Microsoft Windows,
> start Microsoft Visual Studio C++ 6.0 then select **File** from the menu bar then **Open Workspace**
> and select `newth.dsw` in the **Open Workspace** file dialog