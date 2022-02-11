# Copyright 2020-present Facebook. All Rights Reserved.
all: front-paneld

CFLAGS += -Wall -Werror

front-paneld: front-paneld.c
	$(CC) $(CFLAGS) -std=gnu99 -pthread -lpal -lbic -lkv -lsdr -lobmc-i2c -o $@ $^ $(LDFLAGS)

.PHONY: clean

clean:
	rm -rf *.o front-paneld
