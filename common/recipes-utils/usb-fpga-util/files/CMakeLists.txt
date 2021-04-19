cmake_minimum_required(VERSION 2.8)
project(fpgamem)

add_executable(fpgamem
    fpga-mem.cpp
)

install(TARGETS fpgamem DESTINATION bin)
