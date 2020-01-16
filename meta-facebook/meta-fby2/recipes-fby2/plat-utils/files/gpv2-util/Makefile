# Copyright 2015-present Facebook. All Rights Reserved.
all: gpv2-util

gpv2-util: gpv2-util.c
	$(CC) $(CFLAGS) -lbic -lfby2_sensor -lpal -std=c99 -o $@ $^ $(LDFLAGS)

.PHONY: clean

clean:
	rm -rf *.o gpv2-util
