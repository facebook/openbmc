# Copyright 2014-present Facebook. All Rights Reserved.
all: oob-nic i2craw

oob-nic: main.o nic.o intf.o ll_map.o libnetlink.o
	$(CC) -o $@ $^ $(LDFLAGS) -lwedge_eeprom

i2craw: i2craw.o
	$(CC) -o $@ $^ $(LDFLAGS)

.PHONY: clean

clean:
	rm -rf *.o oob-nic i2craw
