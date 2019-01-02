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

lib: libpal.so

C_SRCS := $(SOURCES)
C_OBJS := ${C_SRCS:.c=.o}

libpal.so: $(C_OBJS)
	$(CC) -shared -o libpal.so $^ -fPIC -lc -Wl,--whole-archive -lobmc-pal -Wl,--no-whole-archive -lrt $(LDFLAGS)

$(C_SRCS:.c=.d):%.d:%.c
	$(CC) $(CFLAGS) $< >$@

.PHONY: clean

clean:
	rm -rf *.o libpal.so
