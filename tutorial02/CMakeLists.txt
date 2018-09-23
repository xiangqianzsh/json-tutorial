cmake_minimum_required(VERSION 3.12)
project(tutorial02 C)

set(CMAKE_C_STANDARD 11)

include_directories(.)

add_executable(tutorial02
        leptjson.c
        leptjson.h
        test.c)
