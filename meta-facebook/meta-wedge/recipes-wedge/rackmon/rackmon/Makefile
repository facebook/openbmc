# Copyright 2014-present Facebook. All Rights Reserved.
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

override CFLAGS+=-D_GNU_SOURCE -D_DEFAULT_SOURCE -D_POSIX_C_SOURCE=199309 -Wall -Werror -std=c99
override LDFLAGS+=-pthread -lgpio
all: modbuscmd gpiowatch modbussim rackmond rackmonctl

rackmonctl: rackmonctl.c modbus.c
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS)

rackmond: rackmond.c modbus.c
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS)

modbuscmd: modbuscmd.c modbus.c
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS)

modbussim: modbussim.c modbus.c
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS)

gpiowatch: gpiowatch.c
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS)

.PHONY: clean

clean:
	rm -rf *.o modbuscmd gpiowatch modbussim rackmond rackmonctl
