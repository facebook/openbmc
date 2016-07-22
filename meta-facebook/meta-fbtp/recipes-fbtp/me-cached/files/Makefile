# Copyright 2015-present Facebook. All Rights Reserved.
all: me-cached

me-cached: me-cached.c 
	$(CC) -pthread -lme -std=c99 -o $@ $^ $(LDFLAGS)

.PHONY: clean

clean:
	rm -rf *.o me-cached
