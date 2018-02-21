# Copyright 2015-present Facebook. All Rights Reserved.
all: fw-util

CFLAGS += -Wall -Werror

fw-util: fw-util.c 
	$(CC) -pthread -lbic -lpal -std=c99 -o $@ $^ $(LDFLAGS)

.PHONY: clean

clean:
	rm -rf *.o fw-util
