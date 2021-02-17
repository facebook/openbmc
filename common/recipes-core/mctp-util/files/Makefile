# Copyright 2015-present Facebook. All Rights Reserved.

C_SRCS := $(wildcard *.c)
C_OBJS := ${C_SRCS:.c=.o}

all: mctp-util

CFLAGS += -Wall -Werror

mctp-util: $(C_OBJS)
	$(CC) -pthread $(CFLAGS) -std=gnu11 -o $@ $^ $(LDFLAGS)

.PHONY: clean

clean:
	rm -rf *.o mctp-util
