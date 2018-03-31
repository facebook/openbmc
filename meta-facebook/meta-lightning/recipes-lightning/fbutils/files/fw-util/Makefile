# Copyright 2017-present Facebook. All Rights Reserved.
all: fw-util
CFLAGS += -Wall -Werror

fw-util: fw-util.c 
	$(CC) $(CFLAGS) -lpal -std=c99 -o $@ $^ $(LDFLAGS) 

.PHONY: clean

clean:
	rm -rf *.o fw-util
