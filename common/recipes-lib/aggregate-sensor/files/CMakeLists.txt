cmake_minimum_required(VERSION 2.8)
project(libaggregate-sensor)

set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wall -Wextra -Werror")

add_library(aggregate-sensor SHARED
  aggregate-sensor.c
  aggregate-sensor-json.c
  math_expression.c
)

set_target_properties(aggregate-sensor PROPERTIES VERSION "0.1")

target_link_libraries(aggregate-sensor
  jansson
  pal
  sdr
)

install(TARGETS aggregate-sensor DESTINATION lib)
install(FILES aggregate-sensor-conf.json DESTINATION /etc)

install(FILES
  aggregate-sensor.h
  DESTINATION include/openbmc
)
