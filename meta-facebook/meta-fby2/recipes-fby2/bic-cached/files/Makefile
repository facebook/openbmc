# Copyright 2015-present Facebook. All Rights Reserved.
all: bic-cached

bic-cached: bic-cached.c 
	$(CC) -pthread -lbic -lpal -std=c99 -o $@ $^ $(LDFLAGS)

.PHONY: clean

clean:
	rm -rf *.o bic-cached
