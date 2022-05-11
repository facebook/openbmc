# Copyright 2015-present Facebook. All Rights Reserved.
lib: libbic.so

CFLAGS += -Wall -Werror

# OpenSSL3, which is present in Kirkstone and above, deprecates some
# SHA hash functions we are using.  Turn off the warning until we can
# port to OpenSSL3.
CFLAGS += -Wno-error=deprecated-declarations

libbic.so: bic.c
	$(CC) $(CFLAGS) -fPIC -c -o bic.o bic.c
	$(CC) -lipmb -lkv -shared -o libbic.so bic.o -lc $(LDFLAGS)

.PHONY: clean

clean:
	rm -rf *.o libbic.so
