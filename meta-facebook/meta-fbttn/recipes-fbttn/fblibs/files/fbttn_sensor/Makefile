# Copyright 2015-present Facebook. All Rights Reserved.
lib: libfbttn_sensor.so

libfbttn_sensor.so: fbttn_sensor.c
	$(CC) $(CFLAGS) -fPIC -c -o fbttn_sensor.o fbttn_sensor.c
	$(CC) -lm -lbic -lmctp -lipmi -lipmb -lfbttn_common -shared -o libfbttn_sensor.so fbttn_sensor.o -lc $(LDFLAGS)

.PHONY: clean

clean:
	rm -rf *.o libfbttn_sensor.so
