# Copyright 2017-present Facebook. All Rights Reserved.
lib: libvr.so

CFLAGS += -Wall -Werror

libvr.so: vr.c
	$(CC) $(CFLAGS) -fPIC -c -pthread vr.c
	$(CC) -lkv -shared -o libvr.so vr.o -lc $(LDFLAGS)

.PHONY: clean

clean:
	rm -rf *.o libvr.so
