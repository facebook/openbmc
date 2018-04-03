# Copyright 2015-present Facebook. All Rights Reserved.
lib: libminilaketb_sensor.so

libminilaketb_sensor.so: minilaketb_sensor.c
	$(CC) $(CFLAGS) -fPIC -c -o minilaketb_sensor.o minilaketb_sensor.c
	$(CC) -lm -lbic -lipmi -lipmb -lminilaketb_common -lnvme-mi -shared -o libminilaketb_sensor.so minilaketb_sensor.o -lc $(LDFLAGS)

.PHONY: clean

clean:
	rm -rf *.o libminilaketb_sensor.so
