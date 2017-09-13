# Copyright 2015-present Facebook. All Rights Reserved.
lib: libme.so

libme.so: me.c
	$(CC) $(CFLAGS) -fPIC -c -o me.o me.c
	$(CC) -lipmb -shared -o libme.so me.o -lc $(LDFLAGS)

.PHONY: clean

clean:
	rm -rf *.o libme.so
