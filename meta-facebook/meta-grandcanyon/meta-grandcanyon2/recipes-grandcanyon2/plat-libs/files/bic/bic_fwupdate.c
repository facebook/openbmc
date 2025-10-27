
/*
 *
 * Copyright 2020-present Facebook. All Rights Reserved.
 *
 * This file contains code to support IPMI2.0 Specification available @
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
#include <sys/resource.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/mman.h>
#include "bic_fwupdate.h"
#include "bic_bios_fwupdate.h"
#include "bic_vr_fwupdate.h"
//#define DEBUG

/****************************/
/*      BIC fw update       */
/****************************/
#define I2C_UPDATE_BIC 0x01

#define BIC_CMD_DOWNLOAD 0x21
#define BIC_CMD_DOWNLOAD_SIZE 11

#define BIC_CMD_RUN 0x22
#define BIC_CMD_RUN_SIZE 7

#define BIC_CMD_STS 0x23
#define BIC_CMD_STS_SIZE 3

#define BIC_CMD_DATA 0x24
#define BIC_CMD_DATA_SIZE 0xFF

#define BIC_FLASH_START 0x8000

#define BIC_UPDATE_BLOCK_SIZE 20
#define BIC_BOOTLOADER_COMP_BIT 7

#define IPMB_MAX_SEND 224

#define I2C_CLK_CTRL_REG 0x04
#define PAGE_SIZE        0x1000

#define START_BIC_UPDATE_DATA_LEN 11
#define BIC_ACK_VALIDATE_LEN 2
#define BIC_UPDATE_STAT_VALIDATE_LEN 5
#define BIC_UPDATE_COMPLETE_LEN 7
#define BIC_IMG_DATA_HEADER_LEN 3
#define BIC_IMG_DATA_LEN 256
#define BIC_VALIDATE_READ_LEN 2

#define GET_BIC_UPDATE_STAT 0xCC

#define DEVMEM_READ_CMD  "/sbin/devmem 0x%08x | cut -c 3-" // skip "0x"
#define DEVMEM_WRITE_CMD "/sbin/devmem 0x%08x w 0x%08x"


// I2C frequncy
enum {
  I2C_100K = 0x0,
  I2C_1M
};

#ifdef DEBUG
static void print_data(const char *name, uint8_t netfn, uint8_t cmd, uint8_t *buf, uint8_t len) {
  int i = 0;

  printf("[%s][%d]send: 0x%x 0x%x ", name, len, netfn, cmd);
  for ( i = 0; i < len; i++) {
    printf("0x%x ", buf[i]);
  }

  printf("\n");
}
#endif



static char*
get_component_name(uint8_t comp) {
  switch (comp) {
    case FW_BIC:
      return "Bridge-IC";
    case FW_BIC_BOOTLOADER:
      return "Bridge-IC Bootloader";
    case FW_VR:
      return "VR";
    case FW_BIOS:
      return "BIOS";
    default:
      return "Unknown";
  }
}
static bool
end_with (char* str, uint8_t str_len, char* pattern, uint8_t pattern_len) {
  if ((str == NULL) || (pattern == NULL)) {
    return false;
  }
  return (strncmp(str + (str_len - pattern_len), pattern, pattern_len) == 0);
}

static int
recovery_bic_runtime_fw(uint8_t slot_id, uint8_t comp, uint8_t intf, char *path, uint8_t force) {
  int ret = -1;
  syslog(LOG_INFO, "BIC recovery bypass in current stage, ret: %d\n", ret);
  return ret;
}

static int
update_bic_cpld_altera(uint8_t slot_id, char *path, uint8_t intf, uint8_t force) {
  int ret = -1;
  syslog(LOG_INFO, "BIC CPLD Update bypass in current stage, ret: %d\n", ret);
  return ret;
}


static int
is_valid_intf(uint8_t intf) {
  int ret = BIC_STATUS_FAILURE;
  switch(intf) {
    case NONE_INTF:
      ret = BIC_STATUS_SUCCESS;
      break;
  }

  return ret;
}

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
  memcpy(tbuf, (uint8_t *)&META_IANA_ID, SIZE_IANA_ID);
  
  // Fill the component for which firmware is requested
  // Target is for update component
  tbuf[3] = target;
  memcpy(&tbuf[4], &offset, sizeof(offset));
  tbuf[8] = len & 0xFF;
  tbuf[9] = (len >> 8) & 0xFF;
  memcpy(&tbuf[10], buf, len);

  tlen = len + 10;
  for (retry = 0; retry < 3; retry++) {
#ifdef DEBUG
    print_data(__func__, NETFN_OEM_1S_REQ, CMD_OEM_1S_UPDATE_FW, tbuf, tlen);
#endif
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

static int
update_bic(uint8_t slot_id, int fd, size_t file_size, uint8_t intf) {
  struct timeval start, end;
  int update_rc = -1, cmd_rc = 0;
  uint32_t dsize, last_offset;
  uint32_t offset, boundary;
  volatile uint16_t read_count;
  uint8_t buf[256] = {0};
  uint8_t target;
  uint8_t type = TYPE_1OU_VERNAL_FALLS_WITH_AST;
  ssize_t count;


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
      _set_fw_update_ongoing(60);
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

  if ( update_rc == 0 ) {
    update_rc = cmd_rc;
  }
  return update_rc;
}

static int
update_bic_runtime_fw(uint8_t slot_id, uint8_t comp __attribute__((unused)), uint8_t intf, char *path, uint8_t force __attribute__((unused))) {
  int ret = -1;
  int fd = -1;
  size_t file_size = 0;

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
  if (ret == 0) {
    syslog(LOG_INFO, "BIC Update Response Success\n");
  }
  else
  {
    syslog(LOG_INFO, "BIC Update Response Failed\n");
  }


exit:

  if ( fd >= 0 ) {
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
  intf = NONE_INTF;


  // stop fscd in class 2 system when update firmware through SB BIC to avoid IPMB busy
  // fbgc skip stop fan service

  //run cmd
  switch (comp) {
    case FW_BIC:
      ret = update_bic_runtime_fw(slot_id, UPDATE_BIC, intf, path, force);
      break;
    case FW_BIC_RECOVERY:
      ret = recovery_bic_runtime_fw(slot_id, comp, intf, path, force);
      break;
    case FW_BS_FPGA:
      ret = update_bic_cpld_altera(slot_id, path, intf, force);
      break;
    case FW_BIOS:
      if (loc != NULL) {
        ret = update_bic_bios(slot_id, comp, path, FORCE_UPDATE_SET);
      } else {
        ret = update_bic_usb_bios(comp, path);
      }
      break;
    case FW_VR:
      ret = update_bic_vr(path, force);
      break;
    default:
      syslog(LOG_WARNING, "%s(): component %x not supported", __func__, comp);
      return -1;
  }

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