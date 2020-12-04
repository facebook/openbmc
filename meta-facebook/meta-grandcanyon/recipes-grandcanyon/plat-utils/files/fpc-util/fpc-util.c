/*
 * fpc-util
 * Copyright 2020-present Facebook. All Rights Reserved.
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

#include <errno.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <syslog.h>
#include <unistd.h>
#include <openbmc/pal.h>
#include <openbmc/ipmi.h>

static void
print_usage_help(void) {
  printf("Usage: fpc-util --identify <on/off>\n");
  printf("Usage: fpc-util --status <yellow/blue/off>\n");
  printf("Usage: fpc-util --e1s0 <on/off>\n");
  printf("Usage: fpc-util --e1s1 <on/off>\n");
}

int
main(int argc, char **argv) {
  char key[MAX_KEY_LEN] = {0};

  if (argc != 3) {
    goto err_exit;
  }

  if (!strncmp(argv[1], "--identify", strlen("--identify"))) {
    if((strncmp(argv[2], "on", strlen("on"))) && (strncmp(argv[2], "off", strlen("off")))) {
      goto err_exit;
    }
    printf("fpc-util: identification is %s\n", argv[2]);
    snprintf(key, sizeof(key), "system_identify");
    return pal_set_key_value(key, argv[2]);

  } else if (!strncmp(argv[1], "--status", strlen("--status"))) {
    if (!strncmp(argv[2], "yellow", strlen("yellow"))) {
      return pal_set_status_led(FRU_UIC, STATUS_LED_YELLOW);
    } else if (!strncmp(argv[2], "blue", strlen("blue"))) {
      return pal_set_status_led(FRU_UIC, STATUS_LED_BLUE);
    } else if (!strncmp(argv[2], "off", strlen("off"))) {
      return pal_set_status_led(FRU_UIC, STATUS_LED_OFF);
    } else {
      goto err_exit;
    }
  } else if (!strncmp(argv[1], "--e1s0", strlen("--e1s0"))) {
    if (!strncmp(argv[2], "on", strlen("on"))) {
      return pal_set_e1s_led(FRU_E1S_IOCM, ID_E1S0_LED, LED_ON);
    } else if (!strncmp(argv[2], "off", strlen("off"))) {
      return pal_set_e1s_led(FRU_E1S_IOCM, ID_E1S0_LED, LED_OFF);
    } else {
      goto err_exit;
    }
  } else if (!strncmp(argv[1], "--e1s1", strlen("--e1s1"))) {
    if (!strncmp(argv[2], "on", strlen("on"))) {
      return pal_set_e1s_led(FRU_E1S_IOCM, ID_E1S1_LED, LED_ON);
    } else if (!strncmp(argv[2], "off", strlen("off"))) {
      return pal_set_e1s_led(FRU_E1S_IOCM, ID_E1S1_LED, LED_OFF);
    } else {
      goto err_exit;
    }
  } else {
    goto err_exit;
  }

  return 0;

err_exit:
  print_usage_help();
  return -1;
}

