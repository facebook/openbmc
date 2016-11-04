# Copyright 2015-present Facebook. All Rights Reserved.
all: front-paneld

front-paneld: front-paneld.c 
	$(CC) -pthread -lpal -lbic -o $@ $^ $(LDFLAGS)

.PHONY: clean

clean:
	rm -rf *.o front-paneld
