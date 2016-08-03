cmake_minimum_required(VERSION 2.8)
project(dbus-obtree-testserver)

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

find_library(DBUS-UTILS dbus-utils)
find_library(OBJECT-TREE object-tree)
find_library(GFLAGS gflags)
find_library(GTEST gtest)
find_library(GLOG glog)
find_library(GIO gio-2.0)
find_library(GLIB glib-2.0)

add_library(libsensord
  SensorObjectTree.cpp
  SensorDevice.cpp
  SensorObject.cpp
  SensorSysfsApi.cpp
  SensorJsonParser.cpp
)

target_link_libraries(libsensord
  ${DBUS-UTILS}
  ${OBJECT-TREE}
  ${GLOG}
  ${GFLAGS}
  ${GIO}
  ${GLIB}
  -lpthread
  -lgobject-2.0
)

add_executable(openbmc-sensord
  OpenbmcSensord.cpp
)

target_link_libraries(openbmc-sensord
  libsensord
)

install(TARGETS openbmc-sensord DESTINATION bin)
install(FILES org.openbmc.Sensord.conf DESTINATION /etc/dbus-1/system.d/)

option(BUILD_TESTS "BUILD_TESTS" ON)

if(BUILD_TESTS)
  enable_testing()

  # test bridging object-tree and dbus-utils through ipc-interface
  add_executable(dbus-reg-test
    tests/DBusRegTest.cpp
    tests/DBusObjectTreeInterface.cpp
  )

  target_link_libraries(dbus-reg-test
    ${GTEST}
    ${GLOG}
    ${GIO}
    ${GLIB}
    ${DBUS-UTILS}
    ${OBJECT-TREE}
    -lpthread
    -lgobject-2.0
  )

  add_test(DBusRegTest
    dbus-reg-test
  )

  install(TARGETS dbus-reg-test DESTINATION bin)

  add_executable(dbus-obtree-testserver
    tests/DBusObjectTreeTestServer.cpp
    tests/DBusObjectTreeInterface.cpp
  )

  target_link_libraries(dbus-obtree-testserver
    ${GTEST}
    ${GLOG}
    ${GIO}
    ${GLIB}
    ${DBUS-UTILS}
    ${OBJECT-TREE}
    -lpthread
    -lgobject-2.0
  )

  install(TARGETS dbus-obtree-testserver DESTINATION bin)

  add_executable(sensor-attribute-test
    tests/SensorAttributeTest.cpp
  )

  target_link_libraries(sensor-attribute-test
    ${GTEST}
    ${GLOG}
    ${OBJECT-TREE}
    -lpthread
    -lgobject-2.0
  )

  install(TARGETS sensor-attribute-test DESTINATION bin)

  add_executable(sensor-object-test
    tests/SensorObjectTest.cpp
    SensorDevice.cpp
    SensorObject.cpp
    SensorSysfsApi.cpp
  )

  target_link_libraries(sensor-object-test
    ${GTEST}
    ${GLOG}
    ${OBJECT-TREE}
    -lpthread
    -lgobject-2.0
  )

  install(TARGETS sensor-object-test DESTINATION bin)

  add_executable(sensor-reg-test
    tests/DBusSensorRegTest.cpp
    tests/DBusObjectTreeInterface.cpp
    SensorObjectTree.cpp
    SensorDevice.cpp
    SensorObject.cpp
    SensorSysfsApi.cpp
  )

  target_link_libraries(sensor-reg-test
    ${GTEST}
    ${GLOG}
    ${GIO}
    ${GLIB}
    ${DBUS-UTILS}
    ${OBJECT-TREE}
    -lpthread
    -lgobject-2.0
  )

  install(TARGETS sensor-reg-test DESTINATION bin)

  add_executable(sensor-jsonparser-test
    tests/SensorJsonParserTest.cpp
    tests/DBusObjectTreeInterface.cpp
    SensorJsonParser.cpp
    SensorObjectTree.cpp
    SensorObject.cpp
    SensorDevice.cpp
    SensorSysfsApi.cpp
  )

  target_link_libraries(sensor-jsonparser-test
    ${GTEST}
    ${GLOG}
    ${GIO}
    ${GLIB}
    ${DBUS-UTILS}
    ${OBJECT-TREE}
    -lpthread
    -lgobject-2.0
  )

  install(TARGETS sensor-jsonparser-test DESTINATION bin)
endif ()

