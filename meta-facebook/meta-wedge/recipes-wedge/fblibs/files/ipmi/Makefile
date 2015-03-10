# Copyright 2014-present Facebook. All Rights Reserved.
lib: libipmi.so

libipmi.so: ipmi.c
	$(CC) $(CCFLAGS) -fPIC -c -o ipmi.o ipmi.c
	$(CC) -shared -o libipmi.so ipmi.o -lc

.PHONY: clean

clean:
	rm -rf *.o libipmi.so
