# Copyright 2015-present Facebook. All Rights Reserved.
lib: liblightning_sensor.so

CFLAGS += -Wall -Werror
liblightning_sensor.so: lightning_sensor.c
	$(CC) $(CFLAGS) -fPIC -c -o lightning_sensor.o lightning_sensor.c
	$(CC) -lm -lipmi -llightning_common -llightning_flash -llightning_gpio -shared -o liblightning_sensor.so lightning_sensor.o -lc $(LDFLAGS)

.PHONY: clean

clean:
	rm -rf *.o liblightning_sensor.so
