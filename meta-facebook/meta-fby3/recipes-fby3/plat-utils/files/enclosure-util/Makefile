# Copyright 2015-present Facebook. All Rights Reserved.
all: enclosure-util
CFLAGS += -Wall -Werror -D_XOPEN_SOURCE

enclosure-util: enclosure-util.c
	$(CC) $(CFLAGS) -lbic  -lnvme-mi -lpal -lfby3_common -std=c99 -o $@ $^ $(LDFLAGS)

.PHONY: clean

clean:
	rm -rf *.o enclosure-util
