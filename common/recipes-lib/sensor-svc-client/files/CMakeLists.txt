cmake_minimum_required(VERSION 2.8)
project(libsensor-svc-client)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wextra -Werror")

include_directories($ENV{SINC}/glib-2.0)
include_directories($ENV{SLIB}/glib-2.0/include)

find_library(GIO gio-2.0)
find_library(GLIB glib-2.0)

add_library(sensor-svc-client SHARED
  sensor-svc-client.c
)

set_target_properties(sensor-svc-client PROPERTIES VERSION "0.1")

target_link_libraries(sensor-svc-client
  ${GIO}
  ${GLIB}
)

install(TARGETS sensor-svc-client DESTINATION lib)

install(FILES
  sensor-svc-client.h
  DESTINATION include/openbmc
)
