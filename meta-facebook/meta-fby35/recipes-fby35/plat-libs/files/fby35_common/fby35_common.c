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
#include "fby35_common.h"

const char *slot_usage = "slot1|slot2|slot3|slot4";
const char *slot_list[] = {"all", "slot1", "slot2", "slot3", "slot4", "bb", "nic", "bmc", "nicexp"};
const char platform_signature[PLAT_SIG_SIZE] = "Yosemite V3.5   ";

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

int
fby35_common_is_bic_ready(uint8_t fru, uint8_t *val) {
  int i2cfd = 0;
  int ret = 0;
  uint8_t bus = 0;
  uint8_t tbuf[1] = {0x02};
  uint8_t rbuf[1] = {0};
  uint8_t tlen = 1;
  uint8_t rlen = 1;

  // a bus starts from 4
  ret = fby35_common_get_bus_id(fru) + 4;
  if ( ret < 0 ) {
    syslog(LOG_WARNING, "%s() Cannot get the bus with fru%d", __func__, fru);
    goto error_exit;
  }

  bus = (uint8_t)ret;
  i2cfd = i2c_cdev_slave_open(bus, SB_CPLD_ADDR, I2C_SLAVE_FORCE_CLAIM);
  if ( i2cfd < 0 ) {
    syslog(LOG_WARNING, "%s() Failed to open bus %d. Err: %s", __func__, bus, strerror(errno));
    goto error_exit;
  }

  ret = i2c_rdwr_msg_transfer(i2cfd, (SB_CPLD_ADDR << 1), tbuf, tlen, rbuf, rlen);
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
fby35_common_get_bus_id(uint8_t slot_id) {
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
  int ret = 0;

  ret = fby35_common_check_slot_id(fru);
  if ( ret < 0 ) {
    syslog(LOG_WARNING, "%s() Unknown fru: %d", __func__, fru);
    return -1;
  }

  return SERVER_TYPE_DL;
}

#define CRASHDUMP_BIN       "/usr/local/bin/autodump.sh"

int
fby35_common_crashdump(uint8_t fru, bool ierr, bool platform_reset) {
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
  } else if (!strcmp(str, "1U")) {
    *dev = BOARD_1OU;
  } else if (!strcmp(str, "2U")) {
    *dev = BOARD_2OU;
  } else if (!strcmp(str, "2U-X8")) {
    *dev = BOARD_2OU_X8;
  } else if (!strcmp(str, "2U-X16")) {
    *dev = BOARD_2OU_X16;
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
  } else if (dev == BOARD_2OU_X8) {
    strcpy(str, "2U-X8");
  } else if (dev == BOARD_2OU_X16) {
    strcpy(str, "2U-X16");
  } else {
#ifdef DEBUG
    syslog(LOG_WARNING, "fby35_common_dev_id: Wrong fru id");
#endif
    return -1;
  }
  return 0;
}

