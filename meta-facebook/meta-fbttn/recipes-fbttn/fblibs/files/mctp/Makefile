# Copyright 2015-present Facebook. All Rights Reserved.
lib: libmctp.so

libmctp.so: mctp.c
	$(CC) $(CFLAGS) -fPIC -c -o mctp.o mctp.c
	$(CC) -shared -o libmctp.so mctp.o -lc $(LDFLAGS)

.PHONY: clean

clean:
	rm -rf *.o libmctp.so
