# Copyright 2017-present Facebook. All Rights Reserved.
lib: libmcu.so
CFLAGS += -Wall -Werror
libmcu.so: mcu.c
	$(CC) $(CFLAGS) -fPIC -c -o mcu.o mcu.c
	$(CC) -shared -o libmcu.so mcu.o -lc $(LDFLAGS)

.PHONY: clean

clean:
	rm -rf *.o libmcu.so
