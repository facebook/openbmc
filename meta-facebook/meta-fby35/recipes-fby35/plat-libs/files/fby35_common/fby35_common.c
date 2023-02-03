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
#include <openbmc/kv.h>
#include <openbmc/libgpio.h>
#include <openbmc/misc-utils.h>
#include <openbmc/obmc-i2c.h>
#include "fby35_common.h"

const char *slot_usage = "slot1|slot2|slot3|slot4";
const char *slot_list[] = {"all", "slot1", "slot2", "slot3", "slot4", "bb", "nic", "bmc", "nicexp", "ocpdbg"};
const char plat_sig[PLAT_SIG_SIZE] = "Yosemite V3.5   ";
const char plat_sig_vf[PLAT_SIG_SIZE] = "Yosemite V3     ";

int
fby35_common_set_fru_i2c_isolated(uint8_t fru, uint8_t val) {
  return gpio_set_value_by_shadow(gpio_server_i2c_isolated[fru], (val==0)?GPIO_VALUE_LOW:GPIO_VALUE_HIGH);
}

int
fby35_common_get_fru_id(char *str, uint8_t *fru) {
  int fru_id = 0;
  bool found_id = false;

  for (fru_id = FRU_ALL; fru_id <= MAX_NUM_FRUS; fru_id++) {
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
fby35_common_get_slot_id(char *str, uint8_t *fru) {
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
fby35_common_check_slot_id(uint8_t fru) {
  uint8_t bmc_location = 0;

  if ( fby35_common_get_bmc_location(&bmc_location) < 0 ) {
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
fby35_common_is_fru_prsnt(uint8_t fru, uint8_t *val) {
  uint8_t bmc_location = 0;
  gpio_value_t gpio_value;

  // 0: the fru isn't present
  // 1: the fru is present
  if (val == NULL) {
    return -1;
  }

  if (fby35_common_get_bmc_location(&bmc_location) < 0) {
    syslog(LOG_WARNING, "%s() Cannot get the location of BMC", __func__);
    return -1;
  }

  if (bmc_location == NIC_BMC) {
    if (fru == FRU_SLOT1) {
      *val = SLOT_PRESENT;
    } else {
      *val = SLOT_NOT_PRESENT;
    }
  } else {
    gpio_value = gpio_get_value_by_shadow(gpio_server_prsnt[fru]);
    if (gpio_value == GPIO_VALUE_INVALID) {
      return -1;
    }
    *val = (gpio_value == GPIO_VALUE_HIGH) ? SLOT_NOT_PRESENT : SLOT_PRESENT;
  }

  return 0;
}

int
fby35_common_server_stby_pwr_sts(uint8_t fru, uint8_t *val) {
  int ret = 0;
  uint8_t bmc_location = 0;
  gpio_value_t gpio_value;

  ret = fby35_common_get_bmc_location(&bmc_location);
  if ( ret < 0 ) {
    syslog(LOG_WARNING, "%s() Cannot get the location of BMC", __func__);
    return ret;
  }

  if ( bmc_location == BB_BMC ) {
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

static int
fby35_read_sb_cpld(uint8_t fru, uint8_t offset, uint8_t *data) {
  int ret, i2cfd;
  uint8_t bus = 0;
  uint8_t tbuf[8] = {0};
  uint8_t rbuf[8] = {0};

  ret = fby35_common_get_bus_id(fru);
  if (ret < 0) {
    syslog(LOG_WARNING, "%s() Cannot get the bus with fru%d", __func__, fru);
    return -1;
  }

  bus = (uint8_t)(ret + 4);
  i2cfd = i2c_cdev_slave_open(bus, SB_CPLD_ADDR, I2C_SLAVE_FORCE_CLAIM);
  if (i2cfd < 0) {
    syslog(LOG_WARNING, "%s() Failed to open bus %u: %s", __func__, bus, strerror(errno));
    return -1;
  }

  tbuf[0] = offset;
  ret = i2c_rdwr_msg_transfer(i2cfd, (SB_CPLD_ADDR << 1), tbuf, 1, rbuf, 1);
  close(i2cfd);
  if (ret < 0) {
    syslog(LOG_WARNING, "%s() Failed to read CPLD, offset=%02X", __func__, offset);
    return -1;
  }
  *data = rbuf[0];

  return 0;
}

static int
fby35_read_sb_cpld_checked(uint8_t fru, uint8_t offset, uint8_t *data) {
  uint8_t prsnt = 0, stby_pwr = 0;

  if (data == NULL) {
    return -1;
  }

  if (fby35_common_check_slot_id(fru)) {
    syslog(LOG_WARNING, "%s: Unknown fru: %u", __func__, fru);
    return -1;
  }

  if (fby35_common_is_fru_prsnt(fru, &prsnt)) {
    return -1;
  }
  if (!prsnt) {
    return -1;
  }

  if (fby35_common_server_stby_pwr_sts(fru, &stby_pwr)) {
    return -1;
  }
  if (!stby_pwr) {
    return -1;
  }

  if (retry_cond(!fby35_read_sb_cpld(fru, offset, data), 3, 50)) {
    return -1;
  }

  return 0;
}

#ifndef IGNORE_CHECK_I3C_DEV_STATUS
int
fby35_read_device_status(uint8_t fru) {

  char path[MAX_KEY_LEN] = {0};
  char buf[MAX_KEY_LEN] = {0};
  int i3c_bus = fru - 1;

  sprintf(path, "/sys/bus/i3c/devices/%d-%s/device_status",i3c_bus, ASPEED_PID);

  int fd = open(path, O_RDONLY);
  if( fd < 0 ) {
    syslog(LOG_CRIT, "%s() Failed to open device status. path = %s ", __func__, path);
    return -1;
  }

  int ret = read(fd, buf, MAX_KEY_LEN);
  if(ret < 0) {
    close(fd);
    syslog(LOG_CRIT, "%s() Failed to read device status", __func__);
    return -1;
  }

  close(fd);

  int device_status = atoi(buf);
  if (device_status != 0) {
    return -1;
  }
  
  return 0;
}
#endif

int
fby35_common_is_bic_ready(uint8_t fru, uint8_t *val) {
  uint8_t rbuf[8] = {0};

  if (fby35_read_sb_cpld(fru, CPLD_REG_BIC_READY, rbuf)) {
    return -1;
  }
  *val = (rbuf[0] & 0x2) >> 1;

#ifndef IGNORE_CHECK_I3C_DEV_STATUS
  if (fby35_read_device_status(fru)) {
    return -1;
  }
#endif

  return 0;
}


int
fby35_common_get_bus_id(uint8_t slot_id) {
  int bus_id = 0;
  switch(slot_id) {
    case FRU_SLOT1:
      bus_id = SLOT1_BUS;
    break;
    case FRU_SLOT2:
      bus_id = SLOT2_BUS;
    break;
    case FRU_SLOT3:
      bus_id = SLOT3_BUS;
    break;
    case FRU_SLOT4:
      bus_id = SLOT4_BUS;
    break;
    default:
      bus_id = -1;
    break;
  }
  return bus_id;
}

int
fby35_common_get_bmc_location(uint8_t *id) {
  static bool is_cached = false;
  static uint8_t cached_id = 0;

  if ( is_cached == false ) {
    char value[MAX_VALUE_LEN] = {0};

    if ( kv_get("board_id", value, NULL, 0) ) {
      return -1;
    }

    cached_id = (uint8_t)atoi(value);
    is_cached = true;
  }

  *id = cached_id;
  return 0;
}

int
fby35_common_get_slot_type(uint8_t fru) {
  int ret = SERVER_TYPE_CL;
  char key[MAX_KEY_LEN];
  char value[MAX_VALUE_LEN] = {0};

  snprintf(key, sizeof(key), "fru%u_sb_type", fru);
  if (kv_get(key, value, NULL, 0) == 0) {
    ret = value[0];
    return ret;
  }

  if (fby35_read_sb_cpld_checked(fru, CPLD_REG_BOARD, (uint8_t *)value)) {
    return -1;
  }

  ret = value[0];
  if (kv_set(key, value, 1, KV_FCREATE)) {
    syslog(LOG_WARNING,"%s: kv_set failed, key: %s, val: %u", __func__, key, value[0]);
  }

  return ret;
}

int
fby35_common_get_sb_rev(uint8_t fru) {
  int ret = 0;
  char key[MAX_KEY_LEN];
  char value[MAX_VALUE_LEN] = {0};

  snprintf(key, sizeof(key), "fru%u_sb_rev_id", fru);
  if (kv_get(key, value, NULL, 0) == 0) {
    ret = value[0];
    return ret;
  }

  if (fby35_read_sb_cpld_checked(fru, CPLD_REG_REV_ID, (uint8_t *)value)) {
    return -1;
  }

  ret = value[0];
  if (kv_set(key, value, 1, KV_FCREATE)) {
    syslog(LOG_WARNING,"%s: kv_set failed, key: %s, val: %u", __func__, key, value[0]);
  }

  return ret;
}

int
fby35_common_get_1ou_m2_prsnt(uint8_t fru) {
  int ret = 0;
  char key[MAX_KEY_LEN];
  char value[MAX_VALUE_LEN] = {0};

  if (fby35_read_sb_cpld_checked(fru, CPLD_REG_M2_PRSNT, (uint8_t *)value)) {
    return -1;
  }

  value[0] >>= 1;
  ret = value[0];
  snprintf(key, sizeof(key), "fru%u_1ou_m2_prsnt", fru);
  if (kv_set(key, value, 1, 0)) {
    syslog(LOG_WARNING,"%s: kv_set failed, key: %s, val: %u", __func__, key, value[0]);
  }

  return ret;
}

int
fby35_common_get_sb_pch_bic_pwr_fault(uint8_t fru) {
  int ret = 0;

  if (fby35_read_sb_cpld_checked(fru, CPLD_REG_PCH_BIC_PWR_FAULT, (uint8_t *)&ret)) {
    return -1;
  }

  return ret;
}

int
fby35_common_get_sb_cpu_pwr_fault(uint8_t fru) {
  int ret = 0;

  if (fby35_read_sb_cpld_checked(fru, CPLD_REG_CPU_PWR_FAULT, (uint8_t *)&ret)) {
    return -1;
  }

  return ret;
}

int
fby35_common_get_sb_bic_boot_strap(uint8_t fru) {
  int ret = 0;

  if (fby35_read_sb_cpld_checked(fru, CPLD_REG_SB_BIC_BOOT_STRAP, (uint8_t *)&ret)) {
    return -1;
  }

  return ret;
}

static int
fby35_read_bb_cpld(uint8_t bus, uint8_t offset, uint8_t *data) {
  int ret, i2cfd;
  uint8_t tbuf[8] = {0};
  uint8_t rbuf[8] = {0};

  if (data == NULL) {
    return -1;
  }

  i2cfd = i2c_cdev_slave_open(bus, BB_CPLD_ADDR, I2C_SLAVE_FORCE_CLAIM);
  if (i2cfd < 0) {
    syslog(LOG_WARNING, "%s() Failed to open bus %u: %s", __func__, bus, strerror(errno));
    return -1;
  }

  tbuf[0] = offset;
  ret = i2c_rdwr_msg_transfer(i2cfd, (BB_CPLD_ADDR << 1), tbuf, 1, rbuf, 1);
  close(i2cfd);
  if (ret < 0) {
    syslog(LOG_WARNING, "%s() Failed to read CPLD, offset=%02X", __func__, offset);
    return -1;
  }
  *data = rbuf[0];

  return 0;
}

int
fby35_common_get_bb_rev(void) {
  int ret = 0;
  uint8_t bmc_location = 0;
  uint8_t bus = BB_CPLD_BUS;
  char *key = "board_rev_id";
  char value[MAX_VALUE_LEN] = {0};

  if (fby35_common_get_bmc_location(&bmc_location) != 0) {
    syslog(LOG_WARNING, "%s() Cannot get the location of BMC", __func__);
    return -1;
  }

  if (bmc_location == NIC_BMC) {
    bus = NIC_CPLD_BUS;
    key = "nic_board_rev_id";
  }

  if (kv_get(key, value, NULL, 0) == 0) {
    ret = value[0];
    return ret;
  }

  if (retry_cond(!fby35_read_bb_cpld(bus, CPLD_REG_BB_REV, (uint8_t *)value), 3, 50)) {
    return -1;
  }

  ret = value[0];
  if (kv_set(key, value, 1, KV_FCREATE)) {
    syslog(LOG_WARNING, "%s: kv_set failed, key: %s, val: %u", __func__, key, value[0]);
  }

  return ret;
}

#define CRASHDUMP_BIN       "/usr/bin/autodump.sh"

int
fby35_common_crashdump(uint8_t fru, bool ierr __attribute__((unused)), bool platform_reset) {
  int ret = 0;
  char cmd[128] = "\0";
  int cmd_len = sizeof(cmd);

  //This function not appropriate for AMD platforms
  if (fby35_common_get_slot_type(fru) == SERVER_TYPE_HD) {
    return -1;
  }

  // Check if the crashdump script exist
  if (access(CRASHDUMP_BIN, F_OK) == -1) {
    syslog(LOG_CRIT, "%s() Try to run autodump for %d but %s is not existed", __func__, fru, CRASHDUMP_BIN);
    return ret;
  }

  if ( platform_reset == true ) {
    snprintf(cmd, cmd_len, "%s slot%d --second &", CRASHDUMP_BIN, fru);
  } else {
    snprintf(cmd, cmd_len, "%s slot%d &", CRASHDUMP_BIN, fru);
  }

  if ( system(cmd) != 0 ) {
    syslog(LOG_INFO, "%s() Crashdump for FRU: %d is failed to be generated.", __func__, fru);
  } else {
    syslog(LOG_INFO, "%s() Crashdump for FRU: %d is being generated.", __func__, fru);
  }
  return 0;
}

int
fby35_common_dev_id(char *str, uint8_t *dev) {

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
  } else if (!strcmp(str, "3U-dev0")) {
    *dev = DEV_ID0_3OU;
  } else if (!strcmp(str, "3U-dev1")) {
    *dev = DEV_ID1_3OU;
  } else if (!strcmp(str, "3U-dev2")) {
    *dev = DEV_ID2_3OU;
  } else if (!strcmp(str, "3U-dev3")) {
    *dev = DEV_ID3_3OU;
  } else if (!strcmp(str, "4U-dev0")) {
    *dev = DEV_ID0_4OU;
  } else if (!strcmp(str, "4U-dev1")) {
    *dev = DEV_ID1_4OU;
  } else if (!strcmp(str, "4U-dev2")) {
    *dev = DEV_ID2_4OU;
  } else if (!strcmp(str, "4U-dev3")) {
    *dev = DEV_ID3_4OU;
  } else if (!strcmp(str, "4U-dev4")) {
    *dev = DEV_ID4_4OU;
  } else if (!strcmp(str, "1U")) {
    *dev = BOARD_1OU;
  } else if (!strcmp(str, "2U")) {
    *dev = BOARD_2OU;
  } else if (!strcmp(str, "2U-X8")) {
    *dev = BOARD_2OU_X8;
  } else if (!strcmp(str, "2U-X16")) {
    *dev = BOARD_2OU_X16;
  } else if (!strcmp(str, "PROT")) {
    *dev = BOARD_PROT;
  } else if (!strcmp(str, "3U")) {
    *dev = BOARD_3OU;
  } else if (!strcmp(str, "4U")) {
    *dev = BOARD_3OU;
  } else {
#ifdef DEBUG
    syslog(LOG_WARNING, "fby35_common_dev_id: Wrong fru id");
#endif
    return -1;
  }

  return 0;
}

int
fby35_common_dev_name(uint8_t dev, char *str) {

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
  } else if (dev == DEV_ID0_3OU) {
    strcpy(str, "3U-dev0");
  } else if (dev == DEV_ID1_3OU) {
    strcpy(str, "3U-dev1");
  } else if (dev == DEV_ID2_3OU) {
    strcpy(str, "3U-dev2");
  } else if (dev == DEV_ID3_3OU) {
    strcpy(str, "3U-dev3");
  } else if (dev == DEV_ID0_4OU) {
    strcpy(str, "4U-dev0");
  } else if (dev == DEV_ID1_4OU) {
    strcpy(str, "4U-dev1");
  } else if (dev == DEV_ID2_4OU) {
    strcpy(str, "4U-dev2");
  } else if (dev == DEV_ID3_4OU) {
    strcpy(str, "4U-dev3");
  } else if (dev == DEV_ID4_4OU) {
    strcpy(str, "4U-dev4");
  } else if (dev == BOARD_1OU) {
    strcpy(str, "1U");
  } else if (dev == BOARD_2OU) {
    strcpy(str, "2U");
  } else if (dev == BOARD_3OU) {
    strcpy(str, "3U");
  } else if (dev == BOARD_4OU) {
    strcpy(str, "4U");
  } else if (dev == BOARD_2OU_X8) {
    strcpy(str, "2U-X8");
  } else if (dev == BOARD_2OU_X16) {
    strcpy(str, "2U-X16");
  } else if (dev == BOARD_PROT) {
    strcpy(str, "PROT");
  } else {
#ifdef DEBUG
    syslog(LOG_WARNING, "fby35_common_dev_id: Wrong fru id");
#endif
    return -1;
  }
  return 0;
}

int
fby35_common_get_2ou_board_type(uint8_t fru, uint8_t *board_type) {
  char key[MAX_KEY_LEN];
  char value[MAX_VALUE_LEN] = {0};

  if (board_type == NULL) {
    return -1;
  }

  snprintf(key, sizeof(key), "fru%u_2ou_board_type", fru);
  if (kv_get(key, value, NULL, 0) == 0) {
    *board_type = ((uint8_t *)value)[0];
    return 0;
  }

  if (fby35_read_sb_cpld_checked(fru, CPLD_REG_RISER, board_type)) {
    return -1;
  }

  if (kv_set(key, (char *)board_type, 1, KV_FCREATE)) {
    syslog(LOG_WARNING,"%s: kv_set failed, key: %s, val: %u", __func__, key, board_type[0]);
  }

  return 0;
}

int
fby35_common_fscd_ctrl (uint8_t mode) {
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
fby35_common_check_image_md5(const char* image_path, int cal_size, uint8_t *data, bool is_first, uint8_t comp, uint8_t board_id) {
  int fd = 0, sum = 0, byte_num = 0 , ret = 0, read_bytes = 0;
  int clear_size = 0, padding = 0;
  char read_buf[MD5_READ_BYTES] = {0};
  uint8_t md5_digest[EVP_MAX_MD_SIZE] = {0};
  EVP_MD_CTX* ctx = NULL;

  if (image_path == NULL) {
    if(is_first)
      syslog(LOG_WARNING, "%s(): failed to calculate MD5 due to NULL parameters.", __func__);
    else
      syslog(LOG_WARNING, "%s(): failed to calculate MD5-2 due to NULL parameters.", __func__);
    return -1;
  }

  if (cal_size <= 0) {
    if(is_first)
      syslog(LOG_WARNING, "%s(): failed to calculate MD5 due to wrong calculate size: %d.", __func__, cal_size);
    else
      syslog(LOG_WARNING, "%s(): failed to calculate MD5-2 due to wrong calculate size: %d.", __func__, cal_size);
    return -1;
  }

  fd = open(image_path, O_RDONLY);

  if (fd < 0) {
    if(is_first)
      syslog(LOG_WARNING, "%s(): failed to open %s to calculate MD5.", __func__, image_path);
    else
      syslog(LOG_WARNING, "%s(): failed to open %s to calculate MD5-2.", __func__, image_path);
    return -1;
  }

  if(is_first)
    lseek(fd, 0, SEEK_SET);
  else
    lseek(fd, cal_size, SEEK_SET);

  ctx = EVP_MD_CTX_create();
  EVP_MD_CTX_init(ctx);

  ret = EVP_DigestInit(ctx, EVP_md5());
  if (ret == 0) {
    if(is_first)
      syslog(LOG_WARNING, "%s(): failed to initialize MD5 context.", __func__);
    else
      syslog(LOG_WARNING, "%s(): failed to initialize MD5-2 context.", __func__);
    ret = -1;
    goto exit;
  }


  if(is_first){
    while (sum < cal_size) {
      read_bytes = MD5_READ_BYTES;
      if ((sum + MD5_READ_BYTES) > cal_size) {
        read_bytes = cal_size - sum;
      }

      byte_num = read(fd, read_buf, read_bytes);

      if (comp == FW_BIOS) {
        off_t bios_info_offs = (board_id == BOARD_ID_HD) ? HD_BIOS_IMG_INFO_OFFSET : CL_BIOS_IMG_INFO_OFFSET;
        // BIOS signed information(64Bytes) is filled into BIOS_IMG_INFO_OFFSET (0x2FEF000).
        // We need to clear these 64Bytes to 0xFF then calculate the BIOS MD5.
        if (sum <= bios_info_offs && (sum + byte_num) > bios_info_offs) {
          padding = bios_info_offs - sum;
          if ((byte_num - padding) > IMG_SIGNED_INFO_SIZE) {
            clear_size = IMG_SIGNED_INFO_SIZE;
          } else {
            clear_size = byte_num - padding;
          }
          memset(read_buf + padding, 0xFF, clear_size);
        } else if (sum > bios_info_offs && sum < bios_info_offs + IMG_SIGNED_INFO_SIZE ) {
          clear_size = bios_info_offs + IMG_SIGNED_INFO_SIZE - sum;
          memset(read_buf, 0xFF, clear_size);
        } else {
          // in else case, read_buf does not need to be processed.
        }
      }

      ret = EVP_DigestUpdate(ctx, read_buf, byte_num);
      if (ret == 0) {
        syslog(LOG_WARNING, "%s(): failed to update context to calculate MD5 of %s.", __func__, image_path);
        ret = -1;
        goto exit;
      }
      sum += byte_num;
    }
  }
  else{
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
    if(is_first)
      syslog(LOG_WARNING, "%s(): failed to calculate MD5 of %s.", __func__, image_path);
    else
      syslog(LOG_WARNING, "%s(): failed to calculate MD5-2 of %s.", __func__, image_path);
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
    if(is_first)
      printf("MD5-1 checksum incorrect. This image is corrupted or unsigned\n");
    else
      printf("MD5-2 Checksum incorrect. This image sign information is corrupted or unsigned\n");
    ret = -1;
  }

exit:
  close(fd);
  return ret;
}

int
fby35_common_check_image_signature(uint8_t board_id, uint8_t *data) {
  const char *sig = (board_id == BOARD_ID_VF) ? plat_sig_vf : plat_sig;

  if (data == NULL) {
    return -1;
  }

  if (strncmp(sig, (char *)data, PLAT_SIG_SIZE) != 0) {
    printf("This image is not for Yv3.5 platform \n");
    printf("There is no valid platform signature in image. \n");
    return -1;
  }
  return 0;
}

int
fby35_common_get_img_ver(const char* image_path, char* ver, uint8_t comp) {
  int fd = 0;
  int byte_num = 0;
  int ret = 0;
  char buf[FW_VER_SIZE] = {0};
  struct stat file_info;
  uint32_t offset = 0x0;

  if (stat(image_path, &file_info) < 0) {
    syslog(LOG_WARNING, "%s(): failed to open %s to check file infomation.", __func__, image_path);
    return false;
  }
  offset = file_info.st_size - IMG_SIGNED_INFO_SIZE + IMG_FW_VER_OFFSET;
  fd = open(image_path, O_RDONLY);
  if (fd < 0 ) {
    syslog(LOG_WARNING, "%s(): failed to open %s to check version.", __func__, image_path);
    ret = -1;
    goto exit;
  }
  lseek(fd, offset, SEEK_SET);
  byte_num = read(fd, buf, FW_VER_SIZE);
  if (byte_num != FW_VER_SIZE) {
    syslog(LOG_WARNING, "%s(): failed to get image version", __func__);
    ret = -1;
    goto exit;
  }
  switch(comp) {
    case FW_CPLD:
    case FW_1OU_CPLD:
    case FW_2OU_CPLD:
    case FW_BB_CPLD:
      snprintf(ver, 16, "%02X%02X%02X%02X", buf[3], buf[2], buf[1], buf[0]);
      break;
    case FW_SB_BIC:
    case FW_1OU_BIC:
    case FW_2OU_BIC:
    case FW_BB_BIC:
      snprintf(ver, 16, "v%x.%02X", buf[3], buf[2]);
      break;
    default:
      ret = -1;
      goto exit;
  }

exit:
  if (fd >= 0)
    close(fd);

  return ret;
}

uint8_t
fby35_zero_checksum_calculate(uint8_t *buf, uint8_t len) {
  uint8_t i, ret = 0;

  for (i = 0; i < len; i++) {
    ret += *(buf++);
  }
  ret = (~ret) + 1; //2's complement

  return ret;
}

bool
fby35_is_zero_checksum_valid(uint8_t *buf, uint8_t len) {
  uint8_t i, ret = 0;

  for(i = 0; i < len; i++)
  {
    ret += *(buf++);
  }
  ret += *(buf++); //Add checksum
  ret = (~ret) + 1; //2's complement, should be 0

  return (ret == 0) ? true : false;
}

bool
fby35_common_is_valid_img(const char* img_path, uint8_t comp, uint8_t board_id, uint8_t rev_id) {
  const char *rev_bb[] = {"POC1", "POC2", "EVT", "EVT2", "EVT3", "DVT", "DVT_1C", "PVT", "MP"};
  const char *rev_sb[] = {"POC", "EVT", "EVT2", "EVT3", "EVT3", "DVT", "DVT", "PVT", "PVT", "MP", "MP"};
  const char *rev_hd[] = {"POC", "EVT", "EVT", "EVT", "DVT", "DVT", "DVT", "DVT", "PVT"};
  const char *rev_op[] = {"EVT", "DVT", "PVT", "MP"};
  const char **board_type = rev_sb;
  uint8_t exp_fw_rev = 0;
  uint8_t err_proof_board_id = 0;
  uint8_t err_proof_stage = 0;
  uint8_t err_proof_component = 0;
  int cal_size = 0;
  int fd;
  off_t info_offs;
  struct stat file_info;
  FW_IMG_INFO img_info;

  if (img_path == NULL) {
    return false;
  }

  if (stat(img_path, &file_info) < 0) {
    syslog(LOG_WARNING, "%s(): failed to open %s to check file infomation.", __func__, img_path);
    return false;
  }

  if (file_info.st_size < IMG_SIGNED_INFO_SIZE) {
    return false;
  }

  fd = open(img_path, O_RDONLY);
  if (fd < 0) {
    printf("Cannot open %s for reading\n", img_path);
    return false;
  }

  if (comp == FW_BIOS) {
    info_offs = (board_id == BOARD_ID_HD) ? HD_BIOS_IMG_INFO_OFFSET : CL_BIOS_IMG_INFO_OFFSET;
    cal_size = file_info.st_size;
  } else {
    info_offs = file_info.st_size - IMG_SIGNED_INFO_SIZE;
    cal_size = info_offs;
  }

  if (lseek(fd, info_offs, SEEK_SET) != info_offs) {
    close(fd);
    printf("Cannot seek %s\n", img_path);
    return false;
  }

  if (read(fd, &img_info, IMG_SIGNED_INFO_SIZE) != IMG_SIGNED_INFO_SIZE) {
    close(fd);
    printf("Cannot read %s\n", img_path);
    return false;
  }
  close(fd);

  if (fby35_common_check_image_signature(board_id, img_info.plat_sig) < 0) {
    return false;
  }

  if (fby35_common_check_image_md5(img_path, cal_size, img_info.md5_sum, true, comp, board_id) < 0) {
    return false;
  }

  if (fby35_common_check_image_md5(img_path, info_offs, img_info.md5_sum_second, false, comp, board_id) < 0) {
    return false;
  }

  err_proof_board_id = img_info.err_proof[0] & 0x1F;  //bit[4:0]
  err_proof_stage = img_info.err_proof[0] >> 5;       //bit[7:5]
  err_proof_component = img_info.err_proof[1] & 0x07; //bit[10:8]

  switch (comp) {
    case FW_CPLD:
    case FW_1OU_CPLD:
    case FW_2OU_CPLD:
    case FW_BB_CPLD:
      if (err_proof_component != COMP_CPLD) {
        printf("Not a valid CPLD firmware image.\n");
        return false;
      }
      break;
    case FW_SB_BIC:
    case FW_BIC_RCVY:
    case FW_1OU_BIC:
    case FW_1OU_BIC_RCVY:
    case FW_2OU_BIC:
    case FW_2OU_BIC_RCVY:
    case FW_3OU_BIC:
    case FW_3OU_BIC_RCVY:
    case FW_4OU_BIC:
    case FW_4OU_BIC_RCVY:
    case FW_BB_BIC:
      if (err_proof_component != COMP_BIC) {
        printf("Not a valid BIC firmware image.\n");
        return false;
      }
      break;
    case FW_BIOS:
      if (err_proof_component != COMP_BIOS) {
        printf("Not a valid BIOS firmware image.\n");
        return false;
      }
      break;
  }

  if (err_proof_board_id != board_id) {
    printf("Wrong firmware image component.\n");
    return false;
  }

  switch (board_id) {
    case BOARD_ID_SB:
      if (rev_id >= ARRAY_SIZE(rev_sb)) {
        rev_id = ARRAY_SIZE(rev_sb) - 1;
      }
      break;
    case BOARD_ID_HD:
      board_type = rev_hd;
      //Halfdome hw stage: rev_id bit[3:0]
      rev_id = rev_id & 0x0F;
      if (rev_id >= ARRAY_SIZE(rev_hd)) {
        rev_id = ARRAY_SIZE(rev_hd) - 1;
      }
      break;
    case BOARD_ID_BB:
      board_type = rev_bb;
      if (rev_id >= ARRAY_SIZE(rev_bb)) {
        rev_id = ARRAY_SIZE(rev_bb) - 1;
      }
      break;
    case BOARD_ID_OP:
      board_type = rev_op;
      if (rev_id >= ARRAY_SIZE(rev_op)) {
        rev_id = ARRAY_SIZE(rev_op) - 1;
      }
      break;

    case BOARD_ID_RF:
    case BOARD_ID_VF:
      return true;
  }

  if (board_type == rev_sb) {
    if (SB_REV_EVT <= rev_id && rev_id <= SB_REV_EVT_MPS)
      exp_fw_rev = FW_REV_EVT;
    else if (SB_REV_DVT <= rev_id && rev_id <= SB_REV_DVT_MPS)
      exp_fw_rev = FW_REV_DVT;
    else if (SB_REV_PVT <= rev_id && rev_id <= SB_REV_PVT_MPS)
      exp_fw_rev = FW_REV_PVT;
    else if (SB_REV_MP <= rev_id && rev_id <= SB_REV_MP_MPS)
      exp_fw_rev = FW_REV_MP;
    else
      exp_fw_rev = 0;
  } else if (board_type == rev_hd) {
    if (HD_REV_EVT <= rev_id && rev_id <= HD_REV_EVT2)
      exp_fw_rev = FW_REV_EVT;
    else if (HD_REV_DVT <= rev_id && rev_id <= HD_REV_DVT3)
      exp_fw_rev = FW_REV_DVT;
    else if (HD_REV_PVT <= rev_id)
      exp_fw_rev = FW_REV_PVT;
    else
      exp_fw_rev = 0;
  } else if (board_type == rev_op) {
    switch (rev_id) {
      case OP_REV_EVT:
        exp_fw_rev = FW_REV_EVT;
        break;
      case OP_REV_DVT:
        exp_fw_rev = FW_REV_DVT;
        break;
      case OP_REV_PVT:
        exp_fw_rev = FW_REV_PVT;
        break;
      case OP_REV_MP:
        exp_fw_rev = FW_REV_MP;
        break;
      default:
        exp_fw_rev = 0;
    }
  } else {
    if (BB_REV_EVT <= rev_id && rev_id <= BB_REV_EVT3)
      exp_fw_rev = FW_REV_EVT;
    else if (rev_id == BB_REV_DVT || rev_id == BB_REV_DVT_1C)
      exp_fw_rev = FW_REV_DVT;
    else if (rev_id == BB_REV_PVT)
      exp_fw_rev = FW_REV_PVT;
    else if (rev_id == BB_REV_MP)
      exp_fw_rev = FW_REV_MP;
    else
      exp_fw_rev = 0;
  }

  switch (exp_fw_rev) {
    case FW_REV_POC:
      if (err_proof_stage != FW_REV_POC) {
        printf("Please use POC firmware on POC system\nTo force the update, please use the --force option.\n");
        return false;
      }
      break;
    case FW_REV_EVT:
      if (err_proof_stage != FW_REV_EVT) {
        printf("Please use EVT firmware on %s system\nTo force the update, please use the --force option.\n",
               board_type[rev_id]);
        return false;
      }
      break;
    case FW_REV_DVT:
      if (err_proof_stage != FW_REV_DVT) {
        printf("Please use DVT firmware on %s system\nTo force the update, please use the --force option.\n",
               board_type[rev_id]);
        return false;
      }
      break;
    case FW_REV_PVT:
    case FW_REV_MP:
      if (err_proof_stage != FW_REV_PVT) {
        printf("Please use firmware after PVT on %s system\nTo force the update, please use the --force option.\n",
               board_type[rev_id]);
        return false;
      }
      break;
    default:
      printf("Can't recognize the firmware's stage, please use the --force option\n");
      return false;
  }

  return true;
}

int
fby35_common_get_bb_hsc_type(uint8_t* type) {
  char value[MAX_VALUE_LEN] = {0};
  const char *shadows[] = {
    "HSC_BB_BMC_DETECT0",
    "HSC_BB_BMC_DETECT1",
  };

  if (kv_get(KEY_BB_HSC_TYPE, value, NULL, 0) == 0) {
    *type = (uint8_t)atoi(value);
    return 0;
  } else {
    if (gpio_get_value_by_shadow_list(shadows, ARRAY_SIZE(shadows), (unsigned int*) type)) {
      syslog(LOG_WARNING,"%s: failed to detect HSC type", __func__);
      return -1;
    }
    snprintf(value, sizeof(value), "%x", *type);
    if (kv_set(KEY_BB_HSC_TYPE, (const char *)value, 1, KV_FCREATE)) {
      syslog(LOG_WARNING,"%s: kv_set failed, key: %s", __func__, KEY_BB_HSC_TYPE);
    }
  }
  return 0;
}

bool
fby35_common_is_prot_card_prsnt(uint8_t fru) {
  char key[MAX_KEY_LEN];
  char value[MAX_VALUE_LEN] = {0};

  snprintf(key, sizeof(key), "fru%u_is_prot_prsnt", fru);
  if (kv_get(key, value, NULL, 0) == 0) {
    return value[0] ? true : false;
  }

  if (fby35_read_sb_cpld_checked(fru, CPLD_REG_PROT, (uint8_t *)value)) {
    return false;
  }

  value[0] = (value[0] & 0x01) ^ 0x01;
  if (kv_set(key, value, 1, KV_FCREATE)) {
    syslog(LOG_WARNING,"%s: kv_set failed, key: %s, val: %u", __func__, key, value[0]);
  }

  return value[0] ? true : false;
}

/*
 * copy_eeprom_to_bin - copy the eeprom to binary file im /tmp directory
 *
 * @eeprom_file   : path for the eeprom of the device
 * @bin_file      : path for the binary file
 *
 * returns 0 on successful copy
 * returns non-zero on file operation errors
 */
int copy_eeprom_to_bin(const char *eeprom_file, const char *bin_file) {

  int eeprom;
  int bin;
  uint64_t tmp[FRU_SIZE];
  ssize_t bytes_rd, bytes_wr;

  errno = 0;

  eeprom = open(eeprom_file, O_RDONLY);
  if (eeprom == -1) {
    syslog(LOG_ERR, "%s: unable to open the %s file: %s",
	__func__, eeprom_file, strerror(errno));
    return errno;
  }

  bin = open(bin_file, O_WRONLY | O_CREAT, 0644);
  if (bin == -1) {
    syslog(LOG_ERR, "%s: unable to create %s file: %s",
	__func__, bin_file, strerror(errno));
    goto err;
  }

  bytes_rd = read(eeprom, tmp, FRU_SIZE);
  if (bytes_rd < 0) {
    syslog(LOG_ERR, "%s: read %s file failed: %s",
	__func__, eeprom_file, strerror(errno));
    goto exit;
  } else if (bytes_rd < FRU_SIZE) {
    syslog(LOG_ERR, "%s: less than %d bytes", __func__, FRU_SIZE);
    goto exit;
  }

  bytes_wr = write(bin, tmp, bytes_rd);
  if (bytes_wr != bytes_rd) {
    syslog(LOG_ERR, "%s: write to %s file failed: %s",
	__func__, bin_file, strerror(errno));
    goto exit;
  }

exit:
  close(bin);
err:
  close(eeprom);

  return errno;
}
