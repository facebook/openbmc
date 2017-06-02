# Copyright 2017-present Facebook. All Rights Reserved.
CFLAGS += -Wall -Werror

lib: libasd-jtagintf.so

libasd-jtagintf.so: SoftwareJTAGHandler.o
	$(CC) -shared -o libasd-jtagintf.so SoftwareJTAGHandler.o -lc -lgpio -lpal -pthread

SoftwareJTAGHandler.o: SoftwareJTAGHandler.c
	$(CC) $(CFLAGS) -fPIC -c -o SoftwareJTAGHandler.o SoftwareJTAGHandler.c

.PHONY: clean

clean:
	rm -rf *.o libasd-jtagintf.so
