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

#define IPMB_WRITE_COUNT_MAX 224

static int
check_bios_image(int fd, long size) {
  int offs = 0, read_count = 0, end = 0;
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
_update_fw(uint8_t target, uint32_t offset, uint16_t len, uint8_t *read_buf) {
  uint8_t rbuf[MAX_IPMB_BUFFER] = {0x00};
  uint8_t rlen = 0;
  uint16_t tmp_len = 0;
  uint32_t tmp_offset = 0;
  int ret = 0, i = 0;
  int retries = MAX_RETRY;
  update_request update_req;

  if (read_buf == NULL) {
    printf("ERROR: invalid file path!\n");
    syslog(LOG_ERR, "%s: Update firmware failed because parameter read_buf is NULL\n", __func__);
    return -1;
  }

  if (len > sizeof(update_req.read_buffer)) {
    syslog(LOG_ERR, "%s: Update firmware failed because length is wrong\n", __func__);
    return -1;
  }

  memset(&update_req, 0, sizeof(update_req));
  // Fill the IANA ID
  memcpy(&update_req, (uint8_t *)&IANA_ID, SIZE_IANA_ID);
  // Fill the component for which firmware is requested
  update_req.component = target;

  tmp_len = len;
  tmp_offset = offset;
  // Fill offset
  for (i = 0; i < sizeof(update_req.offset); i++) {
    update_req.offset[i] = tmp_offset & 0xFF;
    tmp_offset = tmp_offset >> SHIFT_BYTE;
  }
  // Fill length
  for (i = 0; i < sizeof(update_req.length); i++) {
    update_req.length[i] = tmp_len & 0xFF;
    tmp_len = tmp_len >> SHIFT_BYTE;
  }
  // Fill read buffer
  memcpy(&update_req.read_buffer, read_buf, len);

  do {
    ret = bic_ipmb_wrapper(NETFN_OEM_1S_REQ, CMD_OEM_1S_UPDATE_FW, (uint8_t *)&update_req, sizeof(update_req), rbuf, &rlen);
    if (ret < 0) {
      sleep(1);
      printf("%s: target %d, offset: %d, len: %d retrying..\n", __func__, target, offset, len);
      retries--;
    }
  } while ((ret < 0) && (retries > 0));

  return ret;
}

int
update_bic_bios(uint8_t comp, char *image, uint8_t force) {
  struct timeval start, end;
  struct stat st;
  int ret = -1, rc = 0;
  int fd = 0;
  int i = 0;
  int remain = 0;
  volatile uint16_t count = 0, read_count = 0;
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

  printf("updating fw on server:\n");

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
    rc = _update_fw(target, offset, count, read_buf);

    if (rc < 0) {
      goto error_exit;
    }

    // Update counter
    offset += count;
    if ((last_offset + dsize) <= offset) {
      _set_fw_update_ongoing(FW_UPDATE_TIMEOUT_1M);
      printf("\rupdated bios: %d %%", offset/dsize);
      fflush(stdout);
      last_offset += dsize;
    }
  }
  printf("\n");

  _set_fw_update_ongoing(FW_UPDATE_TIMEOUT_2M);
  if (verify_bios_image(fd, st.st_size) < 0 ) {
    goto error_exit;
  }

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
