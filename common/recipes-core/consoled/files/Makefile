# Copyright 2015-present Facebook. All Rights Reserved.
all: consoled 


consoled: consoled.o
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS)

.PHONY: clean

clean:
	rm -rf *.o consoled
