# Copyright 2017-present Facebook. All Rights Reserved.
lib: libusb_dbg.so

libusb_dbg.so: usb_dbg.c
	$(CC) $(CFLAGS) -fPIC -c -o usb_dbg.o usb_dbg.c
	$(CC) -shared -o libusb_dbg.so usb_dbg.o -lc

.PHONY: clean

clean:
	rm -rf *.o libusb_dbg.so
