# Copyright 2015-present Facebook. All Rights Reserved.
all: cfg-util 


cfg-util: cfg-util.o
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS)

.PHONY: clean

clean:
	rm -rf *.o cfg-util
