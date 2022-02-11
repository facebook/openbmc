# Copyright 2015-present Facebook. All Rights Reserved.
lib: libfby35_common.so

CFLAGS += -Wall -Werror

libfby35_common.so: fby35_common.c
	$(CC) $(CFLAGS) -pthread -c -o fby35_common.o fby35_common.c
	$(CC) -lpthread -shared -o libfby35_common.so fby35_common.o -lc $(LDFLAGS)

.PHONY: clean

clean:
	rm -rf *.o libfby35_common.so
