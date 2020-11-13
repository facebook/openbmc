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
#include <sys/resource.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/time.h>
#include <time.h>
#include "bic_m2_fwupdate.h"
#include <openbmc/kv.h>

static int
bic_mux_select(uint8_t slot_id, uint8_t bus, uint8_t dev_id, uint8_t intf) {
  uint8_t tbuf[5] = {0x00};
  uint8_t tlen = 5;
  uint8_t rbuf[16] = {0x00};
  uint8_t rlen = 0;
  uint8_t chn = ((dev_id - FW_2OU_M2_DEV0 + 1) % 2 > 0)?0x0:0x1;
  tbuf[0] = (bus << 1) + 1;
  tbuf[1] = 0x02;
  tbuf[2] = 0x00;
  tbuf[3] = 0x00;
  tbuf[4] = chn;
  printf("* Mux selecting...bus %d, chn: %d, dev_id: %d\n", bus, chn, (dev_id - FW_2OU_M2_DEV0 + 1));
  return bic_ipmb_send(slot_id, NETFN_APP_REQ, CMD_APP_MASTER_WRITE_READ, tbuf, tlen, rbuf, &rlen, intf);
}

static int
reverse_bits(int raw_val) {
  int c = 0, reverse_val = 0;

  for (c = 7; c > 0; c--) {
    if (raw_val & 1) {
      reverse_val++;
    }
    reverse_val = reverse_val << 1;
    raw_val = raw_val >> 1;
  }
  if(raw_val & 1) {
    reverse_val++;
  }
  return reverse_val;
}

static int
bic_sph_m2_update(uint8_t slot_id, uint8_t bus, uint8_t comp, int fd, int file_size, uint8_t intf) {
#define ERASE_DEV_FW 0x00
#define PROGRAM_DEV_FW 0x01
#define DEV_UPDATE_BATCH_SIZE 192
#define DEV_UPDATE_IPMI_HEAD_SIZE 6
#define CFM0_START 0x00012800
#define CFM0_END 0x00022fff
  uint8_t buffer[DEV_UPDATE_BATCH_SIZE] = {0};
  uint8_t rbuf[DEV_UPDATE_BATCH_SIZE] = {0};
  uint8_t tbuf[DEV_UPDATE_BATCH_SIZE+DEV_UPDATE_IPMI_HEAD_SIZE+2] = {0};
  uint8_t tlen = 0;
  uint8_t rlen = 0;
 
  if ( bic_mux_select(slot_id, bus, comp, intf) < 0 ) {
    printf("* Failed to select MUX\n");
    return BIC_STATUS_FAILURE;
  }

  // erase cfg
  printf("* Erasing Device CFM0 sector...(%d.%d)\n", bus, ERASE_DEV_FW);
  tbuf[0] = bus;
  tbuf[1] = ERASE_DEV_FW;
  tlen = 2;
  if ( bic_ipmb_send(slot_id, NETFN_OEM_STORAGE_REQ, DEV_UPDATE, tbuf, tlen, rbuf, &rlen, intf) < 0 ) {
    printf("* Failed to erase Device CFM0 sector...\n");
    return BIC_STATUS_FAILURE;
  } else if ( rbuf[0] == CC_CAN_NOT_RESPOND && rlen > 0 ) {
    printf("Command response could not be provided\n");
    return BIC_STATUS_FAILURE;
  }

  int addr = CFM0_START;
  int tmp = addr;
  int i = 0;
  const uint8_t bytes_per_read = DEV_UPDATE_BATCH_SIZE;
  uint16_t read_bytes = 0;
  uint32_t offset = 0;
  uint32_t last_offset = 0;
  uint32_t dsize = file_size / 20;
  tbuf[0] = bus;
  tbuf[1] = PROGRAM_DEV_FW;
  printf("* Starting program...(%d.%d)\n", bus, PROGRAM_DEV_FW);
  //program
  while (1) {
    tmp = addr;
    read_bytes = read(fd, buffer, bytes_per_read);
    if ( read_bytes <= 0 ) {
      //no more bytes can be read
      break;
    }

    for(i = DEV_UPDATE_IPMI_HEAD_SIZE - 1; i > 1; i--) { //offset is 0-based
      tbuf[i] = (tmp & 0xFF);
      tmp /= 0x100;
    }

    for(i = 0; i < DEV_UPDATE_BATCH_SIZE; i++) {
      tbuf[i + DEV_UPDATE_IPMI_HEAD_SIZE] = reverse_bits(buffer[i]);
    }

    if ( bic_ipmb_send(slot_id, NETFN_OEM_STORAGE_REQ, DEV_UPDATE, tbuf, (DEV_UPDATE_BATCH_SIZE + DEV_UPDATE_IPMI_HEAD_SIZE), rbuf, &rlen, intf) < 0 ) {
      printf("Failed to send SPH image data!\n");
      return BIC_STATUS_FAILURE;
    } else if ( rbuf[0] == CC_CAN_NOT_RESPOND && rlen > 0 ) {
      printf("Command response could not be provided\n");
      return BIC_STATUS_FAILURE;
    }

    addr += DEV_UPDATE_BATCH_SIZE / 4;
    offset += read_bytes;
    if ((last_offset + dsize) <= offset) {
      printf("updated M2 dev: %d %%\n", (offset/dsize)*5);
      fflush(stdout);
      _set_fw_update_ongoing(slot_id, 60);
      last_offset += dsize;
    }
 
  }

  printf("* Please do the power-cycle to take it affect!\n"); 
  return BIC_STATUS_SUCCESS;
}

int update_bic_m2_fw(uint8_t slot_id, uint8_t comp, char *image, uint8_t intf, uint8_t force) {
  int fd = 0;
  int ret = 0;
  int file_size = 0;
  uint8_t bus = get_gpv3_bus_number(comp);

  //open fd and get size
  fd = open_and_get_size(image, &file_size);
  if (fd < 0) {
    syslog(LOG_WARNING, "%s() cannot open the file: %s, fd=%d", __func__, image, fd);
    return BIC_STATUS_FAILURE;
  }
  printf("file size = %d bytes, slot = %d, comp = 0x%x\n", file_size, slot_id, comp);

  if ( lseek(fd, 0, SEEK_SET) != 0 ) {
    syslog(LOG_WARNING, "%s() Cannot reinit the fd to the beginning. errstr=%s", __func__, strerror(errno));
    goto error_exit;
  }

  ret = bic_sph_m2_update(slot_id, bus, comp, fd, file_size, intf);
error_exit:
  if ( fd > 0 ) close(fd);
  return ret;
}
