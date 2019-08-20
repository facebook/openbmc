# Copyright 2015-present Facebook. All Rights Reserved.

CFLAGS += -Wall -Werror

liblattice.so: cpld.c lattice.c
	$(CC) $(CFLAGS) -fPIC -c -pthread cpld.c lattice.c
	$(CC) -shared -o $@ cpld.o lattice.o -lc $(LDFLAGS)

.PHONY: clean

clean:
	rm -rf *.o liblattice.so
