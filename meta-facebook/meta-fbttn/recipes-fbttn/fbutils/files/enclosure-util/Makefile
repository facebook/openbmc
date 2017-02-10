# Copyright 2015-present Facebook. All Rights Reserved.
all: enclosure-util

enclosure-util: enclosure-util.o 
	$(CC) $(CFLAGS) -pthread -lipmb -lexp -lpal -std=c99 -o $@ $^ $(LDFLAGS)

.PHONY: clean

clean:
	rm -rf *.o enclosure-util
