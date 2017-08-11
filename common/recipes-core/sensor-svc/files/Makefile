#Copyright 2017-present Facebook. All Rights Reserved.

all: sensor-svcd

sensor-svcd:SensorSvcd.cpp SensorObjectTree.cpp Sensor.cpp SensorJsonParser.cpp \
	SensorAccessViaPath.cpp DBusSensorInterface.cpp DBusSensorTreeInterface.cpp \
	SensorAccessMechanism.cpp SensorAccessAVA.cpp SensorAccessINA230.cpp \
	DBusSensorServiceInterface.cpp SensorAccessNVME.cpp SensorAccessVR.cpp FRU.cpp
	$(CXX) $(CXXFLAGS) -pthread -std=c++11 -o $@ $^ \
	$(LDFLAGS) -I$(SINC)/glib-2.0 -I$(SLIB)/glib-2.0/include
.PHONY: clean

clean:
	rm -rf *.o sensor-svcd
