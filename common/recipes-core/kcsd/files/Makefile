# Copyright 2015-present Facebook. All Rights Reserved.
all: kcsd

CFLAGS += -Wall -Werror -std=gnu99
LDFLAGS += -pthread -lipmi -lgpio-ctrl

kcsd: kcsd.o
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS)

.PHONY: clean

clean:
	rm -rf *.o kcsd
