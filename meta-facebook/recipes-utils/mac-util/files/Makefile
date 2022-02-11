# Copyright 2020-present Facebook. All Rights Reserved.
all: mac-util

CXXFLAGS += -Wall -Werror -std=c++17

mac-util: mac-util.cpp
	$(CXX) $(CXXFLAGS) -o $@ $^ $(LDFLAGS)

.PHONY: clean

clean:
	rm -rf *.o mac-util
