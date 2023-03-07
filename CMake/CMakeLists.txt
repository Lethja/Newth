cmake_minimum_required(VERSION 3.24)
project(OpenWeb C)

set(CMAKE_C_STANDARD 90)

add_executable(client ../src/client.c ../src/platform/unix.h ../src/common/http.h)
add_executable(server ../src/server.c ../src/platform/unix.h ../src/server/http.c ../src/server/http.h ../src/common/http.h ../src/server/routine.c ../src/server/routine.h)

target_compile_definitions(client PUBLIC "$<$<CONFIG:RELEASE>:NDEBUG>")
target_compile_definitions(server PUBLIC "$<$<CONFIG:RELEASE>:NDEBUG>")
