# Copyright 2015-present Facebook. All Rights Reserved.
all: cm-util

CFLAGS += -Wall -Werror

cm-util: cm-util.c
	$(CC) $(CFLAGS) -lpal -std=c99 -o $@ $^ $(LDFLAGS)

.PHONY: clean

clean:
	rm -rf *.o cm-util
