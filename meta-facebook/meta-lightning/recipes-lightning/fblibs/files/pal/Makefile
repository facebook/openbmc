#Copyright 2015-present Facebook. All Rights Reserved.
lib: libpal.so

libpal.so: pal.c
	$(CC) $(CFLAGS) -fPIC -c -pthread -o pal.o pal.c
	$(CC) -llightning_common -llightning_fruid -llightning_sensor -llightning_flash -lkv -ledb -shared -o libpal.so pal.o -lc

.PHONY: clean

clean:
	rm -rf *.o libpal.so
