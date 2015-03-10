# Copyright 2014-present Facebook. All Rights Reserved.
all: modbuscmd gpiowatch modbussim

modbuscmd: modbuscmd.c modbus.c
	$(CC) -D_BSD_SOURCE -Wall -Werror -std=c99 -o $@ $^ $(LDFLAGS)

modbussim: modbussim.c modbus.c
	$(CC) -D_BSD_SOURCE -Wall -Werror -std=c99 -o $@ $^ $(LDFLAGS)

gpiowatch: gpiowatch.c
	$(CC) -D_BSD_SOURCE -Wall -Werror -std=c99 -o $@ $^ $(LDFLAGS)

.PHONY: clean

clean:
	rm -rf *.o modbuscmd gpiowatch modbussim
