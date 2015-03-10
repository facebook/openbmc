# Copyright 2014-present Facebook. All Rights Reserved.
all: ipmid

ipmid: ipmid.c \
	platform/timestamp.c platform/sel.c platform/sdr.c \
	platform/wedge/sensor.c \
	platform/wedge/fruid.c
	$(CC) -pthread -lwedge_eeprom -std=c99 -o $@ $^ $(LDFLAGS)

.PHONY: clean

clean:
	rm -rf *.o platform/*.o platform/wedge/*.o ipmid
