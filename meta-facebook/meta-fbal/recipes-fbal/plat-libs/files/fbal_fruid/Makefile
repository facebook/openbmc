# Copyright 2018-present Facebook. All Rights Reserved.
lib: libfbal-fruid.so

CFLAGS += -Wall -Werror

libfbal-fruid.so: fbal_fruid.c
	$(CC) $(CFLAGS) -fPIC -c -o fbal_fruid.o fbal_fruid.c
	$(CC) -shared -o libfbal-fruid.so fbal_fruid.o -lc $(LDFLAGS)

.PHONY: clean

clean:
	rm -rf *.o libfbal-fruid.so
