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
#include <openbmc/fruid.h>
#include <openbmc/pal.h>

#define FRUID_SIZE        512
#define MAX_RELOAD_FRU 3

#define MB_BIN "/tmp/fruid_mb.bin"
#define MB_EEPROM "/sys/class/i2c-dev/i2c-6/device/6-0054/eeprom"
#define PDB_BIN "/tmp/fruid_pdb.bin"
#define PDB_EEPROM "/sys/class/i2c-dev/i2c-5/device/5-0054/eeprom"
#define BSM_BIN "/tmp/fruid_bsm.bin"
#define BSM_EEPROM "/sys/class/i2c-dev/i2c-23/device/23-0056/eeprom"
#define AVA1_BIN "/tmp/fruid_ava1.bin"
#define AVA1_EEPROM "/sys/class/i2c-dev/i2c-21/device/21-0050/eeprom"
#define AVA2_BIN "/tmp/fruid_ava2.bin"
#define AVA2_EEPROM "/sys/class/i2c-dev/i2c-22/device/22-0050/eeprom"
#define E1S1_BIN "/tmp/fruid_e1s1.bin"
#define E1S1_EEPROM "/sys/class/i2c-dev/i2c-21/device/21-0050/eeprom"
#define E1S2_BIN "/tmp/fruid_e1s2.bin"
#define E1S2_EEPROM "/sys/class/i2c-dev/i2c-22/device/22-0050/eeprom"
#define FIO_EEPROM "/sys/class/i2c-dev/i2c-8/device/8-0051/eeprom"
#define FIO_BIN "/tmp/fruid_fio.bin"
#define NIC0_EEPROM "/sys/class/i2c-dev/i2c-1/device/1-0050/eeprom"
#define NIC0_BIN "/tmp/fruid_nic0.bin"
#define NIC1_EEPROM "/sys/class/i2c-dev/i2c-9/device/9-0050/eeprom"
#define NIC1_BIN "/tmp/fruid_nic1.bin"
#define NIC2_EEPROM "/sys/class/i2c-dev/i2c-2/device/2-0050/eeprom"
#define NIC2_BIN "/tmp/fruid_nic2.bin"
#define NIC3_EEPROM "/sys/class/i2c-dev/i2c-10/device/10-0050/eeprom"
#define NIC3_BIN "/tmp/fruid_nic3.bin"
#define NIC4_EEPROM "/sys/class/i2c-dev/i2c-4/device/4-0050/eeprom"
#define NIC4_BIN "/tmp/fruid_nic4.bin"
#define NIC5_EEPROM "/sys/class/i2c-dev/i2c-11/device/11-0050/eeprom"
#define NIC5_BIN "/tmp/fruid_nic5.bin"
#define NIC6_EEPROM "/sys/class/i2c-dev/i2c-7/device/7-0050/eeprom"
#define NIC6_BIN "/tmp/fruid_nic6.bin"
#define NIC7_EEPROM "/sys/class/i2c-dev/i2c-13/device/13-0050/eeprom"
#define NIC7_BIN "/tmp/fruid_nic7.bin"

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
  uint64_t comp_tmp[FRUID_SIZE];
  ssize_t bytes_rd, bytes_wr, bytes_comp;
  int reload = 0;
  int ret;

  errno = 0;
  fruid_info_t fruid;

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

  do {
    lseek(eeprom, 0, SEEK_SET);
    bytes_rd = read(eeprom, tmp, FRUID_SIZE);
    if (bytes_rd < 0) {
      syslog(LOG_ERR, "%s: read %s file failed: %s",
    __func__, eeprom_file, strerror(errno));
      goto exit;
    } else if (bytes_rd < FRUID_SIZE) {
      syslog(LOG_ERR, "%s: less than %d bytes", __func__, FRUID_SIZE);
      goto exit;
    }

    // make sure the content is equal in two times read
    lseek(eeprom, 0, SEEK_SET);
    bytes_comp = read(eeprom, comp_tmp, FRUID_SIZE);

    if(bytes_comp < 0 || bytes_comp < FRUID_SIZE) {
      syslog(LOG_ERR, "%s: read eeprom failed", __func__);
      goto exit;
    }

    if( memcmp(tmp, comp_tmp, FRUID_SIZE) != 0 ) {
      syslog(LOG_ERR, "%s: eeprom content is not equal\n", __func__);
      reload++;
      continue;
    }
    
    bytes_wr = write(bin, tmp, bytes_rd);
    if (bytes_wr != bytes_rd) {
      syslog(LOG_ERR, "%s: write to %s file failed: %s",
    __func__, bin_file, strerror(errno));
      goto exit;
    }
    reload++;
    ret = fruid_parse(bin_file, &fruid);
    if(ret != 0) {
      syslog(LOG_WARNING, "%s: FRU data checksum is invalid, retry_count:%d", __func__, reload);
    } else {
      free_fruid_info(&fruid);
    }
  } while(ret && reload <= MAX_RELOAD_FRU);

exit:
  close(bin);
err:
  close(eeprom);

  return errno;
}

