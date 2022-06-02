
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
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <syslog.h>
#include <errno.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <openbmc/kv.h>
#include <openbmc/obmc-i2c.h>
#include "bic_fwupdate.h"
#include "bic_ipmi.h"
#include "bic_xfer.h"
#include "bic_bios_fwupdate.h"
#include "bic_cpld_altera_fwupdate.h"
#include "bic_cpld_lattice_fwupdate.h"
#include "bic_m2_fwupdate.h"
#include "bic_mchp_pciesw_fwupdate.h"
#include "bic_vr_fwupdate.h"

//#define DEBUG

#define log_system(cmd)                                                     \
do {                                                                        \
  int sysret = system(cmd);                                                 \
  if (sysret)                                                               \
    syslog(LOG_WARNING, "%s: system command failed, cmd: \"%s\",  ret: %d", \
            __func__, cmd, sysret);                                         \
} while(0)

/****************************/
/*      BIC fw update       */
/****************************/
#define BIC_CMD_DOWNLOAD 0x21
#define BIC_CMD_DOWNLOAD_SIZE 11

#define BIC_CMD_RUN 0x22
#define BIC_CMD_RUN_SIZE 7

#define BIC_CMD_STS 0x23
#define BIC_CMD_STS_SIZE 3

#define BIC_CMD_DATA 0x24
#define BIC_CMD_DATA_SIZE 0xFF

#define BIC_FLASH_START 0x8000

#define FW_UPDATE_FAN_PWM  70
#define FAN_PWM_CNT        4

enum {
  FEXP_BIC_I2C_WRITE   = 0x20,
  FEXP_BIC_I2C_READ    = 0x21,
  FEXP_BIC_I2C_UPDATE  = 0x22,
  FEXP_BIC_IPMI_I2C_SW = 0x23,

  REXP_BIC_I2C_WRITE   = 0x24,
  REXP_BIC_I2C_READ    = 0x25,
  REXP_BIC_I2C_UPDATE  = 0x26,
  REXP_BIC_IPMI_I2C_SW = 0x27,

  BB_BIC_I2C_WRITE     = 0x28,
  BB_BIC_I2C_READ      = 0x29,
  BB_BIC_I2C_UPDATE    = 0x2A,
  BB_BIC_IPMI_I2C_SW   = 0x2B
};

enum {
  I2C_100K = 0x0,
  I2C_1M
};


// Update firmware for various components
static int
_update_fw(uint8_t slot_id, uint8_t target, uint32_t offset, uint16_t len, uint8_t *buf, uint8_t intf) {
  uint8_t tbuf[256] = {0x9c, 0x9c, 0x00}; // IANA ID
  uint8_t rbuf[16] = {0x00};
  uint8_t tlen = 0;
  uint8_t rlen = 0;
  uint8_t cmd = 0x0;
  int ret;
  int retries = 3;
  int index = 3;
  int offset_size = sizeof(offset) / sizeof(tbuf[0]);

  if (intf != NONE_INTF) {
    // Need bridge command to 2nd BIC
    tbuf[index++] = intf;
    tbuf[index++] = NETFN_OEM_1S_REQ << 2;
    tbuf[index++] = CMD_OEM_1S_UPDATE_FW;
    memcpy(&tbuf[index], (uint8_t *)&IANA_ID, IANA_ID_SIZE);
    index += IANA_ID_SIZE;
    cmd = CMD_OEM_1S_MSG_OUT;
  } else {
    cmd = CMD_OEM_1S_UPDATE_FW;
  }

  // Fill the component for which firmware is requested
  tbuf[index++] = target;

  memcpy(&tbuf[index], (uint8_t *)&offset, offset_size);
  index += offset_size;

  tbuf[index++] = len & 0xFF;
  tbuf[index++] = (len >> 8) & 0xFF;

  memcpy(&tbuf[index], buf, len);

  tlen = len + index;

bic_send:
  ret = bic_ipmb_wrapper(slot_id, NETFN_OEM_1S_REQ, cmd, tbuf, tlen, rbuf, &rlen);
  if ((ret) && (retries--)) {
    sleep(1);
    printf("_update_fw: slot: %d, target %d, offset: %u, len: %d retrying..\
           \n",    slot_id, target, offset, len);
    goto bic_send;
  }

  return ret;
}

