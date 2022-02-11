# Copyright 2017-present Facebook. All Rights Reserved.
lib: libbios.so

libbios.so: bios.c
	$(CC) $(CFLAGS) -fPIC -c -pthread bios.c
	$(CC) -shared -o libbios.so bios.o -lc $(LDFLAGS)

.PHONY: clean

clean:
	rm -rf *.o libbios.so
