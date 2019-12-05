# Copyright 2015-present Facebook. All Rights Reserved.

CFLAGS += -Wall -Werror

libast-jtag.so: ast-jtag.c ast-jtag-intf.c ast-jtag-legacy.c
	$(CC) $(CFLAGS) -fPIC -c -pthread ast-jtag.c ast-jtag-intf.c ast-jtag-legacy.c
	$(CC) -shared -o libast-jtag.so ast-jtag.o ast-jtag-intf.o ast-jtag-legacy.o -lc $(LDFLAGS)

.PHONY: clean

clean:
	rm -rf *.o libast-jtag.so
