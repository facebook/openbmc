# Copyright 2014-present Facebook. All Rights Reserved.
lib: libalert_control.so

libalert_control.so: alert_control.c
	$(CC) $(CCFLAGS) -fPIC -c -o alert_control.o alert_control.c
	$(CC) -shared -o libalert_control.so alert_control.o -lc

.PHONY: clean

clean:
	rm -rf *.o libalert_control.so
