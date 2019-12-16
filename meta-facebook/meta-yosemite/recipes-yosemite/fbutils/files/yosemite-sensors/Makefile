# Copyright 2015-present Facebook. All Rights Reserved.
all: yosemite-sensors

CFLAGS += -Wall -Werror

yosemite-sensors: yosemite-sensors.c
	$(CC) $(CFLAGS) -lyosemite_sensor -std=c99 -o $@ $^ $(LDFLAGS)

.PHONY: clean

clean:
	rm -rf *.o yosemite-sensors
