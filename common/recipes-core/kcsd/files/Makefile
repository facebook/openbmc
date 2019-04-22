# Copyright 2015-present Facebook. All Rights Reserved.
all: kcsd

CFLAGS += -Wall -Werror

kcsd: kcsd.o
	$(CC) $(CFLAGS) -pthread -lrt -lipmi -lgpio-ctrl -std=gnu99 -o $@ $^ $(LDFLAGS)

.PHONY: clean

clean:
	rm -rf *.o kcsd
