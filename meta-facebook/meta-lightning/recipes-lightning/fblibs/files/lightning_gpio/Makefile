# Copyright 2018-present Facebook. All Rights Reserved.
lib: liblightning_gpio.so

CFLAGS += -Wall -Werror
liblightning_gpio.so: lightning_gpio.c
	$(CC) $(CFLAGS) -fPIC -c -o lightning_gpio.o lightning_gpio.c
	$(CC) -lm -llightning_common -lgpio -shared -o liblightning_gpio.so lightning_gpio.o -lc $(LDFLAGS)

.PHONY: clean

clean:
	rm -rf *.o liblightning_gpio.so
