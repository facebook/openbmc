# Copyright 2015-present Facebook. All Rights Reserved.
all: yosemite-sensors 

yosemite-sensors: yosemite-sensors.c 
	$(CC) -lyosemite_sensor -std=c99 -o $@ $^ $(LDFLAGS)

.PHONY: clean

clean:
	rm -rf *.o yosemite-sensors
