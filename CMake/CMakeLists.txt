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
    set(PLATFORM_SRC ../src/platform/posix01.c ../src/platform/posix01.c)
elseif (WIN32)
    set(PLATFORM_SRC ../src/platform/winsock.c ../src/platform/winsock.c)
endif ()

#add_executable(client ../src/client.c ../src/platform/unix.h ../src/common/http.h ../src/platform/platform.h)
add_executable(th ../src/server.c ../src/platform/posix01.h ../src/server/http.c
        ../src/server/http.h ../src/common/http.h ../src/server/routine.c ../src/server/routine.h
        ../src/server/sockbufr.c ../src/server/sockbufr.h
        ../src/platform/platform.c ../src/platform/platform.h
        ../src/server/event.c ../src/server/event.h
        ${PLATFORM_SRC})

#target_compile_definitions(client PUBLIC "$<$<CONFIG:RELEASE>:NDEBUG>")
target_compile_definitions(th PUBLIC "$<$<CONFIG:RELEASE>:NDEBUG>")

if (WIN32)
    add_library(thwsipv6 SHARED ../src/platform/winsock/wsipv6.c)
    target_compile_definitions(thwsipv6 PUBLIC _WIN32_WINNT=0x0A00)
    set_target_properties(thwsipv6 PROPERTIES PREFIX "")
    target_link_libraries(thwsipv6 libwsock32.a iphlpapi)

    add_library(thwsock2 SHARED ../src/platform/winsock/wsock2.c)
    set_target_properties(thwsock2 PROPERTIES PREFIX "")
    target_link_libraries(thwsock2 libwsock32.a iphlpapi)

    add_library(thwsock1 SHARED ../src/platform/winsock/wsock1.c)
    set_target_properties(thwsock1 PROPERTIES PREFIX "")
    target_link_libraries(thwsock1 libwsock32.a)

    #    target_link_libraries(client libwsock32.a iphlpapi)
    target_link_libraries(th libwsock32.a)
    add_dependencies(th thwsipv6 thwsock2)
    if (PORTABLE_WIN32)
        add_dependencies(th thwsock1)
    endif ()
endif ()
