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
#include <syslog.h>
#include <errno.h>
#include "bic_m2_fwupdate.h"
#include "bic_ipmi.h"
#include "bic_xfer.h"

#define MAX_RETRY 5

#define BRCM_UPDATE_STATUS 128
#define BRCM_WRITE_CMD 130
#define BRCM_READ_CMD 131

#define GPV3_E1S_BUS 9

int
bic_mux_select(uint8_t slot_id, uint8_t bus, uint8_t dev_id, uint8_t intf) {
  uint8_t tbuf[5] = {0x00};
  uint8_t tlen = 5;
  uint8_t rbuf[16] = {0x00};
  uint8_t rlen = 0;
  uint8_t chn = get_gpv3_channel_number(dev_id);
  tbuf[0] = (bus << 1) + 1;
  if (bus == GPV3_E1S_BUS) {
    tbuf[1] = 0xE2;
  } else {
    tbuf[1] = 0x02;
  }
  tbuf[2] = 0x00;
  tbuf[3] = 0x00;
  tbuf[4] = chn;
  if (dev_id > FW_2OU_M2_DEV0) { // 0-based
    dev_id -= FW_2OU_M2_DEV0;
  } else {
    dev_id -= DEV_ID0_2OU;
  }
  printf("* Mux selecting...bus %d, chn: %d, dev_id: %d\n", bus, chn, dev_id);
  return bic_data_send(slot_id, NETFN_APP_REQ, CMD_APP_MASTER_WRITE_READ, tbuf, tlen, rbuf, &rlen, intf);
}

