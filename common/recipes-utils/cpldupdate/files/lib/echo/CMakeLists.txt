cmake_minimum_required(VERSION 2.8)
project(cpldupdate-dll-echo)

add_library(cpldupdate_dll_echo SHARED
  cpldupdate_dll_echo.c
)

target_link_libraries(cpldupdate_dll_echo
)

install(TARGETS cpldupdate_dll_echo DESTINATION lib)
