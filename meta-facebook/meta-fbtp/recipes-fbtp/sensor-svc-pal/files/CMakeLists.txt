# Copyright 2017-present Facebook. All Rights Reserved.
cmake_minimum_required(VERSION 2.8)
project(libsensor-svc-pal)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wextra -Werror")
set(CMAKE_LINK_FLAGS "-lkv -lipmb -lgpio -lobmc-pal -lobmc-i2c")

add_library(sensor-svc-pal
  sensorsvcpal.c
)

install(TARGETS sensor-svc-pal DESTINATION lib)

install(FILES
  sensorsvcpal.h
  DESTINATION include/openbmc
)
