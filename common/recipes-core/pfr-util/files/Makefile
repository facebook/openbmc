# Copyright 2015-present Facebook. All Rights Reserved.
all: pfr-util

CFLAGS += -Wall -Werror

pfr-util: pfr-util.c
	$(CC) $(CFLAGS) -std=c99 -o $@ $^ $(LDFLAGS)

.PHONY: clean

clean:
	rm -rf *.o pfr-util
