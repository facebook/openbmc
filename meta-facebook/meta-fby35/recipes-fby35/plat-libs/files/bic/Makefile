# Copyright 2015-present Facebook. All Rights Reserved.
lib: libbic.so

C_SRCS := $(SOURCES)
C_OBJS := ${C_SRCS:.c=.o}

libbic.so: $(C_OBJS)
	$(CC) -shared -o libbic.so $^ -fPIC -lc -Wl,--whole-archive -lm -Wl,--no-whole-archive -lrt $(LDFLAGS)

$(C_SRCS:.c=.d):%.d:%.c
	$(CC) $(CFLAGS) $< >$@

.PHONY: clean

clean:
	rm -rf *.o libbic.so
