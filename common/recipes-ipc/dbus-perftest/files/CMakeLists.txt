cmake_minimum_required(VERSION 2.8)
project(dbus-testserver)

# enable the below to allow for dynamic backtrace support
set(CMAKE_EXE_LINKER_FLAGS  "${CMAKE_EXE_LINKER_FLAGS} -rdynamic")

# $ENV{} takes out the environmental variables exported from the recipe
include_directories($ENV{SINC}/glib-2.0)
include_directories($ENV{SLIB}/glib-2.0/include)

find_library(GIO gio-2.0)
find_library(GLIB glib-2.0)

add_executable(dbus-testserver
  DBusServer.c
)

target_link_libraries(dbus-testserver
  ${GIO}
  ${GLIB}
  -lgobject-2.0
  -lpthread
)

project(dbus-latencytest)

add_executable(dbus-latencytest
  DBusLatencyTest.c
)

target_link_libraries(dbus-latencytest
  ${GIO}
  ${GLIB}
  -lgobject-2.0
  -lpthread
  -lm
)

project(dbus-mem-testserver)

add_executable(dbus-mem-testserver
  DBusServerMemtest.c
)

target_link_libraries(dbus-mem-testserver
  ${GIO}
  ${GLIB}
  -lgobject-2.0
  -lpthread
  -lm
)

install(TARGETS dbus-mem-testserver dbus-testserver dbus-latencytest DESTINATION bin)
install(FILES dbus-cputest.sh dbus-latencytest.sh dbus-memtest.sh DESTINATION bin)
