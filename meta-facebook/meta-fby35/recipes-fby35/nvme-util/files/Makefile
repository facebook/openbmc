# Copyright 2015-present Facebook. All Rights Reserved.
all: nvme-util

nvme-util: nvme-util.c
	$(CC) $(CFLAGS) -std=c99 -o $@ $^ $(LDFLAGS)

.PHONY: clean

clean:
	rm -rf *.o nvme-util
