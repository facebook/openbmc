# Copyright 2015-present Facebook. All Rights Reserved.
lib: libipc.so

libipc.so: ipc.c
	$(CC) $(CFLAGS) -fPIC -c -o ipc.o ipc.c
	$(CC) -shared -o libipc.so ipc.o -lc -lrt $(LDFLAGS)

.PHONY: clean

clean:
	rm -rf *.o libipc.so
