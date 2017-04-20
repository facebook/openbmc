# Copyright 2017-present Facebook. All Rights Reserved.
all: ecc-mond

ecc-mond: ecc-mond.o
	$(CC) -pthread -lpal -o $@ $^ $(LDFLAGS)

.PHONY: clean

clean:
	rm -rf *.o ecc-mond
