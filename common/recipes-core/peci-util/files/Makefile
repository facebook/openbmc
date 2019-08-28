# Copyright 2015-present Facebook. All Rights Reserved.

C_SRCS := $(wildcard *.c)
C_OBJS := ${C_SRCS:.c=.o}

CFLAGS += -Wall -Werror

all: peci-util

peci-util: $(C_OBJS)
	$(CC) $(CFLAGS) -pthread --std=c99 -o $@ $^ -lpal $(LDFLAGS)

.PHONY: clean

clean:
	rm -rf *.o peci-util
