# Copyright 2015-present Facebook. All Rights Reserved.
lib: libfby2_common.so

libfby2_common.so: fby2_common.c
	$(CC) $(CFLAGS) -fPIC -pthread -c -o fby2_common.o fby2_common.c
	$(CC) -lpthread -shared -o libfby2_common.so fby2_common.o -lc $(LDFLAGS)

.PHONY: clean

clean:
	rm -rf *.o libfby2_common.so
