# Copyright 2015-present Facebook. All Rights Reserved.
all: asd

LDFLAGS += -lpthread -lpal -lasd-jtagintf
CFLAGS += -Wall -Werror -DREFERENCE_CODE
C_SRCS := $(wildcard *.c)
C_OBJS := ${C_SRCS:.c=.o}

asd: $(C_OBJS)
	$(CC) $(CFLAGS) -std=gnu99 -o $@ $^ $(LDFLAGS)
.PHONY: clean

clean:
	rm -rf *.o asd
