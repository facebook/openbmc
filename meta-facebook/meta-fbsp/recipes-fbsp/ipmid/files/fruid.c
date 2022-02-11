/*
 *
 * Copyright 2015-present Facebook. All Rights Reserved.
 *
 * This file provides platform specific implementation of FRUID information
 *
 * FRUID specification can be found at
 * www.intel.com/content/dam/www/public/us/en/documents/product-briefs/platform-management-fru-document-rev-1-2-feb-2013.pdf
 *
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
#include <string.h>
#include <syslog.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <pthread.h>
#include <sys/stat.h>
#include <openbmc/pal.h>
#include "fruid.h"

#define BIN_MB        "/tmp/fruid_mb.bin"
#define BIN_NIC0      "/tmp/fruid_nic0.bin"
#define BIN_NIC1      "/tmp/fruid_nic1.bin"
#define BIN_RISER1    "/tmp/fruid_riser1.bin"
#define BIN_RISER2    "/tmp/fruid_riser2.bin"
#define BIN_BMC       "/tmp/fruid_bmc.bin"

#define FRUID_SIZE        512
/*
 * copy_eeprom_to_bin - copy the eeprom to binary file im /tmp directory
 *
 * @eeprom_file   : path for the eeprom of the device
 * @bin_file      : path for the binary file
 *
 * returns 0 on successful copy
 * returns non-zero on file operation errors
 */
int copy_eeprom_to_bin(const char * eeprom_file, const char * bin_file) {

  int eeprom;
  int bin;
  uint64_t tmp[FRUID_SIZE];
  ssize_t bytes_rd, bytes_wr;

  errno = 0;

  eeprom = open(eeprom_file, O_RDONLY);
  if (eeprom == -1) {
    syslog(LOG_ERR, "%s: unable to open the %s file: %s",
	__func__, eeprom_file, strerror(errno));
    return errno;
  }

  bin = open(bin_file, O_WRONLY | O_CREAT, 0644);
  if (bin == -1) {
    syslog(LOG_ERR, "%s: unable to create %s file: %s",
	__func__, bin_file, strerror(errno));
    goto err;
  }

  bytes_rd = read(eeprom, tmp, FRUID_SIZE);
  if (bytes_rd < 0) {
    syslog(LOG_ERR, "%s: read %s file failed: %s",
	__func__, eeprom_file, strerror(errno));
    goto exit;
  } else if (bytes_rd < FRUID_SIZE) {
    syslog(LOG_ERR, "%s: less than %d bytes", __func__, FRUID_SIZE);
    goto exit;
  }

  bytes_wr = write(bin, tmp, bytes_rd);
  if (bytes_wr != bytes_rd) {
    syslog(LOG_ERR, "%s: write to %s file failed: %s",
	__func__, bin_file, strerror(errno));
    goto exit;
  }

exit:
  close(bin);
err:
  close(eeprom);

  return errno;
}

/* Populate the platform specific eeprom for fruid info */
int plat_fruid_init(void) {
  char path[128];
  uint8_t rev_id = 0xFF;

  pal_get_platform_id(BOARD_REV_ID, &rev_id);
  if (rev_id == PLATFORM_EVT) {
    sprintf(path, FRU_EEPROM_MB_EVT);
  }
  else if (rev_id == PLATFORM_DVT) {
    sprintf(path, FRU_EEPROM_MB_DVT);
  }

  if (copy_eeprom_to_bin(path, BIN_MB)) {
    syslog(LOG_WARNING, "%s: Copy MB EEPROM Failed", __func__);
  }

  if (copy_eeprom_to_bin(FRU_EEPROM_NIC0, BIN_NIC0)) {
    syslog(LOG_WARNING, "%s: Copy NIC1 EEPROM Failed", __func__);
  }

  if (copy_eeprom_to_bin(FRU_EEPROM_NIC1, BIN_NIC1)) {
    syslog(LOG_WARNING, "%s: Copy NIC2 EEPROM Failed", __func__);
  }

  if (copy_eeprom_to_bin(FRU_EEPROM_RISER1, BIN_RISER1)) {
    syslog(LOG_WARNING, "%s: Copy RISER1 EEPROM Failed", __func__);
  }

  if (copy_eeprom_to_bin(FRU_EEPROM_RISER2, BIN_RISER2)) {
    syslog(LOG_WARNING, "%s: Copy RISER2 EEPROM Failed", __func__);
  }

  if (copy_eeprom_to_bin(FRU_EEPROM_BMC, BIN_BMC)) {
    syslog(LOG_WARNING, "%s: Copy BMC EEPROM Failed", __func__);
  }

  return 0;
}

