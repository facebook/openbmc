# Copyright 2018-present Facebook. All Rights Reserved.

all: sensord

CFLAGS += -Wall -Werror

sensord: sensord.c
	$(CC) -lpal -lbic -pthread -std=gnu99 -o $@ $^ $(LDFLAGS)

.PHONY: clean

clean:
	rm -rf *.o sensord