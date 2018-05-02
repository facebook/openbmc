cmake_minimum_required(VERSION 2.8)
project(cpldupdate-dll-ast-jtag)

add_library(cpldupdate_dll_ast_jtag SHARED
  cpldupdate_dll_ast_jtag.c
)

install(TARGETS cpldupdate_dll_ast_jtag DESTINATION lib)
