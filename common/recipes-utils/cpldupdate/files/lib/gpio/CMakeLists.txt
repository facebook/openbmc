cmake_minimum_required(VERSION 2.8)
project(cpldupdate-dll-gpio)

add_library(cpldupdate_dll_gpio SHARED
  cpldupdate_dll_gpio.c
)

target_link_libraries(cpldupdate_dll_gpio
  -lgpio
)

install(TARGETS cpldupdate_dll_gpio DESTINATION lib)