static int
_fby35_common_get_2ou_board_type(uint8_t fru_id, uint8_t *board_type) {
  uint8_t bus_num = 0;
  int fd = -1, ret = 0;
  int tlen = 0, rlen = 0;
  uint8_t tbuf[64], rbuf[64];

  bus_num = fby35_common_get_bus_id(fru_id) + 4;
  fd = i2c_cdev_slave_open(bus_num, SB_CPLD_ADDR, I2C_SLAVE_FORCE_CLAIM);
  if ( fd < 0 ) {
    syslog(LOG_ERR, "Failed to open bus %d: %s", bus_num, strerror(errno));
    return -1;
  }

  tbuf[0] = CPLD_BOARD_OFFSET;
  tlen = 1;
  rlen = 1;
  ret = i2c_rdwr_msg_transfer(fd, SB_CPLD_ADDR << 1, tbuf, tlen, rbuf, rlen);
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
fby35_common_get_2ou_board_type(uint8_t fru_id, uint8_t *board_type) {
  char key[MAX_VALUE_LEN] = {0};
  char value[MAX_VALUE_LEN] = {0};

  sprintf(key, "fru%u_2ou_board_type", fru_id);

  if (kv_get(key, value, NULL, 0) == 0) {
    *board_type = ((uint8_t*)value)[0];
    return 0;
  }

  if (_fby35_common_get_2ou_board_type(fru_id, board_type) < 0) {
    return -1;
  }

  if (kv_set(key, (char*)board_type, 1, KV_FCREATE)) {
    syslog(LOG_WARNING,"%s: kv_set failed, key: %s, val: %u", __func__, key, board_type[0]);
    return -1;
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
fby35_common_check_image_md5(const char* image_path, int cal_size, uint8_t *data, bool is_first) {
  int fd = 0, sum = 0, byte_num = 0 , ret = 0, read_bytes = 0;
  char read_buf[MD5_READ_BYTES] = {0};
  char md5_digest[MD5_DIGEST_LENGTH] = {0};
  MD5_CTX context;

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

  ret = MD5_Init(&context);
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
      ret = MD5_Update(&context, read_buf, byte_num);
      if (ret == 0) {
        syslog(LOG_WARNING, "%s(): failed to update context to calculate MD5 of %s.", __func__, image_path);
        ret = -1;
        goto exit;
      }
      sum += byte_num;
    }
  }
  else{
    read_bytes = ((IMG_POSTFIX_SIZE) - (MD5_SIZE));

    byte_num = read(fd, read_buf, read_bytes);
    ret = MD5_Update(&context, read_buf, byte_num);
    if (ret == 0) {
      syslog(LOG_WARNING, "%s(): failed to update context to calculate MD5-2 of %s.", __func__, image_path);
      ret = -1;
      goto exit;
    }
  }

  ret = MD5_Final((uint8_t*)md5_digest, &context);
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
  printf("calculated MD5:\n")
  for(i = 0; i < 16; i++) {
    printf("%02X ", ((uint8_t*)md5_digest)[i]);
  }
  printf("\nImage MD5");
  for(i = 0; i < 16; i++) {
    printf("%02X ", data[i]);
  }
  printf("\n");
#endif

  if (strncmp(md5_digest, (char*)data, sizeof(md5_digest)) != 0) {
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
fby35_common_check_image_signature(uint8_t* data) {
  int ret = 0;

  if (strncmp(platform_signature, (char*)data, PLAT_SIG_SIZE) != 0) {
    printf("This image is not for Yv3.5 platform or is not signed md5-2.\n");
    ret = -1;
  }
  return ret;
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
  offset = file_info.st_size - IMG_POSTFIX_SIZE + IMG_FW_VER_OFFSET;
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
    case FW_BIC:
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

bool
fby35_common_is_valid_img(const char* img_path, uint8_t comp, uint8_t rev_id) {
  const char *rev_bb[] = {"POC1", "POC2", "EVT", "EVT2", "EVT3", "DVT", "PVT", "MP"};
  const char *rev_sb[] = {"POC", "EVT", "EVT2", "EVT3", "DVT", "DVT2", "PVT", "PVT2", "MP", "MP2"};
  const char **board_type = rev_sb;
  uint8_t signed_byte = 0x0;
  uint8_t board_id = 0, exp_fw_rev = 0;
  int fd;
  off_t info_offs;
  struct stat file_info;
  FW_IMG_INFO img_info;
  bool is_first = true;

  if (stat(img_path, &file_info) < 0) {
    syslog(LOG_WARNING, "%s(): failed to open %s to check file infomation.", __func__, img_path);
    return false;
  }

  if (file_info.st_size < IMG_POSTFIX_SIZE) {
    return false;
  }
  info_offs = file_info.st_size - IMG_POSTFIX_SIZE;

  fd = open(img_path, O_RDONLY);
  if (fd < 0) {
    printf("Cannot open %s for reading\n", img_path);
    return false;
  }

  if (lseek(fd, info_offs, SEEK_SET) != info_offs) {
    close(fd);
    printf("Cannot seek %s\n", img_path);
    return false;
  }

  if (read(fd, &img_info, IMG_POSTFIX_SIZE) != IMG_POSTFIX_SIZE) {
    close(fd);
    printf("Cannot read %s\n", img_path);
    return false;
  }
  close(fd);

  if (fby35_common_check_image_signature(img_info.plat_sig) < 0) {
    return false;
  }

  if (fby35_common_check_image_md5(img_path, info_offs, img_info.md5_sum, is_first) < 0) {
    return false;
  }

  if (fby35_common_check_image_md5(img_path, info_offs, img_info.md5_sum_second, !is_first) < 0) {
    return false;
  }

  signed_byte = img_info.err_proof;

  switch (comp) {
    case FW_CPLD:
    case FW_BIC:
    case FW_BIOS:
      board_id = BOARD_ID_SB;
      if (rev_id >= ARRAY_SIZE(rev_sb)) {
        rev_id = ARRAY_SIZE(rev_sb) - 1;
      }
      break;
    case FW_BB_BIC:
    case FW_BB_CPLD:
      board_id = BOARD_ID_BB;
      board_type = rev_bb;
      if (rev_id >= ARRAY_SIZE(rev_bb)) {
        rev_id = ARRAY_SIZE(rev_bb) - 1;
      }
      break;
  }
  if (BOARD_ID(signed_byte) != board_id) {
    printf("Wrong firmware image component.\n");
    return false;
  }

  if (board_type == rev_sb) {
    if (SB_REV_EVT <= rev_id && rev_id <= SB_REV_EVT4)
      exp_fw_rev = FW_REV_EVT;
    else if (SB_REV_DVT <= rev_id && rev_id <= SB_REV_DVT2)
      exp_fw_rev = FW_REV_DVT;
    else if (SB_REV_PVT <= rev_id && rev_id <= SB_REV_PVT2)
      exp_fw_rev = FW_REV_PVT;
    else if (SB_REV_MP <= rev_id && rev_id <= SB_REV_MP2)
      exp_fw_rev = FW_REV_MP;
    else
      exp_fw_rev = 0;
  }
  else{
    if (BB_REV_EVT <= rev_id && rev_id <= BB_REV_EVT3)
      exp_fw_rev = FW_REV_EVT;
    else if (rev_id == BB_REV_DVT)
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
      if (REVISION_ID(signed_byte) != FW_REV_POC) {
        printf("Please use POC firmware on POC system\nTo force the update, please use the --force option.\n");
        return false;
      }
      break;
    case FW_REV_EVT:
      if (REVISION_ID(signed_byte) != FW_REV_EVT) {
        printf("Please use EVT firmware on %s system\nTo force the update, please use the --force option.\n",
               board_type[rev_id]);
        return false;
      }
      break;
    case FW_REV_DVT:
      if (REVISION_ID(signed_byte) != FW_REV_DVT) {
        printf("Please use DVT firmware on %s system\nTo force the update, please use the --force option.\n",
               board_type[rev_id]);
        return false;
      }
      break;
    case FW_REV_PVT:
    case FW_REV_MP:
      if (REVISION_ID(signed_byte) != FW_REV_PVT) {
        printf("Please use firmware after PVT on %s system\nTo force the update, please use the --force option.\n",
               board_type[rev_id]);
        return false;
      }
      break;
    default:
      printf("Can't recognize the firmware's stage, please use the --force option\n");
      return false;
  }

  switch (comp) {
    case FW_CPLD:
    case FW_1OU_CPLD:
    case FW_2OU_CPLD:
    case FW_BB_CPLD:
      if (COMPONENT_ID(signed_byte) != COMP_CPLD) {
        printf("Not a valid CPLD firmware image.\n");
        return false;
      }
      break;
    case FW_BIC:
    case FW_1OU_BIC:
    case FW_2OU_BIC:
    case FW_BB_BIC:
      if (COMPONENT_ID(signed_byte) != COMP_BIC) {
        printf("Not a valid BIC firmware image.\n");
        return false;
      }
      break;
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
