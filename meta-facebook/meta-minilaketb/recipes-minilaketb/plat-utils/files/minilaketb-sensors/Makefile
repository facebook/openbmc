# Copyright 2015-present Facebook. All Rights Reserved.
all: minilaketb-sensors

minilaketb-sensors: minilaketb-sensors.c
	$(CC) -lminilaketb_sensor -std=c99 -o $@ $^ $(LDFLAGS)

.PHONY: clean

clean:
	rm -rf *.o minilaketb-sensors
