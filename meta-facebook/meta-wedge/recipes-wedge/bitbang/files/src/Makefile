# Copyright 2014-present Facebook. All Rights Reserved.
all: spi-bb mdio-bb

spi-bb: spi_bb.o bitbang.o gpio.o
	$(CC) -o $@ $^ $(LDFLAGS)

mdio-bb: mdio_bb.o bitbang.o gpio.o
	$(CC) -o $@ $^ $(LDFLAGS)

.PHONY: clean

clean:
	rm -rf *.o spi-bb mdio-bb
