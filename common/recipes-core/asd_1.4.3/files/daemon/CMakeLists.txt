cmake_minimum_required(VERSION 2.8.10 FATAL_ERROR)
project(at-scale-debug C)

set(CMAKE_VERBOSE_MAKEFILE ON)

include_directories(${CMAKE_CURRENT_SOURCE_DIR}/sysroot/include)

# Set APB_FREQ to EXTRA_OECMAKE on bb recipe to modify JTAG frequency
if(${APB_FREQ})
    add_definitions( -DAPB_FREQ=${APB_FREQ} )
endif(${APB_FREQ})

if(${BUILD_UT})
    enable_testing()
    add_subdirectory(tests)
else()
    #add_executable(asd-test jtag_test.c logging.c jtag_handler.c mem_helper.c)
    #target_link_libraries(asd-test -lm -lpal)
    #install (TARGETS asd-test DESTINATION bin)

    add_executable(asd asd_main.c ext_network.c authenticate.c pin_handler.c 
            session.c logging.c config.c jtag_handler.c asd_msg.c ext_tcp.c
            auth_none.c target_handler.c i2c_msg_builder.c i2c_handler.c mem_helper.c)
    target_link_libraries(asd -lm -lpthread -lpal)
    install (TARGETS asd DESTINATION bin)

    # Copy pam-asd to etc/pam.d
    #install (
    #    FILES ${CMAKE_CURRENT_SOURCE_DIR}/pam-asd
    #    DESTINATION /etc/pam.d/
    #    RENAME asd
    #)
endif(${BUILD_UT})

