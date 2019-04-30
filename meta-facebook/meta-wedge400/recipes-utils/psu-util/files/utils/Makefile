# Copyright 2019-present Facebook. All Rights Reserved.

all: psu-util

CFLAGS += -Wall -Werror

psu-util: psu-util.o
	$(CC) $(CFLAGS) -std=gnu99 -o $@ $^ $(LDFLAGS)

.PHONY: clean

clean:
	rm -rf *.o psu-util