int
bic_m2_master_write_read(uint8_t slot_id, uint8_t bus, uint8_t addr, uint8_t *wbuf, uint8_t wcnt, uint8_t *rbuf, uint8_t rcnt) {
  uint8_t tbuf[256];
  uint8_t tlen = 3, rlen = 0;
  int ret;
  int retry = MAX_RETRY;

  tbuf[0] = (bus << 1) + 1;
  tbuf[1] = addr;
  tbuf[2] = rcnt;
  if (wcnt) {
    memcpy(&tbuf[3], wbuf, wcnt);
    tlen += wcnt;
  }

  do {
    ret = bic_data_send(slot_id, NETFN_APP_REQ, CMD_APP_MASTER_WRITE_READ, tbuf, tlen, rbuf, &rlen, REXP_BIC_INTF);
    if (ret < 0 ) msleep(100);
    else break;
  } while ( retry-- >= 0 );

  return ret;
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
  if ( bic_data_send(slot_id, NETFN_OEM_STORAGE_REQ, DEV_UPDATE, tbuf, tlen, rbuf, &rlen, intf) < 0 ) {
    printf("* Failed to erase Device CFM0 sector...\n");
    return BIC_STATUS_FAILURE;
  } else if ( rbuf[0] == CC_CAN_NOT_RESPOND && rlen > 0 ) {
    printf("Command response could not be provided\n");
    return BIC_STATUS_FAILURE;
  }

  int addr = CFM0_START;
  int tmp = addr;
  int i = 0;
  ssize_t read_bytes;
  const uint8_t bytes_per_read = DEV_UPDATE_BATCH_SIZE;
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

    if ( bic_data_send(slot_id, NETFN_OEM_STORAGE_REQ, DEV_UPDATE, tbuf, (DEV_UPDATE_BATCH_SIZE + DEV_UPDATE_IPMI_HEAD_SIZE), rbuf, &rlen, intf) < 0 ) {
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

int
_update_brcm_fw(uint8_t slot_id, uint8_t bus, uint8_t target __attribute__((unused)), uint32_t offset, uint16_t count, uint8_t * buf) {
  uint8_t wbuf[256], rbuf[256];
  int ret = 0;
  int rlen = 0; // write
  wbuf[0] = BRCM_WRITE_CMD;  // offset 130
  wbuf[1] = count+5;
  offset += 0x00400000;
  memcpy(&wbuf[2],&offset, sizeof(uint32_t));
  wbuf[6] = 5;
  memcpy(&wbuf[7],buf, sizeof(uint8_t)*count);
  ret = bic_m2_master_write_read(slot_id, bus, 0xd4, wbuf, count+7, rbuf, rlen);
  if (ret != 0) {
    syslog(LOG_DEBUG,"%s(): bic_m2_master_write_read offset=%d  failed", __func__,wbuf[0]);
  }

  return ret;
}

int
_check_brcm_fw_status(uint8_t slot_id, uint8_t bus) {
  uint8_t wbuf[256], rbuf[256];
  int ret = 0;
  int rlen = 4; // read
  wbuf[0] = BRCM_READ_CMD;  // offset 131
  wbuf[1] = 9;
  wbuf[2] = 0x20;
  wbuf[3] = 0x04;
  wbuf[4] = 0x07;
  wbuf[5] = 0x40;
  wbuf[6] = 4;//count;
  ret = bic_m2_master_write_read(slot_id, bus, 0xd4, wbuf, 7, rbuf, rlen);
  if (ret != 0) {
    syslog(LOG_DEBUG,"%s(): bic_m2_master_write_read offset=%d  failed", __func__,wbuf[0]);
  } else {
    syslog(LOG_DEBUG,"%s(): rbuf[0]=0x%X rbuf[1]=0x%X rbuf[2]=0x%X rbuf[3]=0x%X", __func__,rbuf[0],rbuf[1],rbuf[2],rbuf[3]);
    uint32_t result;
    memcpy(&result,&rbuf[0], sizeof(uint32_t));
    syslog(LOG_DEBUG,"%s(): result=0x%X", __func__,result);
  }
  return ret;
}

/* future for vk recovery update
int
bic_disable_brcm_parity_init(uint8_t slot_id, uint8_t bus, uint8_t comp, uint8_t intf) {
  uint8_t wbuf[256], rbuf[256];
  int ret = 0;
  int rlen = 0; // write

  ret = bic_enable_ssd_sensor_monitor(slot_id, false, intf);
  sleep(2);

  // MUX select
  if ( bic_mux_select(slot_id, bus, comp, intf) < 0 ) {
    printf("* Failed to select MUX\n");
    return BIC_STATUS_FAILURE;
  }

  wbuf[0] = BRCM_WRITE_CMD;  // offset 130
  wbuf[1] = 0x08;
  wbuf[2] = 0x78;
  wbuf[3] = 0x0c;
  wbuf[4] = 0x07;
  wbuf[5] = 0x40;
  wbuf[6] = 0x04;
  wbuf[7] = 0x00;
  wbuf[8] = 0x40;
  wbuf[9] = 0x00;
  wbuf[10] = 0x00;
  ret = bic_master_write_read(slot_id, bus, 0xd4, wbuf, 11, rbuf, rlen);
  if (ret != 0) {
    syslog(LOG_DEBUG,"%s(): bic_master_write_read offset=%d  failed", __func__,wbuf[0]);
  }

  ret = bic_enable_ssd_sensor_monitor(slot_id, true, intf);

  sleep(2);
  return ret;
} */


static int
bic_vk_m2_update(uint8_t slot_id, uint8_t bus, uint8_t comp, int fd, int file_size, uint8_t intf) {

#define DEV_I2C_WRITE_COUNT_MAX 224

  int ret = -1;
  ssize_t count;
  uint32_t offset;
  uint8_t buf[256] = {0};

  printf("updating fw on slot %d device %d:\n", slot_id,comp-FW_2OU_M2_DEV0);

  uint32_t dsize, last_offset;

  if (file_size/100 < 100)
    dsize = file_size;
  else
    dsize = file_size/100;

  offset = 0;
  last_offset = 0;
  uint8_t wbuf[256], rbuf[256];
  int rlen = 0;

  if ( bic_mux_select(slot_id, bus, comp, intf) < 0 ) {
    printf("* Failed to select MUX\n");
    return BIC_STATUS_FAILURE;
  }

  wbuf[0] = BRCM_UPDATE_STATUS;  // offset 128 process bit == 0?
  rlen = 1;
  ret = bic_m2_master_write_read(slot_id, bus, 0xd4, wbuf, 1, rbuf, rlen);
  if (ret != 0) {
    syslog(LOG_DEBUG,"%s(): process bit == 0? offset=%d read length=%d failed", __func__,wbuf[0],rlen);
    return BIC_STATUS_FAILURE;
  }
  if (rbuf[0] & 2) {
    syslog(LOG_DEBUG,"%s(): process bit == 1 offset=%d rbuf[0]=%d", __func__,wbuf[0],rbuf[0]);
    return BIC_STATUS_FAILURE;
  } else {
    syslog(LOG_DEBUG,"%s(): process bit == 0 offset=%d rbuf[0]=%d", __func__,wbuf[0],rbuf[0]);
  }
  wbuf[0] = BRCM_UPDATE_STATUS;  // offset 128 set download init
  wbuf[1] = rbuf[0] | 0x80;
  rlen = 0; // write
  ret = bic_m2_master_write_read(slot_id, bus, 0xd4, wbuf, 2, rbuf, rlen);
  if (ret != 0) {
    syslog(LOG_DEBUG,"%s(): set download init offset=%d read length=%d failed", __func__,wbuf[0],rlen);
    return BIC_STATUS_FAILURE;
  } else {
    syslog(LOG_DEBUG,"%s(): set download init offset=%d", __func__,wbuf[0]);
  }

  sleep(2);
  wbuf[0] = BRCM_UPDATE_STATUS;  // offset 128 reday bit == 1?
  rlen = 1;
  ret = bic_m2_master_write_read(slot_id, bus, 0xd4, wbuf, 1, rbuf, rlen);
  if (ret != 0) {
    syslog(LOG_DEBUG,"%s(): reday bit == 1? offset=%d read length=%d failed", __func__,wbuf[0],rlen);
    return BIC_STATUS_FAILURE;
  }

  if (!(rbuf[0] & 0x40)) {
    syslog(LOG_DEBUG,"%s(): reday bit == 0", __func__);
    return BIC_STATUS_FAILURE;
  } else {
    syslog(LOG_DEBUG,"%s(): reday bit == 1 offset=%d rbuf[0]=%d", __func__,wbuf[0],rbuf[0]);
  }

  wbuf[0] = BRCM_WRITE_CMD;  // offset 130
  wbuf[1] = 9;
  uint32_t addr = 0x00400001;
  memcpy(&wbuf[2],&addr, sizeof(uint32_t));
  wbuf[6] = 4;
  wbuf[7] = 0x00;
  wbuf[8] = 0x00;
  wbuf[9] = 0x00;
  wbuf[10] = 0x00;
  rlen = 0; // write
  ret = bic_m2_master_write_read(slot_id, bus, 0xd4, wbuf, 11, rbuf, rlen);
  if (ret != 0) {
    syslog(LOG_DEBUG,"%s(): bic_m2_master_write_read offset=%d read length=%d failed", __func__,wbuf[0],rlen);
    return BIC_STATUS_FAILURE;
  }

  // Write chunks of binary data in a loop
  syslog(LOG_DEBUG,"%s(): Start a loop", __func__);
  while (1) {

    // Read from file
    count = read(fd, buf, DEV_I2C_WRITE_COUNT_MAX);
    if (count <= 0 || count > DEV_I2C_WRITE_COUNT_MAX) {
      break;
    }

    ret = _update_brcm_fw(slot_id, bus, comp, offset, count, buf);

    if (ret) {
      return BIC_STATUS_FAILURE;
    }

    msleep(1); // wait

    wbuf[0] = BRCM_UPDATE_STATUS;  // offset 128 process bit == 1?
    rlen = 1;
    ret = bic_m2_master_write_read(slot_id, bus, 0xd4, wbuf, 1, rbuf, rlen);
    if (ret != 0) {
      syslog(LOG_DEBUG,"%s(): process bit == 1? offset=%d read length=%d failed", __func__,wbuf[0],rlen);
      return BIC_STATUS_FAILURE;
    }
    if (!(rbuf[0] & 2)) {
      syslog(LOG_DEBUG,"%s(): process bit == 0", __func__);
      return BIC_STATUS_FAILURE;
    }
    // Update counter
    offset += count;
    if ((last_offset + dsize) <= offset) {
      _set_fw_update_ongoing(slot_id, 60);
      if (file_size/100 < 100)
        printf("\rupdated brcm vk: %u %%", offset/dsize*100);
      else
        printf("\rupdated brcm vk: %u %%", offset/dsize);
      fflush(stdout);
      last_offset += dsize;
    }
  }
  syslog(LOG_DEBUG,"%s(): End a loop", __func__);

  wbuf[0] = BRCM_UPDATE_STATUS;  // offset 128 set activation
  wbuf[1] = rbuf[0] | 0x8;
  rlen = 0; // write
  ret = bic_m2_master_write_read(slot_id, bus, 0xd4, wbuf, 2, rbuf, rlen);
  if (ret != 0) {
    syslog(LOG_DEBUG,"%s(): set activation offset=%d read length=%d failed", __func__,wbuf[0],rlen);
    return BIC_STATUS_FAILURE;
  }

  wbuf[0] = BRCM_UPDATE_STATUS;  // offset 128 readiness bit == 1?
  rlen = 1;
  ret = bic_m2_master_write_read(slot_id, bus, 0xd4, wbuf, 1, rbuf, rlen);
  if (ret != 0) {
    syslog(LOG_DEBUG,"%s(): readiness bit == 1? offset=%d read length=%d failed", __func__,wbuf[0],rlen);
    return BIC_STATUS_FAILURE;
  }

  if (!(rbuf[0] & 4)) {
    syslog(LOG_DEBUG,"%s(): readiness bit == 0", __func__);
    return BIC_STATUS_FAILURE;
  } else {
    syslog(LOG_DEBUG,"%s(): readiness bit == 1 offset=%d rbuf[0]=%d", __func__,wbuf[0],rbuf[0]);
  }

  for (int i=0;i<5;i++) {
    _check_brcm_fw_status(slot_id,bus);
    sleep(1);
  }

  return BIC_STATUS_SUCCESS;
}


int update_bic_m2_fw(uint8_t slot_id, uint8_t comp, char *image, uint8_t intf, uint8_t force __attribute__((unused)), uint8_t type) {
  int fd = 0;
  int ret = 0;
  size_t file_size = 0;
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

  if (type == DEV_TYPE_SPH_ACC) {
    ret = bic_sph_m2_update(slot_id, bus, comp, fd, file_size, intf);
  } else { // VK update
    ret = bic_vk_m2_update(slot_id, bus, comp, fd, file_size, intf);
  }
error_exit:
  if ( fd > 0 ) close(fd);
  return ret;
}
