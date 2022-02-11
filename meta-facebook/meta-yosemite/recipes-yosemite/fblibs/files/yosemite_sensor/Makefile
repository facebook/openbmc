# Copyright 2015-present Facebook. All Rights Reserved.
lib: libyosemite_sensor.so

libyosemite_sensor.so: yosemite_sensor.c
	$(CC) $(CFLAGS) -fPIC -c -o yosemite_sensor.o yosemite_sensor.c
	$(CC) -lm -lrt -lbic -lipmi -lipmb -lyosemite_common -shared -o libyosemite_sensor.so yosemite_sensor.o -lc  $(LDFLAGS)

.PHONY: clean

clean:
	rm -rf *.o libyosemite_sensor.so
