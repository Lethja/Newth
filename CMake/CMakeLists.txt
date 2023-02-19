cmake_minimum_required(VERSION 3.24)
project(OpenWeb C)

set(CMAKE_C_STANDARD 90)

add_executable(Client ../src/client.c ../src/platform/unix.h)

add_executable(Server ../src/server.c ../src/platform/unix.h)
