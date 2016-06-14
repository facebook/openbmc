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

TARGET   = stress
OBJECTS += stress.o
LFLAGS += -lm
EXTRA_CFLAGS += -DPACKAGE=\"$(TARGET)\" -DVERSION=\"1.0.4\"
.c.o:
	$(CC) $(EXTRA_CFLAGS) -c $(CFLAGS) $(INCPATH) -o $@ $<
$(TARGET):$(OBJECTS)
	$(CC) $(LFLAGS) -o $(TARGET) $(OBJECTS)

.PHONY: clean

clean:
	rm -rf *.o $(TARGET)