#ifdef DEBUG
static void print_data(const char *name, uint8_t netfn, uint8_t cmd, uint8_t *buf, uint8_t len) {
  int i;
  printf("[%s][%d]send: 0x%x 0x%x ", name, len, netfn, cmd);
  for ( i = 0; i < len; i++) {
    printf("0x%x ", buf[i]);
  }

  printf("\n");
}
#endif

static int
update_bic(uint8_t slot_id, int fd, int file_size, uint8_t intf) {
  struct timeval start, end;
  int update_rc = -1, cmd_rc = 0;
  uint32_t dsize, last_offset;
  uint32_t offset, boundary;
  volatile uint16_t read_count;
  uint8_t buf[256] = {0};
  uint8_t target;
  uint8_t bmc_location = 0;
  ssize_t count;

  if (fby35_common_get_bmc_location(&bmc_location) != 0) {
    printf("Cannot get the location of BMC");
    return -1;
  }
  if ((intf == BB_BIC_INTF) && (bmc_location == NIC_BMC)) {
    if (bic_check_bb_fw_update_ongoing() != 0) {
      return -1;
    }
    if (bic_set_bb_fw_update_ongoing(FW_BB_BIC, SEL_ASSERT) != 0) {
      printf("Failed to set firmware update ongoing\n");
      return -1;
    }
  }
  printf("updating fw on slot %d:\n", slot_id);

  // Write chunks of binary data in a loop
  dsize = file_size/100;
  last_offset = 0;
  offset = 0;
  boundary = PKT_SIZE;
  target = UPDATE_BIC;
  gettimeofday(&start, NULL);
  while (1) {
    // send packets in blocks of 64K
    if ((offset + AST_BIC_IPMB_WRITE_COUNT_MAX) < boundary) {
      read_count = AST_BIC_IPMB_WRITE_COUNT_MAX;
    } else {
      read_count = boundary - offset;
    }

    // Read from file
    count = read(fd, buf, read_count);
    if ((count < 0) && (errno == EINTR)) {
      continue;
    }
    if (count <= 0 || count > read_count) {
      break;
    }

    if ((offset + count) >= file_size) {
      target |= 0x80;
    }
    // Send data to Bridge-IC
    cmd_rc = _update_fw(slot_id, target, offset, count, buf, intf);
    if (cmd_rc) {
      goto error_exit;
    }

    // Update counter
    offset += count;
    if (offset >= boundary) {
      boundary += PKT_SIZE;
    }
    if ((last_offset + dsize) <= offset) {
      _set_fw_update_ongoing(slot_id, 60);
      printf("\rupdated bic: %u %%", offset/dsize);
      fflush(stdout);
      last_offset += dsize;
    }
  }
  printf("\n");

  gettimeofday(&end, NULL);
  printf("Elapsed time:  %d   sec.\n", (int)(end.tv_sec - start.tv_sec));
  if (offset >= file_size) {
    update_rc = 0;
  }

error_exit:

  printf("\n");

  if ((intf == BB_BIC_INTF) && (bmc_location == NIC_BMC)) {
    if (bic_set_bb_fw_update_ongoing(FW_BB_BIC, SEL_DEASSERT) != 0) {
      syslog(LOG_WARNING, "Failed to notify BB firmware complete");
    }
  }

  if ( update_rc == 0 ) {
    update_rc = cmd_rc;
  }
  return update_rc;
}

static int
is_valid_intf(uint8_t intf) {
  int ret = BIC_STATUS_FAILURE;
  switch(intf) {
    case FEXP_BIC_INTF:
    case BB_BIC_INTF:
    case REXP_BIC_INTF:
    case NONE_INTF:
      ret = BIC_STATUS_SUCCESS;
      break;
  }

  return ret;
}

