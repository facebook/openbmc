# Copyright 2015-present Facebook. All Rights Reserved.
all: exp-cached

exp-cached: exp-cached.c 
	$(CC) -pthread -lpal -lexp -std=c99 -o $@ $^ $(LDFLAGS)

.PHONY: clean

clean:
	rm -rf *.o exp-cached
