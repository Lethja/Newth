cmake_minimum_required(VERSION 3.24)
project(OpenWeb C)

set(CMAKE_C_STANDARD 90)

add_executable(client ../src/client.c ../src/platform/unix.h)
add_executable(server ../src/server.c ../src/platform/unix.h)
