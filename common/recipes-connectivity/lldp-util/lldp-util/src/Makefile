# Copyright 2014-present Facebook. All Rights Reserved.
all: lldp-util

# Yocto overrides anything we do to CFLAGS using "make -e",
# so use our own EXTRA_CFLAGS variable instead of CFLAGS.
EXTRA_CFLAGS = -std=gnu99 -Wall -Wextra -Werror \
	       -Wno-unused -Wno-unused-parameter

lldp-util: lldp_util.o
	$(CC) -o $@ $^ $(LDFLAGS)

%.o: %.c
	@echo $(CC) $(CFLAGS) $(EXTRA_CFLAGS) -c -o $@ $<
	$(CC) $(CFLAGS) $(EXTRA_CFLAGS) -c -o $@ $<

.PHONY: clean

clean:
	rm -rf *.o lldp-util
