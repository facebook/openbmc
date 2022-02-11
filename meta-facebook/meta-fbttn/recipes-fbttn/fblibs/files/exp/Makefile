# Copyright 2015-present Facebook. All Rights Reserved.
lib: libexp.so

libexp.so: exp.c
	$(CC) $(CFLAGS) -fPIC -c -o exp.o exp.c
	$(CC) -lipmb -shared -o libexp.so exp.o -lc $(LDFLAGS)

.PHONY: clean

clean:
	rm -rf *.o libexp.so
