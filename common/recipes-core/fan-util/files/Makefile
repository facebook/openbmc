# Copyright 2016-present Facebook. All Rights Reserved.
all: fan-util 


fan-util: fan-util.o
	$(CC) $(CFLAGS) -lpal -lrt -lm -std=gnu99 -o $@ $^ $(LDFLAGS)

.PHONY: clean

clean:
	rm -rf *.o fan-util
