# Copyright 2015-present Facebook. All Rights Reserved.
lib: libipmb.so

libipmb.so: ipmb.c
	$(CC) $(CFLAGS) -fPIC -c -o ipmb.o ipmb.c
	$(CC) -shared -o libipmb.so ipmb.o -lc -lrt -lipc $(LDFLAGS)

.PHONY: clean

clean:
	rm -rf *.o libipmb.so
