
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
#include <openbmc/libgpio.h>
#include <openbmc/misc-utils.h>
#include <openbmc/obmc-i2c.h>
#include "bic_fwupdate.h"
#include "bic_ipmi.h"
#include "bic_xfer.h"
#include "bic_bios_fwupdate.h"
#include "bic_cpld_altera_fwupdate.h"

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

#define VF2_IOEXP_ADDR  0x92  // 8-bit address

#define SB_TYPE_CLASS1       0x01
#define SB_TYPE_CLASS2       0x02
#define UART_CH_RCVY_1OU     0x01
#define UART_CH_RCVY_2OU     0x02
#define UART_CH_RCVY_SB      0x03
#define UART_CH_RCVY_BB      0x04
#define UART_CH_RCVY_OP_1_2  0x05
#define UART_CH_RCVY_OP_3_4  0x02
#define FORCE_BOOT_FROM_SPI  0x00
#define FORCE_BOOT_FROM_UART 0x01
#define OP_1OU_IOEXP_ADDR    0x4C
#define OP_2OU_IOEXP_ADDR    0x44
#define OP_3OU_IOEXP_ADDR    0x4E
#define OP_4OU_IOEXP_ADDR    0x46

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
_update_fw(uint8_t slot_id, uint8_t target, uint8_t type, uint32_t offset,
           uint16_t len, uint8_t *buf, uint8_t intf) {
  uint8_t tbuf[256] = {0x00};
  uint8_t rbuf[16] = {0x00};
  uint8_t tlen = 0;
  uint8_t rlen = sizeof(rbuf);
  int ret = 0, retry;

  if (buf == NULL) {
    return -1;
  }

  // Fill the IANA ID
  if ((intf == FEXP_BIC_INTF) && (type == TYPE_1OU_VERNAL_FALLS_WITH_AST)) {
    memcpy(tbuf, (uint8_t *)&VF_IANA_ID, IANA_ID_SIZE);
  } else {
    memcpy(tbuf, (uint8_t *)&IANA_ID, IANA_ID_SIZE);
  }

  // Fill the component for which firmware is requested
  tbuf[3] = target;
  memcpy(&tbuf[4], &offset, sizeof(offset));
  tbuf[8] = len & 0xFF;
  tbuf[9] = (len >> 8) & 0xFF;
  memcpy(&tbuf[10], buf, len);

  tlen = len + 10;
  for (retry = 0; retry < 3; retry++) {
    ret = bic_data_send(slot_id, NETFN_OEM_1S_REQ, CMD_OEM_1S_UPDATE_FW, tbuf, tlen, rbuf, &rlen, intf);
    if (ret == 0) {
      return ret;
    }

    sleep(1);
    printf("_update_fw: slot: %u, target %u, offset: %u, len: %u retrying..\n",
           slot_id, target, offset, len);
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
update_bic(uint8_t slot_id, int fd, size_t file_size, uint8_t intf) {
  struct timeval start, end;
  int update_rc = -1, cmd_rc = 0;
  uint32_t dsize, last_offset;
  uint32_t offset, boundary;
  volatile uint16_t read_count;
  uint8_t buf[256] = {0};
  uint8_t target;
  uint8_t bmc_location = 0;
  uint8_t type = TYPE_1OU_UNKNOWN;
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
  if (intf == FEXP_BIC_INTF) {
    bic_get_1ou_type(slot_id, &type);
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
    cmd_rc = _update_fw(slot_id, target, type, offset, count, buf, intf);
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
    case EXP3_BIC_INTF:
    case EXP4_BIC_INTF:
      ret = BIC_STATUS_SUCCESS;
      break;
  }

  return ret;
}

static int
update_bic_runtime_fw(uint8_t slot_id, uint8_t comp __attribute__((unused)), uint8_t intf, char *path, uint8_t force __attribute__((unused))) {
  #define MAX_CMD_LEN 120
  #define MAX_RETRY 10
  char cmd[MAX_CMD_LEN] = {0};
  uint8_t self_test_result[2] = {0};
  int ret = -1;
  int fd = -1;
  size_t file_size;

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

  printf("file size = %zu bytes, slot = %u, intf = 0x%x\n", file_size, slot_id, intf);

  //run into the different function based on the interface
  ret = update_bic(slot_id, fd, file_size, intf);

  if ( (ret == 0) && (comp == FW_SB_BIC) ) {
    ret = fby35_common_get_sb_bic_boot_strap(slot_id);
    if (ret == FORCE_BOOT_FROM_UART) {
      // For BIC recovery case, we can exit here to skip the following IPMB communication which is not accessible at that moment
      ret = 0;
      goto exit;
    }

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
recovery_bic_runtime_fw(uint8_t slot_id, uint8_t comp, uint8_t intf, char *path, uint8_t force) {
  int ret = -1;
  size_t file_size = 0;
  int fd = -1, i2cfd = -1, ttyfd = -1, ioexpfd = -1, uart_ioexpfd = -1;
  bool set_strap = false;
  uint8_t bus, addr, uart_ioexp_addr;
  uint8_t type = TYPE_1OU_UNKNOWN;
  uint8_t uart_ch = UART_CH_RCVY_SB;
  uint8_t strap_reg = CPLD_REG_SB_BIC_BOOT_STRAP;
  uint8_t buf[64], bmc_location = 0;
  char cmd[64];
  gpio_desc_t *vf_fwspick = NULL, *vf_srst = NULL;
  uint8_t op_index = 0;
  uint8_t tbuf[MAX_IPMB_RES_LEN] = {0};
  uint8_t rbuf[MAX_IPMB_RES_LEN] = {0};
  uint8_t val_reg = 0xFF, dir_reg = 0xFF;
  typedef struct {
    uint8_t comp;
    int mux_ctrl;
    uint8_t uart_mux_ioexp_addr;
    uint8_t ioexp_addr;
    uint8_t uart_ch;
    uint8_t component;
  } OP_BIC_RCVY_INFO;
  static const OP_BIC_RCVY_INFO op_info[4] = {
    {FW_1OU_BIC, 1, OP_1OU_IOEXP_ADDR, OP_1OU_IOEXP_ADDR, UART_CH_RCVY_OP_1_2, FW_1OU_BIC_RCVY},
    {FW_2OU_BIC, 0, OP_1OU_IOEXP_ADDR, OP_2OU_IOEXP_ADDR, UART_CH_RCVY_OP_1_2, FW_2OU_BIC_RCVY},
    {FW_3OU_BIC, 1, OP_3OU_IOEXP_ADDR, OP_3OU_IOEXP_ADDR, UART_CH_RCVY_OP_3_4, FW_3OU_BIC_RCVY},
    {FW_4OU_BIC, 0, OP_3OU_IOEXP_ADDR, OP_4OU_IOEXP_ADDR, UART_CH_RCVY_OP_3_4, FW_4OU_BIC_RCVY},
  };

  if (path == NULL) {
    return -1;
  }

  switch (comp) {
    case FW_BIC_RCVY:
      uart_ch = UART_CH_RCVY_SB;
      strap_reg = CPLD_REG_SB_BIC_BOOT_STRAP;
      comp = FW_SB_BIC;
      break;
    case FW_1OU_BIC_RCVY:
    case FW_2OU_BIC_RCVY:
    case FW_3OU_BIC_RCVY:
    case FW_4OU_BIC_RCVY:
      uart_ch = UART_CH_RCVY_1OU;
      strap_reg = CPLD_REG_1OU_BIC_BOOT_STRAP;
      if (bic_get_1ou_type(slot_id, &type) < 0) {
        syslog(LOG_WARNING, "%s[%u] Failed to get 1ou type", __func__, slot_id);
      }
      if (type == TYPE_1OU_OLMSTEAD_POINT) {
        for (op_index = 0; op_index < sizeof(op_info)/sizeof(OP_BIC_RCVY_INFO); op_index++) {
          if (op_info[op_index].component == comp) {
            break;
          }
        }
        comp = op_info[op_index].component;
        uart_ch = op_info[op_index].uart_ch;
      } else {
        comp = FW_1OU_BIC;
      }
      break;
    default:
      return -1;
  }

  // get fd and file size
  fd = open_and_get_size(path, &file_size);
  if (fd < 0) {
    syslog(LOG_ERR, "%s[%u] Cannot open %s", __func__, slot_id, path);
    return -1;
  }
  printf("file size = %zu bytes, slot = %u, intf = 0x%x\n", file_size, slot_id, intf);

  bus = slot_id + SLOT_BUS_BASE;
  addr = (SB_CPLD_ADDR << 1);

  do {
    if (fby35_common_get_bmc_location(&bmc_location) < 0) {
      syslog(LOG_ERR, "%s[%u] Cannot get the location of BMC", __func__, slot_id);
      break;
    }

    i2cfd = i2c_cdev_slave_open(bus, (addr >> 1), I2C_SLAVE_FORCE_CLAIM);
    if (i2cfd < 0) {
      syslog(LOG_ERR, "%s[%u] Failed to open bus %u", __func__, slot_id, bus);
      break;
    }

    printf("Setting slot UART to BIC\n");
    buf[0] = CPLD_REG_SB_CLASS;
    buf[1] = (bmc_location == NIC_BMC) ? SB_TYPE_CLASS2 : SB_TYPE_CLASS1;
    if (retry_cond(!i2c_rdwr_msg_transfer(i2cfd, addr, buf, 2, NULL, 0), 3, 50)) {
      syslog(LOG_ERR, "%s[%u] Failed to write cpld 0x%02X", __func__, slot_id, buf[0]);
      break;
    }

    buf[0] = CPLD_REG_UART_MUX;
    buf[1] = uart_ch;
    if (retry_cond(!i2c_rdwr_msg_transfer(i2cfd, addr, buf, 2, NULL, 0), 3, 50)) {
      syslog(LOG_ERR, "%s[%u] Failed to write cpld 0x%02X", __func__, slot_id, buf[0]);
      break;
    }

    printf("Setting BIC boot from UART\n");
    if (type == TYPE_1OU_VERNAL_FALLS_WITH_AST) {
      // set BIC_FWSPICK_IO_EXP (bit2) to high
      snprintf(cmd, sizeof(cmd), BIC_FWSPICK_SHADOW, slot_id);
      if (!(vf_fwspick = gpio_open_by_shadow(cmd))) {
        syslog(LOG_ERR, "%s[%u] Failed to open %s", __func__, slot_id, cmd);
        break;
      }
      if (gpio_set_init_value(vf_fwspick, GPIO_VALUE_HIGH)) {
        syslog(LOG_ERR, "%s[%u] Failed to set BIC_FWSPICK_IO_EXP to high", __func__, slot_id);
        break;
      }

      // set BIC_SRST_N_IO_EXP_R (bit0) to low
      snprintf(cmd, sizeof(cmd), BIC_SRST_SHADOW, slot_id);
      if (!(vf_srst = gpio_open_by_shadow(cmd))) {
        syslog(LOG_ERR, "%s[%u] Failed to open %s", __func__, slot_id, cmd);
        break;
      }
      if (gpio_set_init_value(vf_srst, GPIO_VALUE_LOW)) {
        syslog(LOG_ERR, "%s[%u] Failed to set BIC_SRST_N_IO_EXP_R to low", __func__, slot_id);
        break;
      }

      msleep(100);
      // let BIC_SRST_N_IO_EXP_R (bit0) be high
      if (gpio_set_direction(vf_srst, GPIO_DIRECTION_IN)) {
        syslog(LOG_ERR, "%s[%u] Failed to let BIC_SRST_N_IO_EXP_R be high", __func__, slot_id);
        break;
      }
    } else if (type == TYPE_1OU_OLMSTEAD_POINT) {
      uart_ioexp_addr = op_info[op_index].uart_mux_ioexp_addr;
      uart_ioexpfd = i2c_cdev_slave_open(bus, (uart_ioexp_addr >> 1), I2C_SLAVE_FORCE_CLAIM);
      if (uart_ioexpfd < 0) {
        syslog(LOG_ERR, "%s[%u] Failed to open bus %u", __func__, slot_id, bus);
        break;
      }
      addr = op_info[op_index].ioexp_addr;
      if (addr == uart_ioexp_addr) {
        ioexpfd = uart_ioexpfd;
      } else {
        ioexpfd = i2c_cdev_slave_open(bus, (addr >> 1), I2C_SLAVE_FORCE_CLAIM);
      }
      if (ioexpfd < 0) {
        syslog(LOG_ERR, "%s[%u] Failed to open bus %u", __func__, slot_id, bus);
        break;
      }
      // select UART MUX
      tbuf[0] = 0x01; // value register
      if (retry_cond(!i2c_rdwr_msg_transfer(uart_ioexpfd, uart_ioexp_addr, tbuf, 1, rbuf, 1), 3, 50)) {
        syslog(LOG_ERR, "%s[%u] Failed to read IOEXP 0x%02X", __func__, slot_id, tbuf[0]);
        break;
      }
      val_reg = rbuf[0];

      tbuf[0] = 0x01; // value
      val_reg = (op_info[op_index].mux_ctrl)? SET_OP_IOEXP(val_reg, OP_IOEXP_UART_IOEXP_MUX_CTRL):
                  CLEAR_OP_IOEXP(val_reg, OP_IOEXP_UART_IOEXP_MUX_CTRL);
      tbuf[1] = val_reg;
      if (retry_cond(!i2c_rdwr_msg_transfer(uart_ioexpfd, uart_ioexp_addr, tbuf, 2, NULL, 0), 3, 50)) {
        syslog(LOG_ERR, "%s[%u] Failed to set IOEXP 0x%02X", __func__, slot_id, tbuf[0]);
        break;
      }
      tbuf[0] = 0x03; // direction
      dir_reg = CLEAR_OP_IOEXP(dir_reg, OP_IOEXP_UART_IOEXP_MUX_CTRL); // output
      tbuf[1] = dir_reg;
      if (retry_cond(!i2c_rdwr_msg_transfer(uart_ioexpfd, uart_ioexp_addr, tbuf, 2, NULL, 0), 3, 50)) {
        syslog(LOG_ERR, "%s[%u] Failed to set IOEXP 0x%02X", __func__, slot_id, tbuf[0]);
        break;
      }
      msleep(20);

      // set boot from UART
      tbuf[0] = 0x01; // value
      val_reg = SET_OP_IOEXP(val_reg, OP_IOEXP_BIC_FWSPICK_R); // high
      tbuf[1] = val_reg;
      if (retry_cond(!i2c_rdwr_msg_transfer(ioexpfd, addr, tbuf, 2, NULL, 0), 3, 50)) {
        syslog(LOG_ERR, "%s[%u] Failed to set IOEXP 0x%02X", __func__, slot_id, tbuf[0]);
        break;
      }
      tbuf[0] = 0x03; // direction
      dir_reg = CLEAR_OP_IOEXP(dir_reg, OP_IOEXP_BIC_FWSPICK_R); // output
      tbuf[1] = dir_reg;
      if (retry_cond(!i2c_rdwr_msg_transfer(ioexpfd, addr, tbuf, 2, NULL, 0), 3, 50)) {
        syslog(LOG_ERR, "%s[%u] Failed to set IOEXP 0x%02X", __func__, slot_id, tbuf[0]);
        break;
      }

      // BIC EXRST
      tbuf[0] = 0x01; // value
      val_reg = CLEAR_OP_IOEXP(val_reg, OP_IOEXP_BIC_SRST_N_R); // low
      tbuf[1] = val_reg;
      if (retry_cond(!i2c_rdwr_msg_transfer(ioexpfd, addr, tbuf, 2, NULL, 0), 3, 50)) {
        syslog(LOG_ERR, "%s[%u] Failed to set IOEXP 0x%02X", __func__, slot_id, tbuf[0]);
        break;
      }
      tbuf[0] = 0x03; // direction
      dir_reg = CLEAR_OP_IOEXP(dir_reg, OP_IOEXP_BIC_SRST_N_R); // output
      tbuf[1] = dir_reg;
      if (retry_cond(!i2c_rdwr_msg_transfer(ioexpfd, addr, tbuf, 2, NULL, 0), 3, 50)) {
        syslog(LOG_ERR, "%s[%u] Failed to set IOEXP 0x%02X", __func__, slot_id, tbuf[0]);
        break;
      }
      msleep(100);
      tbuf[0] = 0x01; // value
      val_reg = SET_OP_IOEXP(val_reg, OP_IOEXP_BIC_SRST_N_R); // high
      tbuf[1] = val_reg;
      if (retry_cond(!i2c_rdwr_msg_transfer(ioexpfd, addr, tbuf, 2, NULL, 0), 3, 50)) {
        syslog(LOG_ERR, "%s[%u] Failed to set IOEXP 0x%02X", __func__, slot_id, tbuf[0]);
        break;
      }
    } else {
      buf[0] = strap_reg;
      buf[1] = FORCE_BOOT_FROM_SPI;
      if (retry_cond(!i2c_rdwr_msg_transfer(i2cfd, addr, buf, 2, NULL, 0), 3, 50)) {
        syslog(LOG_ERR, "%s[%u] Failed to write cpld 0x%02X", __func__, slot_id, buf[0]);
        break;
      }
      msleep(20);
      buf[0] = strap_reg;
      buf[1] = FORCE_BOOT_FROM_UART;
      if (retry_cond(!i2c_rdwr_msg_transfer(i2cfd, addr, buf, 2, NULL, 0), 3, 50)) {
        syslog(LOG_ERR, "%s[%u] Failed to write cpld 0x%02X", __func__, slot_id, buf[0]);
        break;
      }
    }
    set_strap = true;
    sleep(2);

    snprintf(cmd, sizeof(cmd), "/bin/stty -F /dev/ttyS%u 115200", slot_id);
    if (system(cmd) != 0) {
      syslog(LOG_WARNING, "%s[%u] %s failed", __func__, slot_id, cmd);
    }

    snprintf(cmd, sizeof(cmd), "/dev/ttyS%u", slot_id);
    ttyfd = open(cmd, O_RDWR | O_NOCTTY);
    if (ttyfd < 0) {
      syslog(LOG_ERR, "%s[%u] Cannot open %s", __func__, slot_id, cmd);
      break;
    }

    printf("Doing the recovery update...\n");
    size_t r_b = 0;
    if (write(ttyfd, &file_size, sizeof(int32_t)) == sizeof(int32_t)) {
      size_t dsize = file_size/100;
      size_t last_offset = 0;
      int w_b, rc, wc;
      for (r_b = 0; r_b < file_size;) {
        rc = read(fd, buf, sizeof(buf));
        if (rc <= 0) {
          if (rc < 0 && errno == EINTR) {
            continue;
          }
          break;
        }

        for (w_b = 0; w_b < rc;) {
          wc = write(ttyfd, &buf[w_b], rc - w_b);
          if (wc > 0) {
            w_b += wc;
          } else {
            if (wc < 0 && errno == EINTR) {
              continue;
            }
            break;
          }
        }
        if (w_b != rc) {
          break;
        }
        r_b += rc;

        if ((last_offset + dsize) <= r_b) {
          _set_fw_update_ongoing(slot_id, 60);
          printf("\ruploaded bic: %zu %%", r_b/dsize);
          fflush(stdout);
          last_offset += dsize;
        }
      }
      printf("\n");
    }

    snprintf(cmd, sizeof(cmd), "/bin/stty -F /dev/ttyS%u 57600", slot_id);
    if (system(cmd) != 0) {
      syslog(LOG_WARNING, "%s[%u] %s failed", __func__, slot_id, cmd);
    }

    if (r_b != file_size) {
      syslog(LOG_ERR, "%s[%u] uploaded bic failed", __func__, slot_id);
      break;
    }
    sleep(5);

    ret = update_bic_runtime_fw(slot_id, comp, intf, path, force);
    sleep(5);
  } while (0);

  if (set_strap) {
    if (type == TYPE_1OU_VERNAL_FALLS_WITH_AST) {
      // let BIC_FWSPICK_IO_EXP (bit2) be low
      if (gpio_set_direction(vf_fwspick, GPIO_DIRECTION_IN)) {
        syslog(LOG_ERR, "%s[%u] Failed to let BIC_FWSPICK_IO_EXP be low", __func__, slot_id);
      }

      // set BIC_SRST_N_IO_EXP_R (bit0) to low
      if (gpio_set_init_value(vf_srst, GPIO_VALUE_LOW)) {
        syslog(LOG_ERR, "%s[%u] Failed to set BIC_SRST_N_IO_EXP_R to low", __func__, slot_id);
      }

      msleep(100);
      // let BIC_SRST_N_IO_EXP_R (bit0) be high
      if (gpio_set_direction(vf_srst, GPIO_DIRECTION_IN)) {
        syslog(LOG_ERR, "%s[%u] Failed to let BIC_SRST_N_IO_EXP_R be high", __func__, slot_id);
      }
    } else if (type == TYPE_1OU_OLMSTEAD_POINT) {
      // set boot from SPI
      tbuf[0] = 0x01; // data
      val_reg = CLEAR_OP_IOEXP(val_reg, OP_IOEXP_BIC_FWSPICK_R); // low
      tbuf[1] = val_reg;
      if (retry_cond(!i2c_rdwr_msg_transfer(ioexpfd, addr, tbuf, 2, NULL, 0), 3, 50)) {
        syslog(LOG_ERR, "%s[%u] Failed to set IOEXP 0x%02X", __func__, slot_id, tbuf[0]);
      }
      // BIC EXRST
      tbuf[0] = 0x01; // data
      val_reg = CLEAR_OP_IOEXP(val_reg, OP_IOEXP_BIC_SRST_N_R); // low
      tbuf[1] = val_reg;
      if (retry_cond(!i2c_rdwr_msg_transfer(ioexpfd, addr, tbuf, 2, NULL, 0), 3, 50)) {
        syslog(LOG_ERR, "%s[%u] Failed to set IOEXP 0x%02X", __func__, slot_id, tbuf[0]);
      }
      msleep(100);
      tbuf[0] = 0x01; // data
      val_reg = SET_OP_IOEXP(val_reg, OP_IOEXP_BIC_SRST_N_R); // high
      tbuf[1] = val_reg;
      if (retry_cond(!i2c_rdwr_msg_transfer(ioexpfd, addr, tbuf, 2, NULL, 0), 3, 50)) {
        syslog(LOG_ERR, "%s[%u] Failed to set IOEXP 0x%02X", __func__, slot_id, tbuf[0]);
      }
    } else {
      buf[0] = strap_reg;
      buf[1] = FORCE_BOOT_FROM_SPI;
      if (retry_cond(!i2c_rdwr_msg_transfer(i2cfd, addr, buf, 2, NULL, 0), 3, 50)) {
        syslog(LOG_WARNING, "%s[%u] Failed to write cpld 0x%02X", __func__, slot_id, buf[0]);
      }
    }
  }

  if (fd >= 0) {
    close(fd);
  }
  if (i2cfd >= 0) {
    close(i2cfd);
  }
  if (ttyfd >= 0) {
    close(ttyfd);
  }
  if (ioexpfd >= 0) {
    close(ioexpfd);
  }
  if ((uart_ioexpfd >= 0) && (uart_ioexpfd != ioexpfd)) {
    close(uart_ioexpfd);
  }
  if (vf_fwspick) {
    gpio_close(vf_fwspick);
  }
  if (vf_srst) {
    gpio_close(vf_srst);
  }

  return ret;
}

char*
get_component_name(uint8_t comp) {
  switch (comp) {
    case FW_CPLD:
      return "SB CPLD";
    case FW_SB_BIC:
      return "SB BIC";
    case FW_BIC_RCVY:
      return "SB BIC_Recovery";
    case FW_VR_VCCIN:
      return "VCCIN/VCCFA_EHV_FIVRA";
    case FW_VR_VCCD:
      return "VCCD";
    case FW_VR_VCCINFAON:
      return "VCCINFAON/VCCFA_EHV";
    case FW_VR_VCCIN_EHV:
      return "VCCIN/VCCFA_EHV";
    case FW_VR_VCCD_HV:
      return "VCCD_HV";
    case FW_VR_VCCINF:
      return "VCCINF/VCCFA_EHV_FIVRA";
    case FW_VR_VDDCRCPU0:
      return "VDDCR_CPU0/VDDCR_SOC";
    case FW_VR_VDDCRCPU1:
      return "VDDCR_CPU1/VDDIO";
    case FW_VR_VDD11S3:
      return "VDD11_S3";
    case FW_BIOS:
    case FW_BIOS_SPIB:
      return "BIOS";
    case FW_PROT:
    case FW_PROT_SPIB:
      return "PRoT";
    case FW_1OU_BIC:
      return "1OU BIC";
    case FW_1OU_BIC_RCVY:
      return "1OU BIC_Recovery";
    case FW_1OU_CPLD:
      return "1OU CPLD";
    case FW_2OU_BIC:
      return "2OU BIC";
    case FW_2OU_BIC_RCVY:
      return "2OU BIC_Recovery";
    case FW_2OU_CPLD:
      return "2OU CPLD";
    case FW_BB_BIC:
      return "BB BIC";
    case FW_BB_CPLD:
      return "BB CPLD";
    case FW_1OU_CXL:
      return "1OU CXL";
    case FW_2OU_PESW:
      return "2OU PCIe Switch";
    case FW_1OU_VR_V9_ASICA:
      return "1OU_VR_P0V9/P0V8_ASICA";
    case FW_1OU_VR_VDDQAB:
      return "1OU_VR_VDDQAB/D0V8";
    case FW_1OU_VR_VDDQCD:
      return "1OU_VR_VDDQCD";
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
    case FW_3OU_BIC:
      return "3OU BIC";
    case FW_3OU_BIC_RCVY:
      return "3OU BIC_Recovery";
    case FW_4OU_BIC:
      return "4OU BIC";
    case FW_4OU_BIC_RCVY:
      return "4OU BIC_Recovery";
    case FW_VPDB_VR:
      return "VPDB VR";
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
update_bic_retimer(uint8_t slot_id, char* path, uint8_t intf) {
#define RETIMER_UPDATE_PACKET_SIZE 128
  size_t file_size = 0;
  int fd = -1;
  uint32_t image_size = 0;
  struct timeval start, end;
  int ret = 0;
  uint32_t dsize = 0, last_offset = 0;
  uint32_t offset = 0;
  volatile uint16_t read_count = 0;
  uint8_t buf[256] = {0};
  uint8_t target = UPDATE_RETIMER;
  ssize_t count = 0;
  bool is_latest_packet = false;
  int retry = 3;

  if (path == NULL) {
    syslog(LOG_WARNING, "%s() NULL file path\n", __func__);
    return -1;
  }
  //get fd and file size
  fd = open_and_get_size(path, &file_size);
  if ( fd < 0 ) {
    syslog(LOG_WARNING, "%s() cannot open the file: %s, fd=%d\n", __func__, path, fd);
    goto error_exit;
  }
  // Find the latest non-zero byte as the end of image
  for (image_size = file_size; image_size > 0; image_size--) {
    lseek(fd, image_size, SEEK_SET);
    if (read(fd, buf, 1) < 0) {
      syslog(LOG_WARNING, "%s(): read failed while calculating size", __func__);
      break;
    }
    if ((buf[0] != 0x0)) {
      break;
    }
  }
  dsize = image_size / 100;
  gettimeofday(&start, NULL);

  lseek(fd, 0, SEEK_SET);
  while (1) {
    read_count = RETIMER_UPDATE_PACKET_SIZE;
    // Read from file
    count = read(fd, buf, read_count);
    if (count < 0) {
      if ((retry == 0) || (errno != EINTR)) {
        syslog(LOG_WARNING, "%s(): read image failed, offset = %x", __func__, offset);
        ret = -1;
        goto error_exit;
      } else {
        retry--;
        continue;
      }
    }
    if ((offset + count) >= image_size) {
      target |= 0x80; //to indicate last byte to bic
      is_latest_packet = true;
      count = image_size - offset + 1;
    }
    // Send data to Bridge-IC
    ret = _update_fw(slot_id, target, TYPE_1OU_OLMSTEAD_POINT, offset, count, buf, intf);
    if (ret) {
      goto error_exit;
    }

    // Update counter
    offset += count;
    if ((last_offset + dsize) <= offset) {
      _set_fw_update_ongoing(slot_id, 60); //60s timeout for flag
      printf("\rupdated retimer: %u %%", offset/dsize);
      fflush(stdout);
      last_offset += dsize;
    }
    if (is_latest_packet) {
      break;
    }
  }
  printf("\n");
  gettimeofday(&end, NULL);
  printf("Elapsed time:  %d   sec.\n", (int)(end.tv_sec - start.tv_sec));

error_exit:
  printf("\n");
  if (fd >= 0) {
    close(fd);
  }
  return ret;
}

static int
bic_update_fw_path_or_fd(uint8_t slot_id, uint8_t comp, char *path, int fd, uint8_t force) {
  int ret = BIC_STATUS_SUCCESS;
  uint8_t intf = 0x0;
  char ipmb_content[] = "ipmb";
  char tmp_posfix[] = "-tmp";
  char* loc = NULL;
  bool stop_fscd_service = false;
  uint8_t bmc_location = 0;
  uint8_t status = 0;
  int i = 0;
  char fdstr[32] = {0};
  bool fd_opened = false;
  size_t origin_len = 0;
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

  //get the intf
  switch (comp) {
    case FW_ME:
    case FW_SB_BIC:
    case FW_BIC_RCVY:
      intf = NONE_INTF;
      break;
    case FW_1OU_BIC:
    case FW_1OU_BIC_RCVY:
    case FW_1OU_RETIMER:
      intf = FEXP_BIC_INTF;
      break;
    case FW_2OU_BIC:
    case FW_2OU_BIC_RCVY:
    case FW_2OU_PESW:
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
    case FW_3OU_BIC:
    case FW_3OU_BIC_RCVY:
    case FW_3OU_RETIMER:
      intf = EXP3_BIC_INTF;
      break;
    case FW_4OU_BIC:
    case FW_4OU_BIC_RCVY:
      intf = EXP4_BIC_INTF;
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
    case FW_3OU_BIC:
    case FW_4OU_BIC:
      ret = update_bic_runtime_fw(slot_id, UPDATE_BIC, intf, path, force);
      break;
    case FW_BIC_RCVY:
    case FW_1OU_BIC_RCVY:
    case FW_2OU_BIC_RCVY:
    case FW_3OU_BIC_RCVY:
    case FW_4OU_BIC_RCVY:
      ret = recovery_bic_runtime_fw(slot_id, comp, intf, path, force);
      break;
    case FW_BB_CPLD:
      ret = update_bic_cpld_altera(slot_id, path, intf, force);
      break;
    case FW_BIOS:
    case FW_BIOS_SPIB:
      if (loc != NULL) {
        ret = update_bic_bios(slot_id, comp, path, FORCE_UPDATE_SET);
      } else {
        ret = update_bic_usb_bios(slot_id, comp, fd, force);
      }
      break;
    case FW_PROT:
    case FW_PROT_SPIB:
    case FW_1OU_CXL:
      ret = update_bic_usb_bios(slot_id, comp, fd, force);
      break;
    case FW_1OU_RETIMER:
    case FW_3OU_RETIMER:
      ret = update_bic_retimer(slot_id, path, intf);
      break;
    default:
      syslog(LOG_WARNING, "%s(): component %x not supported", __func__, comp);
      return -1;
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
get_board_revid_from_bbbic(uint8_t slot_id, uint8_t* rev_id) {
  uint8_t tbuf[4] = {0x01, 0x1E, 0x01, 0x08}; // 0x01=bus, 0x1E=cpld address, 0x01=read count, 0x08=CPLD offset to get board rev
  uint8_t rbuf[1] = {0};
  uint8_t tlen = 4;
  uint8_t rlen = 0;

  if (rev_id == NULL) {
    syslog(LOG_WARNING, "%s: fail to get board revision ID due to getting NULL input *rev_id  \n", __func__);
    return -1;
  }

  int ret = bic_data_send(slot_id, NETFN_APP_REQ, CMD_APP_MASTER_WRITE_READ, tbuf, tlen, rbuf, &rlen, BB_BIC_INTF);
  if (ret < 0) {
    syslog(LOG_WARNING, "%s: fail to get board revision ID because IPMB command fail. Ret: %d\n", __func__, ret);
    return ret;
  }
  *rev_id = rbuf[0] & 0x07; // Board revision only composed of bit 0-2
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
        if ((ret = fby35_common_get_bb_rev()) < 0) {
          return -1;
        }
        *rev_id = (uint8_t)ret;
        ret = 0;
        break;
      case BOARD_ID_SB:
        if ((ret = fby35_common_get_sb_rev(slot_id)) < 0) {
          return -1;
        }
        *rev_id = (uint8_t)ret;
        ret = 0;
        break;
      default:
        syslog(LOG_WARNING, "%s() Not supported board id %x", __func__, board_id);
        return -1;
    }
  } else { // class 2
    switch (board_id) {
      case BOARD_ID_NIC_EXP:
        if ((ret = fby35_common_get_bb_rev()) < 0) {
          return -1;
        }
        *rev_id = (uint8_t)ret;
        ret = 0;
        break;
      case BOARD_ID_BB:
        if ( kv_get("board_rev_id", value, NULL, 0) == 0 ) {
          *rev_id = strtol(value, NULL, 10); // convert string to number
        } else {
          ret = get_board_revid_from_bbbic(FRU_SLOT1, rev_id);
          if (ret == 0) {
            snprintf(value, sizeof(value), "%x", *rev_id);
            if (kv_set("board_rev_id", (char*)value, 1, KV_FCREATE)) {
              syslog(LOG_WARNING,"%s: kv_set failed, key: board_rev_id, val: %x", __func__, *rev_id);
            }
          }
        }
        break;
      case BOARD_ID_SB:
        if ((ret = fby35_common_get_sb_rev(slot_id)) < 0) {
          return -1;
        }
        *rev_id = (uint8_t)ret;
        ret = 0;
        break;
      default:
        syslog(LOG_WARNING, "%s() Not supported board id %x", __func__, board_id);
        return -1;
    }
  }

  return ret;
}
