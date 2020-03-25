# Copyright 2015-present Facebook. All Rights Reserved.
all: vboot-util

CFLAGS += -Wall -Werror

vboot-util: vboot-util.o
	$(CC) $(CFLAGS) -std=gnu99 -o $@ $^ -lkv -lvbs $(LDFLAGS)

.PHONY: clean

clean:
	rm -rf *.o vboot-util
