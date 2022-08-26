cmake_minimum_required(VERSION 2.8.10 FATAL_ERROR)
project(at-scale-debug C)

find_package (PkgConfig REQUIRED)
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/sysroot/include)
pkg_check_modules (SAFEC REQUIRED libsafec)
include_directories (${SAFEC_INCLUDE_DIRS})
link_directories (${SAFEC_LIBRARY_DIRS})

# Set APB_FREQ to EXTRA_OECMAKE on bb recipe to modify JTAG frequency
if(${APB_FREQ})
    add_definitions( -DAPB_FREQ=${APB_FREQ} )
endif(${APB_FREQ})

if(${BUILD_UT})
    enable_testing()
    add_subdirectory(tests)
else()
    #add_executable(jtag_test jtag_test.c logging.c jtag_handler.c)
    #target_link_libraries(jtag_test -lm ${SAFEC_LIBRARIES})
    #install (TARGETS jtag_test DESTINATION bin)

    add_executable(asd asd_main.c ext_network.c authenticate.c
            session.c config.c asd_msg.c ext_tcp.c
            auth_none.c target_handler.c
            i2c_msg_builder.c i2c_handler.c i3c_handler.c
            vprobe_handler.c)
    target_link_libraries(asd -lm -lpthread -lasd-jtagintf -lpal ${SAFEC_LIBRARIES})
    install (TARGETS asd DESTINATION bin)

    # Copy pam-asd to etc/pam.d
    #install (
    #    FILES ${CMAKE_CURRENT_SOURCE_DIR}/pam-asd
    #    DESTINATION /etc/pam.d/
    #    RENAME asd
    #)
endif(${BUILD_UT})

