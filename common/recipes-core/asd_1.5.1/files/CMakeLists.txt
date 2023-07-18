cmake_minimum_required(VERSION 2.8.10 FATAL_ERROR)
project(at-scale-debug C)

find_package (PkgConfig REQUIRED)
set(ASD_DIR ${PROJECT_SOURCE_DIR})

add_subdirectory(jtag_test)
add_subdirectory(server)
add_subdirectory(target)

