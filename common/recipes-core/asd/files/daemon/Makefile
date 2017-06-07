# Copyright 2015-present Facebook. All Rights Reserved.
all: asd

LDFLAGS += -lpthread -lgpio -lpal -lasd-jtagintf
CFLAGS += -Wall -Werror

asd: logging.o socket_main.o target_handler.o
	$(CC) $(CFLAGS) -std=gnu99 -o $@ $^ $(LDFLAGS)
.PHONY: clean

clean:
	rm -rf *.o asd
