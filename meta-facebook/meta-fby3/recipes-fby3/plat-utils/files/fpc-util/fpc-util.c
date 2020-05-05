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

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <facebook/bic.h>
#include <openbmc/pal.h>
#include <openbmc/ipmi.h>

static void
print_usage(void) {
  printf("fpc-util <slot1|slot2|slot3|slot4> <1U-dev0|1U-dev1|1U-dev2|1U-dev3> --identify <on/off>\n");
}

int
main(int argc, char **argv) {
  // only support 1OU E1S identify LED function
  uint8_t fru = 0;
  uint8_t type = 0;
  uint8_t dev_id = DEV_NONE;
  int ret = -1;
  int retry = 0;

  if (argc != 5) {
    goto err_exit;
  }
  
  ret = pal_get_fru_id(argv[1], &fru);
  if (ret < 0 || fru < 1 || fru > 4) {
    printf("Wrong fru: %s\n", argv[1]);
    goto err_exit;
  }
  
  bic_get_1ou_type(fru, &type);
  if (type != EDSFF_1U) {
    printf("only support 1OU E1S board, please check the board type. \n");
    goto err_exit;
  }
  
  ret = pal_get_dev_id(argv[2], &dev_id);
  if (ret < 0) {
    printf("pal_get_dev_id failed for %s %s\n", argv[1], argv[2]);
    goto err_exit;
  } else if (dev_id < 1 || dev_id > 4) {
    printf("only support 1OU E1S board, please check the board type. \n");
    goto err_exit;
  }

  if (strcmp(argv[3], "--identify")) {
    goto err_exit;
  }

  if (!strcmp(argv[4], "on")) {
    bic_set_amber_led(fru, dev_id, 0xFE);
  } else if (!strcmp(argv[4], "off")) {
    bic_set_amber_led(fru, dev_id, 0x00);
  } else {
    goto err_exit;
  }
  
  printf("fpc-util: identification for %s %s is %s\n", argv[1], argv[2], argv[4]);
  return 0;


  
  
err_exit:
  print_usage();
  return -1;
}
