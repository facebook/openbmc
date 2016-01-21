/*
 * Copyright 2016-present Facebook. All Rights Reserved.
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
#include <errno.h>
#include <syslog.h>
#include <stdint.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <openbmc/pal.h>

#define CMD_SET_FAN_STR "--set"
#define CMD_GET_FAN_STR "--get"
#define ALL_FAN_NUM     0xFF

enum {
  CMD_SET_FAN = 0,
  CMD_GET_FAN,
};

static void
print_usage(void) {
  printf("Usage: fan-util --set <[0..100] %> < Fan# [%s] >\n", pal_pwm_list);
  printf("       fan-util --get < Fan# [%s] >\n", pal_tach_list);
}

int
main(int argc, char **argv) {

  uint8_t cmd;
  uint8_t pwm;
  uint8_t fan = ALL_FAN_NUM;
  int i, j;
  int ret;
  int rpm = 0;
  char fan_name[32];

  if (argc < 2 || argc > 4) {
    print_usage();
    return -1;
  }

  if ((!strcmp(argv[1], CMD_SET_FAN_STR)) && (argc == 3 || argc == 4)) {

    /* fan-util --set <pwm> <fan#> */
    cmd = CMD_SET_FAN;
    pwm = (uint8_t) atoi(argv[2]);
    if (pwm > 100) {
      print_usage();
      return -1;
    }

    // Get Fan Number, if mentioned
    if (argc == 4) {
      fan = (uint8_t) atoi(argv[3]);
      if (fan >= pal_pwm_cnt) {
        print_usage();
        return -1;
      }
    }

  } else if (!strcmp(argv[1], CMD_GET_FAN_STR) && (argc == 2 || argc == 3)) {

    /* fan-util --get <fan#> */
    cmd = CMD_GET_FAN;

    // Get Fan Number, if mentioned
    if (argc == 3) {
      fan = (uint8_t) atoi(argv[2]);
      if (fan >= pal_tach_cnt) {
        print_usage();
        return -1;
      }
    }

  } else {
    print_usage();
    return -1;
  }

  if (cmd == CMD_SET_FAN) {
    // Set the Fan Speed.
    // If the fan num is not mentioned, set all fans to given pwm.
    for (i = 0; i < pal_pwm_cnt; i++) {

      // Check if the fan number is mentioned.
      if (fan != ALL_FAN_NUM && fan != i)
          continue;

      ret = pal_set_fan_speed(i, pwm);
      if (!ret) {
        memset(fan_name, 0, 32);
        pal_get_fan_name(i, fan_name);
        printf("Setting %s speed to %d%\n", fan_name, pwm);
      } else
        printf("Error while setting fan speed for Fan %d\n", i);
    }

  } else if (cmd == CMD_GET_FAN) {

    for (i = 0; i < pal_tach_cnt; i++) {

      // Check if the fan number is mentioned.
      if (fan != ALL_FAN_NUM && fan != i)
          continue;

      // Get the Fan Speed
      ret = pal_get_fan_speed(i, &rpm);
      if (!ret) {
        memset(fan_name, 0, 32);
        pal_get_fan_name(i, fan_name);
        printf("%s Speed: %d RPM\n", fan_name, rpm);
      } else {
        printf("Error while getting fan speed for Fan %d\n", i);
      }
    }
  }

  return 0;
}
