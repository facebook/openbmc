# Copyright 2015-present Facebook. All Rights Reserved.
#
# This program file is free software; you can redistribute it and/or modify it
# under the terms of the GNU General Public License as published by the
# Free Software Foundation; version 2 of the License.
#
# This program is distributed in the hope that it will be useful, but WITHOUT
# ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
# FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License
# for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program in a file named COPYING; if not, write to the
# Free Software Foundation, Inc.,
# 51 Franklin Street, Fifth Floor,
# Boston, MA 02110-1301 USA

lib: libkv.so

CFLAGS += -Wall -Werror

libkv.so: kv.c
	$(CC) $(CFLAGS) -fPIC -c -o kv.o kv.c
	$(CC) -shared -o libkv.so kv.o -lc $(LDFLAGS)

test_kv: kv.c
	$(CC) $(CFLAGS) -D__TEST__ -DDEBUG -c -o $@.o $^
	$(CC) -o $@ -lc $(LDFLAGS) $@.o -v

.PHONY: clean

clean:
	rm -rf *.o libkv.so test_kv
