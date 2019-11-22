# Copyright 2015-present Facebook. All Rights Reserved.
lib: libbic.so

CFLAGS += -Wall -Werror

libbic.so: bic.c
	$(CC) $(CFLAGS) -fPIC -c -o bic.o bic.c
	$(CC) $(CFLAGS) -fPIC -c -o bic_platform.o bic_platform.c
	$(CC) -lipmb -lkv -shared -o libbic.so bic.o bic_platform.o -lc $(LDFLAGS)

.PHONY: clean

clean:
	rm -rf *.o libbic.so
