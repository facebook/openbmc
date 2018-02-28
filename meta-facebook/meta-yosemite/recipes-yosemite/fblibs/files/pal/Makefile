# Copyright 2015-present Facebook. All Rights Reserved.
lib: libpal.so

CFLAGS += -Wall -Werror

libpal.so: pal.c
	$(CC) $(CFLAGS) -fPIC -c -pthread -o pal.o pal.c
	$(CC) -lbic -lyosemite_common -lyosemite_fruid -lyosemite_sensor -lkv -ledb -shared -o libpal.so pal.o -lc $(LDFLAGS) -lrt -Wl,--whole-archive -lobmc-pal -Wl,--no-whole-archive

.PHONY: clean

clean:
	rm -rf *.o libpal.so
