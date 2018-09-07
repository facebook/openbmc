# Copyright 2015-present Facebook. All Rights Reserved.
lib: libncsi.so

CFLAGS += -Wall -Werror

libncsi.so: ncsi.o aen.o
	$(CC) -shared -o libncsi.so ncsi.o aen.o -lc -lrt $(LDFLAGS)

ncsi.o: ncsi.c
	$(CC) $(CFLAGS) -fPIC -c -o ncsi.o ncsi.c

aen.o: aen.c
	$(CC) $(CFLAGS) -fPIC -c -o aen.o aen.c

.PHONY: clean

clean:
	rm -rf *.o libncsi.so
