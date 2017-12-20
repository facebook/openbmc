# Copyright 2015-present Facebook. All Rights Reserved.
lib: libcpld.so

libcpld.so: cpld.c lattice.c ast-jtag.c
	$(CC) $(CFLAGS) -fPIC -c -pthread cpld.c lattice.c ast-jtag.c
	$(CC) -shared -o libcpld.so cpld.o lattice.o ast-jtag.o -lc $(LDFLAGS)

.PHONY: clean

clean:
	rm -rf *.o libcpld.so
