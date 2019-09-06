#
# Copyright Altera 2013
# All Rights Reserved.
#
all: fruid_util_remote

fruid_util_remote:  fruid_util_remote.c
	$(CC) -o $@ $^ $(LDFLAGS)

.PHONY: clean

clean:
	rm -rf *.o fruid_util_remote

