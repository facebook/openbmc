all: gpiointrd

CFLAGS += -Wall -Werror

gpiointrd: gpiointrd.c
	$(CC) $(CFLAGS) -D _XOPEN_SOURCE -pthread -std=c99 -o $@ $^ $(LDFLAGS)

.PHONY: clean

clean:
	rm -rf *.o gpiointrd
