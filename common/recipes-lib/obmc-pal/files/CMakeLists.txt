cmake_minimum_required(VERSION 2.8)
project(obmc-pal)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wextra -Werror")
set(CMAKE_LINK_FLAGS "-ledb")

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
