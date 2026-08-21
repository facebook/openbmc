#
# Minimum version of cmake required
#
cmake_minimum_required(VERSION 3.5.0)
#
#   cmake command line options:
#
#   1) Set CMAKE_TOOLCHAIN_FILE=<path_of_toolchain_file>
#      For Cross-compilation, Set the flag as mentioned.
#
#      example #cmake -DCMAKE_TOOLCHAIN_FILE=<path> ..

## Set default module path if not already set
if(NOT DEFINED CMAKE_MODULE_PATH)
    set(CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake_modules/")
endif()
## Include common cmake modules
include(utils)

set(APML_LIB "apml")
set(APML_LIB_COMPONENT "lib${APML_LIB}")
set(APML_LIB_TARGET "${APML_LIB}64")
set(SRC_DIR "src/esmi_oob")
set(TOOL_DIR "tools")
set(INC_DIR "include/esmi_oob")

# The following default version values should be updated as appropriate for
# ABI breaks (update MAJOR and MINOR), and ABI/API additions (update MINOR).
# Until ABI stabilizes VERSION_MAJOR will be 0. This should be over-ridden
# by git tags (through "git describe") when they are present.
set(APML_LIB_VERSION_MAJOR 5)
set(APML_LIB_VERSION_MINOR 4)
set(APML_LIB_VERSION_PATCH 1)

################# Determine the library version #########################
## Setup the package version based on git tags.
set(LIB_SO_VERSION_STR
              "${APML_LIB_VERSION_MAJOR}.${APML_LIB_VERSION_MINOR}.${APML_LIB_VERSION_PATCH}")
get_version(${LIB_SO_VERSION_STR} "apml")

# VERSION_* variables should be set by get_version
set(LIB_SO_VERSION_STR ${VERSION_STRING})
set(${APML_LIB}_VERSION_MAJOR "${VERSION_MAJOR}")
set(${APML_LIB}_VERSION_MINOR "${VERSION_MINOR}")
set(${APML_LIB}_VERSION_PATCH "${VERSION_PATCH}")
set(${APML_LIB}_VERSION_BUILD "${VERSION_BUILD}")

# Debian package specific variables
# Set a default value for the package version
set(VERSION_STRING "1.0.0")
get_version(${VERSION_STRING} "e")

# VERSION_* variables should be set by get_version
set(BUILD_VERSION_STRING "${VERSION_MAJOR}.${VERSION_MINOR}.${VERSION_PATCH}")

## Define default variable and variables for the optional build target
##  esmi_oob_lib-dev
set(SOURCE_DIR ${CMAKE_CURRENT_SOURCE_DIR}
	CACHE STRING "Location of APML_LIB source code.")
set(CMAKE_INSTALL_PREFIX "${CMAKE_CURRENT_BINARY_DIR}/${APML_LIB}/usr/"
					CACHE STRING "Default installation directory.")
## Commenting as not supporting packaging from cmake
# set(CPACK_PACKAGING_INSTALL_PREFIX "${CMAKE_CURRENT_BINARY_DIR}/esmi_oob"
#                                      CACHE STRING "Default packaging prefix.")

project(${APML_LIB_TARGET})

# Create a configure file to get version info from within library
configure_file(
  "${PROJECT_SOURCE_DIR}/${SRC_DIR}/${APML_LIB_TARGET}Config.in"
  "${PROJECT_SOURCE_DIR}/${INC_DIR}/${APML_LIB_TARGET}Config.h")

if (NOT DEFINED CPACK_PACKAGE_VENDOR)
    set(CPACK_PACKAGE_VENDOR "AMD")
endif()

if (NOT DEFINED CPACK_PACKAGE_CONTACT)
    set(CPACK_PACKAGE_CONTACT "Advanced Micro Devices Inc.")
endif()

if (NOT DEFINED CPACK_PACKAGE_DESCRIPTION_SUMMARY)
set(CPACK_PACKAGE_DESCRIPTION_SUMMARY
		"EPYC™ Side Band System Management Interface library")
endif()

set(CPACK_PACKAGE_FILE_NAME "apml_lib64-${BUILD_VERSION_STRING}")
## Verbose output.
set(CMAKE_VERBOSE_MAKEFILE on)

## Compiler flags
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wextra -fno-rtti -m64")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -msse -msse2 -std=c++11 ")
# Use this instead of above for 32 bit
# set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -m32")

if ("${CMAKE_BUILD_TYPE}" STREQUAL Release)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -O2")
else ()
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -ggdb -O0 -DDEBUG")
endif ()

