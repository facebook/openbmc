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
#include <time.h>
#include <syslog.h>
#include <errno.h>
#include <sys/resource.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/time.h>
#include <openbmc/kv.h>
#include "bic_bios_fwupdate.h"

/****************************/
/*      BIOS fw update      */                            
/****************************/
#define MAX_RETRY 3
#define SIZE_IANA_ID 3
#define IPMB_WRITE_COUNT_MAX 224
#define BIOS_ERASE_PKT_SIZE (64*1024)
#define BIOS_VERIFY_PKT_SIZE (32*1024)
#define BIOS_VER_REGION_SIZE (4*1024*1024)
#define BIOS_VER_STR "F09_"

// Update firmware for various components
static int
_update_fw(uint8_t slot_id, uint8_t target, uint32_t offset, uint16_t len, uint8_t *buf) {
  uint8_t tbuf[256] = {0x9c, 0x9c, 0x00}; // IANA ID
  uint8_t rbuf[16] = {0x00};
  uint8_t tlen = 0;
  uint8_t rlen = 0;
  int ret;
  int retries = 3;

  // Fill the component for which firmware is requested
  tbuf[3] = target;

  tbuf[4] = (offset) & 0xFF;
  tbuf[5] = (offset >> 8) & 0xFF;
  tbuf[6] = (offset >> 16) & 0xFF;
  tbuf[7] = (offset >> 24) & 0xFF;

  tbuf[8] = len & 0xFF;
  tbuf[9] = (len >> 8) & 0xFF;

  memcpy(&tbuf[10], buf, len);

  tlen = len + 10;

bic_send:
  ret = bic_ipmb_wrapper(slot_id, NETFN_OEM_1S_REQ, CMD_OEM_1S_UPDATE_FW, tbuf, tlen, rbuf, &rlen);
  if ((ret) && (retries--)) {
    sleep(1);
    printf("_update_fw: slot: %d, target %d, offset: %d, len: %d retrying..\
           \n",    slot_id, target, offset, len);
    goto bic_send;
  }

  return ret;
}

static int
_set_fw_update_ongoing(uint8_t slot_id, uint16_t tmout) {
  char key[64];
  char value[64] = {0};
  struct timespec ts;

  sprintf(key, "fru%u_fwupd", slot_id);

  clock_gettime(CLOCK_MONOTONIC, &ts);
  ts.tv_sec += tmout;
  sprintf(value, "%ld", ts.tv_sec);

  if (kv_set(key, value, 0, 0) < 0) {
     return -1;
  }

  return 0;
}

static int
check_bios_image(uint8_t slot_id, int fd, long size) {
  int offs, rcnt, end;
  uint8_t *buf;
  uint8_t ver_sig[] = { 0x46, 0x49, 0x44, 0x04, 0x78, 0x00 };

  if (size < BIOS_VER_REGION_SIZE)
    return -1;

  buf = (uint8_t *)malloc(BIOS_VER_REGION_SIZE);
  if (!buf) {
    return -1;
  }

  offs = size - BIOS_VER_REGION_SIZE;
  if (lseek(fd, offs, SEEK_SET) != (off_t)offs) {
    syslog(LOG_ERR, "%s: lseek to %d failed", __func__, offs);
    return -1;
  }

  offs = 0;
  while (offs < BIOS_VER_REGION_SIZE) {
    rcnt = read(fd, (buf + offs), BIOS_ERASE_PKT_SIZE);
    if (rcnt <= 0) {
      if (errno == EINTR) {
        continue;
      }
      syslog(LOG_ERR, "check_bios_image: unexpected rcnt: %d", rcnt);
      free(buf);
      return -1;
    }
    offs += rcnt;
  }

  end = BIOS_VER_REGION_SIZE - (sizeof(ver_sig) + strlen(BIOS_VER_STR));
  for (offs = 0; offs < end; offs++) {
    if (!memcmp(buf+offs, ver_sig, sizeof(ver_sig))) {
      if (memcmp(buf+offs+sizeof(ver_sig), BIOS_VER_STR, strlen(BIOS_VER_STR))){
        offs = end;
      }
      break;
    }
  }
  free(buf);

  if (offs >= end)
    return -1;

  if ((offs = lseek(fd, 0, SEEK_SET))) {
    syslog(LOG_ERR, "%s: fail to init file offset %d, errno=%d", __func__, offs, errno);
    return -1;
  }
  return 0;
}

