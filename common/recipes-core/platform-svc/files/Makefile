#Copyright 2017-present Facebook. All Rights Reserved.

all: platform-svcd

platform-svcd:PlatformSvcd.cpp PlatformObjectTree.cpp PlatformJsonParser.cpp \
	SensorService.cpp DBusPlatformSvcInterface.cpp FruService.cpp DBusHPExtDectectionFruInterface.cpp
	$(CXX) $(CXXFLAGS) -pthread -std=c++11 -o $@ $^ -I$(SINC)/glib-2.0 -I$(SLIB)/glib-2.0/include \
	-lpthread -lgobject-2.0 -lobject-tree -lgflags -lglog -lgio-2.0 -lglib-2.0 -ldbus-utils

.PHONY: clean

clean:
	rm -rf *.o platform-svcd
