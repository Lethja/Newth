# This cross compiler should be used with a toolchain build that links the Windows 95 compatible C runtime (CRTDLL.DLL)
# The easiest way to build a toolchain for this is to clone https://github.com/Zeranoe/mingw-w64-build.git then run
# `./mingw-w64-build i586 --linked-runtime crtdll --disable-threads` from a POSIX machine

set(CMAKE_SYSTEM_NAME Windows)

# Modify these directories as needed
set(CMAKE_C_COMPILER        /opt/mingw-crtdll/i586/bin/i586-w64-mingw32-gcc )
set(CMAKE_CXX_COMPILER      /opt/mingw-crtdll/i586/bin/i586-w64-mingw32-g++ )
set(CMAKE_FIND_ROOT_PATH    /opt/mingw-crtdll/i586                          )
set(WIN32 1)

set(CMAKE_FIND_ROOT_PATH_MODE_PROGRAM NEVER)

set(CMAKE_FIND_ROOT_PATH_MODE_LIBRARY ONLY)
set(CMAKE_FIND_ROOT_PATH_MODE_INCLUDE ONLY)
