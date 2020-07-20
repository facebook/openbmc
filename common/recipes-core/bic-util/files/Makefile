# Copyright 2015-present Facebook. All Rights Reserved.

all: bic-util

CFLAGS += -Wall -Werror

bic-util: bic-util.c
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS)

.PHONY: clean

clean:
	rm -rf *.o bic-util
