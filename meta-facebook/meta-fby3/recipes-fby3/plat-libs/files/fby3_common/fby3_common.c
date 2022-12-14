/*
 *
 * Copyright 2015-present Facebook. All Rights Reserved.
 *
 * This file contains code to support IPMI2.0 Specificaton available @
 * http://www.intel.com/content/www/us/en/servers/ipmi/ipmi-specifications.html
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
#include <stdint.h>
#include <stdbool.h>
#include <unistd.h>
#include <math.h>
#include <fcntl.h>
#include <errno.h>
#include <syslog.h>
#include <pthread.h>
#include <string.h>
#include <openbmc/libgpio.h>
#include <openbmc/obmc-i2c.h>
#include <openbmc/kv.h>
#include "fby3_common.h"

const char *slot_usage = "slot1|slot2|slot3|slot4";
const char *slot_list[] = {"all", "slot1", "slot2", "slot3", "slot4", "bb", "nic", "bmc", "nicexp", "slot1-2U", "slot1-2U-exp", "slot1-2U-top", "slot1-2U-bot", "slot3-2U", "ocpdbg"};
const char *exp_list[] = {"2U", "2U-cwc", "2U-top", "2U-bot"};
const char plat_sig[PLAT_SIG_SIZE] = "Yosemite V3     ";
const uint8_t exp_id_list[] = {FRU_2U, FRU_CWC, FRU_2U_TOP, FRU_2U_BOT};
const uint8_t slot3_exp_id_list[] = {FRU_2U_SLOT3};
const char *slot1_exp_list[] = {"slot1-2U", "slot1-2U-exp", "slot1-2U-top", "slot1-2U-bot"};
const char *slot3_exp_list[] = {"slot3-2U"};
const int slot1_exp_size = sizeof(slot1_exp_list)/sizeof(slot1_exp_list[0]);
const int slot3_exp_size = sizeof(slot3_exp_list)/sizeof(slot3_exp_list[0]);

int
fby3_common_set_fru_i2c_isolated(uint8_t fru, uint8_t val) {
  return gpio_set_value_by_shadow(gpio_server_i2c_isolated[fru], (val==0)?GPIO_VALUE_LOW:GPIO_VALUE_HIGH);
}

int
fby3_common_get_fru_id(char *str, uint8_t *fru) {
  int fru_id = 0;

  for (fru_id = FRU_ALL; fru_id <= MAX_NUM_FRUS; fru_id++) {
    if ( strcmp(str, slot_list[fru_id]) == 0 ) {
      *fru = fru_id;
      return 0;
    }
  }
  return -1;
}

int
fby3_common_get_slot_id(char *str, uint8_t *fru) {
  int fru_id = 0;
  bool found_id = false;

  for (fru_id = FRU_ALL; fru_id <= FRU_SLOT4; fru_id++) {
    if ( strcmp(str, slot_list[fru_id]) == 0 ) {
      *fru = fru_id;
      found_id = true;
      break;
    }
  }

  if ( found_id == false ) {
    return -1;
  }

  return 0;
}

int
fby3_common_exp_get_num_devs(uint8_t fru, uint8_t *num) {
  switch (fru) {
    case FRU_2U_TOP:
    case FRU_2U_BOT:
      *num = DEV_ID13_2OU;
      return 0;
    break;
  }

  return -1;
}

int
fby3_common_check_slot_id(uint8_t fru) {
  uint8_t bmc_location = 0;

  if ( fby3_common_get_bmc_location(&bmc_location) < 0 ) {
    syslog(LOG_WARNING, "%s() Cannot get the location of BMC", __func__);
    goto error_exit;
  }

  if (bmc_location == NIC_BMC && fru != FRU_SLOT1) {
    goto error_exit;
  }

  switch (fru) {
    case FRU_SLOT1:
    case FRU_SLOT2:
    case FRU_SLOT3:
    case FRU_SLOT4:
      return 0;
    break;
  }

error_exit:
  return -1;
}

int
fby3_common_is_fru_prsnt(uint8_t fru, uint8_t *val) {
  gpio_value_t gpio_value;
  gpio_value = gpio_get_value_by_shadow(gpio_server_prsnt[fru]);
  if ( gpio_value == GPIO_VALUE_INVALID ) {
    return -1;
  }
  //0: the fru isn't present
  //1: the fru is present
  *val = (gpio_value == GPIO_VALUE_HIGH)?0:1;
  return 0;
}

int
fby3_common_server_stby_pwr_sts(uint8_t fru, uint8_t *val) {
  int ret = 0;
  uint8_t bmc_location = 0;
  gpio_value_t gpio_value;

  ret = fby3_common_get_bmc_location(&bmc_location);
  if ( ret < 0 ) {
    syslog(LOG_WARNING, "%s() Cannot get the location of BMC", __func__);
    return ret;
  }

  if ( bmc_location == BB_BMC || bmc_location == DVT_BB_BMC ) {
    gpio_value = gpio_get_value_by_shadow(gpio_server_stby_pwr_sts[fru]);
    if ( gpio_value == GPIO_VALUE_INVALID ) {
      return -1;
    }
    *val = (uint8_t)gpio_value;
  } else {
    //1: a server is always existed on class2
    *val = 1;
  }

  return ret;
}

int
fby3_common_is_bic_ready(uint8_t fru, uint8_t *val) {
  int i2cfd = 0;
  int ret = 0;
  uint8_t bmc_location = 0;
  uint8_t bus = 0;
  uint8_t tbuf[1] = {0x02};
  uint8_t rbuf[1] = {0};
  uint8_t tlen = 1;
  uint8_t rlen = 1;

  ret = fby3_common_get_bmc_location(&bmc_location);
  if ( ret < 0 ) {
    syslog(LOG_WARNING, "%s() Cannot get the location of BMC", __func__);
    goto error_exit;
  }

  //a bus starts from 4
  ret = fby3_common_get_bus_id(fru) + 4;
  if ( ret < 0 ) {
    syslog(LOG_WARNING, "%s() Cannot get the bus with fru%d", __func__, fru);
    goto error_exit;
  }

  bus = (uint8_t)ret;
  i2cfd = i2c_cdev_slave_open(bus, CPLD_ADDRESS >> 1, I2C_SLAVE_FORCE_CLAIM);
  if ( i2cfd < 0 ) {
    syslog(LOG_WARNING, "%s() Failed to open bus %d. Err: %s", __func__, bus, strerror(errno));
    goto error_exit;
  }

  ret = i2c_rdwr_msg_transfer(i2cfd, CPLD_ADDRESS, tbuf, tlen, rbuf, rlen);
  if ( ret < 0 ) {
    syslog(LOG_WARNING, "%s() Failed to do i2c_rdwr_msg_transfer, tlen=%d", __func__, tlen);
    goto error_exit;
  }

  *val = (rbuf[0] & 0x2) >> 1;

error_exit:
  if ( i2cfd > 0 ) {
    close(i2cfd);
  }

  return ret;
}

int
fby3_common_get_bus_id(uint8_t slot_id) {
  int bus_id = 0;
  switch(slot_id) {
    case FRU_SLOT1:
      bus_id = IPMB_SLOT1_I2C_BUS;
    break;
    case FRU_SLOT2:
      bus_id = IPMB_SLOT2_I2C_BUS;
    break;
    case FRU_SLOT3:
      bus_id = IPMB_SLOT3_I2C_BUS;
    break;
    case FRU_SLOT4:
      bus_id = IPMB_SLOT4_I2C_BUS;
    break;
    default:
      bus_id = -1;
    break;
  }
  return bus_id;
}

int
fby3_common_get_bmc_location(uint8_t *id) {
  static bool is_cached = false;
  static unsigned int cached_id = 0;

  if ( is_cached == false ) {
    const char *shadows[] = {
      "BOARD_BMC_ID0_R",
      "BOARD_BMC_ID1_R",
      "BOARD_BMC_ID2_R",
      "BOARD_BMC_ID3_R",
    };

    if ( gpio_get_value_by_shadow_list(shadows, ARRAY_SIZE(shadows), &cached_id) ) {
      return -1;
    }

    is_cached = true;
  }

  *id = (uint8_t)cached_id;

  return 0;
}

int
fby3_common_get_slot_type(uint8_t fru) {
  int ret = 0;

  ret = fby3_common_check_slot_id(fru);
  if ( ret < 0 ) {
    syslog(LOG_WARNING, "%s() Unknown fru: %d", __func__, fru);
    return -1;
  }

  return SERVER_TYPE_DL;
}

#define CRASHDUMP_BIN       "/usr/local/bin/autodump.sh"

int
fby3_common_crashdump(uint8_t fru, bool ierr, bool platform_reset, bool power_off) {
  int ret = 0;
  char cmd[128] = "\0";
  int cmd_len = sizeof(cmd);

  // Check if the crashdump script exist
  if (access(CRASHDUMP_BIN, F_OK) == -1) {
    syslog(LOG_CRIT, "%s() Try to run autodump for %d but %s is not existed", __func__, fru, CRASHDUMP_BIN);
    return ret;
  }

  if ( platform_reset == true ) {
    snprintf(cmd, cmd_len, "%s slot%d --second &", CRASHDUMP_BIN, fru);
  } else {
    if (power_off) {
      snprintf(cmd, cmd_len, "%s slot%d --power_off &", CRASHDUMP_BIN, fru);
    } else {
      snprintf(cmd, cmd_len, "%s slot%d &", CRASHDUMP_BIN, fru);
    }
  }

  if ( system(cmd) != 0 ) {
    syslog(LOG_INFO, "%s() Crashdump for FRU: %d is failed to be generated.", __func__, fru);
  } else {
    syslog(LOG_INFO, "%s() Crashdump for FRU: %d is being generated.", __func__, fru);
  }
  return 0;
}

int
fby3_common_dev_id(char *str, uint8_t *dev) {

  if (!strcmp(str, "all")) {
    *dev = FRU_ALL;
  } else if (!strcmp(str, "1U-dev0")) {
    *dev = DEV_ID0_1OU;
  } else if (!strcmp(str, "1U-dev1")) {
    *dev = DEV_ID1_1OU;
  } else if (!strcmp(str, "1U-dev2")) {
    *dev = DEV_ID2_1OU;
  } else if (!strcmp(str, "1U-dev3")) {
    *dev = DEV_ID3_1OU;
  } else if (!strcmp(str, "2U-dev0")) {
    *dev = DEV_ID0_2OU;
  } else if (!strcmp(str, "2U-dev1")) {
    *dev = DEV_ID1_2OU;
  } else if (!strcmp(str, "2U-dev2")) {
    *dev = DEV_ID2_2OU;
  } else if (!strcmp(str, "2U-dev3")) {
    *dev = DEV_ID3_2OU;
  } else if (!strcmp(str, "2U-dev4")) {
    *dev = DEV_ID4_2OU;
  } else if (!strcmp(str, "2U-dev5")) {
    *dev = DEV_ID5_2OU;
  } else if (!strcmp(str, "2U-dev6")) {
    *dev = DEV_ID6_2OU;
  } else if (!strcmp(str, "2U-dev7")) {
    *dev = DEV_ID7_2OU;
  } else if (!strcmp(str, "2U-dev8")) {
    *dev = DEV_ID8_2OU;
  } else if (!strcmp(str, "2U-dev9")) {
    *dev = DEV_ID9_2OU;
  } else if (!strcmp(str, "2U-dev10")) {
    *dev = DEV_ID10_2OU;
  } else if (!strcmp(str, "2U-dev11")) {
    *dev = DEV_ID11_2OU;
  } else if (!strcmp(str, "2U-dev12")) {
    *dev = DEV_ID12_2OU;
  } else if (!strcmp(str, "2U-dev13")) {
    *dev = DEV_ID13_2OU;
  } else if (!strcmp(str, "2U-dev0_1")) {
    *dev = DUAL_DEV_ID0_2OU;
  } else if (!strcmp(str, "2U-dev2_3")) {
    *dev = DUAL_DEV_ID1_2OU;
  } else if (!strcmp(str, "2U-dev4_5")) {
    *dev = DUAL_DEV_ID2_2OU;
  } else if (!strcmp(str, "2U-dev6_7")) {
    *dev = DUAL_DEV_ID3_2OU;
  } else if (!strcmp(str, "2U-dev8_9")) {
    *dev = DUAL_DEV_ID4_2OU;
  } else if (!strcmp(str, "2U-dev10_11")) {
    *dev = DUAL_DEV_ID5_2OU;
  } else if (!strcmp(str, "1U")) {
    *dev = BOARD_1OU;
  } else if (!strcmp(str, "2U")) {
    *dev = BOARD_2OU;
  } else if (!strcmp(str, DEV_NAME_2U_TOP)) {
    *dev = BOARD_2OU_TOP;
  } else if (!strcmp(str, DEV_NAME_2U_BOT)) {
    *dev = BOARD_2OU_BOT;
  } else if (!strcmp(str, DEV_NAME_2U_CWC)) {
    *dev = BOARD_2OU_CWC;
  } else {
#ifdef DEBUG
    syslog(LOG_WARNING, "fby3_common_dev_id: Wrong fru id");
#endif
    return -1;
  }

  return 0;
}

int
fby3_common_get_exp_dev_id(char *str, uint8_t *dev) {

  if (!strcmp(str, "all")) {
    *dev = FRU_ALL;
  } else if (!strcmp(str, "2U-dev0")) {
    *dev = DEV_ID0_2OU;
  } else if (!strcmp(str, "2U-dev1")) {
    *dev = DEV_ID1_2OU;
  } else if (!strcmp(str, "2U-dev2")) {
    *dev = DEV_ID2_2OU;
  } else if (!strcmp(str, "2U-dev3")) {
    *dev = DEV_ID3_2OU;
  } else if (!strcmp(str, "2U-dev4")) {
    *dev = DEV_ID4_2OU;
  } else if (!strcmp(str, "2U-dev5")) {
    *dev = DEV_ID5_2OU;
  } else if (!strcmp(str, "2U-dev6")) {
    *dev = DEV_ID6_2OU;
  } else if (!strcmp(str, "2U-dev7")) {
    *dev = DEV_ID7_2OU;
  } else if (!strcmp(str, "2U-dev8")) {
    *dev = DEV_ID8_2OU;
  } else if (!strcmp(str, "2U-dev9")) {
    *dev = DEV_ID9_2OU;
  } else if (!strcmp(str, "2U-dev10")) {
    *dev = DEV_ID10_2OU;
  } else if (!strcmp(str, "2U-dev11")) {
    *dev = DEV_ID11_2OU;
  } else if (!strcmp(str, "2U-dev12")) {
    *dev = DEV_ID12_2OU;
  } else if (!strcmp(str, "2U-dev13")) {
    *dev = DEV_ID13_2OU;
  }  else {
#ifdef DEBUG
    syslog(LOG_WARNING, "fby3_common_dev_id: Wrong fru id");
#endif
    return -1;
  }

  return 0;
}


int
fby3_common_dev_name(uint8_t dev, char *str) {

  if (dev == FRU_ALL) {
    strcpy(str, "all");
  } else if (dev == DEV_ID0_1OU) {
    strcpy(str, "1U-dev0");
  } else if (dev == DEV_ID1_1OU) {
    strcpy(str, "1U-dev1");
  } else if (dev == DEV_ID2_1OU) {
    strcpy(str, "1U-dev2");
  } else if (dev == DEV_ID3_1OU) {
    strcpy(str, "1U-dev3");
  } else if (dev == DEV_ID0_2OU) {
    strcpy(str, "2U-dev0");
  } else if (dev == DEV_ID1_2OU) {
    strcpy(str, "2U-dev1");
  } else if (dev == DEV_ID2_2OU) {
    strcpy(str, "2U-dev2");
  } else if (dev == DEV_ID3_2OU) {
    strcpy(str, "2U-dev3");
  } else if (dev == DEV_ID4_2OU) {
    strcpy(str, "2U-dev4");
  } else if (dev == DEV_ID5_2OU) {
    strcpy(str, "2U-dev5");
  } else if (dev == DEV_ID6_2OU) {
    strcpy(str, "2U-dev6");
  } else if (dev == DEV_ID7_2OU) {
    strcpy(str, "2U-dev7");
  } else if (dev == DEV_ID8_2OU) {
    strcpy(str, "2U-dev8");
  } else if (dev == DEV_ID9_2OU) {
    strcpy(str, "2U-dev9");
  } else if (dev == DEV_ID10_2OU) {
    strcpy(str, "2U-dev10");
  } else if (dev == DEV_ID11_2OU) {
    strcpy(str, "2U-dev11");
  } else if (dev == DEV_ID12_2OU) {
    strcpy(str, "2U-dev12");
  } else if (dev == DEV_ID13_2OU) {
    strcpy(str, "2U-dev13");
  } else if (dev == BOARD_1OU) {
    strcpy(str, "1U");
  } else if (dev == BOARD_2OU) {
    strcpy(str, "2U");
  } else if (dev == BOARD_2OU_TOP) {
    strcpy(str, DEV_NAME_2U_TOP);
  } else if (dev == BOARD_2OU_BOT) {
    strcpy(str, DEV_NAME_2U_BOT);
  } else if (dev == BOARD_2OU_CWC) {
    strcpy(str, DEV_NAME_2U_CWC);
  } else {
#ifdef DEBUG
    syslog(LOG_WARNING, "fby3_common_dev_id: Wrong fru id");
#endif
    return -1;
  }
  return 0;
}

int
fby3_common_exp_dev_name(uint8_t dev, char *str) {

  if (dev == FRU_ALL) {
    strcpy(str, "all");
  } else if (dev == DEV_ID0_1OU) {
    strcpy(str, "1U-dev0");
  } else if (dev == DEV_ID1_1OU) {
    strcpy(str, "1U-dev1");
  } else if (dev == DEV_ID2_1OU) {
    strcpy(str, "1U-dev2");
  } else if (dev == DEV_ID3_1OU) {
    strcpy(str, "1U-dev3");
  } else if (dev == DEV_ID0_2OU) {
    strcpy(str, "2U-dev0");
  } else if (dev == DEV_ID1_2OU) {
    strcpy(str, "2U-dev1");
  } else if (dev == DEV_ID2_2OU) {
    strcpy(str, "2U-dev2");
  } else if (dev == DEV_ID3_2OU) {
    strcpy(str, "2U-dev3");
  } else if (dev == DEV_ID4_2OU) {
    strcpy(str, "2U-dev4");
  } else if (dev == DEV_ID5_2OU) {
    strcpy(str, "2U-dev5");
  } else if (dev == DEV_ID6_2OU) {
    strcpy(str, "2U-dev6");
  } else if (dev == DEV_ID7_2OU) {
    strcpy(str, "2U-dev7");
  } else if (dev == DEV_ID8_2OU) {
    strcpy(str, "2U-dev8");
  } else if (dev == DEV_ID9_2OU) {
    strcpy(str, "2U-dev9");
  } else if (dev == DEV_ID10_2OU) {
    strcpy(str, "2U-dev10");
  } else if (dev == DEV_ID11_2OU) {
    strcpy(str, "2U-dev11");
  } else if (dev == DEV_ID12_2OU) {
    strcpy(str, "2U-dev12");
  } else if (dev == DEV_ID13_2OU) {
    strcpy(str, "2U-dev13");
  } else {
#ifdef DEBUG
    syslog(LOG_WARNING, "fby3_common_dev_id: Wrong fru id");
#endif
    return -1;
  }
  return 0;
}


static int
_fby3_common_get_2ou_board_type(uint8_t fru_id, uint8_t *board_type) {
  uint8_t bus_num = 0;
  int fd = -1, ret = 0;
  int tlen = 0, rlen = 0;
  uint8_t tbuf[64], rbuf[64];

  bus_num = fby3_common_get_bus_id(fru_id) + 4;
  fd = i2c_cdev_slave_open(bus_num, CPLD_ADDRESS >> 1, I2C_SLAVE_FORCE_CLAIM);
  if ( fd < 0 ) {
    syslog(LOG_ERR, "Failed to open bus %d: %s", bus_num, strerror(errno));
    return -1;
  }

  tbuf[0] = CPLD_BOARD_OFFSET;
  tlen = 1;
  rlen = 1;
  ret = i2c_rdwr_msg_transfer(fd, CPLD_ADDRESS, tbuf, tlen, rbuf, rlen);
  if ( ret < 0 ) {
    syslog(LOG_WARNING, "%s() Failed to do i2c_rdwr_msg_transfer, tlen=%d, errno: %s", __func__, tlen, strerror(errno));
    goto error_exit;
  }
  *board_type = rbuf[0];

error_exit:
  close(fd);

  return ret;
}

int
fby3_common_get_2ou_board_type(uint8_t fru_id, uint8_t *board_type) {
  char key[MAX_VALUE_LEN] = {0};
  char value[MAX_VALUE_LEN] = {0};

  sprintf(key, "fru%u_2ou_board_type", fru_id);

  if (kv_get(key, value, NULL, 0) == 0) {
    *board_type = ((uint8_t*)value)[0];
    return 0;
  }

  if (_fby3_common_get_2ou_board_type(fru_id, board_type) < 0) {
    return -1;
  }

  if (kv_set(key, (char*)board_type, 1, KV_FCREATE)) {
    syslog(LOG_WARNING,"%s: kv_set failed, key: %s, val: %u", __func__, key, board_type[0]);
  }

  return 0;
}

int
fby3_common_get_sb_board_rev(uint8_t slot_id, uint8_t *rev) {
  int i2cfd = 0;
  int retry = 3;
  uint8_t tbuf[1] = {SB_CPLD_BOARD_REV_ID_REGISTER};
  uint8_t rbuf[1] = {0};

  i2cfd = i2c_cdev_slave_open(slot_id + SLOT_BUS_BASE, CPLD_ADDRESS >> 1, I2C_SLAVE_FORCE_CLAIM);
  if ( i2cfd < 0 ) {
    syslog(LOG_WARNING, "%s() Failed to open bus %d: %s",
           __func__, slot_id + SLOT_BUS_BASE, strerror(errno));
    return -1;
  }

  do {
    if (!i2c_rdwr_msg_transfer(i2cfd, CPLD_ADDRESS, tbuf, 1, rbuf, 1)) {
      break;
    }
    if (--retry) {
      usleep(100*1000);
    }
  } while (retry > 0);

  close(i2cfd);
  if (retry <= 0) {
    return -1;
  }

  *rev = rbuf[0];
  return 0;
}

int
fby3_common_get_bb_board_rev(uint8_t *rev) {
  int i2cfd;
  int retry = 3;
  uint8_t tbuf[4] = {BB_CPLD_BOARD_REV_ID_REGISTER};
  uint8_t rbuf[4] = {0};
  static bool is_cached = false;
  static uint8_t cached_id = 0;

  if ( is_cached == false ) {
    i2cfd = i2c_cdev_slave_open(BB_CPLD_BUS, CPLD_ADDRESS >> 1, I2C_SLAVE_FORCE_CLAIM);
    if ( i2cfd < 0 ) {
      syslog(LOG_WARNING, "%s() Failed to open bus %d: %s",
             __func__, BB_CPLD_BUS, strerror(errno));
      return -1;
    }

    do {
      if (!i2c_rdwr_msg_transfer(i2cfd, CPLD_ADDRESS, tbuf, 1, rbuf, 1)) {
        break;
      }
      if (--retry) {
        usleep(100*1000);
      }
    } while (retry > 0);

    close(i2cfd);
    if (retry <= 0) {
      return -1;
    }
    cached_id = rbuf[0];
    is_cached = true;
  }

  *rev = cached_id;
  return 0;
}

int
fby3_common_get_hsc_bb_detect(uint8_t *id) {
  static bool is_cached = false;
  static unsigned int cached_id = 0;

  if ( is_cached == false ) {
    const char *shadows[] = {
      "HSC_BB_BMC_DETECT0",
      "HSC_BB_BMC_DETECT1",
      "HSC_BB_BMC_DETECT2",
    };

    if ( gpio_get_value_by_shadow_list(shadows, ARRAY_SIZE(shadows), &cached_id) ) {
      return -1;
    }
    is_cached = true;
  }

  *id = (uint8_t)cached_id;
  return 0;
}

int
fby3_common_fscd_ctrl(uint8_t mode) {
  int ret = 0;
  char cmd[MAX_SYS_REQ_LEN] = {0};
  char buf[MAX_SYS_RESP_LEN] = {0};
  bool is_fscd_run = false;
  FILE* fp = NULL;
  FILE* fp2 = NULL;

  // check fscd status
  snprintf(cmd, sizeof(cmd), "sv status fscd");
  if ((fp = popen(cmd, "r")) == NULL) {
    syslog(LOG_WARNING, "%s() failed to get fscd status", __func__);
    return -1;
  }
  memset(buf, 0, sizeof(buf));
  if (fgets(buf, sizeof(buf), fp) == NULL) {
    syslog(LOG_WARNING, "%s() read popen failed, cmd: %s", __func__, cmd);
    ret = -1;
    goto exit;
  }

  if (strstr(buf, "run") != NULL) {
    is_fscd_run = true;
  } else {
    is_fscd_run = false;
  }

  if (mode == FAN_MANUAL_MODE) {
    if (is_fscd_run == false) {
      // fscd already stopped
      goto exit;
    }
    snprintf(cmd, sizeof(cmd), "sv force-stop fscd > /dev/null 2>&1");
    if (system(cmd) != 0) {
      // Although sv force-stop sends kill (-9) signal after timeout,
      // it still returns an error code.
      // we will check status here to ensure that fscd has stopped completely.
      syslog(LOG_WARNING, "%s() fscd force-stop timeout", __func__);
      snprintf(cmd, sizeof(cmd), "sv status fscd");
      if ((fp2 = popen(cmd, "r")) == NULL) {
        syslog(LOG_WARNING, "%s() popen failed, cmd: %s", __func__, cmd);
        ret = -1;
        goto exit;
      }
      memset(buf, 0, sizeof(buf));
      if (fgets(buf, sizeof(buf), fp2) == NULL) {
        syslog(LOG_WARNING, "%s() read popen failed, cmd: %s", __func__, cmd);
        ret = -1;
        goto exit;
      }
      if (strstr(buf, "down") == NULL) {
        syslog(LOG_WARNING, "%s() failed to terminate fscd", __func__);
        ret = -1;
      }
    }
  } else if (mode == FAN_AUTO_MODE) {
    if (is_fscd_run == true) {
      // fscd already running
      goto exit;
    }
    snprintf(cmd, sizeof(cmd), "sv start fscd > /dev/null 2>&1");
    if (system(cmd) != 0) {
      syslog(LOG_WARNING, "%s() start fscd failed", __func__);
    }
  } else {
    syslog(LOG_ERR, "%s(), fan mode %d not supported", __func__, mode);
    ret = -1;
  }

exit:
  if (fp != NULL) {
    pclose(fp);
  }
  if (fp2 != NULL) {
    pclose(fp2);
  }

  return ret;
}

int
fby3_common_check_ast_image_md5(char* image_path, int cal_size, uint8_t *data, bool is_first, uint8_t comp) {
  int fd = 0, sum = 0, byte_num = 0 , ret = 0, read_bytes = 0;
  char read_buf[MD5_READ_BYTES] = {0};
  uint8_t md5_digest[EVP_MAX_MD_SIZE] = {0};
  EVP_MD_CTX* ctx = NULL;

  if ((image_path == NULL) || (data == NULL)) {
    syslog(LOG_WARNING, "%s(): failed to calculate MD5-%c due to NULL parameters.", __func__, ((is_first)?'1':'2'));
    return -1;
  }

  if (cal_size <= 0) {
    syslog(LOG_WARNING, "%s(): failed to calculate MD5-%c due to wrong calculate size: %d.", __func__, ((is_first)?'1':'2'), cal_size);
    return -1;
  }

  fd = open(image_path, O_RDONLY);

  if (fd < 0) {
    syslog(LOG_WARNING, "%s(): failed to open %s to calculate MD5-%c.", __func__, image_path, ((is_first)?'1':'2'));
    return -1;
  }

  if (is_first) {
    lseek(fd, 0, SEEK_SET);
  } else {
    lseek(fd, cal_size, SEEK_SET);
  }

  ctx = EVP_MD_CTX_create();
  EVP_MD_CTX_init(ctx);

  ret = EVP_DigestInit(ctx, EVP_md5());
  if (ret == 0) {
    syslog(LOG_WARNING, "%s(): failed to initialize MD5-%c context.", __func__, ((is_first)?'1':'2'));
    ret = -1;
    goto exit;
  }

  if (is_first) {
    while (sum < cal_size) {
      read_bytes = MD5_READ_BYTES;
      if ((sum + MD5_READ_BYTES) > cal_size) {
        read_bytes = cal_size - sum;
      }

      byte_num = read(fd, read_buf, read_bytes);

      ret = EVP_DigestUpdate(ctx, read_buf, byte_num);
      if (ret == 0) {
        syslog(LOG_WARNING, "%s(): failed to update context to calculate MD5-1 of %s.", __func__, image_path);
        ret = -1;
        goto exit;
      }
      sum += byte_num;
    }
  } else {
    read_bytes = ((IMG_SIGNED_INFO_SIZE) - (MD5_SIZE));
    byte_num = read(fd, read_buf, read_bytes);
    ret = EVP_DigestUpdate(ctx, read_buf, byte_num);
    if (ret == 0) {
      syslog(LOG_WARNING, "%s(): failed to update context to calculate MD5-2 of %s.", __func__, image_path);
      ret = -1;
      goto exit;
    }
  }

  ret = EVP_DigestFinal(ctx, md5_digest, NULL);
  if (ret == 0) {
    syslog(LOG_WARNING, "%s(): failed to calculate MD5-%c of %s.", __func__, ((is_first)?'1':'2'), image_path);
    ret = -1;
    goto exit;
  }

#ifdef DEBUG
  int i = 0;
  printf("calculated MD5:\n");
  for(i = 0; i < MD5_SIZE; i++) {
    printf("%02X ", md5_digest[i]);
  }
  printf("\nImage MD5\n");
  for(i = 0; i < MD5_SIZE; i++) {
    printf("%02X ", data[i]);
  }
  printf("\n");
#endif

  if (memcmp(md5_digest, data, MD5_SIZE) != 0) {
    if(is_first) {
      printf("MD5-1 checksum incorrect. This image is corrupted or unsigned\n");
    } else {
      printf("MD5-2 Checksum incorrect. This image sign information is corrupted or unsigned\n");
    }
    ret = -1;
  }

exit:
  close(fd);
  return ret;
}

int
fby3_common_check_ast_image_signature(uint8_t* data) {
  int ret = 0;

  if (data == NULL) {
    syslog(LOG_WARNING, "%s(): failed to check signature due to NULL parameters.", __func__);
    ret = -1;
  }

  if (strncmp(plat_sig, (char*)data, PLAT_SIG_SIZE) != 0) {
    printf("This image is not for Yv3 platform AST BIC \n");
    printf("There is no valid platform signature in image. \n");
    ret = -1;
  }
  return ret;
}
