# Copyright 2019-present Facebook. All Rights Reserved.
lib: libnl-wrapper.so

CFLAGS += -Wall -Werror

libnl-wrapper.so: nl-wrapper.c
	$(CC) $(CFLAGS) -fPIC -c -o nl-wrapper.o nl-wrapper.c
	$(CC) -shared -o libnl-wrapper.so nl-wrapper.o -lc $(LDFLAGS)

.PHONY: clean

clean:
	rm -rf *.o libnl-wrapper.so