static int
update_bic_runtime_fw(uint8_t slot_id, uint8_t comp,uint8_t intf, char *path, uint8_t force) {
  #define MAX_CMD_LEN 100
  #define MAX_RETRY 10
  char cmd[MAX_CMD_LEN] = {0};
  uint8_t self_test_result[2] = {0};
  int ret = -1;
  int fd = -1;
  int file_size;

  //check params
  ret = is_valid_intf(intf);
  if ( ret < 0 ) {
    syslog(LOG_WARNING, "%s() invalid intf(val=0x%x) was caught!\n", __func__, intf);
    goto exit;
  }

  //get fd and file size
  fd = open_and_get_size(path, &file_size);
  if ( fd < 0 ) {
    syslog(LOG_WARNING, "%s() cannot open the file: %s, fd=%d\n", __func__, path, fd);
    goto exit;
  }

  printf("file size = %d bytes, slot = %d, intf = 0x%x\n", file_size, slot_id, intf);

  //run into the different function based on the interface
  ret = update_bic(slot_id, fd, file_size, intf);

  if ( (ret == 0) && (comp == FW_SB_BIC) ) {
    sleep(5);
    // Check BIC self-test results to prevent bic-cached from getting stuck forever
    for (int i = 0; i < MAX_RETRY; i++) {
      ret = bic_get_self_test_result(slot_id, (uint8_t *)&self_test_result, NONE_INTF);
      if (ret == 0) {
        syslog(LOG_INFO, "bic_get_self_test_result of slot%u: %X %X", slot_id, self_test_result[0], self_test_result[1]);
        break;
      }
      sleep(1);
    }

    if (ret == 0) {
      printf("get new SDR cache from BIC \n");
      memset(cmd, 0, sizeof(cmd));
      snprintf(cmd, MAX_CMD_LEN, "/usr/local/bin/bic-cached -s slot%d; /usr/bin/kv set slot%d_sdr_thresh_update 1", slot_id, slot_id);   //retrieve SDR data after BIC FW update
      log_system(cmd);
    }
  }

exit:

  if ( fd >= 0 ) {
    close(fd);
  }

  return ret;
}

static int
recovery_bic_runtime_fw(uint8_t slot_id, uint8_t comp,uint8_t intf, char *path, uint8_t force) {
  int ret = -1, retry = 0;
  int fd = 0, i2cfd = 0;
  int file_size;
  uint8_t tbuf[2] = {0x00};
  uint8_t tlen = 2;
  uint8_t bmc_location = 0;
  char cmd[64] = {0};

  //check params
  ret = is_valid_intf(intf);
  if ( ret < 0 ) {
    syslog(LOG_WARNING, "%s() invalid intf(val=0x%x) was caught!\n", __func__, intf);
    goto error_exit;
  }

  //get fd and file size
  fd = open_and_get_size(path, &file_size);
  if ( fd < 0 ) {
    syslog(LOG_WARNING, "%s() cannot open the file: %s, fd=%d\n", __func__, path, fd);
    goto error_exit;
  }

  printf("file size = %d bytes, slot = %d, intf = 0x%x\n", file_size, slot_id, intf);

  printf("Set slot UART to SB BIC\n");
  ret = fby35_common_get_bmc_location(&bmc_location);
  if ( ret < 0 ) {
    syslog(LOG_WARNING, "%s() Cannot get the location of BMC", __func__);
    return ret;
  }
  if ( bmc_location == NIC_BMC ) {
    tbuf[1] = 0x02;
  } else {
    tbuf[1] = 0x01;
  }

  i2cfd = i2c_cdev_slave_open(slot_id + SLOT_BUS_BASE, CPLD_ADDRESS >> 1, I2C_SLAVE_FORCE_CLAIM);
  if ( i2cfd < 0 ) {
    syslog(LOG_WARNING, "%s() Failed to open %d", __func__, CPLD_ADDRESS);
    goto error_exit;
  }

  retry = 0;
  while (retry < RETRY_TIME) {
    ret = i2c_rdwr_msg_transfer(i2cfd, CPLD_ADDRESS, tbuf, tlen, NULL, 0);
    if ( ret < 0 ) {
      retry++;
      msleep(100);
    } else {
      break;
    }
  }
  if (retry == RETRY_TIME) {
    syslog(LOG_WARNING, "%s() Failed to do i2c_rdwr_msg_transfer, tlen=%d", __func__, tlen);
    goto error_exit;
  }

  tbuf[0]=0x01;
  tbuf[1]=0x03;
  retry = 0;
  while (retry < RETRY_TIME) {
    ret = i2c_rdwr_msg_transfer(i2cfd, CPLD_ADDRESS, tbuf, tlen, NULL, 0);
    if ( ret < 0 ) {
      retry++;
      msleep(100);
    } else {
      break;
    }
  }
  if (retry == RETRY_TIME) {
    syslog(LOG_WARNING, "%s() Failed to do i2c_rdwr_msg_transfer, tlen=%d", __func__, tlen);
    goto error_exit;
  }

  printf("Setting BIC boot from UART\n");
  tbuf[0]=0x10;
  tbuf[1]=0x01;
  retry = 0;
  while (retry < RETRY_TIME) {
    ret = i2c_rdwr_msg_transfer(i2cfd, CPLD_ADDRESS, tbuf, tlen, NULL, 0);
    if ( ret < 0 ) {
      retry++;
      msleep(100);
    } else {
      break;
    }
  }
  if (retry == RETRY_TIME) {
    syslog(LOG_WARNING, "%s() Failed to do i2c_rdwr_msg_transfer, tlen=%d", __func__, tlen);
    goto error_exit;
  }
  if ( i2cfd > 0 ) close(i2cfd);

  snprintf(cmd, sizeof(cmd), "/bin/stty -F /dev/ttyS%d 115200", slot_id);
  if (system(cmd) != 0) {
    syslog(LOG_WARNING, "[%s] %s failed\n", __func__, cmd);
    goto error_exit;
  }

  snprintf(cmd, sizeof(cmd), "cat %s > /dev/ttyS%d", path, slot_id);
  if (system(cmd) != 0) {
    syslog(LOG_WARNING, "[%s] %s failed\n", __func__, cmd);
    goto error_exit;
  }

  snprintf(cmd, sizeof(cmd), "/bin/stty -F /dev/ttyS%d 57600", slot_id);
  if (system(cmd) != 0) {
    syslog(LOG_WARNING, "[%s] %s failed\n", __func__, cmd);
    goto error_exit;
  }

  printf("Please execute BIC firmware update and"
    " do the slot 12-cycle for finishing the BIC recovery\n");

error_exit:
  if ( i2cfd > 0 ) close(i2cfd);
  if ( fd > 0 ) close(fd);

  return ret;
}

