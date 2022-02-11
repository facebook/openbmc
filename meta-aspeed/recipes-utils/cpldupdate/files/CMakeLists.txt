cmake_minimum_required(VERSION 2.8)
project(cpldupdate-dll-ast-jtag)

set(CMAKE_C_FLAGS "${OECMAKE_C_FLAGS}" CACHE STRING "CFLAGS")

add_library(cpldupdate_dll_ast_jtag SHARED
  cpldupdate_dll_ast_jtag.c
)

target_link_libraries(cpldupdate_dll_ast_jtag
  log
  misc-utils
)

install(TARGETS cpldupdate_dll_ast_jtag DESTINATION lib)
