# Copyright 2015-present Facebook. All Rights Reserved.
lib: libncsi.so

C_SRCS := $(wildcard *.c)
C_OBJS := ${C_SRCS:.c=.o}

CFLAGS += -Wall -Werror -fPIC

libncsi.so: $(C_OBJS)
	$(CC) -shared -o libncsi.so $^ -lc -lrt $(LDFLAGS)

$(C_SRCS:.c=.d):%.d:%.c
	$(CC) $(CFLAGS) $< >$@

.PHONY: clean

clean:
	rm -rf *.o libncsi.so
