# Copyright 2020-present Facebook. All Rights Reserved.
all: asic-util

CXXFLAGS += -Wall -Werror -std=c++17

asic-util: asic-util.cpp
	$(CXX) $(CXXFLAGS) -o $@ $^ $(LDFLAGS)

.PHONY: clean

clean:
	rm -rf *.o asic-util
