# Copyright 2015-present Facebook. All Rights Reserved.
all: snapshot-util

CFLAGS += -Wall -Werror

snapshot-util: snapshot-util.c
	$(CC) $(CFLAGS) -lbic -lpal -std=c99 -o $@ $^ $(LDFLAGS)

.PHONY: clean

clean:
	rm -rf *.o snapshot-util
