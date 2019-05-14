# Copyright 2017-present Facebook. All Rights Reserved.
CFLAGS += -Wall -Werror

lib: libasd-jtagintf.so

libasd-jtagintf.so: SoftwareJTAGHandler.o pin_interface.o
	$(CC) -shared -o libasd-jtagintf.so SoftwareJTAGHandler.o pin_interface.o -lc -pthread $(LDFLAGS)

SoftwareJTAGHandler.o: SoftwareJTAGHandler.c
	$(CC) $(CFLAGS) -fPIC -c -o SoftwareJTAGHandler.o SoftwareJTAGHandler.c

pin_interface.o: pin_interface.c
	$(CC) $(CFLAGS) -fPIC -c -o pin_interface.o pin_interface.c

.PHONY: clean

clean:
	rm -rf *.o libasd-jtagintf.so
