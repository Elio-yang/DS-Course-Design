cmake_minimum_required(VERSION 3.17)
project(Nogo)

set(CMAKE_CXX_STANDARD 14)

add_executable(
        nogo_main

        nogo_main.cpp
)

add_executable(
        nogo_maint

        newt.cpp
)