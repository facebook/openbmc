# Copyright 2015-present Facebook. All Rights Reserved.
all: enclosure-util
CFLAGS += -Wall -Werror

enclosure-util: enclosure-util.c 
	$(CC) $(CFLAGS) -lpal -pthread -std=c99 -o $@ $^ $(LDFLAGS)

.PHONY: clean

clean:
	rm -rf *.o enclosure-util
