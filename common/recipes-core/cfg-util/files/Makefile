# Copyright 2015-present Facebook. All Rights Reserved.
all: cfg-util 


cfg-util: cfg-util.o
	$(CXX) $(CXXFLAGS) -o $@ $^ $(LDFLAGS)

.PHONY: clean

clean:
	rm -rf *.o cfg-util
