#Copyright 2017-present Facebook. All Rights Reserved.

CPP_SRCS := $(wildcard *.cpp)
CXXFLAGS += -std=c++11 -Wall -Werror -g

all: fw-util

fw-util: $(CPP_SRCS)
	$(CXX) $(CXXFLAGS) -pthread -o $@ $^ $(LDFLAGS)

.PHONY: clean

clean:
	rm -rf *.o fw-util
