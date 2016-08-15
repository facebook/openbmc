# Copyright 2015-present Facebook. All Rights Reserved.
all: ipmi-util

ipmi-util: ipmi-util.o 
	$(CC) $(CFLAGS) -pthread -lipmi --std=c99 -o $@ $^ $(LDFLAGS)

.PHONY: clean

clean:
	rm -rf *.o ipmi-util
