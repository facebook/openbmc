/*
 * Copyright 2019-present Facebook. All Rights Reserved.
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
#include <openbmc/obmc-sensors.h>
#include <stdlib.h>
#include <errno.h>
#include <stdio.h>
#include <string.h>

int main(int argc, char *argv[])
{
  float value;
  int ret;
  const char *chip = argv[1];
  const char *label = argv[2];
  if (argc < 3) {
    printf("USAGE: %s CHIP LABEL\n", argv[0]);
    printf("Pass null as CHIP if you want to find chip\n");
    return -1;
  }
  if (argc >= 4) {
    value = atof(argv[3]);
    printf("Setting: %s::%s to %f\n", chip, label, value);
    if (!strcmp(chip, "fan")) {
      ret = sensors_write_fan(label, value);
    } else {
      ret = sensors_write(chip, label, value);
    }
  } else {
    if (!strcmp(chip, "fan")) {
      ret = sensors_read_fan(label, &value);
    } else if(!strcmp(chip, "adc")) {
      ret = sensors_read_adc(label, &value);
    } else if (!strcmp(chip, "null") || !strcmp(chip, "NULL")) {
      ret = sensors_read(NULL, label, &value);
    } else {
      ret = sensors_read(chip, label, &value);
    }
    if (ret == 0)
      printf("%f\n", value);
  }
  if (ret) {
    printf("Operation failed: %s\n", strerror(errno));
    return -1;
  }
  return 0;
}
