# Copyright 2017-present Facebook. All Rights Reserved.
lib: libobmc-sensor.so

libobmc-sensor.so: obmc-sensor.c
	$(CC) $(CFLAGS) -Werror -fPIC -c -o obmc-sensor.o obmc-sensor.c
	$(CC) -lpal -ledb -shared -o libobmc-sensor.so obmc-sensor.o -lc -lrt

.PHONY: clean

clean:
	rm -rf *.o libobmc-sensor.so
