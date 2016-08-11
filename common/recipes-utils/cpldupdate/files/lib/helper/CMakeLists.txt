cmake_minimum_required(VERSION 2.8)
project(cpldupdate-dll-helper)

add_library(cpldupdate_dll_helper
  cpldupdate_dll.c
)

target_link_libraries(cpldupdate_dll_helper
    -ldl
)

install(TARGETS cpldupdate_dll_helper DESTINATION lib)

install(FILES
  cpldupdate_dll.h
  DESTINATION include/openbmc
)
