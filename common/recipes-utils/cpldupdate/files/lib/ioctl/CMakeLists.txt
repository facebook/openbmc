cmake_minimum_required(VERSION 2.8)
project(cpldupdate-dll-ioctl)

add_library(cpldupdate_dll_ioctl SHARED
  cpldupdate_dll_ioctl.c
)

target_link_libraries(cpldupdate_dll_ioctl
)

install(TARGETS cpldupdate_dll_ioctl DESTINATION lib)
