
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
#include <time.h>
#include <sys/time.h>

// #include <openbmc/obmc-i2c.h>
#include <openbmc/kv.h>
#include "bic_fwupdate.h"
#include "bic_ipmi.h"
#include "bic_xfer.h"
#include "bic.h"
#include "bic_bios_fwupdate.h"


int
_set_fw_update_ongoing(uint16_t tmout) {
  char key[64];
  char value[64] = {0};
  struct timespec ts;

  sprintf(key, "frumb_fwupd");

  clock_gettime(CLOCK_MONOTONIC, &ts);
  ts.tv_sec += tmout;
  sprintf(value, "%ld", ts.tv_sec);

  if (kv_set(key, value, 0, 0) < 0) {
     return -1;
  }

  return 0;
}

int
open_and_get_size(char *path, int *file_size) {
  struct stat finfo;
  int fd;

  fd = open(path, O_RDONLY, 0666);
  if ( fd < 0 ) {
    return fd;
  }

  fstat(fd, &finfo);
  *file_size = finfo.st_size;

  return fd;
}


static int
update_bic_runtime_fw(uint8_t comp, char *path, uint8_t force) {
  #define MAX_CMD_LEN 100
  int ret = -1;
  int fd = -1;
  int file_size;
  struct timeval start, end;
  uint32_t dsize, last_offset;
  uint32_t offset, boundary;
  volatile uint16_t read_count;
  uint8_t buf[256] = {0};
  uint8_t target;
  ssize_t count;

  fd = open_and_get_size(path, &file_size);
  if ( fd < 0 ) {
    syslog(LOG_WARNING, "%s() cannot open the file: %s, fd=%d\n", __func__, path, fd);
    return -1;
  }

  printf("updating fw on MB, file size = %d bytes \n", file_size );

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
    ret = bic_update_firmware(target, offset, count, buf);
    if (ret) {
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

error_exit:
  printf("\n");
  if ( fd >= 0 ) {
    close(fd);
  }

  return ret;
}

int
bic_update_fw(uint8_t slot_id, uint8_t comp, char *path, int fd, uint8_t force) {
  int ret = BIC_STATUS_SUCCESS;

  if (path == NULL && comp != FW_BIOS) {
    syslog(LOG_ERR, "%s(): Update aborted due to NULL parameter: *path", __func__);
    return -1;
  }

  //run cmd
  switch (comp) {
    case FW_MB_BIC:
      ret = update_bic_runtime_fw(UPDATE_BIC, path, force);
      break;
    case FW_BIOS:
      if (fd < 0) {
        fd = open(path, O_RDONLY);
        if (fd < 0) {
          syslog(LOG_ERR, "failed to open %s for read", path);
          return -1;
        }
      }
      ret = update_bic_usb_bios(FRU_MB, comp, fd);
      break;
    default:
      syslog(LOG_WARNING, "Unknown compoet %x", comp);
      break;
  }
  syslog(LOG_CRIT, "Updated %s on MB. File: %s. Result: %s", get_component_name(comp), path, (ret != 0)? "Fail": "Success");

  return ret;
}
