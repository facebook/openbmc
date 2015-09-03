/*
 * yosemite-sensors
 *
 * Copyright 2015-present Facebook. All Rights Reserved.
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
#include <unistd.h>
#include <syslog.h>
#include <stdint.h>
#include <string.h>
#include <errno.h>
#include <openbmc/pal.h>

static int
print_usage() {
  printf("Usage: sensor-util [ %s ] <sensor num>\n"
      "sensor num is optional.", pal_fru_list);
}

static void
print_single_sensor_reading(uint8_t fru, uint8_t *sensor_list, int sensor_cnt, uint8_t num) {

  int i;
  float fvalue;
  char name[24];
  char units[64];
  int ret = 0;

  for (i = 0; i < sensor_cnt; i++) {

    if (sensor_list[i] == num) {
      ret = 1;
      pal_get_sensor_name(fru, sensor_list[i], name);
      pal_get_sensor_units(fru, sensor_list[i], units);

      if (pal_sensor_read(fru, sensor_list[i], &fvalue) < 0) {

        printf("pal_sensor_read failed: fru: %d num: 0x%X name: %-23s\n",
            fru, sensor_list[i], name);
      } else {
  	    printf("%-23s: %.2f %s\n", name, fvalue, units);
      }

      break;
    }
  }

  if (!ret) {
    printf("Wrong sensor number!\n");
    print_usage();
    exit(-1);
  }
}

static void
print_sensor_reading(uint8_t fru, uint8_t *sensor_list, int sensor_cnt) {

  int i;
  float fvalue;
  char name[24];
  char units[64];

  for (i = 0; i < sensor_cnt; i++) {

    /* Clear the variable */
    sprintf(name, "");
    sprintf(units, "");

    pal_get_sensor_name(fru, sensor_list[i], name);
    pal_get_sensor_units(fru, sensor_list[i], units);

    if (pal_sensor_read(fru, sensor_list[i], &fvalue) < 0) {

      printf("pal_sensor_read failed: fru: %d num: 0x%X name: %-23s\n",
          fru, sensor_list[i], name);
    }   else {
  	  printf("%-23s: %.2f %s\n", name, fvalue, units);
    }
  }
}

int
main(int argc, char **argv) {

  int ret;
  int sensor_cnt;
  uint8_t *sensor_list;
  uint8_t fru;
  uint8_t num;

  if (argc < 2 || argc > 3) {
    print_usage();
    exit(-1);
  }

  if (argc == 3) {
    errno = 0;
    num = (uint8_t) strtol(argv[2], NULL, 0);
    if (errno) {
      printf("Sensor number format incorrect.\n");
      print_usage();
      exit(-1);
    }
  }


  ret = pal_get_fru_id(argv[1], &fru);
  if (ret < 0) {
    print_usage();
    return ret;
  }

  if (fru == 0) {
    fru = 1;
    while (fru <= MAX_NUM_FRUS) {

      if (fru == FRU_NIC) {
        printf("\nsensor-util does not support nic\n");
        exit(-1);
      }

      ret = pal_get_fru_sensor_list(fru, &sensor_list, &sensor_cnt);
      if (ret < 0) {
        return ret;
      }

      if (num) {
        print_single_sensor_reading(fru, sensor_list, sensor_cnt, num);
      } else {
        print_sensor_reading(fru, sensor_list, sensor_cnt);
      }

      fru++;
      printf("\n");
    }
  } else {

    if (fru == FRU_NIC) {
      printf("\nsensor-util does not support nic\n");
      //exit(-1);
    }

    ret = pal_get_fru_sensor_list(fru, &sensor_list, &sensor_cnt);
    if (ret < 0) {
      return ret;
    }

    if (num) {
      print_single_sensor_reading(fru, sensor_list, sensor_cnt, num);
    } else {
        print_sensor_reading(fru, sensor_list, sensor_cnt);
      }
  }

  return 0;
}
