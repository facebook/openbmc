# Copyright 2015-present Facebook. All Rights Reserved.
all: guid-util

guid-util: guid-util.c
	$(CC) -lpal -std=c99 -o $@ $^ $(LDFLAGS)

.PHONY: clean

clean:
	rm -rf *.o guid-util