// Read checksum of various components
int
bic_get_fw_cksum(uint8_t slot_id, uint8_t target, uint32_t offset, uint32_t len, uint8_t *ver) {
  uint8_t tbuf[12] = {0x9c, 0x9c, 0x00}; // IANA ID
  uint8_t rbuf[16] = {0x00};
  uint8_t rlen = 0;
  int ret;
  int retries = 3;

  // Fill the component for which firmware is requested
  tbuf[3] = target;

  // Fill the offset
  tbuf[4] = (offset) & 0xFF;
  tbuf[5] = (offset >> 8) & 0xFF;
  tbuf[6] = (offset >> 16) & 0xFF;
  tbuf[7] = (offset >> 24) & 0xFF;

  // Fill the length
  tbuf[8] = (len) & 0xFF;
  tbuf[9] = (len >> 8) & 0xFF;
  tbuf[10] = (len >> 16) & 0xFF;
  tbuf[11] = (len >> 24) & 0xFF;

bic_send:
  ret = bic_ipmb_wrapper(slot_id, NETFN_OEM_1S_REQ, CMD_OEM_1S_GET_FW_CKSUM, tbuf, 12, rbuf, &rlen);
  if ((ret || (rlen != 4+SIZE_IANA_ID)) && (retries--)) {  // checksum has to be 4 bytes
    sleep(1);
    syslog(LOG_ERR, "bic_get_fw_cksum: slot: %d, target %d, offset: %d, ret: %d, rlen: %d\n", slot_id, target, offset, ret, rlen);
    goto bic_send;
  }
  if (ret || (rlen != 4+SIZE_IANA_ID)) {
    return -1;
  }

#ifdef DEBUG
  printf("cksum returns: %x:%x:%x::%x:%x:%x:%x\n", rbuf[0], rbuf[1], rbuf[2], rbuf[3], rbuf[4], rbuf[5], rbuf[6]);
#endif

  //Ignore IANA ID
  memcpy(ver, &rbuf[SIZE_IANA_ID], rlen-SIZE_IANA_ID);

  return ret;
}


static int
verify_bios_image(uint8_t slot_id, int fd, long size) {
  int ret = -1;
  int rc, i;
  uint32_t offset;
  uint32_t tcksum, gcksum;
  volatile uint16_t count;
  uint8_t target, last_pkt = 0x00;
  uint8_t *tbuf = NULL;

  // Checksum calculation for BIOS image
  tbuf = malloc(BIOS_VERIFY_PKT_SIZE * sizeof(uint8_t));
  if (!tbuf) {
    return -1;
  }

  if ((offset = lseek(fd, 0, SEEK_SET))) {
    syslog(LOG_ERR, "%s: fail to init file offset %d, errno=%d", __func__, offset, errno);
    return -1;
  }
  while (1) {
    count = read(fd, tbuf, BIOS_VERIFY_PKT_SIZE);
    if (count <= 0) {
      if (offset >= size) {
        ret = 0;
      }
      break;
    }

    tcksum = 0;
    for (i = 0; i < count; i++) {
      tcksum += tbuf[i];
    }

    target = ((offset + count) >= size) ? (UPDATE_BIOS | last_pkt) : UPDATE_BIOS;

    // Get the checksum of binary image
    rc = bic_get_fw_cksum(slot_id, target, offset, count, (uint8_t*)&gcksum);
    if (rc) {
      printf("get checksum failed, offset:0x%x\n", offset);
      break;
    }

    // Compare both and see if they match or not
    if (gcksum != tcksum) {
      printf("checksum does not match, offset:0x%x, 0x%x:0x%x\n", offset, tcksum, gcksum);
      break;
    }

    offset += count;
  }

  free(tbuf);
  return ret;
}

int 
update_bic_bios(uint8_t slot_id, char *image, uint8_t force) {
  struct timeval start, end;
  int ret = -1, rc;
  uint32_t offset;
  volatile uint16_t count, read_count;
  uint8_t buf[256] = {0};
  uint8_t target;
  int fd;
  int i;
  
  printf("updating fw on slot %d:\n", slot_id);

  uint32_t dsize, last_offset;
  struct stat st;
  // Open the file exclusively for read
  fd = open(image, O_RDONLY, 0666);
  if (fd < 0) {
    printf("ERROR: invalid file path!\n");
    syslog(LOG_ERR, "bic_update_fw: open fails for path: %s\n", image);
    goto error_exit;
  }

  stat(image, &st);
  if (!force && check_bios_image(slot_id, fd, st.st_size)) {
    printf("invalid BIOS file!\n");
    goto error_exit;
  }
  syslog(LOG_CRIT, "Update BIOS: update bios firmware on slot %d\n", slot_id);
  dsize = st.st_size/100;

  // Write chunks of binary data in a loop
  offset = 0;
  last_offset = 0;
  i = 1;
  target = UPDATE_BIOS;
  gettimeofday(&start, NULL);
  while (1) {
    // For BIOS, send packets in blocks of 64K
    if ((offset+IPMB_WRITE_COUNT_MAX) > (i * BIOS_ERASE_PKT_SIZE)) {
      read_count = (i * BIOS_ERASE_PKT_SIZE) - offset;
      i++;
    } else {
      read_count = IPMB_WRITE_COUNT_MAX;
    }

    // Read from file
    count = read(fd, buf, read_count);
    if (count <= 0 || count > read_count) {
      break;
    }
    // Send data to Bridge-IC
    rc = _update_fw(slot_id, target, offset, count, buf);

    if (rc) {
      goto error_exit;
    }

    // Update counter
    offset += count;
    if ((last_offset + dsize) <= offset) {
      _set_fw_update_ongoing(slot_id, 60);
      printf("\rupdated bios: %d %%", offset/dsize);
      fflush(stdout);
      last_offset += dsize;
    }
  }

  gettimeofday(&end, NULL);
  printf("Elapsed time:  %d   sec.\n", (int)(end.tv_sec - start.tv_sec));

  _set_fw_update_ongoing(slot_id, 60 * 2);
  if (verify_bios_image(slot_id, fd, st.st_size)) {
    goto error_exit;
  }

  ret = 0;
error_exit:
  printf("\n");
  syslog(LOG_CRIT, "Update BIOS: updating bios firmware is exiting on slot %d\n", slot_id);

  if (fd > 0 ) {
    close(fd);
  }

  return ret;
}