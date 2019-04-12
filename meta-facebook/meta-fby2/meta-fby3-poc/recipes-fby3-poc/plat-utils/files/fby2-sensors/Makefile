# Copyright 2015-present Facebook. All Rights Reserved.
all: fby2-sensors

fby2-sensors: fby2-sensors.c
	$(CC) -lfby2_common -lfby2_sensor -std=c99 -o $@ $^ $(LDFLAGS)

.PHONY: clean

clean:
	rm -rf *.o fby2-sensors
