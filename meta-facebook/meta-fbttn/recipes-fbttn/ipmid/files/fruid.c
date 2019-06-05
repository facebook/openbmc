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
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <syslog.h>
#include <string.h>
#include <stdint.h>
#include <sys/stat.h>
#include "fruid.h"

#define EEPROM_IOM      "/sys/class/i2c-adapter/i2c-0/0-0050/eeprom"
#define EEPROM_NIC      "/sys/class/i2c-adapter/i2c-12/12-0051/eeprom"

#define BIN_IOM         "/tmp/fruid_iom.bin"
#define BIN_DPB         "/tmp/fruid_dpb.bin"
#define BIN_NIC         "/tmp/fruid_nic.bin"
#define BIN_SLOT        "/tmp/fruid_server.bin"

#define FRUID_SIZE        256

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

  if (access(eeprom_file, F_OK) != -1) {

    eeprom = open(eeprom_file, O_RDONLY);
    if (eeprom == -1) {
      syslog(LOG_ERR, "copy_eeprom_to_bin: unable to open the %s file: %s",
          eeprom_file, strerror(errno));
      return errno;
    }

    bin = open(bin_file, O_WRONLY | O_CREAT, 0644);
    if (bin == -1) {
      syslog(LOG_ERR, "copy_eeprom_to_bin: unable to create %s file: %s",
          bin_file, strerror(errno));
      return errno;
    }

    bytes_rd = read(eeprom, tmp, FRUID_SIZE);
    if (bytes_rd != FRUID_SIZE) {
      syslog(LOG_ERR, "copy_eeprom_to_bin: write to %s file failed: %s",
          eeprom_file, strerror(errno));
      return errno;
    }

    bytes_wr = write(bin, tmp, bytes_rd);
    if (bytes_wr != bytes_rd) {
      syslog(LOG_ERR, "copy_eeprom_to_bin: write to %s file failed: %s",
          bin_file, strerror(errno));
      return errno;
    }

    close(bin);
    close(eeprom);
  }

  return 0;
}


int check_fru_is_empty(const char * bin_file)
{
  int bin;
  unsigned char head_buf[8];
  unsigned char empty_buf[8];
  ssize_t bytes_rd;
  memset(head_buf,0,8);
  memset(empty_buf,0xff,8);
  if (access(bin_file, F_OK) != -1) {

    bin = open(bin_file, O_RDONLY);
      if (bin == -1) {
        syslog(LOG_WARNING, "check_fru_is_empty: unable to open the %s file",bin_file);
      return errno;
     }
     bytes_rd = read(bin, head_buf, 8);
     if(bytes_rd != 8 || !memcmp(head_buf,empty_buf,8)) {
	   syslog(LOG_CRIT, "FRU header %s is empty", bin_file);
       return errno;
     }
  }
  return 0;
}

/* Populate the platform specific eeprom for fruid info */
int plat_fruid_init(void) {

  int ret;

  ret = copy_eeprom_to_bin(EEPROM_IOM, BIN_IOM);
  check_fru_is_empty(BIN_IOM);
  ret = copy_eeprom_to_bin(EEPROM_NIC, BIN_NIC);
  check_fru_is_empty(BIN_NIC);

  return ret;
}

int plat_fruid_size(unsigned char payload_id) {
  char fpath[64] = {0};
  struct stat buf;
  int ret;

  // Fill the file path for a given slot
  sprintf(fpath, BIN_SLOT);

  // check the size of the file and return size
  ret = stat(fpath, &buf);
  if (ret) {
    return 0;
  }

  return buf.st_size;
}

int plat_fruid_data(unsigned char payload_id, int fru_id, int offset, int count, unsigned char *data) {
  char fpath[64] = {0};
  int fd;
  int ret;

  // Fill the file path for a given slot
  sprintf(fpath, BIN_SLOT);

  // open file for read purpose
  fd = open(fpath, O_RDONLY);
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
