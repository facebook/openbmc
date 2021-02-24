# Copyright 2015-present Facebook. All Rights Reserved.

CPP_SRCS := $(wildcard *.cpp)
CPP_OBJS := ${CPP_SRCS:.cpp=.o}

all: mctp-util

CXXFLAGS += -std=c++17 -Wall -Werror

mctp-util: $(CPP_OBJS)
	$(CXX) -pthread $(CXXFLAGS) -std=gnu11 -o $@ $^ $(LDFLAGS)

.PHONY: clean

clean:
	rm -rf *.o mctp-util
