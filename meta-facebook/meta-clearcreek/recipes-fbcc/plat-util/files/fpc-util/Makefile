# Copyright 2015-present Facebook. All Rights Reserved.
all: fpc-util

CFLAGS += -Wall -Werror

fpc-util: fpc-util.c
	$(CC) $(CFLAGS) -lpal -std=c99 -o $@ $^ $(LDFLAGS)

.PHONY: clean

clean:
	rm -rf *.o fpc-util