static int
stop_bic_sensor_monitor(uint8_t slot_id, uint8_t intf) {
  int ret = 0;
  printf("* Turning off BIC sensor monitor...\n");
  ret = bic_enable_ssd_sensor_monitor(slot_id, false, intf);
  sleep(2);
  return ret;
}

static int
start_bic_sensor_monitor(uint8_t slot_id, uint8_t intf) {
  int ret = 0;
  printf("* Turning on BIC sensor monitor...\n");
  ret = bic_enable_ssd_sensor_monitor(slot_id, true, intf);
  sleep(2);
  return ret;
}

char*
get_component_name(uint8_t comp) {
  switch (comp) {
    case FW_CPLD:
      return "SB CPLD";
    case FW_SB_BIC:
      return "SB BIC";
    case FW_VR_VCCIN:
      return "VCCIN/VCCFA_EHV_FIVRA";
    case FW_VR_VCCD:
      return "VCCD";
    case FW_VR_VCCINFAON:
      return "VCCINFAON/VCCFA_EHV";
    case FW_BIOS:
      return "BIOS";
    case FW_1OU_BIC:
      return "1OU BIC";
    case FW_1OU_CPLD:
      return "1OU CPLD";
    case FW_2OU_BIC:
      return "2OU BIC";
    case FW_2OU_CPLD:
      return "2OU CPLD";
    case FW_BB_BIC:
      return "BB BIC";
    case FW_BB_CPLD:
      return "BB CPLD";
    case FW_2OU_PESW:
      return "2OU PCIe Switch";
    case FW_2OU_PESW_VR:
      return "2OU PCIe VR";
    case FW_2OU_3V3_VR1:
      return "VR_P3V3_STBY1";
    case FW_2OU_3V3_VR2:
      return "VR_P3V3_STBY2";
    case FW_2OU_3V3_VR3:
      return "VR_P3V3_STBY3";
    case FW_2OU_1V8_VR:
      return "VR_P1V8";
    case FW_2OU_M2_DEV0:
      return "2OU M2 Dev0";
    case FW_2OU_M2_DEV1:
      return "2OU M2 Dev1";
    case FW_2OU_M2_DEV2:
      return "2OU M2 Dev2";
    case FW_2OU_M2_DEV3:
      return "2OU M2 Dev3";
    case FW_2OU_M2_DEV4:
      return "2OU M2 Dev4";
    case FW_2OU_M2_DEV5:
      return "2OU M2 Dev5";
    case FW_2OU_M2_DEV6:
      return "2OU M2 Dev6";
    case FW_2OU_M2_DEV7:
      return "2OU M2 Dev7";
    case FW_2OU_M2_DEV8:
      return "2OU M2 Dev8";
    case FW_2OU_M2_DEV9:
      return "2OU M2 Dev9";
    case FW_2OU_M2_DEV10:
      return "2OU M2 Dev10";
    case FW_2OU_M2_DEV11:
      return "2OU M2 Dev11";
    default:
      return "Unknown";
  }

  return "NULL";
}

