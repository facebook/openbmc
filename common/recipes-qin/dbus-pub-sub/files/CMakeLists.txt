cmake_minimum_required(VERSION 2.8)
project(dbus-publisher)

include(CheckCXXCompilerFlag)
CHECK_CXX_COMPILER_FLAG("-std=c++11" COMPILER_SUPPORTS_CXX11)
CHECK_CXX_COMPILER_FLAG("-std=c++0x" COMPILER_SUPPORTS_CXX0X)
if(COMPILER_SUPPORTS_CXX11)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11")
elseif(COMPILER_SUPPORTS_CXX0X)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++0x")
else()
    message(STATUS "The compiler ${CMAKE_CXX_COMPILER} has no C++11 support. Please use a different C++ compiler.")
endif()
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall")

# enable the below to allow for dynamic backtrace support
set(CMAKE_EXE_LINKER_FLAGS  "${CMAKE_EXE_LINKER_FLAGS} -rdynamic")

# $ENV{} takes out the environmental variables exported from the recipe
include_directories($ENV{SINC}/glib-2.0)
include_directories($ENV{SLIB}/glib-2.0/include)

find_library(GIO gio-2.0)
find_library(GLIB glib-2.0)

add_executable(dbus-publisher
  DBusPublisher.cpp
)

target_link_libraries(dbus-publisher
  ${GIO}
  ${GLIB}
  -lgobject-2.0
)

project(dbus-subscriber)

add_executable(dbus-subscriber
  DBusSubscriber.cpp
)

target_link_libraries(dbus-subscriber
  ${GIO}
  ${GLIB}
  -lgobject-2.0
)

install(TARGETS dbus-publisher dbus-subscriber DESTINATION bin)
install(FILES org.openbmc.TimerService.conf DESTINATION /etc/dbus-1/system.d/)
