# Copyright 2014-present Facebook. All Rights Reserved.
lib: libwedge_eeprom.so

libwedge_eeprom.so: wedge_eeprom.c
	$(CC) $(CCFLAGS) -fPIC -c -o wedge_eeprom.o wedge_eeprom.c
	$(CC) -shared -o libwedge_eeprom.so wedge_eeprom.o -lc

.PHONY: clean

clean:
	rm -rf *.o libwedge_eeprom.so
