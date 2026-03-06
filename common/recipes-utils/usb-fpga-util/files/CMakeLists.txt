cmake_minimum_required(VERSION 3.8)
project(fpgamem)

add_executable(fpgamem
    fpga-mem.cpp
)

install(TARGETS fpgamem DESTINATION bin)
