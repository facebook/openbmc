# Copyright 2015-present Facebook. All Rights Reserved.
all: fbttn-sensors 

fbttn-sensors: fbttn-sensors.c 
	$(CC) -lfbttn_sensor -std=c99 -o $@ $^ $(LDFLAGS)

.PHONY: clean

clean:
	rm -rf *.o fbttn-sensors
