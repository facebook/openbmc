# Copyright 2015-present Facebook. All Rights Reserved.
lib: libfby3_common.so

CFLAGS += -Wall -Werror

libfby3_common.so: fby3_common.c
	$(CC) $(CFLAGS) -pthread -c -o fby3_common.o fby3_common.c
	$(CC) -lpthread -shared -o libfby3_common.so fby3_common.o -lc $(LDFLAGS)

.PHONY: clean

clean:
	rm -rf *.o libfby3_common.so
