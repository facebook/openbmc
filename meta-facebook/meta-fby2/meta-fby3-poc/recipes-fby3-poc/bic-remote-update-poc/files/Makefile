#
# Copyright Altera 2013
# All Rights Reserved.
#
all: bic_remote_update

bic_remote_update:  bic_remote_update.c
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS)

.PHONY: clean

clean:
	rm -rf *.o bic_remote_update

