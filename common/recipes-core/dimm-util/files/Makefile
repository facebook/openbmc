# Copyright 2015-present Facebook. All Rights Reserved.
all: dimm-util

CFLAGS += -Wall -Werror

dimm-util: *.cpp
	$(CXX) -pthread $(CXXFLAGS)  -o $@ $^ $(LDFLAGS)

.PHONY: clean

clean:
	rm -rf *.o dimm-util
