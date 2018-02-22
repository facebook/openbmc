# Copyright 2015-present Facebook. All Rights Reserved.
all: me-util

CFLAGS += -Wall -Werror

me-util: me-util.o 
	$(CC) $(CFLAGS) -pthread -lbic -lpal -std=c99 -o $@ $^ $(LDFLAGS)

.PHONY: clean

clean:
	rm -rf *.o me-util
