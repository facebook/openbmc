# Copyright 2015-present Facebook. All Rights Reserved.
all: name-util

CXXFLAGS += -Wall -Werror

name-util: name-util.o
	$(CXX) $(CFLAGS) -pthread -std=gnu11 -o $@ $^ $(LDFLAGS)

.PHONY: clean

clean:
	rm -rf *.o name-util
