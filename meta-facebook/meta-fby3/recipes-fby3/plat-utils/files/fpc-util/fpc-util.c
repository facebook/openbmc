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
  printf("fpc-util <slot1|slot2|slot3|slot4> <1U-dev0|1U-dev1|1U-dev2|1U-dev3> --identify <on/off>\n");
}

static int
sb_set_amber_led(uint8_t fru, bool led_on) {
  int ret = 0;
  int i2cfd = -1;
  uint8_t bus = 0;

  ret = fby3_common_get_bus_id(fru);
  if ( ret < 0 ) {
    printf("%s() Couldn't get the bus id of fru%d\n", __func__, fru);
    goto err_exit;
  }
  bus = (uint8_t)ret + 4;

  i2cfd = i2c_cdev_slave_open(bus, SB_CPLD_ADDR, I2C_SLAVE_FORCE_CLAIM);
  if ( i2cfd < 0 ) {
    printf("%s() Couldn't open i2c bus%d, err: %s\n", __func__, bus, strerror(errno));
    goto err_exit;
  }

  uint8_t tbuf[2] = {0xf, (led_on == true)?0x01:0x00};
  ret = i2c_rdwr_msg_transfer(i2cfd, (SB_CPLD_ADDR << 1), tbuf, 2, NULL, 0);
  if ( ret < 0 ) {
    printf("%s() Couldn't write data to addr %02X, err: %s\n",  __func__, SB_CPLD_ADDR, strerror(errno));
  }

err_exit:
  if ( i2cfd > 0 ) close(i2cfd);
  return ret;
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

  if ( strcmp(argv[identify_idx], "--identify") != 0 ) goto err_exit;
  if ( strcmp(argv[on_off_idx], "on") == 0 ) is_led_on = true;
  else if ( strcmp(argv[on_off_idx], "off") == 0 ) is_led_on = false;
  else goto err_exit;

  if ( fby3_common_get_bmc_location(&bmc_location) < 0 ) {
    printf("Couldn't get the location of BMC\n");
    goto err_exit;
  }

  if ( fby3_common_get_slot_id(argv[1], &fru) < 0 || fru == FRU_ALL ) {
    printf("Wrong fru: %s\n", argv[1]);
    goto err_exit;
  }

  if ( pal_is_fru_prsnt(fru, &status) < 0 || status == 0 ) {
    printf("slot%d is not present!\n", fru);
    goto err_exit;
  }

  if ( is_dev == true ) {
    uint8_t type = 0;
    uint8_t dev_id = DEV_NONE;

    // check the location
    if ( bmc_location == NIC_BMC ) {
      printf("1OU is not supported!\n");
      goto err_exit;
    }

    // reset status and read bic_is_m2_exp_prsnt_cache
    status = CONFIG_UNKNOWN;
    ret = bic_is_m2_exp_prsnt_cache(fru);
    if ( ret < 0 ) {
      printf("Couldn't read bic_is_m2_exp_prsnt_cache\n");
      goto err_exit;
    }

    // is EDSFF_1U present?
    status = (uint8_t)ret;
    if ( bic_get_1ou_type(fru, &type) < 0 || type != EDSFF_1U ) {
      printf("Only support 1OU E1S board, get %02X(Expected: %02X)\n", type, EDSFF_1U);
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

    if ( is_led_on == true ) ret = bic_set_amber_led(fru, dev_id, 0xFE);
    else ret = bic_set_amber_led(fru, dev_id, 0x00);
    printf("fpc-util: identification for %s %s is set to %s %ssuccessfully\n", \
                         argv[1], argv[2], argv[4], (ret < 0)?"un":"");
  } else {
    ret = sb_set_amber_led(fru, is_led_on);
    printf("fpc-util: identification for %s is set to %s %ssuccessfully\n", \
                         argv[1], argv[3], (ret < 0)?"un":"");
  }

  return 0;

err_exit:
  print_usage();
  return -1;
}
