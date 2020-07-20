#Copyright 2017-present Facebook. All Rights Reserved.

CPP_SRCS := $(wildcard *.cpp)
TEST_SRCS := $(wildcard tests/*.cpp)
CPP_OBJS := ${CPP_SRCS:.cpp=.o}
TEST_OBJS := ${TEST_SRCS:.cpp=.o}
TEST_CPP_OBJS := ${CPP_SRCS:.cpp=.test.o}

CXXFLAGS += -std=c++17 -Wall -Werror -I.

all: fw-util

fw-util: $(CPP_OBJS)
	$(CXX) -o $@ $^ $(LDFLAGS)

fw-util-test: $(TEST_CPP_OBJS) $(TEST_OBJS)
	$(CXX) -o $@ $^ $(LDFLAGS) -g -lgtest -lgmock

$(CPP_SRCS:.cpp=.d):%.d:%.cpp
	$(CXX) $(CXXFLAGS) $< >$@

$(TEST_SRCS:.cpp=.d):%.d:%.cpp
	$(CXX) $(CXXFLAGS) -D__TEST__ -g $< >$@

$(TEST_CPP_OBJS):%.test.o:%.cpp
	$(CXX) $(CXXFLAGS) -c -D__TEST__ -g $< -o $@

.PHONY: clean

clean:
	rm -rf *.o tests/*.o fw-util fw-util-test
