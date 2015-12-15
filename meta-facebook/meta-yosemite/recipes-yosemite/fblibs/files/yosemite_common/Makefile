# Copyright 2015-present Facebook. All Rights Reserved.
lib: libyosemite_common.so

libyosemite_common.so: yosemite_common.c
	$(CC) $(CFLAGS) -fPIC -pthread -c -o yosemite_common.o yosemite_common.c
	$(CC) -lpthread -shared -o libyosemite_common.so yosemite_common.o -lc

.PHONY: clean

clean:
	rm -rf *.o libyosemite_common.so
