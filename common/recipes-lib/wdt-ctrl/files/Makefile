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

lib: libwdt-ctrl.so

CFLAGS += -Wall -Werror

libwdt-ctrl.so: wdt-ctrl.o
	$(CC) -shared -o libwdt-ctrl.so wdt-ctrl.o -lc -pthread $(LDFLAGS)

wdt-ctrl.o: wdt-ctrl.c
	$(CC) $(CFLAGS) -fPIC -c -o wdt-ctrl.o wdt-ctrl.c

.PHONY: clean

clean:
	rm -rf *.o libwdt-ctrl.so
