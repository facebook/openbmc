# Copyright 2015-present Facebook. All Rights Reserved.


lib: libphymem.so

CFLAGS += -Wall -Werror

libphymem.so: phymem.c
	$(CC) $(CFLAGS) -fPIC -c -o phymem.o phymem.c
	$(CC) -shared -o libphymem.so phymem.o -lc $(LDFLAGS)

.PHONY: clean

clean:
	rm -rf *.o libphymem.so
