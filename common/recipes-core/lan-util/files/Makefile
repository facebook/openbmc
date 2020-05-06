# Copyright 2015-present Facebook. All Rights Reserved.
all: lan-util


lan-util: lan-util.c if_parser.c net_lib.c strlib.c
        $(CC) $(CFLAGS) -std=c99 -o $@ $^ $(LDFLAGS)

.PHONY: clean

clean:
	rm -rf *.o lan-util
