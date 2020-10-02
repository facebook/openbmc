# Copyright 2015-present Facebook. All Rights Reserved.
all: sensor-history

CFLAGS += -Wall -Werror

sensor-history: sensor-history.o
	$(CC) $(CFLAGS) -lpal -pthread -lrt -lm -std=gnu99 -o $@ $^ $(LDFLAGS)

.PHONY: clean

clean:
	rm -rf *.o sensor-history
