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

lib: liblightning_fruid.so

liblightning_fruid.so: lightning_fruid.c
	$(CC) $(CFLAGS) -fPIC -c -o lightning_fruid.o lightning_fruid.c
	$(CC) -llightning_common -shared -o liblightning_fruid.so lightning_fruid.o -lc

.PHONY: clean

clean:
	rm -rf *.o liblightning_fruid.so
