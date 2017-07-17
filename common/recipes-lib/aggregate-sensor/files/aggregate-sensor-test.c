/*
 *
 * Copyright 2017-present Facebook. All Rights Reserved.
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

#include "aggregate-sensor.h"
#include <stdio.h>
#include <stdlib.h>

extern void aggregate_sensors_conf_print(void);

#ifdef __TEST__
int aggregate_sensor_test(const char *file)
{
  size_t num = 0, i;
  if (aggregate_sensor_init(file)) {
    printf("Sensor initialization failed!\n");
    return -1;
  }

  aggregate_sensors_conf_print();

  if (aggregate_sensor_count(&num)) {
    printf("Getting sensor count failed!\n");
    return -1;
  }
  if (num == 0) {
    printf("No sensors! Nothing to do\n");
    return 0;
  }
  for(i = 0; i < num; i++) {
    char sensor_name[128];
    char sensor_unit[128];
    char status[128] = "ok";
    float value;
    int ret;

    if (aggregate_sensor_name(i, sensor_name) ||
        aggregate_sensor_units(i, sensor_unit)) {
      printf("[%zu] Getting name/unit failed\n", i);
      continue;
    }
    ret = aggregate_sensor_read(i, &value);
    if (!ret) {
      printf("%-28s (0x%X) : %7.2f %-5s (%s)\n",
        sensor_name, (unsigned int)i, value, sensor_unit, status);
    } else {
      printf("%-28s (0x%X) : NA %-5s (NA)\n",
        sensor_name, (unsigned int)i, sensor_unit);
    }
  }
  return 0;
}

int main(int argc, char *argv[])
{
  if (argc < 2) {
    printf("Usage: %s <JSON Config>\n", argv[0]);
    return -1;
  }
  return aggregate_sensor_test(argv[1]);
}
#endif
