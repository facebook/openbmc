# Copyright 2015-present Facebook. All Rights Reserved.

CFLAGS += -Wall -Werror

libast-jtag.so: ast-jtag.c
	$(CC) $(CFLAGS) -fPIC -c -pthread ast-jtag.c
	$(CC) -shared -o libast-jtag.so ast-jtag.o -lc $(LDFLAGS)

.PHONY: clean

clean:
	rm -rf *.o libast-jtag.so
