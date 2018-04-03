# Copyright 2015-present Facebook. All Rights Reserved.
lib: libpal.so

libpal.so: pal.c
	$(CC) $(CFLAGS) -fPIC -c -pthread -o pal.o pal.c
	$(CC) -lbic -lminilaketb_common -lminilaketb_fruid -lminilaketb_sensor -lkv -ledb -shared -o libpal.so pal.o -lc $(LDFLAGS) -Wl,--whole-archive -lobmc-pal -Wl,--no-whole-archive

.PHONY: clean

clean:
	rm -rf *.o libpal.so
