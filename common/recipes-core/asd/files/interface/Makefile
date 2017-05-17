# Copyright 2017-present Facebook. All Rights Reserved.
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

CFLAGS += -Wall -Werror

lib: libasd-jtagintf.so

libasd-jtagintf.so: SoftwareJTAGHandler.o
	$(CC) -shared -o libasd-jtagintf.so SoftwareJTAGHandler.o -lc -lgpio -lpal -pthread

SoftwareJTAGHandler.o: SoftwareJTAGHandler.c
	$(CC) $(CFLAGS) -fPIC -c -o SoftwareJTAGHandler.o SoftwareJTAGHandler.c

.PHONY: clean

clean:
	rm -rf *.o libasd-jtagintf.so
