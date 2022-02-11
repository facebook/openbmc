#Copyright 2017-present Facebook. All Rights Reserved.

all: fru-svcd

fru-svcd:FruSvcd.cpp FruObjectTree.cpp DBusFruInterface.cpp DBusFruServiceInterface.cpp FruIdAccessI2CEEPROM.cpp
	$(CXX) $(CXXFLAGS) -pthread -std=c++11 -o $@ $^ $(LDFLAGS) -I$(SINC)/glib-2.0 -I$(SLIB)/glib-2.0/include
.PHONY: clean

clean:
	rm -rf *.o fru-svcd
