# Copyright 2018-present Facebook. All Rights Reserved.
lib: libpal.so

CFLAGS += -Wall -Werror

C_SRCS := $(wildcard *.c)
C_OBJS := ${C_SRCS:.c=.o}

libpal.so: $(C_OBJS)
	$(CC) $(CFLAGS) -fPIC -c -pthread -o pal.o pal.c
	$(CC) $(CFLAGS) -lm -lbic -lkv -lsensor-correction -shared -o libpal.so $^ -lc -lrt -Wl,--whole-archive -lobmc-pal -Wl,--no-whole-archive

.PHONY: clean

clean:
	rm -rf *.o libpal.so
