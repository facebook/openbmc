# Copyright 2014-present Facebook. All Rights Reserved.
all: fand

fand: fand.cpp watchdog.cpp
	$(CXX) -pthread -o $@ $^ $(LDFLAGS) -lwedge_eeprom

.PHONY: clean

clean:
	rm -rf *.o fand
