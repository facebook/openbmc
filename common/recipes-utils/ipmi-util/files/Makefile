# Copyright 2015-present Facebook. All Rights Reserved.
all: ipmi-util

CFLAGS += -Wall -Werror

ipmi-util: ipmi-util.o
	$(CC) $(CFLAGS) -pthread -lipmi -lpal --std=c99 -o $@ $^ $(LDFLAGS)

.PHONY: clean

clean:
	rm -rf *.o ipmi-util
