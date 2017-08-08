cmake_minimum_required(VERSION 2.8)
project(platform-svc-unittest)

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

include_directories($ENV{SINC}/glib-2.0)
include_directories($ENV{SLIB}/glib-2.0/include)

find_library(OBJECT-TREE object-tree)
find_library(GTEST gtest)
find_library(GLOG glog)
find_library(DBUS-UTILS dbus-utils)
find_library(GIO gio-2.0)
find_library(GLIB glib-2.0)

add_executable(test-platform-svc-fru
  FRUTest.cpp
)

target_link_libraries(test-platform-svc-fru
  ${GLOG}
  ${GTEST}
  ${OBJECT-TREE}
  -lpthread
)

install(TARGETS test-platform-svc-fru DESTINATION bin)

add_executable(test-platform-svc-hotplugdetectionmechanism
  HotPlugDetectionMechanismTest.cpp
)

target_link_libraries(test-platform-svc-hotplugdetectionmechanism
  ${GLOG}
  ${GTEST}
  -lpthread
)

install(TARGETS test-platform-svc-hotplugdetectionmechanism DESTINATION bin)

add_executable(test-platform-svc-platform-object-tree
  PlatformObjectTreeTest.cpp
  ../PlatformObjectTree.cpp
  ../PlatformJsonParser.cpp
  ../SensorService.cpp
  ../DBusPlatformSvcInterface.cpp
  ../FruService.cpp
  ../DBusHPExtDectectionFruInterface.cpp
)

target_link_libraries(test-platform-svc-platform-object-tree
  ${GLOG}
  ${GFLAGS}
  ${GIO}
  ${GLIB}
  ${GTEST}
  ${DBUS-UTILS}
  ${OBJECT-TREE}
  -lgobject-2.0
  -lpthread
)

install(TARGETS test-platform-svc-platform-object-tree DESTINATION bin)
install(FILES test-platform-svcd-config.json DESTINATION /etc)

install(FILES platform-svc-unit-tests.sh DESTINATION bin)
