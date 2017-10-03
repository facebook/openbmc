# Copyright 2015-present Facebook. All Rights Reserved.
all: ncsi-util

ncsi-util: ncsi-util.c
	$(CC) -pthread $(CFLAGS)  -std=c99 -o $@ $^ $(LDFLAGS)

.PHONY: clean

clean:
	rm -rf *.o ncsi-util
