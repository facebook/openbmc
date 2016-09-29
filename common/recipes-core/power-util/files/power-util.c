/*
 * power-util
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
#include <getopt.h>
#include <stdbool.h>
#include <fcntl.h>
#include <openbmc/pal.h>

#define POWER_ON_STR        "on"
#define POWER_OFF_STR       "off"

#define MAX_RETRIES          10

const char *pwr_option_list = "status, graceful-shutdown, off, on, cycle, "
  "12V-off, 12V-on, 12V-cycle";

enum {
  PWR_STATUS = 1,
  PWR_GRACEFUL_SHUTDOWN,
  PWR_OFF,
  PWR_ON,
  PWR_CYCLE,
  PWR_12V_OFF,
  PWR_12V_ON,
  PWR_12V_CYCLE,
  PWR_SLED_CYCLE
};

static void
print_usage() {
  printf("Usage: power-util [ %s ] [ %s ]\nUsage: power-util sled-cycle\n",
      pal_server_list, pwr_option_list);
}

static int
get_power_opt(char *option, uint8_t *opt) {

  if (!strcmp(option, "status")) {
    *opt = PWR_STATUS;
  } else if (!strcmp(option, "graceful-shutdown")) {
    *opt = PWR_GRACEFUL_SHUTDOWN;
  } else if (!strcmp(option, "off")) {
    *opt = PWR_OFF;
  } else if (!strcmp(option, "on")) {
    *opt = PWR_ON;
  } else if (!strcmp(option, "cycle")) {
    *opt = PWR_CYCLE;
  } else if (!strcmp(option, "12V-off")) {
    *opt = PWR_12V_OFF;
  } else if (!strcmp(option, "12V-on")) {
    *opt = PWR_12V_ON;
  } else if (!strcmp(option, "12V-cycle")) {
    *opt = PWR_12V_CYCLE;
  } else if (!strcmp(option, "sled-cycle")) {
    *opt = PWR_SLED_CYCLE;
  } else {
    return -1;
  }

  return 0;
}

static int
power_util(uint8_t fru, uint8_t opt) {

  int ret;
  uint8_t status;
  int retries;

  switch(opt) {
    case PWR_STATUS:
      ret = pal_get_server_power(fru, &status);
      if (ret < 0) {
        syslog(LOG_WARNING, "power_util: pal_get_server_power failed for fru %u\n", fru);
        return ret;
      }
      //printf("Power status for fru %u : %s\n", fru, status?"ON":"OFF");
      printf("Power status for fru %u : ", fru);
      switch(status) {
        case SERVER_POWER_ON:
          printf("ON\n");
          break;
        case SERVER_POWER_OFF:
          printf("OFF\n");
          break;
        case SERVER_12V_OFF:
          printf("OFF (12V-OFF)\n");
          break;
      }

      break;

    case PWR_GRACEFUL_SHUTDOWN:

      printf("Shutting down fru %u gracefully...\n", fru);

      ret = pal_set_server_power(fru, SERVER_GRACEFUL_SHUTDOWN);
      if (ret < 0) {
        syslog(LOG_WARNING, "power_util: pal_set_server_power failed for"
          " fru %u", fru);
        return ret;
      } else if (ret == 1) {
        printf("fru %u is already powered OFF...\n", fru);
        return 0;
      } else {
        syslog(LOG_CRIT, "SERVER_GRACEFUL_SHUTDOWN successful for FRU: %d", fru);
      }

      ret = pal_set_last_pwr_state(fru, POWER_OFF_STR);
      if (ret < 0) {
        return ret;
      }

      ret = pal_set_led(fru, LED_STATE_OFF);
      if (ret < 0) {
        syslog(LOG_WARNING, "power_util: pal_set_led failed for fru %u", fru);
        return ret;
      }
      break;

    case PWR_OFF:

      printf("Powering fru %u to OFF state...\n", fru);

      ret = pal_set_server_power(fru, SERVER_POWER_OFF);
      if (ret < 0) {
        syslog(LOG_WARNING, "power_util: pal_set_server_power failed for"
          " fru %u", fru);
        return ret;
      } else if (ret == 1) {
        printf("fru %u is already powered OFF...\n", fru);
        return 0;
      } else {
        syslog(LOG_CRIT, "SERVER_POWER_OFF successful for FRU: %d", fru);
      }

      ret = pal_set_last_pwr_state(fru, POWER_OFF_STR);
      if (ret < 0) {
        return ret;
      }

      ret = pal_set_led(fru, LED_STATE_OFF);
      if (ret < 0) {
        syslog(LOG_WARNING, "power_util: pal_set_led failed for fru %u", fru);
        return ret;
      }
      break;

    case PWR_ON:

      printf("Powering fru %u to ON state...\n", fru);

      ret = pal_set_server_power(fru, SERVER_POWER_ON);
      if (ret == 1) {
        printf("fru %u is already powered ON...\n", fru);
        return 0;
      }
      for (retries = 0; retries < MAX_RETRIES; retries++) {
         sleep(3);
         ret = pal_get_server_power(fru, &status);
         if ((ret >= 0) && (status == SERVER_POWER_ON)) {
           syslog(LOG_CRIT, "SERVER_POWER_ON successful for FRU: %d", fru);
           break;
         }
         ret = pal_set_server_power(fru, SERVER_POWER_ON);
      }
      if (ret < 0) {
        syslog(LOG_WARNING, "power_util: pal_set_server_power failed for"
          " fru %u", fru);
        return ret;
      }

      ret = pal_set_last_pwr_state(fru, POWER_ON_STR);
      if (ret < 0) {
        return ret;
      }

      ret = pal_set_led(fru, LED_STATE_ON);
      if (ret < 0) {
        syslog(LOG_WARNING, "power_util: pal_set_led failed for fru %u", fru);
        return ret;
      }
      break;

    case PWR_CYCLE:

      printf("Power cycling fru %u...\n", fru);

      ret = pal_set_server_power(fru, SERVER_POWER_CYCLE);
      if (ret < 0) {
        syslog(LOG_WARNING, "power_util: pal_set_server_power failed for"
          " fru %u", fru);
        return ret;
      } else {
        syslog(LOG_CRIT, "SERVER_POWER_CYCLE successful for FRU: %d", fru);
      }

      ret = pal_set_last_pwr_state(fru, POWER_ON_STR);
      if (ret < 0) {
        return ret;
      }

      ret = pal_set_led(fru, LED_STATE_ON);
      if (ret < 0) {
        syslog(LOG_WARNING, "power_util: pal_set_led failed for fru %u", fru);
        return ret;
      }
      break;

    case PWR_12V_OFF:

      printf("12V Powering fru %u to OFF state...\n", fru);

      ret = pal_set_server_power(fru, SERVER_12V_OFF);
      if (ret < 0) {
        syslog(LOG_WARNING, "power_util: pal_set_server_power failed for"
          " fru %u", fru);
        return ret;
      } else if (ret == 1) {
        printf("fru %u is already powered 12V-OFF...\n", fru);
        return 0;
      } else {
        syslog(LOG_CRIT, "SERVER_12V_OFF successful for FRU: %d", fru);
      }

      ret = pal_set_last_pwr_state(fru, POWER_OFF_STR);
      if (ret < 0) {
        return ret;
      }

      ret = pal_set_led(fru, LED_STATE_OFF);
      if (ret < 0) {
        syslog(LOG_WARNING, "power_util: pal_set_led failed for fru %u", fru);
        return ret;
      }
      break;

    case PWR_12V_ON:

      printf("12V Powering fru %u to ON state...\n", fru);

      ret = pal_set_server_power(fru, SERVER_12V_ON);
      if (ret < 0) {
        syslog(LOG_WARNING, "power_util: pal_set_server_power failed for"
          " fru %u", fru);
        return ret;
      } else if (ret == 1) {
        printf("fru %u is already powered 12V-ON...\n", fru);
        return 0;
      } else {
        syslog(LOG_CRIT, "SERVER_12V_ON successful for FRU: %d", fru);
      }
      break;

    case PWR_12V_CYCLE:

      printf("12V Power cycling fru %u...\n", fru);

      ret = pal_set_server_power(fru, SERVER_12V_CYCLE);
      if (ret < 0) {
        syslog(LOG_WARNING, "power_util: pal_set_server_power failed for"
          " fru %u", fru);
        return ret;
      } else {
        syslog(LOG_CRIT, "SERVER_12V_CYCLE successful for FRU: %d", fru);
      }

      ret = pal_set_last_pwr_state(fru, POWER_OFF_STR);
      if (ret < 0) {
        return ret;
      }

      ret = pal_set_led(fru, LED_STATE_OFF);
      if (ret < 0) {
        syslog(LOG_WARNING, "power_util: pal_set_led failed for fru %u", fru);
        return ret;
      }
      break;

    case PWR_SLED_CYCLE:
      syslog(LOG_CRIT, "SLED_CYCLE successful");
      sleep(1);
      pal_sled_cycle();
      break;

    default:
      syslog(LOG_WARNING, "power_util: wrong option");

  }

  return ret;
}

int
main(int argc, char **argv) {

  int ret;

  uint8_t fru, status, opt;
  char *option;

  /* Check for sled-cycle */
  if (argc < 2 || argc > 3) {
    print_usage();
    exit (-1);
  }

  option =  argc == 2 ? argv[1] : argv [2];

  ret = get_power_opt(option, &opt);
  /* If argc is 2, the option is sled-cycle */
  if ((ret < 0) || (argc == 2 && opt != PWR_SLED_CYCLE)) {
    printf("Wrong option: %s\n", option);
    print_usage();
    exit(-1);
  }

  if (argc > 2) {
    ret = pal_get_fru_id(argv[1], &fru);
    if (ret < 0) {
      printf("Wrong fru: %s\n", argv[1]);
      print_usage();
      exit(-1);
    }
  } else {
    fru = -1;
  }

  if (argc > 2) {
    ret = pal_is_fru_prsnt(fru, &status);
    if (ret < 0) {
      printf("pal_is_fru_prsnt failed for fru: %d\n", fru);
      print_usage();
      exit(-1);
    }
    if (status == 0) {
      printf("%s is empty!\n", argv[1]);
      print_usage();
      exit(-1);
    }
  }

  ret = power_util(fru, opt);
  if (ret < 0) {
    print_usage();
    return ret;
  }

  return ret;
}
