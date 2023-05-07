cmake_minimum_required(VERSION 3.24)
project(Newth C)

set(CMAKE_C_STANDARD 90)
set(CMAKE_C_FLAGS "-pedantic -Wall -Wno-unknown-pragmas")

option(PORTABLE_WIN32 "Force a portable Windows binary" ON)
if (PORTABLE_WIN32)
    set(CMAKE_SYSTEM_NAME Windows)
    set(CMAKE_C_FLAGS "-m32 -march=i386")
endif ()

set(CMAKE_C_FLAGS_RELEASE "-O2 -s")

if (UNIX)
    set(TH_PLATFORM_SRC ../src/platform/posix01.c)
elseif (WIN32)
    if (PORTABLE_WIN32)
        set(TH_PLATFORM_SRC ../src/platform/mscrtdl.c ../src/platform/winsock/wsock1.c ../src/platform/winsock/wsock2.c ../src/platform/winsock/wsipv6.c)
    else ()
        set(TH_PLATFORM_SRC ../src/platform/mscrtdl.c ../src/platform/winsock/wsock2.c ../src/platform/winsock/wsipv6.c)
    endif ()
endif ()

add_executable(dl ../src/client.c ../src/common/http.h ../src/platform/platform.h)
add_executable(th ../src/server.c ../src/platform/posix01.h ../src/server/http.c
        ../src/server/http.h ../src/common/http.h ../src/server/routine.c ../src/server/routine.h
        ../src/server/sockbufr.c ../src/server/sockbufr.h
        ../src/platform/platform.c ../src/platform/platform.h
        ../src/server/event.c ../src/server/event.h
        ${TH_PLATFORM_SRC})

target_compile_definitions(dl PUBLIC "$<$<CONFIG:RELEASE>:NDEBUG>")
target_compile_definitions(th PUBLIC "$<$<CONFIG:RELEASE>:NDEBUG>")

if (WIN32)
    if (PORTABLE_WIN32)
        target_compile_definitions(th PUBLIC PORTABLE_WIN32=1)
    endif ()

    target_link_libraries(dl wsock32)
    target_link_libraries(th wsock32)
endif ()
