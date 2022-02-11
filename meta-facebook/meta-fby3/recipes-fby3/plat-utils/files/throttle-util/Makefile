# Copyright 2015-present Facebook. All Rights Reserved.
all: throttle-util

CFLAGS += -Wall -Werror

throttle-util: throttle-util.c
	$(CC) -pthread $(CFLAGS) -std=c99 -o $@ $^ $(LDFLAGS)

.PHONY: clean

clean:
	rm -rf *.o throttle-util
