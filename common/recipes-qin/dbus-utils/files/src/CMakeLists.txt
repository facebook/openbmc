cmake_minimum_required(VERSION 2.8)
project(dbus-utils)

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

find_library(OBJECT_TREE object-tree)
find_library(GIO gio-2.0)
find_library(GLIB glib-2.0)
find_library(GTEST gtest)
find_library(GLOG glog)
find_library(GFLAGS gflags)

add_library(dbus-utils
  DBus.cpp
  DBusObject.cpp
  dbus-interface/DBusDefaultInterface.cpp
  dbus-interface/DBusObjectInterface.cpp
)

target_link_libraries(dbus-utils
    ${OBJECT_TREE}
    ${GLOG}
    ${GIO}
    ${GLIB}
    -lpthread
    -lgobject-2.0
)

install(TARGETS dbus-utils DESTINATION lib)

install(FILES
  DBus.h
  DBusObject.h
  DBusInterfaceBase.h
  DESTINATION include/dbus-utils
)

install(FILES
  dbus-interface/DBusDefaultInterface.h
  dbus-interface/DBusObjectInterface.h
  DESTINATION include/dbus-utils/dbus-interface
)

add_executable(dump-object
  ObjectDumper.cpp
)

target_link_libraries(dump-object
  dbus-utils
  ${GFLAGS}
)

install(TARGETS dump-object DESTINATION bin)

option(BUILD_TESTS "BUILD_TESTS" ON)

if(BUILD_TESTS)
  enable_testing()

  add_executable(dbus-interface-test
    tests/DBusInterfaceTest.cpp
    dbus-interface/DBusDefaultInterface.cpp
  )

  target_link_libraries(dbus-interface-test
    ${GLOG}
    ${GIO}
    ${GLIB}
    -lpthread
    -lgobject-2.0
  )

  add_test(DBusInterfaceTest
    dbus-interface-test
  )

  install(TARGETS dbus-interface-test DESTINATION bin)
  install(FILES tests/interface-test.sh DESTINATION bin)

  add_executable(dbus-object-test
    tests/DBusObjectTest.cpp
    DBusObject.cpp
  )

  target_link_libraries(dbus-object-test
    ${GTEST}
    ${GLOG}
    ${GIO}
    ${GLIB}
    -lpthread
    -lgobject-2.0
  )

  add_test(DBusObjectTest
    dbus-object-test
  )

  install(TARGETS dbus-object-test DESTINATION bin)

  add_executable(dbus-test
    tests/DBusTest.cpp
    DBus.cpp
    DBusObject.cpp
    dbus-interface/DBusDefaultInterface.cpp
  )

  target_link_libraries(dbus-test
    ${GTEST}
    ${GLOG}
    ${GIO}
    ${GLIB}
    -lpthread
    -lgobject-2.0
  )

  add_test(DBusTest
    dbus-test
  )

  install(TARGETS dbus-test DESTINATION bin)
  install(FILES tests/org.openbmc.Chassis.conf DESTINATION /etc/dbus-1/system.d)

  add_executable(dbus-object-interface-test
    tests/DBusObjectTestServer.cpp
    DBus.cpp
    DBusObject.cpp
    dbus-interface/DBusDefaultInterface.cpp
    dbus-interface/DBusObjectInterface.cpp
  )

  target_link_libraries(dbus-object-interface-test
    ${OBJECT_TREE}
    ${GLOG}
    ${GIO}
    ${GLIB}
    -lpthread
    -lgobject-2.0
  )

  add_test(DBusObjectInterfaceTest
    dbus-object-interface-test
  )

  install(TARGETS dbus-object-interface-test DESTINATION bin)
endif ()