static bool
end_with (char* str, uint8_t str_len, char* pattern, uint8_t pattern_len) {
  if ((str == NULL) || (pattern == NULL)) {
    return false;
  }
  return (strncmp(str + (str_len - pattern_len), pattern, pattern_len) == 0);
}

static int
bic_update_fw_path_or_fd(uint8_t slot_id, uint8_t comp, char *path, int fd, uint8_t force) {
  int ret = BIC_STATUS_SUCCESS;
  uint8_t intf = 0x0;
  char ipmb_content[] = "ipmb";
  char tmp_posfix[] = "-tmp";
  char* loc = NULL;
  bool stop_bic_monitoring = false;
  bool stop_fscd_service = false;
  uint8_t bmc_location = 0;
  uint8_t status = 0;
  int i = 0;
  char fdstr[32] = {0};
  bool fd_opened = false;
  int origin_len = 0;
  char origin_path[128] = {0};

  if (path == NULL) {
    if (fd < 0) {
      syslog(LOG_ERR, "%s(): Update aborted due to NULL pointer: *path", __func__);
      return -1;
    }
    snprintf(fdstr, sizeof(fdstr) - 1, "<%d>", fd);
    path = fdstr;
  } else {
    fd = open(path, O_RDONLY, 0);
    if (fd < 0) {
      syslog(LOG_ERR, "%s(): Unable to open %s: %d", __func__, path, errno);
      return -1;
    }
    fd_opened = true;
  }

  loc = strstr(path, ipmb_content);

  if (end_with(path, strlen(path), tmp_posfix, strlen(tmp_posfix))) {
    origin_len = strlen(path) - strlen(tmp_posfix) + 1;
    if (origin_len > sizeof(origin_path)) {
      origin_len = sizeof(origin_path);
    }
    snprintf(origin_path, origin_len, "%s", path);
  } else {
    snprintf(origin_path, sizeof(origin_path), "%s", path);
  }

  fprintf(stderr, "slot_id: %x, comp: %x, intf: %x, img: %s, force: %x\n", slot_id, comp, intf, origin_path, force);
  syslog(LOG_CRIT, "Updating %s on slot%d. File: %s", get_component_name(comp), slot_id, origin_path);

  uint8_t board_type = 0;
  if ( fby35_common_get_2ou_board_type(slot_id, &board_type) < 0 ) {
    syslog(LOG_WARNING, "Failed to get 2ou board type\n");
  } else if ( board_type == GPV3_MCHP_BOARD ||
              board_type == GPV3_BRCM_BOARD ) {
    stop_bic_monitoring = true;
  }

  //get the intf
  switch (comp) {
    case FW_CPLD:
    case FW_ME:
    case FW_SB_BIC:
    case FW_BIC_RCVY:
    case FW_VR_VCCIN:
    case FW_VR_VCCD:
    case FW_VR_VCCINFAON:
      intf = NONE_INTF;
      break;
    case FW_1OU_BIC:
    case FW_1OU_CPLD:
      intf = FEXP_BIC_INTF;
      break;
    case FW_2OU_BIC:
    case FW_2OU_CPLD:
    case FW_2OU_PESW:
    case FW_2OU_PESW_VR:
    case FW_2OU_3V3_VR1:
    case FW_2OU_3V3_VR2:
    case FW_2OU_3V3_VR3:
    case FW_2OU_1V8_VR:
    case FW_2OU_M2_DEV0:
    case FW_2OU_M2_DEV1:
    case FW_2OU_M2_DEV2:
    case FW_2OU_M2_DEV3:
    case FW_2OU_M2_DEV4:
    case FW_2OU_M2_DEV5:
    case FW_2OU_M2_DEV6:
    case FW_2OU_M2_DEV7:
    case FW_2OU_M2_DEV8:
    case FW_2OU_M2_DEV9:
    case FW_2OU_M2_DEV10:
    case FW_2OU_M2_DEV11:
      intf = REXP_BIC_INTF;
      break;
    case FW_BB_BIC:
    case FW_BB_CPLD:
      intf = BB_BIC_INTF;
      break;
  }

  // stop fscd in class 2 system when update firmware through SB BIC to avoid IPMB busy
  ret = fby35_common_get_bmc_location(&bmc_location);
  if (ret < 0) {
    syslog(LOG_WARNING, "%s() Cannot get the location of BMC", __func__);
  }
  if (bmc_location == NIC_BMC) {
    switch (comp) {
      case FW_BIOS:
        if (loc != NULL) {
          stop_fscd_service = true;
        }
        break;
      default:
        stop_fscd_service = true;
        break;
    }
  }
  if (stop_fscd_service == true) {
    printf("Set fan mode to manual and set PWM to %d%%\n", FW_UPDATE_FAN_PWM);
    if (fby35_common_fscd_ctrl(FAN_MANUAL_MODE) < 0) {
      printf("Failed to stop fscd service\n");
      ret = -1;
      goto err_exit;
    }
    if (bic_set_fan_auto_mode(FAN_MANUAL_MODE, &status) < 0) {
      printf("Failed to set fan mode to auto\n");
      ret = -1;
      goto err_exit;
    }
    if (bic_notify_fan_mode(FAN_MANUAL_MODE) < 0) {
      printf("Failed to notify another BMC to set fan manual mode\n");
      ret = -1;
      goto err_exit;
    }
    for (i = 0; i < FAN_PWM_CNT; i++) {
      if (bic_manual_set_fan_speed(i, FW_UPDATE_FAN_PWM) < 0) {
        printf("Failed to set fan %d PWM to %d\n", i, FW_UPDATE_FAN_PWM);
        ret = -1;
        goto err_exit;
      }
    }
  }

  //run cmd
  switch (comp) {
    case FW_SB_BIC:
    case FW_1OU_BIC:
    case FW_2OU_BIC:
    case FW_BB_BIC:
      ret = update_bic_runtime_fw(slot_id, UPDATE_BIC, intf, path, force);
      break;
    case FW_BIC_RCVY:
      ret = recovery_bic_runtime_fw(slot_id, UPDATE_BIC, intf, path, force);
      break;
    case FW_1OU_CPLD:
    case FW_2OU_CPLD:
      if ( stop_bic_monitoring == true && (ret = stop_bic_sensor_monitor(slot_id, intf)) < 0 ) {
        printf("* Failed to stop bic sensor monitor\n");
        break;
      }

      ret = (loc != NULL)?update_bic_cpld_lattice(slot_id, path, intf, force): \
                          update_bic_cpld_lattice_usb(slot_id, path, intf, force);

      //check ret first and then check stop_bic_monitoring flag
      //run start_bic_sensor_monitor() and assgin a new val to ret in the end
      if ( (ret == BIC_STATUS_SUCCESS) && (stop_bic_monitoring == true) && \
           (ret = start_bic_sensor_monitor(slot_id, intf)) < 0 ) {
        printf("* Failed to start bic sensor monitor\n");
        break;
      }
      break;
    case FW_BB_CPLD:
      ret = update_bic_cpld_altera(slot_id, path, intf, force);
      break;
    case FW_CPLD:
      ret = update_bic_cpld_lattice(slot_id, path, intf, force);
      break;
    case FW_BIOS:
      if (loc != NULL) {
        ret = update_bic_bios(slot_id, comp, path, FORCE_UPDATE_SET);
      } else {
        ret = update_bic_usb_bios(slot_id, comp, fd);
      }
      break;
    case FW_VR_VCCIN:
    case FW_VR_VCCD:
    case FW_VR_VCCINFAON:
      ret = update_bic_vr(slot_id, comp, path, intf, force, false/*usb update?*/);
      break;
    case FW_2OU_3V3_VR1:
    case FW_2OU_3V3_VR2:
    case FW_2OU_3V3_VR3:
    case FW_2OU_1V8_VR:
      ret = update_bic_vr(slot_id, comp, path, intf, force, true/*usb update*/);
      break;
    case FW_2OU_PESW_VR:
      ret = (loc != NULL)?update_bic_vr(slot_id, comp, path, intf, force, false/*usb update*/): \
                          update_bic_vr(slot_id, comp, path, intf, force, true/*usb update*/);
      break;
    case FW_2OU_PESW:
      if (board_type == GPV3_BRCM_BOARD) {
        ret = BIC_STATUS_FAILURE; /*not supported*/
      } else {
        ret = update_bic_mchp(slot_id, comp, path, intf, force, (loc != NULL)?false:true);
      }
      break;
    case FW_2OU_M2_DEV0:
    case FW_2OU_M2_DEV1:
    case FW_2OU_M2_DEV2:
    case FW_2OU_M2_DEV3:
    case FW_2OU_M2_DEV4:
    case FW_2OU_M2_DEV5:
    case FW_2OU_M2_DEV6:
    case FW_2OU_M2_DEV7:
    case FW_2OU_M2_DEV8:
    case FW_2OU_M2_DEV9:
    case FW_2OU_M2_DEV10:
    case FW_2OU_M2_DEV11:
      if ( stop_bic_monitoring == true && stop_bic_sensor_monitor(slot_id, intf) < 0 ) {
        printf("* Failed to stop bic sensor monitor\n");
        break;
      }
      uint8_t nvme_ready = 0, type = 0;
      ret = bic_get_dev_info(slot_id, (comp - FW_2OU_M2_DEV0) + 1, &nvme_ready ,&status, &type);
      if (ret) {
        printf("* Failed to read m.2 device's info\n");
        ret = BIC_STATUS_FAILURE;
        break;
      } else {
        ret = update_bic_m2_fw(slot_id, comp, path, intf, force, type);
      }
      //run it anyway
      if ( stop_bic_monitoring == true && start_bic_sensor_monitor(slot_id, intf) < 0 ) {
        printf("* Failed to start bic sensor monitor\n");
        ret = BIC_STATUS_FAILURE;
        break;
      }
      break;
  }
  if (stop_fscd_service == true) {
    printf("Set fan mode to auto and start fscd\n");
    if (bic_set_fan_auto_mode(FAN_AUTO_MODE, &status) < 0) {
      printf("Failed to set fan mode to auto\n");
      ret = -1;
      goto err_exit;
    }
    if (bic_notify_fan_mode(FAN_AUTO_MODE) < 0) {
      printf("Failed to notify another BMC to set fan manual mode\n");
      ret = -1;
      goto err_exit;
    }
    if (fby35_common_fscd_ctrl(FAN_AUTO_MODE) < 0) {
       printf("Failed to start fscd service, please start fscd manually.\nCommand: sv start fscd\n");
    }
  }

err_exit:
  syslog(LOG_CRIT, "Updated %s on slot%d. File: %s. Result: %s", get_component_name(comp), slot_id, origin_path, (ret != 0)?"Fail":"Success");
  if (fd_opened) {
    close(fd);
  }
  return ret;
}

