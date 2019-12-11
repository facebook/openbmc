# Copyright 2018-present Facebook. All Rights Reserved.
lib: libvr.so

C_SRCS := $(wildcard *.c)
C_OBJS := ${C_SRCS:.c=.o}

CFLAGS += -Wall -Werror -fPIC

libvr.so: $(C_OBJS)
	$(CC) -shared -o $@ $^ -lc $(LDFLAGS)

$(C_SRCS:.c=.d):%.d:%.c
	$(CC) $(CFLAGS) $< >$@

.PHONY: clean

clean:
	rm -rf *.o libvr.so
