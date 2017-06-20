/*
 * fpc-util
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
#include <errno.h>
#include <syslog.h>
#include <stdint.h>
#include <pthread.h>
#include <facebook/bic.h>
#include <openbmc/pal.h>
#include <openbmc/ipmi.h>

static void
print_usage_help(void) {

  int sku = pal_get_iom_type();

  if (sku == IOM_IOC) {
    printf("Usage: fpc-util --ext1 <warning/off>\n");
    printf("Usage: fpc-util --ext2 <warning/off>\n");
  }

  printf("Usage: fpc-util --identify <on/off>\n");
}

int
main(int argc, char **argv) {

  uint8_t slot_id;
  char tstr[MAX_KEY_LEN] = {0};

  if (argc != 3) {
    goto err_exit;
  }

  if (!strcmp(argv[1], "--ext2")) {

    printf("fpc-util: Enable/Disable miniSAS2 Error LED\n");

    if (!strcmp(argv[2], "warning"))
      return pal_minisas_led(SAS_EXT_PORT_2, LED_ON);
    else if (!strcmp(argv[2], "off"))
      return pal_minisas_led(SAS_EXT_PORT_2, LED_OFF);
    else
      goto err_exit;

  } else if (!strcmp(argv[1], "--ext1")) {

    printf("fpc-util: Enable/Disable miniSAS1 Error LED\n");

    if (!strcmp(argv[2], "warning"))
      return pal_minisas_led(SAS_EXT_PORT_1, LED_ON);
    else if (!strcmp(argv[2], "off"))
      return pal_minisas_led(SAS_EXT_PORT_1, LED_OFF);
    else
      goto err_exit;

  } else if (!strcmp(argv[1], "--identify")) {

    if((strcmp(argv[2], "on")) && (strcmp(argv[2], "off")))
      goto err_exit;

    printf("fpc-util: identification is %s\n", argv[2]);
    sprintf(tstr, "identify_slot1");
    return pal_set_key_value(tstr, argv[2]);

  } else {
    goto err_exit;
  }

  return 0;

err_exit:
  print_usage_help();
  return -1;
}
