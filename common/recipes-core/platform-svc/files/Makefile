#Copyright 2017-present Facebook. All Rights Reserved.

all: platform-svcd

platform-svcd:PlatformSvcd.cpp PlatformObjectTree.cpp PlatformJsonParser.cpp SensorService.cpp
	$(CXX) $(CXXFLAGS) -pthread -std=c++11 -o $@ $^ $(LDFLAGS) -I$(SINC)/glib-2.0 -I$(SLIB)/glib-2.0/include
.PHONY: clean

clean:
	rm -rf *.o platform-svcd
