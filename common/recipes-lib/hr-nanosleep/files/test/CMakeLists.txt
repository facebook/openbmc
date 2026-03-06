cmake_minimum_required(VERSION 3.8)
project(hr_nanosleep_test)

add_executable(hr_nanosleep_test
    hr_nanosleep_test.c
)

install(TARGETS hr_nanosleep_test DESTINATION bin)
