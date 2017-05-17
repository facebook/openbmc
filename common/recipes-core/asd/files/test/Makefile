# Copyright 2015-present Facebook. All Rights Reserved.
all: asd-test

LDFLAGS += -lpthread -lgpio -lasd-jtagintf
CFLAGS += -Wall -Werror

asd-test: jtagtest.o
	$(CC) $(CFLAGS) -std=gnu99 -o $@ $^ $(LDFLAGS)
.PHONY: clean

clean:
	rm -rf *.o asd-test
