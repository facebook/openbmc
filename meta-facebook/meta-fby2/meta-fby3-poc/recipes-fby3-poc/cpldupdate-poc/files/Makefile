#
# Copyright Altera 2013
# All Rights Reserved.
#
all: cpldupdate_poc

cpldupdate_poc:  i2cupdate.c max10_i2c_update.c
	$(CC) -o $@ $^ $(LDFLAGS)

.PHONY: clean

clean:
	rm -rf *.o cpldupdate_poc

