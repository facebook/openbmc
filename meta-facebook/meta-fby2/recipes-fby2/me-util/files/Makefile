# Copyright 2015-present Facebook. All Rights Reserved.
all: me-util

me-util: me-util.o 
	$(CC) $(CFLAGS) -pthread -lipmi -lipmb -lfby2_sensor -lbic -lpal -std=c99 -o $@ $^ $(LDFLAGS)

.PHONY: clean

clean:
	rm -rf *.o me-util
