# Copyright 2021-present Facebook. All Rights Reserved.
all: front-paneld

CFLAGS += -Wall -Werror

front-paneld: front-paneld.c
	$(CC) $(CFLAGS) -std=gnu99 -pthread -lpal -lkv -lgpio-ctrl -lsdr -lobmc-i2c -o $@ $^ $(LDFLAGS)

.PHONY: clean

clean:
	rm -rf *.o front-paneld
