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

TARGET   = cpldprog
# OBJECTS += main.o lattice.o ast-jtag.o machxo2.o jedecfile.o
OBJECTS += main.o lattice_cpld.o jedecfile.o jtag_common.o jtag_interface.o

CFLAGS += -Wall -Werror

.c.o:
	$(CC) -c $(CFLAGS) $(INCPATH) -o $@ $<
$(TARGET):$(OBJECTS)
	$(CC) $(LFLAGS) -o $(TARGET) $(OBJECTS) $(LDFLAGS)


.PHONY: clean

clean:
	rm -rf *.o $(TARGET)


