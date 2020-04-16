# Copyright 2015-present Facebook. All Rights Reserved.
all: emmcd

CFLAGS += -Wall -Werror

emmcd: emmcd.c
	$(CC) $(CFLAGS) -pthread -o $@ $^ $(LDFLAGS)

.PHONY: clean

clean:
	rm -rf *.o emmcd
