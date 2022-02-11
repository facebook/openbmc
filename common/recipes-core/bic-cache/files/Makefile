# Copyright 2015-present Facebook. All Rights Reserved.
all: bic-cache

CFLAGS += -Wall -Werror

bic-cache: bic-cache.c
	$(CC) $(CFLAGS) -pthread -std=c99 -o $@ $^ $(LDFLAGS)

.PHONY: clean

clean:
	rm -rf *.o bic-cache
