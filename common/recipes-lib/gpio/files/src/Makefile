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

lib: libgpio.so

CFLAGS += -Wall -Werror

libgpio.so: gpio.o gpio_name.o
	$(CC) -shared -o libgpio.so gpio.o gpio_name.o -lc -pthread $(LDFLAGS)

gpio.o: gpio.c
	$(CC) $(CFLAGS) -fPIC -c -o gpio.o gpio.c

gpio_name.o: gpio_name.c
	$(CC) $(CFLAGS) -fPIC -c -o gpio_name.o gpio_name.c

.PHONY: clean

clean:
	rm -rf *.o libgpio.so
