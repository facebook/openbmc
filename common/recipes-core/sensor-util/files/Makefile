# Copyright 2015-present Facebook. All Rights Reserved.
all: sensor-util 

CFLAGS += -Wall -Werror

sensor-util: sensor-util.o
	$(CC) $(CFLAGS) -lsdr -lpal -laggregate-sensor -pthread -lrt -lm -std=gnu99 -o $@ $^ $(LDFLAGS)

.PHONY: clean

clean:
	rm -rf *.o sensor-util