int
bic_update_fw(uint8_t slot_id, uint8_t comp, char *path, uint8_t force) {
  return bic_update_fw_path_or_fd(slot_id, comp, path, -1, force);
}

int
bic_update_fw_fd(uint8_t slot_id, uint8_t comp, int fd, uint8_t force) {
  return bic_update_fw_path_or_fd(slot_id, comp, NULL, fd, force);
}

int
get_board_revid_from_cpld(uint8_t cpld_bus, uint8_t reg_addr, uint8_t* rev_id){
  int i2cfd = 0;
  uint8_t rbuf[2] = {0};
  uint8_t rlen = sizeof(rbuf);
  uint8_t tbuf[4] = {0};
  uint8_t tlen = 0;
  int retry = 0;
  int ret = 0;

  i2cfd = i2c_cdev_slave_open(cpld_bus, CPLD_ADDRESS >> 1, I2C_SLAVE_FORCE_CLAIM);
  if ( i2cfd < 0 ) {
    syslog(LOG_WARNING, "%s() Failed to open %d", __func__, CPLD_ADDRESS);
    goto error_exit;
  }

  tbuf[0] = reg_addr;
  tlen = 1;
  rlen = 1;
  retry = 0;
  while (retry < RETRY_TIME) {
    ret = i2c_rdwr_msg_transfer(i2cfd, CPLD_ADDRESS, tbuf, tlen, rbuf, rlen);
    if ( ret < 0 ) {
      retry++;
      msleep(100);
    } else {
      break;
    }
  }
  if (retry == RETRY_TIME) {
    syslog(LOG_WARNING, "%s() Failed to do i2c_rdwr_msg_transfer, tlen=%d", __func__, tlen);
    ret = -1;
    goto error_exit;
  }
  *rev_id = rbuf[0];

  error_exit:
  if (i2cfd >= 0)
    close(i2cfd);

  return ret;
}

