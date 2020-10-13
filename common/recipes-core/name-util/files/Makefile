# Copyright 2015-present Facebook. All Rights Reserved.
all: name-util

CFLAGS += -Wall -Werror

name-util: name-util.o
	$(CC) $(CFLAGS) -pthread -std=gnu11 -o $@ $^ $(LDFLAGS)

.PHONY: clean

clean:
	rm -rf *.o name-util
