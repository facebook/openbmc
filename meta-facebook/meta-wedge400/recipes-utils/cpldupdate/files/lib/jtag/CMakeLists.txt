cmake_minimum_required(VERSION 2.8)
project(cpldupdate-dll-jtag)

add_library(cpldupdate_dll_jtag SHARED
  cpldupdate_dll_jtag.c
)

install(TARGETS cpldupdate_dll_jtag DESTINATION lib)
