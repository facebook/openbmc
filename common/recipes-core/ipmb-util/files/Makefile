# Copyright 2015-present Facebook. All Rights Reserved.
all: ipmb-util

CFLAGS += -Wall -Werror

ipmb-util: ipmb-util.o 
	$(CC) $(CFLAGS) -pthread -lipmi -lipmb --std=c99 -o $@ $^ $(LDFLAGS)

.PHONY: clean

clean:
	rm -rf *.o ipmb-util
