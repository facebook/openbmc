# Copyright 2015-present Facebook. All Rights Reserved.
all: imc-util

imc-util: imc-util.o 
	$(CC) $(CFLAGS) -pthread -lipmi -lipmb -lfby2_sensor -lbic -lpal -std=c99 -o $@ $^ $(LDFLAGS)

.PHONY: clean

clean:
	rm -rf *.o imc-util
