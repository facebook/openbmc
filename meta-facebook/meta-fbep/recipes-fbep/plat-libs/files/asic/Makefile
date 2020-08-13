# Copyright 2017-present Facebook. All Rights Reserved.
lib: libasic.so

C_SRCS := $(wildcard *.c)
C_OBJS := ${C_SRCS:.c=.o}

CFLAGS += -Wall -Werror -fPIC

libasic.so: $(C_OBJS)
	$(CC) -shared -o $@ $^ -lc $(LDFLAGS)

$(C_SRCS:.c=.d):%.d:%.c
	$(CC) $(CFLAGS) $< >$@

.PHONY: clean

clean:
	rm -rf *.o libasic.so
