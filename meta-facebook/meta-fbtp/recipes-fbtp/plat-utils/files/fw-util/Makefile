# Copyright 2015-present Facebook. All Rights Reserved.
all: fw-util

CFLAGS += -Wall -Werror

fw-util: fw-util.c
	$(CC) $(CFLAGS) -pthread -lme -lgpio -lpal -lcpld -lbios -lvr -locpdbg-lcd -ljansson -std=c99 -o $@ $^ $(LDFLAGS)

.PHONY: clean

clean:
	rm -rf *.o fw-util
