# Copyright 2015-present Facebook. All Rights Reserved.
lib: libminilaketb_common.so

libminilaketb_common.so: minilaketb_common.c
	$(CC) $(CFLAGS) -fPIC -pthread -c -o minilaketb_common.o minilaketb_common.c
	$(CC) -lpthread -shared -o libminilaketb_common.so minilaketb_common.o -lc $(LDFLAGS)

.PHONY: clean

clean:
	rm -rf *.o libminilaketb_common.so
