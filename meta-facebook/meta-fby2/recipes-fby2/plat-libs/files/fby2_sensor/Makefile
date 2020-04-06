# Copyright 2015-present Facebook. All Rights Reserved.
lib: libfby2_sensor.so

CFLAGS += -Wall -Werror

libfby2_sensor.so: fby2_sensor.c
	$(CC) $(CFLAGS) -fPIC -c -o fby2_sensor.o fby2_sensor.c
	$(CC) -lm -lrt -lbic -lipmi -lipmb -lfby2_common -lnvme-mi -shared -o libfby2_sensor.so fby2_sensor.o -lc $(LDFLAGS)

.PHONY: clean

clean:
	rm -rf *.o libfby2_sensor.so
