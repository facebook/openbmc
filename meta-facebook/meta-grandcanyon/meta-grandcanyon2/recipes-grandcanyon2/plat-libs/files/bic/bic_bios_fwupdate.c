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
#include <time.h>
#include <syslog.h>
#include <errno.h>
#include <sys/resource.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/time.h>
#include "bic_fwupdate.h"
#include "bic_bios_fwupdate.h"

#define IPMB_READ_COUNT_MAX 224
#define IPMB_WRITE_COUNT_MAX 224

static int
check_bios_image(int fd, long size) {
  int offs = 0, read_count, end = 0;
  uint8_t *buf = NULL;
  // BIOS image signature
  uint8_t ver_signature[] = { 0x46, 0x49, 0x44, 0x04, 0x78, 0x00 };

  if (size < BIOS_VER_REGION_SIZE) {
    syslog(LOG_ERR, "%s: Failed to check BIOS image because file size is wrong", __func__);
    return -1;
  }

  buf = (uint8_t *)calloc(BIOS_VER_REGION_SIZE, sizeof(uint8_t));
  if (buf == NULL) {
    syslog(LOG_ERR, "%s: Failed to check BIOS image because create buffer failed.", __func__);
    return -1;
  }

  offs = size - BIOS_VER_REGION_SIZE;
  if (lseek(fd, offs, SEEK_SET) != (off_t)offs) {
    syslog(LOG_ERR, "%s: Failed to check BIOS image because failed to set the offset %d of the image, error: %s", __func__, offs, strerror(errno));
    free(buf);
    return -1;
  }

  offs = 0;
  while (offs < BIOS_VER_REGION_SIZE) {
    read_count = read(fd, (buf + offs), BIOS_ERASE_PKT_SIZE);
    if (read_count <= 0) {
      if (errno == EINTR) {
        continue;
      }
      syslog(LOG_ERR, "%s: Failed to check BIOS image because unexpected read count: %d", __func__, read_count);
      free(buf);
      return -1;
    }
    offs += read_count;
  }

  end = BIOS_VER_REGION_SIZE - sizeof(ver_signature);
  for (offs = 0; offs < end; offs++) {
    if (memcmp(buf + offs, ver_signature, sizeof(ver_signature)) == 0) {
      break;
    }
  }
  free(buf);

  if (offs >= end) {
    return -1;
  }

  if ((offs = lseek(fd, 0, SEEK_SET))) {
    syslog(LOG_ERR, "%s: Failed to check BIOS image because fail to init file offset %d, error: %s", __func__, offs, strerror(errno));
    return -1;
  }

  return 0;
}

static int
_update_fw(uint8_t slot_id, uint8_t target, uint32_t offset, uint16_t len, uint8_t *buf) {
  uint8_t tbuf[256] = {0x00}; // IANA ID
  uint8_t rbuf[16] = {0x00};
  uint8_t tlen = 0;
  uint8_t rlen = 0;
  int ret;
  int retries = MAX_RETRY;
  if (!buf) return -1;
  if (len > AST_BIC_IPMB_WRITE_COUNT_MAX) return -1;  
  
  // Fill the IANA ID
  memcpy(tbuf, (uint8_t *)&META_IANA_ID, SIZE_IANA_ID);
  tbuf[3] = target;

  tbuf[4] =  offset        & 0xFF;
  tbuf[5] = (offset >>  8) & 0xFF;
  tbuf[6] = (offset >> 16) & 0xFF;
  tbuf[7] = (offset >> 24) & 0xFF;

  tbuf[8] =  len & 0xFF;
  tbuf[9] = (len >> 8)  & 0xFF;

  memcpy(&tbuf[10], buf, len);

  tlen = len + 10;

bic_send:
  ret = bic_data_send(slot_id, NETFN_OEM_1S_REQ, CMD_OEM_1S_UPDATE_FW, tbuf, tlen, rbuf, &rlen, NONE_INTF);
  if ((ret) && (retries--)) {
    sleep(1);
    printf("_update_fw: slot: %d, target %d, offset: %u, len: %d retrying..\
           \n",    slot_id, target, offset, len);
    goto bic_send;
  }

  return ret;
}

