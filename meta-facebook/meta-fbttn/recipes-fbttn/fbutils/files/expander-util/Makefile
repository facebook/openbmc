# Copyright 2015-present Facebook. All Rights Reserved.
all: expander-util

expander-util: expander-util.o 
	$(CC) $(CFLAGS) -pthread -lipmb -lexp -std=c99 -o $@ $^ $(LDFLAGS)

.PHONY: clean

clean:
	rm -rf *.o expander-util
