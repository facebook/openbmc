# Copyright 2015-present Facebook. All Rights Reserved.
all: thread-benchmark

CFLAGS += -Wall -Werror

thread-benchmark: thread-benchmark.o
	$(CC) $(CFLAGS) -pthread -lrt -lm -std=gnu99 -o $@ $^ $(LDFLAGS)

.PHONY: clean

clean:
	rm -rf *.o thread-benchmark
