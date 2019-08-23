# Copyright 2015-present Facebook. All Rights Reserved.
all: ncsi-util

CFLAGS += -Wall -Werror

ncsi-util: ncsi-util.c libnl-wrapper.c
	$(CC) -pthread $(CFLAGS)  -std=gnu11 -o $@ $^ $(LDFLAGS)

.PHONY: clean

clean:
	rm -rf *.o ncsi-util
