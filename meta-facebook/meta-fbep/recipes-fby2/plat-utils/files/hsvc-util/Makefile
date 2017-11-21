# Copyright 2015-present Facebook. All Rights Reserved.
all: hsvc-util

hsvc-util: hsvc-util.c 
	$(CC) -pthread -lfby2_common -lfby2_sensor -lpal -std=c99 -o $@ $^ $(LDFLAGS)

.PHONY: clean

clean:
	rm -rf *.o hsvc-util
