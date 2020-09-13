# Copyright 2015-present Facebook. All Rights Reserved.

C_SRCS := $(wildcard *.c)
C_OBJS := ${C_SRCS:.c=.o}

all: ncsi-util

CFLAGS += -Wall -Werror

ncsi-util: $(C_OBJS)
	$(CC) -pthread $(CFLAGS) -std=gnu11 -o $@ $^ $(LDFLAGS)

.PHONY: clean

clean:
	rm -rf *.o ncsi-util
