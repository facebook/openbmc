# Copyright 2015-present Facebook. All Rights Reserved.
all: show_sys_config

show_sys_config: show_sys_config.o
	$(CC) $(CFLAGS) -pthread -std=c99 -o $@ $^ $(LDFLAGS)

.PHONY: clean

clean:
	rm -rf *.o show_sys_config
