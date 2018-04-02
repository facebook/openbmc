# Copyright 2015-present Facebook. All Rights Reserved.
lib: libfbttn_common.so

libfbttn_common.so: fbttn_common.c
	$(CC) $(CFLAGS) -fPIC -pthread -c -o fbttn_common.o fbttn_common.c
	$(CC) -lpthread -shared -o libfbttn_common.so fbttn_common.o -lc $(LDFLAGS)

.PHONY: clean

clean:
	rm -rf *.o libfbttn_common.so
