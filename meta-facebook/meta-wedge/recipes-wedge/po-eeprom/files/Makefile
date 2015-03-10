# Copyright 2014-present Facebook. All Rights Reserved.
all: po-eeprom

po-eeprom: po-eeprom.o
	$(CC) -o $@ $^ $(LDFLAGS)

.PHONY: clean

clean:
	rm -rf *.o po-eeprom
