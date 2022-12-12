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
#include <openbmc/pal.h>
#include <openbmc/obmc-i2c.h>

static void
print_usage(void) {
  printf("Usage: \n");
  printf("fpc-util <slot1|slot2|slot3|slot4> <1U-dev0|1U-dev1|1U-dev2|1U-dev3> --identify <on/off>\n");
  printf("fpc-util sled --identify <on/off>\n");
}

int
main(int argc, char **argv) {
  uint8_t fru = 0;
  uint8_t bmc_location = 0;
  uint8_t status = 0;
  uint8_t identify_idx = 0;
  uint8_t on_off_idx = 0;
  bool is_dev = false;
  bool is_led_on = false;
  int ret = -1;

  // fpc-util slot[1|2|3|4] --identify [on|off]
  // fpc-util slot[1|2|3|4] <dev> --identify [on|off]
  if ( argc != 5 && argc != 4 ) {
    goto err_exit;
  }

  // it's used to identify which component
  if ( argc == 5 ) {
    identify_idx = 3;
    on_off_idx = 4;
    is_dev = true;
  } else {
    identify_idx = 2;
    on_off_idx = 3;
    is_dev = false;
  }

  if ( strcmp(argv[identify_idx], "--identify") != 0 ) {
    goto err_exit;
  }

  if ( strcmp(argv[on_off_idx], "on") == 0 ) {
    is_led_on = true;
  } else if ( strcmp(argv[on_off_idx], "off") == 0 ) {
    is_led_on = false;
  } else {
    goto err_exit;
  }

  if (!strcmp(argv[1] , "sled")) {
    if (is_dev) {
      goto err_exit;
    }
    fru = FRU_ALL;
  } else {
    if ( fby35_common_get_slot_id(argv[1], &fru) < 0 || fru == FRU_ALL ) {
      printf("Wrong fru: %s\n", argv[1]);
      goto err_exit;
    }

    if ( pal_is_fru_prsnt(fru, &status) < 0 || status == 0 ) {
      printf("slot%d is not present!\n", fru);
      goto err_exit;
    }
  }
  if ( is_dev == true ) {
    uint8_t type = TYPE_1OU_UNKNOWN;
    uint8_t dev_id = DEV_NONE;

    // check the location
    if ( fby35_common_get_bmc_location(&bmc_location) < 0 ) {
      printf("Couldn't get the location of BMC\n");
      goto err_exit;
    }

    if ( bmc_location == NIC_BMC ) {
      printf("Config D is not supported!\n");
      goto err_exit;
    }

    ret = bic_is_exp_prsnt(fru);
    if ( ret < 0 ) {
      printf("Couldn't get the status of 1OU/2OU\n");
      goto err_exit;
    }

    // is 1OU present?
    if ( (ret & PRESENT_1OU) != PRESENT_1OU ) {
      printf("1OU board is not present, device identification only support in E1S 1OU board\n");
      goto err_exit;
    }

    if ( bic_get_1ou_type(fru, &type) < 0 || type != TYPE_1OU_VERNAL_FALLS_WITH_AST ) {
      printf("Device identification only support in E1S 1OU board, get %02X (Expected: %02X)\n",
             type, TYPE_1OU_VERNAL_FALLS_WITH_AST);
      goto err_exit;
    }

    // get the dev id
    if ( pal_get_dev_id(argv[2], &dev_id) < 0 ) {
      printf("pal_get_dev_id failed for %s %s\n", argv[1], argv[2]);
      goto err_exit;
    }

    // is dev_id 2ou device?
    if ( dev_id >= DEV_ID0_2OU ) {
      printf("%s is not supported!\n", argv[2]);
      goto err_exit;
    }

    if ( is_led_on == true ) {
      ret = bic_set_amber_led(fru, dev_id, 0x01);
    } else {
      ret = bic_set_amber_led(fru, dev_id, 0x00);
    }
    printf("fpc-util: identification for %s %s is set to %s %ssuccessfully\n", \
                         argv[1], argv[2], argv[4], (ret < 0)?"un":"");
  } else {
    if (fru == FRU_ALL) {
      ret = 0;
      for (fru = FRU_SLOT1; fru <= FRU_SLOT4; fru++) {
        if ( pal_is_fru_prsnt(fru, &status)  == 0 && status == 1 ) {
          ret |= pal_sb_set_amber_led(fru, is_led_on, LED_LOCATE_MODE);
        }
      }
    } else {
      ret = pal_sb_set_amber_led(fru, is_led_on, LED_LOCATE_MODE);
    }
    printf("fpc-util: identification for %s is set to %s %ssuccessfully\n", \
                         argv[1], argv[3], (ret < 0)?"un":"");
  }

  return 0;

err_exit:
  print_usage();
  return -1;
}
