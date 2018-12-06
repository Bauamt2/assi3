cmake_minimum_required(VERSION 3.12)
project(assi3v9 C)

set(CMAKE_C_STANDARD 99)

add_executable(client main.c)
add_executable(server main_s.c)