include_directories(${CMAKE_CURRENT_SOURCE_DIR}/include)
set(APML_LIB_SRC_LIST ${APML_LIB_SRC_LIST} "${SRC_DIR}/apml_err.c")
set(APML_LIB_SRC_LIST ${APML_LIB_SRC_LIST} "${SRC_DIR}/apml.c")
set(APML_LIB_SRC_LIST ${APML_LIB_SRC_LIST} "${SRC_DIR}/esmi_cpuid_msr.c")
set(APML_LIB_SRC_LIST ${APML_LIB_SRC_LIST} "${SRC_DIR}/esmi_mailbox.c")
set(APML_LIB_SRC_LIST ${APML_LIB_SRC_LIST} "${SRC_DIR}/rmi_mailbox_mi300.c")
set(APML_LIB_SRC_LIST ${APML_LIB_SRC_LIST} "${SRC_DIR}/esmi_rmi.c")
set(APML_LIB_SRC_LIST ${APML_LIB_SRC_LIST} "${SRC_DIR}/esmi_tsi.c")
set(APML_LIB_SRC_LIST ${APML_LIB_SRC_LIST} "${SRC_DIR}/apml_recovery.c")
set(APML_LIB_SRC_LIST ${APML_LIB_SRC_LIST} "${SRC_DIR}/tsi_mi300.c")

set(SMI_TOOL "apml_tool")
set(SMI_CPUID "apml_cpuid_tool")

add_executable(${SMI_TOOL} "${TOOL_DIR}/apml_tool.c"
		"${TOOL_DIR}/mi300_tool.c")
add_executable(${SMI_CPUID} "${TOOL_DIR}/apml_cpuid_tool.c")

target_link_libraries(${SMI_TOOL} ${APML_LIB_TARGET})
target_link_libraries(${SMI_CPUID} ${APML_LIB_TARGET})

add_library(${APML_LIB_TARGET} SHARED ${APML_LIB_SRC_LIST} ${SMI_INC_LIST})
target_link_libraries(${APML_LIB_TARGET} pthread rt m)

## Set the VERSION and SOVERSION values
set_property(TARGET ${APML_LIB_TARGET}
                             PROPERTY VERSION "${LIB_SO_VERSION_STR}")
set_property(TARGET ${APML_LIB_TARGET}
             PROPERTY SOVERSION "${APML_LIB_VERSION_MAJOR}")

## If the library is a release, strip the target library
if ("${CMAKE_BUILD_TYPE}" STREQUAL Release)
    add_custom_command(
          TARGET ${APML_LIB_TARGET}
          POST_BUILD COMMAND ${CMAKE_STRIP} lib${APML_LIB_TARGET}.so)
endif ()

## Add the install directives for the runtime library.
install(TARGETS ${APML_LIB_TARGET}
          LIBRARY DESTINATION lib COMPONENT ${APML_LIB_COMPONENT})
install(DIRECTORY ${SOURCE_DIR}/include/esmi_oob/
					DESTINATION include)
install(PROGRAMS ${CMAKE_CURRENT_BINARY_DIR}/${SMI_TOOL}
					DESTINATION bin)
install(PROGRAMS ${CMAKE_CURRENT_BINARY_DIR}/${SMI_CPUID}
					DESTINATION bin)

find_package(Doxygen)
if (DOXYGEN_FOUND)
  configure_file(${CMAKE_CURRENT_SOURCE_DIR}/docs/apml_doxygen.cfg
                        ${CMAKE_CURRENT_BINARY_DIR}/Doxyfile @ONLY)

  add_custom_command(OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/latex/refman.tex
        COMMAND ${DOXYGEN_EXECUTABLE} ${CMAKE_CURRENT_BINARY_DIR}/Doxyfile
        DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/docs/apml_doxygen.cfg
        WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR})
  add_custom_command(OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/latex/refman.pdf
        COMMAND make  > /dev/null
        COMMAND cp  ${CMAKE_CURRENT_BINARY_DIR}/latex/refman.pdf
	${CMAKE_CURRENT_SOURCE_DIR}/APML_Library_Manual.pdf
        DEPENDS ${CMAKE_CURRENT_BINARY_DIR}/latex/refman.tex
        WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/latex)

  # To create directory "rel" to contain seprate latex
  set(folder "rel")
  add_custom_command(OUTPUT ${folder}
        COMMAND mkdir ${folder}
  )
  configure_file(${CMAKE_CURRENT_SOURCE_DIR}/docs/release_notes_doxygen.cfg
                        ${CMAKE_CURRENT_BINARY_DIR}/rel/Doxyfile @ONLY)
  add_custom_command(OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/rel/latex/refman.tex
        COMMAND ${DOXYGEN_EXECUTABLE} ${CMAKE_CURRENT_BINARY_DIR}/rel/Doxyfile
        DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/docs/release_notes_doxygen.cfg
        WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/rel)
  add_custom_command(OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/rel/latex/refman.pdf
        COMMAND make  > /dev/null
        COMMAND cp  ${CMAKE_CURRENT_BINARY_DIR}/rel/latex/refman.pdf
                ${CMAKE_CURRENT_SOURCE_DIR}/APML_Library_Release_Notes.pdf
        DEPENDS ${CMAKE_CURRENT_BINARY_DIR}/rel/latex/refman.tex
        WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/rel/latex)

  add_custom_target(doc DEPENDS ${folder} ${CMAKE_CURRENT_BINARY_DIR}/latex/refman.pdf
                        ${CMAKE_CURRENT_BINARY_DIR}/rel/latex/refman.pdf)

else()
  message("Doxygen is not found. Will not generate documents.")
endif(DOXYGEN_FOUND)
