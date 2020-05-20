# Copyright 2015-present Facebook. All Rights Reserved.
all: ncsid

CFLAGS += -Wall -Werror

ncsid: ncsid.o
	$(CC) $(CFLAGS) -pthread -lrt  -lpal -std=gnu99 -o $@ $^ $(LDFLAGS)

.PHONY: clean

clean:
	rm -rf *.o ncsid
