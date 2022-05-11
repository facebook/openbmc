# Copyright 2015-present Facebook. All Rights Reserved.
lib: libbic.so

C_SRCS := $(SOURCES)
C_OBJS := ${C_SRCS:.c=.o}

# OpenSSL3, which is present in Kirkstone and above, deprecates some
# SHA hash functions we are using.  Turn off the warning until we can
# port to OpenSSL3.
CFLAGS += -Wno-error=deprecated-declarations

libbic.so: $(C_OBJS)
	$(CC) -shared -o libbic.so $^ -fPIC -lc -Wl,--whole-archive -lm -Wl,--no-whole-archive -lrt $(LDFLAGS)

$(C_SRCS:.c=.d):%.d:%.c
	$(CC) $(CFLAGS) $< >$@

.PHONY: clean

clean:
	rm -rf *.o libbic.so
