# Copyright 2020-present Facebook. All Rights Reserved.
lib: libobmc-mctp.so

C_SRCS := $(wildcard *.c)
C_OBJS := ${C_SRCS:.c=.o}

CFLAGS += -Wall -Werror -fPIC

libobmc-mctp.so: $(C_OBJS)
	$(CC) -shared -o $@ $^ -lc $(LDFLAGS)

$(C_SRCS:.c=.d):%.d:%.c
	$(CC) $(CFLAGS) -c $< >$@

.PHONY: clean

clean:
	rm -rf *.o libobmc-mctp.so
