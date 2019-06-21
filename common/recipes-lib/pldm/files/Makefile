# Copyright 2015-present Facebook. All Rights Reserved.
lib: libpldm.so

CFLAGS += -Wall -Werror

libpldm.so: pldm.o pldm_pmc.o
	$(CC) -shared -o libpldm.so pldm.o pldm_pmc.o -lc -lrt -lncsi $(LDFLAGS)

pldm.o: pldm.c
	$(CC) $(CFLAGS) -fPIC -c -o pldm.o pldm.c

pldm_pmc.o: pldm_pmc.c
	$(CC) $(CFLAGS) -fPIC -c -o pldm_pmc.o pldm_pmc.c

.PHONY: clean

clean:
	rm -rf *.o libpldm.so
