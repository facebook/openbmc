# Copyright 2015-present Facebook. All Rights Reserved.
lib: libpal.so

libpal.so: pal.c
	$(CC) $(CFLAGS) -fPIC -c -pthread -o pal.o pal.c
	$(CC) -lbic -lyosemite_common -lyosemite_fruid -lyosemite_sensor -lkv -ledb -shared -o libpal.so pal.o -lc

.PHONY: clean

clean:
	rm -rf *.o libpal.so
