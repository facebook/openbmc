#
# Copyright Altera 2013
# All Rights Reserved.
#
all: vrupdate_poc

vrupdate_poc:  vr_update.c
	$(CC) -o $@ $^ $(LDFLAGS)

.PHONY: clean

clean:
	rm -rf *.o vrupdate_poc

