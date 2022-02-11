# Copyright 2015-present Facebook. All Rights Reserved.
all: libvbs.so

CFLAGS += -Wall -Werror

libvbs.so: vbs.c
	$(CC) $(CFLAGS) -fPIC -c -o vbs.o vbs.c
	$(CC) -shared -o libvbs.so vbs.o -lc $(LDFLAGS)

.PHONY: clean

clean:
	rm -rf *.o libvbs.so
