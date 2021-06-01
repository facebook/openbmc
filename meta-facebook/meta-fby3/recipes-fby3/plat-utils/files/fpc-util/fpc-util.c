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
#include <openbmc/kv.h>

#define MAX_OPTION_NUM 5

uint8_t type_2ou = 0;

static void
print_usage(void) {
  if (type_2ou == E1S_BOARD) {
    printf("fpc-util slot1 <2U-dev0|2U-dev1|2U-dev2|2U-dev3|2U-dev4|2U-dev5> --identify <on/off/blink/stop/status>\n");
  } else {
    printf("fpc-util <slot1|slot2|slot3|slot4> <1U-dev0|1U-dev1|1U-dev2|1U-dev3> --identify <on/off>\n");
  }  
}

int
main(int argc, char **argv) {
  uint8_t fru = 0;
  uint8_t bmc_location = 0;
  uint8_t status = 0;
  uint8_t identify_idx = 0;
  uint8_t on_off_idx = 0;
  uint8_t option = CMD_UNKNOWN;
  bool is_dev = false;
  int ret = -1;
  int i = 0;
  char* opt_str[MAX_OPTION_NUM] = {"off", "on", "blink", "stop", "status"};
  char sys_conf[MAX_VALUE_LEN] = {0};

  if (kv_get("sled_system_conf", sys_conf, NULL, KV_FPERSIST) < 0) {
    printf("Failed to read sled_system_conf");
    return -1;
  }
  if (strcmp(sys_conf, "Type_8") == 0) {
    type_2ou = E1S_BOARD;
  }

  // fpc-util slot[1|2|3|4] --identify [on|off]
  // fpc-util slot[1|2|3|4] <dev> --identify [on|off]
  if ( argc != 5 && argc != 4 ) {
    goto err_exit;
  }
  // check the location
  if ( fby3_common_get_bmc_location(&bmc_location) < 0 ) {
    printf("Couldn't get the location of BMC\n");
    goto err_exit;
  }

  if ( fby3_common_get_slot_id(argv[1], &fru) < 0 || fru == FRU_ALL ) {
    printf("Wrong fru: %s\n", argv[1]);
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

  for (i = 0; i < MAX_OPTION_NUM; i++) {
    if (strcmp(argv[on_off_idx], opt_str[i]) == 0) {
      option = i;
    }
  }
  if (option == CMD_UNKNOWN) {
    goto err_exit;
  }

  if (type_2ou != E1S_BOARD) {
    if ((option == SET_LED_BLINK) || (option == SET_LED_STOP) || (option == GET_LED_STAT)) {
      printf("Option %s only supported on Sierra Point system\n", argv[on_off_idx]);
      goto err_exit;
    }
  }
 
  if ( pal_is_fru_prsnt(fru, &status) < 0 || status == 0 ) {
    printf("slot%d is not present!\n", fru);
    goto err_exit;
  }

  if ( is_dev == true ) {
    uint8_t type = 0;
    uint8_t dev_id = DEV_NONE;

    if ((bmc_location == NIC_BMC) && (type_2ou != E1S_BOARD)) {
      printf("Config C only support Sierra Point system!\n");
      goto err_exit;
    }

    // reset status and read bic_is_m2_exp_prsnt_cache
    status = CONFIG_UNKNOWN;
    status = bic_is_m2_exp_prsnt_cache(fru);
    if ( status < 0 ) {
      printf("Couldn't read bic_is_m2_exp_prsnt_cache\n");
      goto err_exit;
    }

    if (bmc_location != NIC_BMC) {
      // is 1OU present?
      if ( (status & PRESENT_1OU) != PRESENT_1OU ) {
        printf("1OU board is not present, device identification only support in E1S 1OU board\n");
        goto err_exit;
      }

      if ( bic_get_1ou_type(fru, &type) < 0 || type != EDSFF_1U ) {
        printf("Device identification only support in E1S 1OU board, get %02X (Expected: %02X)\n", type, EDSFF_1U);
        goto err_exit;
      }
    }

    // get the dev id
    if ( pal_get_dev_id(argv[2], &dev_id) < 0 ) {
      printf("pal_get_dev_id failed for %s %s\n", argv[1], argv[2]);
      goto err_exit;
    }

    // is dev_id 2ou device?
    if ( (dev_id >= DEV_ID0_2OU) && (type_2ou != E1S_BOARD) ) {
      printf("%s is not supported!\n", argv[2]);
      goto err_exit;
    }

    if (type_2ou == E1S_BOARD) {
      ret = bic_spe_led_ctrl(dev_id, option, &status);
      if (option == GET_LED_STAT) {
        if (ret == 0) {
          printf("fpc-util: %s LED status: %s\n", argv[2], opt_str[status]);
        } else {
          printf("fpc-util: failed to get %s LED status\n", argv[2]);
        }
        return 0;        
      }
    } else {
      if ( option == SET_LED_ON ) {
        ret = bic_set_amber_led(fru, dev_id, 0x01);
      } else {
        ret = bic_set_amber_led(fru, dev_id, 0x00);
      }
    }

    printf("fpc-util: identification for %s %s is set to %s %ssuccessfully\n", \
                         argv[1], argv[2], argv[4], (ret < 0)?"un":"");
  } else {
    ret = pal_sb_set_amber_led(fru, option, LED_LOCATE_MODE);
    printf("fpc-util: identification for %s is set to %s %ssuccessfully\n", \
                         argv[1], argv[3], (ret < 0)?"un":"");
  }

  return 0;

err_exit:
  print_usage();
  return -1;
}
