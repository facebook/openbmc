/*
 * Copyright 2014-present Facebook. All Rights Reserved.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <errno.h>
#include <sys/types.h>

#define SERIAL_LEN 17

usage()
{
  fprintf(stderr, "Usage:  po-eeprom filename [filename...]\n"
	  "\twhere filename is the location of the PowerOne EEPROM, likely\n"
	  "\t/sys/bus/i2c/drivers/at24/7-0051/eeprom or\n"
	  "\t/sys/bus/i2c/drivers/at24/7-0052/eeprom\n");
  exit(2);
}

int safe_read(int fd, void *buf, size_t size, char *msg)
{
  if (read(fd, buf, size) != size) {
    perror(msg);
    exit(3);
  }
}

int main(int argc, char **argv)
{
  int fd;
  int file = 1;
  uint8_t size;
  uint8_t junk;
  uint16_t crc;
  char serial[SERIAL_LEN + 1];

  if (argc < 2)
    usage();

  while (file < argc) {
    fd = open(argv[file], O_RDONLY);
    if (fd < 0) {
      fprintf(stderr, "Couldn't open %s for EEPROM reading\n", argv[1]);
      exit(1);
    }

    safe_read(fd, &size, sizeof(size), "read size");
    safe_read(fd, &junk, sizeof(junk), "read junk");
    /*
     * Should probably check CRC here, PowerOne provided some code where
     * it looks like a pretty standard CCITT CRC16.
     */
    safe_read(fd, &crc, sizeof(crc), "read CRC len");
    safe_read(fd, serial, SERIAL_LEN, "read serial number");

    serial[SERIAL_LEN] = 0;
    printf("Serial number:  %s\n", serial);
    close(fd);
    file++;
  }
}
