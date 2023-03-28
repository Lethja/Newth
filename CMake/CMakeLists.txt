cmake_minimum_required(VERSION 3.24)
project(OpenWeb C)

set(CMAKE_C_STANDARD 90)
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -std=c89 -pedantic -Wall -Wno-unknown-pragmas")

if (UNIX)
    set(PLATFORM_SRC ../src/platform/unix.c ../src/platform/unix.c)
elseif (WIN32)
    set(PLATFORM_SRC ../src/platform/ws2.c ../src/platform/ws2.c)
endif ()

add_executable(client ../src/client.c ../src/platform/unix.h ../src/common/http.h ../src/platform/platform.h)
add_executable(server ../src/server.c ../src/platform/unix.h ../src/server/http.c
        ../src/server/http.h ../src/common/http.h ../src/server/routine.c ../src/server/routine.h
        ../src/server/sockbufr.c ../src/server/sockbufr.h
        ../src/platform/platform.c ../src/platform/platform.h
        ../src/server/event.c ../src/server/event.h
        ${PLATFORM_SRC})

target_compile_definitions(client PUBLIC "$<$<CONFIG:RELEASE>:NDEBUG>")
target_compile_definitions(server PUBLIC "$<$<CONFIG:RELEASE>:NDEBUG>")

if (WIN32)
    target_link_libraries(client libwsock32.a iphlpapi)
    target_link_libraries(server libwsock32.a iphlpapi)
endif ()
