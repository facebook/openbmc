# Copyright 2015-present Facebook. All Rights Reserved.
all: nvme-util
CFLAGS += -Wall -Werror -D_XOPEN_SOURCE

nvme-util: nvme-util.c
	$(CC) $(CFLAGS) -lbic -lfby2_sensor -lpal -std=c99 -o $@ $^ $(LDFLAGS)

.PHONY: clean

clean:
	rm -rf *.o nvme-util
