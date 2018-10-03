cmake_minimum_required(VERSION 2.8)
project(obmc-pal)

set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wall -Werror -fPIC")
set(CMAKE_LINK_FLAGS "-lkv")

add_library(obmc-pal
  obmc-pal
  obmc-sensor
)

target_link_libraries(obmc-pal
)

install(TARGETS obmc-pal DESTINATION lib)

install(FILES
  obmc-pal.h
  obmc-sensor.h
  DESTINATION include/openbmc
)
