# Copyright 2018-present Facebook. All Rights Reserved.

CFLAGS += -Wall -Werror

all: cpldupdate-i2c

cpldupdate-i2c: cpldupdate-i2c.o
	$(CC) $(CFLAGS) -std=gnu99 -o $@ $^ $(LDFLAGS)

.PHONY: clean

clean:
	rm -rf *.o cpldupdate-i2c
