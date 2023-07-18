cmake_minimum_required(VERSION 2.8.10 FATAL_ERROR)
project(at-scale-debug-jtag-test C)

find_package (PkgConfig REQUIRED)
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/sysroot/include)
pkg_check_modules (SAFEC REQUIRED libsafec)
# Define HAVE_C99 to include sprintf_s macro in safec library
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -DHAVE_C99")
include_directories (${SAFEC_INCLUDE_DIRS})
include_directories (${ASD_DIR}/include)
include_directories (${ASD_DIR}/target)
link_directories (${SAFEC_LIBRARY_DIRS})

# Set APB_FREQ to EXTRA_OECMAKE on bb recipe to modify JTAG frequency
if(${APB_FREQ})
    add_definitions( -DAPB_FREQ=${APB_FREQ} )
endif(${APB_FREQ})

#if(NOT ${BUILD_UT})
    add_executable(jtag_test jtag_test.c
    ${ASD_DIR}/server/logging.c
    ${ASD_DIR}/target/jtag_handler.c)
    target_link_libraries(jtag_test -lm ${SAFEC_LIBRARIES} -lasd-jtagintf -lpal)
    install (TARGETS jtag_test DESTINATION bin)
#endif(${BUILD_UT})
