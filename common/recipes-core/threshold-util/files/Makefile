# Copyright 2015-present Facebook. All Rights Reserved.
all: threshold-util 

CFLAGS += -Wall -Werror

threshold-util: threshold-util.o
	$(CC) $(CFLAGS) -lsdr -lpal -lrt -lm -std=gnu99 -o $@ $^ $(LDFLAGS)

.PHONY: clean

clean:
	rm -rf *.o threshold-util
