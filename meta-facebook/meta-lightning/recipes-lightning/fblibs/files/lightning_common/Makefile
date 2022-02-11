# Copyright 2015-present Facebook. All Rights Reserved.
lib: liblightning_common.so
CFLAGS += -Wall -Werror

liblightning_common.so: lightning_common.c
	$(CC) $(CFLAGS) -fPIC -pthread -c -o lightning_common.o lightning_common.c
	$(CC) -lpthread -lkv -shared -o liblightning_common.so lightning_common.o -lc $(LDFLAGS)

.PHONY: clean

clean:
	rm -rf *.o liblightning_common.so
