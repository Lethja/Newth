# Microsoft Developer Studio Project File - Name="th gui" - Package Owner=<4>
# Microsoft Developer Studio Generated Build File, Format Version 6.00
# ** DO NOT EDIT **

# TARGTYPE "Win32 (x86) Application" 0x0101

CFG=th gui - Win32 Debug
!MESSAGE This is not a valid makefile. To build this project using NMAKE,
!MESSAGE use the Export Makefile command and run
!MESSAGE 
!MESSAGE NMAKE /f "th gui.mak".
!MESSAGE 
!MESSAGE You can specify a configuration when running NMAKE
!MESSAGE by defining the macro CFG on the command line. For example:
!MESSAGE 
!MESSAGE NMAKE /f "th gui.mak" CFG="th gui - Win32 Debug"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "th gui - Win32 Release" (based on "Win32 (x86) Application")
!MESSAGE "th gui - Win32 Debug" (based on "Win32 (x86) Application")
!MESSAGE 

# Begin Project
# PROP AllowPerConfigDependencies 0
# PROP Scc_ProjName ""
# PROP Scc_LocalPath ""
CPP=cl.exe
MTL=midl.exe
RSC=rc.exe

!IF  "$(CFG)" == "th gui - Win32 Release"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir "Release"
# PROP BASE Intermediate_Dir "Release"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 0
# PROP Output_Dir "Release"
# PROP Intermediate_Dir "Release"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
# ADD BASE CPP /nologo /W3 /GX /O2 /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "_MBCS" /YX /FD /c
# ADD CPP /nologo /W3 /GX /O2 /D "NDEBUG" /D "WIN32" /D "_WINDOWS" /D "_MBCS" /D MSVC89=1 /D ENABLE_WS1=1 /D PLATFORM_NET_ADAPTER=1 /D PLATFORM_NET_LISTEN=1 /FR /YX /FD /c
# ADD BASE MTL /nologo /D "NDEBUG" /mktyplib203 /win32
# ADD MTL /nologo /D "NDEBUG" /mktyplib203 /win32
# ADD BASE RSC /l 0x409 /d "NDEBUG"
# ADD RSC /l 0x409 /d "NDEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:windows /machine:I386
# ADD LINK32 wsock32.lib kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib comctl32.lib /nologo /subsystem:windows /machine:I386

!ELSEIF  "$(CFG)" == "th gui - Win32 Debug"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir "Debug"
# PROP BASE Intermediate_Dir "Debug"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 1
# PROP Output_Dir "Debug"
# PROP Intermediate_Dir "Debug"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
# ADD BASE CPP /nologo /W3 /Gm /GX /ZI /Od /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /D "_MBCS" /YX /FD /GZ /c
# ADD CPP /nologo /W3 /Gm /GX /ZI /Od /D "_DEBUG" /D "WIN32" /D "_WINDOWS" /D "_MBCS" /D MSVC89=1 /D ENABLE_WS1=1 /D PLATFORM_NET_ADAPTER=1 /D PLATFORM_NET_LISTEN=1 /YX /FD /GZ /c
# ADD BASE MTL /nologo /D "_DEBUG" /mktyplib203 /win32
# ADD MTL /nologo /D "_DEBUG" /mktyplib203 /win32
# ADD BASE RSC /l 0x409 /d "_DEBUG"
# ADD RSC /l 0x409 /d "_DEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:windows /debug /machine:I386 /pdbtype:sept
# ADD LINK32 wsock32.lib kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib comctl32.lib /nologo /subsystem:windows /debug /machine:I386 /pdbtype:sept

!ENDIF 

# Begin Target

# Name "th gui - Win32 Release"
# Name "th gui - Win32 Debug"
# Begin Group "Source Files"

# PROP Default_Filter "cpp;c;cxx;rc;def;r;odl;idl;hpj;bat"
# Begin Source File

SOURCE=..\src\server\event.c
# End Source File
# Begin Source File

SOURCE=..\src\common\hex.c
# End Source File
# Begin Source File

SOURCE=..\src\server\http.c
# End Source File
# Begin Source File

SOURCE=..\src\gui\win32\iwindows.c
# End Source File
# Begin Source File

SOURCE=..\src\gui\win32\main.c
# End Source File
# Begin Source File

SOURCE=..\src\platform\mscrtdl.c
# End Source File
# Begin Source File

SOURCE=..\src\platform\platform.c
# End Source File
# Begin Source File

SOURCE=..\src\server\routine.c
# End Source File
# Begin Source File

SOURCE=..\src\server\mltiplex\select.c
# End Source File
# Begin Source File

SOURCE=..\src\server\server.c
# End Source File
# Begin Source File

SOURCE=..\src\server\sendbufr.c
# End Source File
# Begin Source File

SOURCE=..\src\gui\win32\wnewserv.c
# End Source File
# Begin Source File

SOURCE=..\src\gui\win32\wrunserv.c
# End Source File
# Begin Source File

SOURCE=..\src\platform\winsock\wsipv6.c
# End Source File
# Begin Source File

SOURCE=..\src\platform\winsock\wsock1.c
# End Source File
# Begin Source File

SOURCE=..\src\platform\winsock\wsock2.c
# End Source File
# End Group
# Begin Group "Header Files"

# PROP Default_Filter "h;hpp;hxx;hm;inl"
# Begin Source File

SOURCE=..\src\server\event.h
# End Source File
# Begin Source File

SOURCE=..\src\common\hex.h
# End Source File
# Begin Source File

SOURCE=..\src\server\http.h
# End Source File
# Begin Source File

SOURCE=..\src\gui\win32\iwindows.h
# End Source File
# Begin Source File

SOURCE=..\src\platform\mscrtdl.h
# End Source File
# Begin Source File

SOURCE=..\src\platform\platform.h
# End Source File
# Begin Source File

SOURCE=..\src\gui\win32\res.h
# End Source File
# Begin Source File

SOURCE=..\src\server\routine.h
# End Source File
# Begin Source File

SOURCE=..\src\server\server.h
# End Source File
# Begin Source File

SOURCE=..\src\server\sendbufr.h
# End Source File
# Begin Source File

SOURCE=..\src\gui\win32\wnewserv.h
# End Source File
# Begin Source File

SOURCE=..\src\gui\win32\wrunserv.h
# End Source File
# Begin Source File

SOURCE=..\src\platform\winsock\wsipv6.h
# End Source File
# Begin Source File

SOURCE=..\src\platform\winsock\wsock1.h
# End Source File
# Begin Source File

SOURCE=..\src\platform\winsock\wsock2.h
# End Source File
# End Group
# Begin Group "Resource Files"

# PROP Default_Filter "ico;cur;bmp;dlg;rc2;rct;bin;rgs;gif;jpg;jpeg;jpe"
# Begin Source File

SOURCE=..\src\gui\win32\res.rc
# End Source File
# End Group
# Begin Source File

SOURCE=..\src\gui\win32\vsxp.xml
# End Source File
# End Target
# End Project
