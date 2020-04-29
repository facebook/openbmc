# Copyright 2015-present Facebook. All Rights Reserved.
all: cm-util

CXXFLAGS += -Wall -Werror

cm-util: cm-util.cpp
	$(CXX) $(CXXFLAGS) -lpal -o $@ $^ $(LDFLAGS)

.PHONY: clean

clean:
	rm -rf *.o cm-util