int
get_board_revid_from_bbbic(uint8_t slot_id, uint8_t* rev_id) {
  uint8_t tbuf[4] = {0x01, 0x1E, 0x01, 0x08}; // 0x01=bus, 0x1E=cpld address, 0x01=read count, 0x08=CPLD offset to get board rev
  uint8_t rbuf[1] = {0};
  uint8_t tlen = 4;
  uint8_t rlen = 0;

  if (rev_id == NULL) {
    syslog(LOG_WARNING, "%s: fail to get board revision ID due to getting NULL input *rev_id  \n", __func__);
    return -1;
  }

  int ret = bic_ipmb_send(slot_id, NETFN_APP_REQ, CMD_APP_MASTER_WRITE_READ, tbuf, tlen, rbuf, &rlen, BB_BIC_INTF);
  if (ret < 0) {
    syslog(LOG_WARNING, "%s: fail to get board revision ID because IPMB command fail. Ret: %d\n", __func__, ret);
    return ret;
  }
  *rev_id = rbuf[0];
  return ret;
}

int
get_board_rev(uint8_t slot_id, uint8_t board_id, uint8_t* rev_id) {
  uint8_t bmc_location = 0;
  int ret = 0;
  char value[MAX_VALUE_LEN] = {0};

  if ( rev_id == NULL ) {
    syslog(LOG_WARNING, "%s: fail to get board revision ID due to getting NULL input *rev_id \n", __func__);
    return -1;
  }

  ret = fby35_common_get_bmc_location(&bmc_location);
  if ( ret < 0 ) {
    syslog(LOG_WARNING, "%s() Cannot get the location of BMC", __func__);
    return ret;
  }

  if ( bmc_location == BB_BMC ) { // class 1
    switch (board_id) {
      case BOARD_ID_BB:
        if ( kv_get("board_rev_id", value, NULL, 0) == 0 ) {
          *rev_id = strtol(value, NULL, 10); // convert string to number
        } else {
          ret = get_board_revid_from_cpld(BB_CPLD_BUS, BB_CPLD_BOARD_REV_ID_REGISTER, rev_id);
          if (ret >= 0) {
            snprintf(value, sizeof(value), "%x", *rev_id);
            if (kv_set("board_rev_id", (char*)value, 1, KV_FCREATE)) {
              syslog(LOG_WARNING,"%s: kv_set failed, key: board_rev_id, val: %x", __func__, *rev_id);
              return -1;
            }
          }
        }
        break;
      case BOARD_ID_SB:
        ret = get_board_revid_from_cpld(slot_id + SLOT_BUS_BASE, SB_CPLD_BOARD_REV_ID_REGISTER, rev_id);
        break;
      default:
        syslog(LOG_WARNING, "%s() Not supported board id %x", __func__, board_id);
        return -1;
    }
  } else { // class 2
    switch (board_id) {
      case BOARD_ID_NIC_EXP:
        ret = get_board_revid_from_cpld(NIC_CPLD_BUS, BB_CPLD_BOARD_REV_ID_REGISTER, rev_id);
        break;
      case BOARD_ID_BB:
        if ( kv_get("board_rev_id", value, NULL, 0) == 0 ) {
          *rev_id = strtol(value, NULL, 10); // convert string to number
        } else {
          ret = get_board_revid_from_bbbic(FRU_SLOT1, rev_id);
          if (ret >= 0) {
            snprintf(value, sizeof(value), "%x", *rev_id);
            if (kv_set("board_rev_id", (char*)value, 1, KV_FCREATE)) {
              syslog(LOG_WARNING,"%s: kv_set failed, key: board_rev_id, val: %x", __func__, *rev_id);
              return -1;
            }
          }
        }
        break;
      case BOARD_ID_SB:
        ret = get_board_revid_from_cpld(slot_id + SLOT_BUS_BASE, SB_CPLD_BOARD_REV_ID_REGISTER, rev_id);
        break;
      default:
        syslog(LOG_WARNING, "%s() Not supported board id %x", __func__, board_id);
        return -1;
    }
  }

  return ret;
}
