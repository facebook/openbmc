# Copyright 2015-present Facebook. All Rights Reserved.
lib: libpal.so

libpal.so: pal.c
	$(CC) $(CFLAGS) -fPIC -c -pthread -o pal.o pal.c
	$(CC) -lbic -lexp -lmctp -lfbttn_common -lfbttn_fruid -lfbttn_sensor -lkv -ledb -lnvme-mi -shared -o libpal.so pal.o -lc -Wl,--whole-archive -lobmc-pal -Wl,--no-whole-archive

.PHONY: clean

clean:
	rm -rf *.o libpal.so
