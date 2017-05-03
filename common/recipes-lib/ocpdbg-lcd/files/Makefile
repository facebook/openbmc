# Copyright 2017-present Facebook. All Rights Reserved.
lib: libocpdbg-lcd.so

libocpdbg-lcd.so: ocp-dbg-lcd.c
	$(CC) $(CFLAGS) -fPIC -c -o ocp-dbg-lcd.o ocp-dbg-lcd.c
	$(CC) -shared -o libocpdbg-lcd.so ocp-dbg-lcd.o -lc

.PHONY: clean

clean:
	rm -rf *.o libocpdbg-lcd.so
