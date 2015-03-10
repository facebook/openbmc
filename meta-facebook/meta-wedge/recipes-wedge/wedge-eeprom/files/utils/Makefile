# Copyright 2014-present Facebook. All Rights Reserved.
utils: weutil

weutil: weutil.o
	$(CC) $(LDFLAGS) -o $@ $^ -lwedge_eeprom

.PHONY: clean

clean:
	rm -rf *.o weutil
