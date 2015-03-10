# Copyright 2014-present Facebook. All Rights Reserved.
all: sms-kcsd

sms-kcsd: sms-kcsd.c 
	$(CC) -pthread -lalert_control -lipmi -std=c99 -o $@ $^ $(LDFLAGS)

.PHONY: clean

clean:
	rm -rf *.o sms-kcsd