int plat_fruid_size(unsigned char payload_id) {
  struct stat buf;
  int ret;

  // check the size of the file and return size
  ret = stat(BIN_MB, &buf);
  if (ret) {
    return 0;
  }

  return buf.st_size;
}

int plat_fruid_data(unsigned char payload_id, int fru_id, int offset, int count, unsigned char *data) {
  int fd;
  int ret;
  char fname[LARGEST_DEVICE_NAME]={0};

  // open file for read purpose
  ret=pal_get_fruid_path(fru_id,fname);
  if (ret < 0) {
    return ret;
  }

  fd = open(fname, O_RDONLY);
  if (fd < 0) {
    return fd;
  }

  // seek position based on given offset
  ret = lseek(fd, offset, SEEK_SET);
  if (ret < 0) {
    close(fd);
    return ret;
  }

  // read the file content
  ret = read(fd, data, count);
  if (ret != count) {
    close(fd);
    return -1;
  }

  close(fd);
  return 0;
}

int
pal_fruid_write(uint8_t fru, char *path)
{
  int fru_size = 0;
  char command[128]={0};
  char bin_path[128]={0};
  char fru_name[20]={0};
  int ret=PAL_EOK;
  FILE *fruid_fd;
  uint8_t rev_id = 0xFF;

  if(pal_get_fru_name(fru, fru_name) < 0) {
	  return PAL_ENOTSUP;
  }

  fruid_fd = fopen(path, "rb");
  if ( NULL == fruid_fd ) {
    syslog(LOG_WARNING, "[%s] unable to open the file: %s", __func__, path);
    return PAL_ENOTSUP;
  }

  fseek(fruid_fd, 0, SEEK_END);
  fru_size = (uint32_t) ftell(fruid_fd);
  fclose(fruid_fd);
  printf("[%s]FRU Size: %d\n", __func__, fru_size);

  switch (fru)
  {
    case FRU_MB:
      pal_get_platform_id(BOARD_REV_ID, &rev_id);
      if (rev_id == PLATFORM_EVT) {
        sprintf(command, "dd if=%s of=%s bs=%d count=1", path, FRU_EEPROM_MB_EVT, fru_size);
      }
      else if (rev_id == PLATFORM_DVT) {
        sprintf(command, "dd if=%s of=%s bs=%d count=1", path, FRU_EEPROM_MB_DVT, fru_size);
      }
      break;
    case FRU_NIC0:
      sprintf(command, "dd if=%s of=%s bs=%d count=1", path, FRU_EEPROM_NIC0, fru_size);
      break;
    case FRU_NIC1:
      sprintf(command, "dd if=%s of=%s bs=%d count=1", path, FRU_EEPROM_NIC1,  fru_size);
      break;
    case FRU_RISER1:
      sprintf(command, "dd if=%s of=%s bs=%d count=1", path, FRU_EEPROM_RISER1, fru_size);
      break;
    case FRU_RISER2:
      sprintf(command, "dd if=%s of=%s bs=%d count=1", path, FRU_EEPROM_RISER2, fru_size);
      break;
    case FRU_BMC:
      sprintf(command, "dd if=%s of=%s bs=%d count=1", path, FRU_EEPROM_BMC, fru_size);
      break;
    default:
      //if there is an unknown device on the slot, return
      syslog(LOG_WARNING, "[%s] Unknown fru: %s", __func__, fru_name);
      return PAL_ENOTSUP;
    break;
  }

  ret=pal_get_fruid_eeprom_path(fru, bin_path);
  if(ret < 0) {
      syslog(LOG_WARNING, "[%s] get %s eeprom path failed\n", __func__, fru_name);
      return PAL_ENOTSUP;
  }

  if (system(command) != 0) {
      syslog(LOG_WARNING, "[%s] %s failed\n", __func__, command);
      return PAL_ENOTSUP;
  }
    //compare the in and out data
  ret=pal_compare_fru_data(bin_path, path, fru_size);
  if (ret < 0) {
      syslog(LOG_ERR, "[%s] %s  Write Fail",  __func__, fru_name);
      return PAL_ENOTSUP;
  }

  return ret;
}
