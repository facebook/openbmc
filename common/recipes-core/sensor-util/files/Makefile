# Copyright 2015-present Facebook. All Rights Reserved.
all: sensor-util 


sensor-util: sensor-util.o
	$(CC) $(CFLAGS) -lsdr -lpal -lobmc-sensor -lrt -lm -std=gnu99 -o $@ $^ $(LDFLAGS)

.PHONY: clean

clean:
	rm -rf *.o sensor-util
