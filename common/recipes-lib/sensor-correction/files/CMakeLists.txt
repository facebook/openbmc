cmake_minimum_required(VERSION 2.8)
project(libsensor-correction)

set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wall -Wextra -Werror")

add_library(sensor-correction SHARED
  sensor-correction.c
)

set_target_properties(sensor-correction PROPERTIES VERSION "0.1")

target_link_libraries(sensor-correction
  jansson
  edb
)

install(TARGETS sensor-correction DESTINATION lib)

install(FILES
  sensor-correction.h
  DESTINATION include/openbmc
)
