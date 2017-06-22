# Copyright 2017-present Facebook. All Rights Reserved.
all: fw-util

fw-util: fw-util.c 
	$(CC) -lpal -std=c99 -o $@ $^

.PHONY: clean

clean:
	rm -rf *.o fw-util
