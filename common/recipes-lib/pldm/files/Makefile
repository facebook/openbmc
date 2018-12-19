# Copyright 2015-present Facebook. All Rights Reserved.
lib: libpldm.so

CFLAGS += -Wall -Werror

libpldm.so: pldm.o
	$(CC) -shared -o libpldm.so pldm.o -lc -lrt -lncsi $(LDFLAGS)

pldm.o: pldm.c
	$(CC) $(CFLAGS) -fPIC -c -o pldm.o pldm.c

.PHONY: clean

clean:
	rm -rf *.o libpldm.so
