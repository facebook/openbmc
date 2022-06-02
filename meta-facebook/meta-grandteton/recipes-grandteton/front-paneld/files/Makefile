# Copyright 2015-present Facebook. All Rights Reserved.
all: front-paneld

CFLAGS += -Wall -Werror

front-paneld: front-paneld.c 
	$(CC) $(CFLAGS) $(LDFLAGS) -o $@ $^ $(LDFLAGS)

.PHONY: clean

clean:
	rm -rf *.o front-paneld
