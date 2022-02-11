# Copyright 2015-present Facebook. All Rights Reserved.
all: enclosure-util
CFLAGS += -Wall -Werror -D_XOPEN_SOURCE

enclosure-util: enclosure-util.c
	$(CC) $(CFLAGS) -lbic -lfby2_sensor -lnvme-mi -lpal -std=c99 -o $@ $^ $(LDFLAGS)

.PHONY: clean

clean:
	rm -rf *.o enclosure-util
