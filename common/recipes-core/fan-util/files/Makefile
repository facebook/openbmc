# Copyright 2016-present Facebook. All Rights Reserved.
all: fan-util

fan-util: fan-util.o
	$(CXX) $(CXXFLAGS) -std=c++11 -lpal -lrt -lm -o $@ $^ $(LDFLAGS) 

.PHONY: clean

clean:
	rm -rf *.o fan-util
