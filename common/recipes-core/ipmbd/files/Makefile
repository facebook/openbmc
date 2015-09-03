# Copyright 2015-present Facebook. All Rights Reserved.
all: ipmbd

ipmbd: ipmbd.o
	$(CC) $(CFLAGS) -pthread -lrt -lipmi -std=gnu99 -o $@ $^ $(LDFLAGS)

.PHONY: clean

clean:
	rm -rf *.o ipmbd
