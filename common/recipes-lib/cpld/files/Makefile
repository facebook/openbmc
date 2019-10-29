# Copyright 2015-present Facebook. All Rights Reserved.

CFLAGS += -Wall -Werror

libfpga.so: cpld.c lattice.c altera.c
	$(CC) $(CFLAGS) -fPIC -c -pthread cpld.c lattice.c altera.c
	$(CC) -shared -o $@ cpld.o lattice.o altera.o -lc $(LDFLAGS)

.PHONY: clean

clean:
	rm -rf *.o libfpga.so
