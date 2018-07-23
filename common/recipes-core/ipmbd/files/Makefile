# Copyright 2015-present Facebook. All Rights Reserved.
all: ipmbd

CFLAGS += -Wall -Werror

ipmbd: ipmbd.o
	$(CC) $(CFLAGS) -pthread -lrt -lipmi -lpal -lipc -std=gnu99 -o $@ $^ $(LDFLAGS)

.PHONY: clean

clean:
	rm -rf *.o ipmbd
