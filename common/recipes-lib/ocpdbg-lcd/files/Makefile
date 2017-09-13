# Copyright 2017-present Facebook. All Rights Reserved.
lib: libocpdbg-lcd.so
CFLAGS += -Wall -Werror
libocpdbg-lcd.so: ocp-dbg-lcd.c
	$(CC) $(CFLAGS) -fPIC -c -o ocp-dbg-lcd.o ocp-dbg-lcd.c
	$(CC) -shared -o libocpdbg-lcd.so ocp-dbg-lcd.o -lc $(LDFLAGS)

.PHONY: clean

clean:
	rm -rf *.o libocpdbg-lcd.so
