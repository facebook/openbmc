# Copyright 2015-present Facebook. All Rights Reserved.
all: power-util

CFLAGS += -Wall -Werror

power-util: power-util.o
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS)

.PHONY: clean

clean:
	rm -rf *.o power-util
