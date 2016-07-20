cmake_minimum_required(VERSION 2.8)
project(object-tree)

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

find_library(GTEST gtest)
find_library(GLOG glog)

add_library(object-tree
  ObjectTree.cpp
  Object.cpp
  Attribute.cpp
)

target_link_libraries(object-tree
  ${GLOG}
  ${GTEST}
  -lpthread
)

install(TARGETS object-tree DESTINATION lib)

install(FILES
  ObjectTree.h
  Object.h
  Attribute.h
  DESTINATION include/object-tree
)

option(BUILD_TESTS "BUILD_TESTS" ON)

if(BUILD_TESTS)
  enable_testing()

  add_executable(attribute-test
    tests/AttributeTest.cpp
    Attribute.cpp
  )

  target_link_libraries(attribute-test
    ${GLOG}
    ${GTEST}
    -lpthread
  )

  add_test(AttributeTest
    attribute-test
  )

  install(TARGETS attribute-test DESTINATION bin)

  add_executable(object-test
    tests/ObjectTest.cpp
    Object.cpp
    Attribute.cpp
  )

  target_link_libraries(object-test
    ${GLOG}
    ${GTEST}
    -lpthread
  )

  add_test(ObjectTest
    object-test
  )

  install(TARGETS object-test DESTINATION bin)

  add_executable(object-tree-test
    tests/ObjectTreeTest.cpp
    ObjectTree.cpp
    Object.cpp
    Attribute.cpp
  )

  target_link_libraries(object-tree-test
    ${GLOG}
    ${GTEST}
    -lpthread
  )

  add_test(ObjectTreeTest
    object-tree-test
  )

  install(TARGETS object-tree-test DESTINATION bin)
endif ()

