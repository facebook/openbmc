# Copyright 2015-present Facebook. All Rights Reserved.
all: bic-util

bic-util: bic-util.c 
	$(CC) -pthread -lipmi -lipmb -lbic -lfbttn_gpio -std=c99 -o $@ $^ $(LDFLAGS)

.PHONY: clean

clean:
	rm -rf *.o bic-util
