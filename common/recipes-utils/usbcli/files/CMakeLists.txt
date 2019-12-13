cmake_minimum_required(VERSION 2.8)
project(usbcli)

find_library(LIBUSB1 libusb1)

add_executable(usbcli
    usbcli.cpp
)

target_link_libraries(usbcli
    -lusb-1.0
)

install(TARGETS usbcli DESTINATION bin)
