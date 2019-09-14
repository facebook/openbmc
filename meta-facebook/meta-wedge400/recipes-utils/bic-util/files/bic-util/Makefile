# Copyright 2019-present Facebook. All Rights Reserved.
all: bic-util

bic-util: bic-util.c
	$(CC) -lipmi -pthread $(CFLAGS) -std=c99 -o $@ $^ $(LDFLAGS)

.PHONY: clean

clean:
	rm -rf *.o bic-util