int
update_bic_bios(uint8_t slot_id, uint8_t comp, char *image, uint8_t force) {
  struct timeval start, end;
  struct stat st;
  int ret = -1, rc = 0;
  int fd = 0;
  int i = 0;
  int remain = 0;
  volatile int count = 0, read_count = 0;
  uint8_t read_buf[MAX_READ_BUFFER_SIZE] = {0};
  uint8_t target = 0;
  uint32_t offset = 0;
  uint32_t dsize = 0, last_offset = 0;
  unsigned char initial_buf[1] = {0};

  if (image == NULL) {
    printf("ERROR: invalid file path!\n");
    syslog(LOG_ERR, "%s: Update firmware failed because parameter image is NULL\n", __func__);
    goto error_exit;
  }

  printf("updating fw on server: %d\n", slot_id);

  // Open the file exclusively for read
  fd = open(image, O_RDONLY, 0666);
  if (fd < 0) {
    printf("ERROR: invalid file path!\n");
    syslog(LOG_ERR, "%s: Update firmware failed because open fails for path: %s\n", __func__, image);
    goto error_exit;
  }

  stat(image, &st);
  if ((force == 0) && (check_bios_image(fd, st.st_size) != 0)) {
    printf("invalid BIOS file!\n");
    goto error_exit;
  }

  syslog(LOG_CRIT, "Update BIOS: update bios firmware on server\n");

  if (fd >= 0) {
    close(fd);
  }

  // align 64K
  if ((remain = (st.st_size % BIOS_ERASE_PKT_SIZE)) != 0) {
    remain = BIOS_ERASE_PKT_SIZE - remain;
  }

  // Set the remain bytes of image to 0xFF
  FILE *image_file = fopen(image, "ab");
  
  if (image_file == NULL) {
    syslog(LOG_ERR, "%s: Update firmware failed because fail to open image, error: %s", __func__, strerror(errno));
    goto error_exit;
  }
  
  initial_buf[0] = INITIAL_BYTE;
  while (remain > 0) {
    if ((fwrite(initial_buf, sizeof(unsigned char), 1, image_file)) != sizeof(unsigned char)) {
      syslog(LOG_ERR, "%s: Update firmware failed because fail to write remain byte of image, error: %s", __func__, strerror(errno));

      fclose(image_file);
      goto error_exit;
    }
    remain -= 1;
  }
  fclose(image_file);

  fd = open(image, O_RDONLY, 0666);
  if (fd < 0) {
    printf("ERROR: invalid file path!\n");
    syslog(LOG_ERR, "%s: Update firmware failed because open fails for path: %s\n",__func__, image);
    goto error_exit;
  }

  stat(image, &st);
  dsize = st.st_size/100;

  // Write chunks of binary data in a loop
  offset = 0;
  last_offset = 0;
  i = 1;
  target = UPDATE_BIOS;
  gettimeofday(&start, NULL);
  while (1) {
    memset(read_buf, INITIAL_BYTE, sizeof(read_buf));
    // For BIOS, send packets in blocks of 64K
    if ((offset + IPMB_WRITE_COUNT_MAX) > (i * BIOS_ERASE_PKT_SIZE)) {
      read_count = (i * BIOS_ERASE_PKT_SIZE) - offset;
      i++;
    } else {
      read_count = IPMB_WRITE_COUNT_MAX;
    }

    // Read from file
    count = read(fd, read_buf, read_count);
    if ((count <= 0) || (count > read_count)) {
      break;
    }
    // Send data to Bridge-IC
    rc = _update_fw(slot_id, target, offset, count, read_buf);

    if (rc < 0) {
      goto error_exit;
    }

    // Update counter
    offset += count;
    if ((last_offset + dsize) <= offset) {
      _set_fw_update_ongoing(FW_UPDATE_TIMEOUT_1M);
      printf("\rupdated bios: %u %%", offset/dsize);
      fflush(stdout);
      last_offset += dsize;
    }
  }
  printf("\n");

  gettimeofday(&end, NULL);
  printf("Elapsed time:  %d   sec.\n", (int)(end.tv_sec - start.tv_sec));

  ret = 0;
error_exit:
  printf("\n");
  syslog(LOG_CRIT, "%s: updating bios firmware is exiting on server\n", __func__);

  if (fd >= 0) {
    close(fd);
  }

  return ret;
}

// Read firmware for various components
static int
_dump_fw(uint32_t offset, uint8_t len, uint8_t *rbuf, uint8_t *rlen) {
  uint8_t tbuf[MAX_IPMB_REQ_LEN] = {0x15, 0xA0, 0x00};  // IANA ID
  int ret;
  int retries = 3;

  if (rbuf == NULL) {
    printf("Response buffer is missing\n");
    return -1;
  }
  if (rlen == NULL) {
    printf("Response length is missing\n");
    return -1;
  }

  // Fill the component for which firmware is requested
  tbuf[3] = DUMP_BIOS;
  memcpy(&tbuf[4], &offset, sizeof(uint32_t));
  tbuf[8] = len;

  do {
    ret = bic_ipmb_wrapper(NETFN_OEM_1S_REQ, CMD_OEM_1S_READ_FW_IMAGE, tbuf, 9, rbuf, rlen);
    if (!ret && (len == (*rlen - 3))) // 3 byte IANA ID
      return 0;

    sleep(1);
    printf("_dump_fw: offset: %u, rlen: %u retrying..\n", offset, *rlen);
  } while ((--retries));

  return -1;
}

int
bic_dump_bios_fw(char *path) {
  int ret = -1, fd = 0;
  uint32_t offset = 0x0, next_doffset = 0x0;
  uint32_t img_size = 0x4000000, dsize = 0;
  int count = 0, read_count;
  uint8_t buf[MAX_READ_BUFFER_SIZE] = {0}, rlen = 0;

  if (path == NULL) {
    printf("Please provide the file path\n");
    return -1;
  }
  fd = open(path, O_WRONLY | O_CREAT | O_TRUNC, 0644);
  if (fd < 0) {
    printf("ERROR: invalid file path!\n");
    return -1;
  }

  // Write chunks of binary data in a loop
  dsize = img_size / 100;
  offset = 0;
  next_doffset = offset + dsize;
  while (1) {
    read_count = ((offset + IPMB_READ_COUNT_MAX) <= img_size) ? IPMB_READ_COUNT_MAX : (img_size - offset);

    // read image from Bridge-IC
    ret = _dump_fw(offset, read_count, buf, &rlen);
    if (ret != 0) {
      printf("Failed to dump offset 0x%x\n", offset);
      goto error_exit;
    }

    // Write to file
    count = write(fd, &buf[3], rlen);
    if (count <= 0) {
      ret = -1;
      goto error_exit;
    }

    // Update the counter
    offset += count;
    if (offset >= next_doffset) {
      _set_fw_update_ongoing(FW_UPDATE_TIMEOUT_1M);
      printf("\rdumped bios: %u %%", offset/dsize);
      fflush(stdout);
      next_doffset += dsize;
    }

    if (offset >= img_size) {
      break;
    }
  }
  _set_fw_update_ongoing(FW_UPDATE_TIMEOUT_2M);
  ret = 0;

error_exit:
  printf("\n");
  if (fd >= 0 ) {
    close(fd);
  }

  return ret;
}