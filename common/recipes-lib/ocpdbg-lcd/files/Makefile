# Copyright 2017-present Facebook. All Rights Reserved.
lib: libocp-dbg-lcd.so

libocp-dbg-lcd.so: ocp-dbg-lcd.c
	$(CC) $(CFLAGS) -fPIC -c -o ocp-dbg-lcd.o ocp-dbg-lcd.c
	$(CC) -shared -o libocp-dbg-lcd.so ocp-dbg-lcd.o -lc

.PHONY: clean

clean:
	rm -rf *.o libocp-dbg-lcd.so
