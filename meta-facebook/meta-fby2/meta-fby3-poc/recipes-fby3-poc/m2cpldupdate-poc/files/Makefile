#
# Copyright Altera 2013
# All Rights Reserved.
#
all: m2cpldupdate_poc

m2cpldupdate_poc:  cpld_update.c
	$(CC) -o $@ $^ $(LDFLAGS)

.PHONY: clean

clean:
	rm -rf *.o m2cpldupdate_poc

