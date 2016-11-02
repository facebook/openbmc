# Copyright 2015-present Facebook. All Rights Reserved.
all: exp-cached

bic-cached: exp-cached.c 
	$(CC) -pthread -lexp -std=c99 -o $@ $^ $(LDFLAGS)

.PHONY: clean

clean:
	rm -rf *.o exp-cached
