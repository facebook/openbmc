# Copyright 2015-present Facebook. All Rights Reserved.
all: front-paneld

CFLAGS += -Wall -Werror

front-paneld: front-paneld.c 
	$(CC) $(CFLAGS) -pthread -lpal -lmisc-utils -lgpio-ctrl -o $@ $^ $(LDFLAGS)

.PHONY: clean

clean:
	rm -rf *.o front-paneld