/* Populate the platform specific eeprom for fruid info */
int plat_fruid_init(void)
{
  if (copy_eeprom_to_bin(MB_EEPROM, MB_BIN))
    syslog(LOG_WARNING, "[%s]Copy EEPROM to %s Failed",__func__, MB_BIN);

  if (copy_eeprom_to_bin(PDB_EEPROM, PDB_BIN))
    syslog(LOG_WARNING, "[%s]Copy EEPROM to %s Failed",__func__, PDB_BIN);

  if (copy_eeprom_to_bin(BSM_EEPROM, BSM_BIN))
    syslog(LOG_WARNING, "[%s]Copy EEPROM to %s Failed",__func__, BSM_BIN);

  if (copy_eeprom_to_bin(AVA1_EEPROM, AVA1_BIN))
    syslog(LOG_WARNING, "[%s]Copy EEPROM to %s Failed",__func__, AVA1_BIN);

  if (copy_eeprom_to_bin(AVA2_EEPROM, AVA2_BIN))
    syslog(LOG_WARNING, "[%s]Copy EEPROM to %s Failed",__func__, AVA2_BIN);

  if (copy_eeprom_to_bin(E1S1_EEPROM, E1S1_BIN))
    syslog(LOG_WARNING, "[%s]Copy EEPROM to %s Failed",__func__, E1S1_BIN);

  if (copy_eeprom_to_bin(E1S2_EEPROM, E1S2_BIN))
    syslog(LOG_WARNING, "[%s]Copy EEPROM to %s Failed",__func__, E1S2_BIN);

  if (copy_eeprom_to_bin(FIO_EEPROM, FIO_BIN))
    syslog(LOG_WARNING, "[%s]Copy EEPROM to %s Failed",__func__, FIO_BIN);

  if (copy_eeprom_to_bin(NIC0_EEPROM, NIC0_BIN))
    syslog(LOG_WARNING, "[%s]Copy EEPROM to %s Failed",__func__, NIC0_BIN);

  if (copy_eeprom_to_bin(NIC1_EEPROM, NIC1_BIN))
    syslog(LOG_WARNING, "[%s]Copy EEPROM to %s Failed",__func__, NIC1_BIN);

  if (copy_eeprom_to_bin(NIC2_EEPROM, NIC2_BIN))
    syslog(LOG_WARNING, "[%s]Copy EEPROM to %s Failed",__func__, NIC2_BIN);

  if (copy_eeprom_to_bin(NIC3_EEPROM, NIC3_BIN))
    syslog(LOG_WARNING, "[%s]Copy EEPROM to %s Failed",__func__, NIC3_BIN);

  if (copy_eeprom_to_bin(NIC4_EEPROM, NIC4_BIN))
    syslog(LOG_WARNING, "[%s]Copy EEPROM to %s Failed",__func__, NIC4_BIN);

  if (copy_eeprom_to_bin(NIC5_EEPROM, NIC5_BIN))
    syslog(LOG_WARNING, "[%s]Copy EEPROM to %s Failed",__func__, NIC5_BIN);

  if (copy_eeprom_to_bin(NIC6_EEPROM, NIC6_BIN))
    syslog(LOG_WARNING, "[%s]Copy EEPROM to %s Failed",__func__, NIC6_BIN);

  if (copy_eeprom_to_bin(NIC7_EEPROM, NIC7_BIN))
    syslog(LOG_WARNING, "[%s]Copy EEPROM to %s Failed",__func__, NIC7_BIN);

  return 0;
}

int plat_fruid_size(unsigned char payload_id)
{
  struct stat buf;
  int ret;

  // check the size of the file and return size
  ret = stat(MB_BIN, &buf);
  if (ret) {
    return 0;
  }

  return buf.st_size;
}

int plat_fruid_data(unsigned char payload_id, int fru_id, int offset, int count, unsigned char *data) {
  int fd;
  int ret;
  char fru_dev[LARGEST_DEVICE_NAME] = {0};

  // Align wiht IPMI FRU ID, 0-based
  if (fru_id == FRU_MB-1)
    snprintf(fru_dev, LARGEST_DEVICE_NAME, MB_BIN);
  else if (fru_id == FRU_PDB-1)
    snprintf(fru_dev, LARGEST_DEVICE_NAME, PDB_BIN);
  else if (fru_id == FRU_BSM-1)
    snprintf(fru_dev, LARGEST_DEVICE_NAME, BSM_BIN);
  else if (fru_id == FRU_CARRIER1-1) {
    if( pal_check_carrier_type(0) == M2 )
      snprintf(fru_dev, LARGEST_DEVICE_NAME, AVA1_BIN);
    else
      snprintf(fru_dev, LARGEST_DEVICE_NAME, E1S1_BIN);
  }
  else if (fru_id == FRU_CARRIER2-1) {
    if( pal_check_carrier_type(1) == M2 )
      snprintf(fru_dev, LARGEST_DEVICE_NAME, AVA2_BIN);
    else
      snprintf(fru_dev, LARGEST_DEVICE_NAME, E1S2_BIN);
  }
  else if (fru_id == FRU_FIO-1)
    snprintf(fru_dev, LARGEST_DEVICE_NAME, FIO_BIN);
  else
    return -1;

  // open file for read purpose
  fd = open(fru_dev, O_RDONLY);
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
