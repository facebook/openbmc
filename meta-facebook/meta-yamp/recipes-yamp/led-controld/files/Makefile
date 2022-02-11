# Copyright 2015-present Facebook. All Rights Reserved.
all: led-controld

CFLAGS += -Wall -Werror

led-controld: led-controld.c
	$(CC) $(CFLAGS) -std=gnu99 -o $@ $^ $(LDFLAGS)

.PHONY: clean

clean:
	rm -rf *.o led-controld
