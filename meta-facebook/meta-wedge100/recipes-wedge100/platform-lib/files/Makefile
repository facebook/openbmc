# Copyright 2017-present Facebook. All Rights Reserved.
lib: libpal.so

CFLAGS += -Wall -Werror

C_SRCS := $(wildcard *.c)
C_OBJS := ${C_SRCS:.c=.o}

libpal.so: $(C_OBJS)
	$(CC) $(CFLAGS) -lkv -lgpio -lsensor-correction -shared -o libpal.so $^ -lc -lrt $(LDFLAGS) -Wl,--whole-archive -lobmc-pal -Wl,--no-whole-archive

.PHONY: clean

clean:
	rm -rf *.o libpal.so
