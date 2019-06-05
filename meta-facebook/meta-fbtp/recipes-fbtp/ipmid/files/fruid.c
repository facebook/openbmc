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
#include <openbmc/pal.h>

#define EEPROM_RETIMER "/sys/devices/platform/ast-i2c.3/i2c-3/3-0055/eeprom"
#define EEPROM_RISER     "/sys/devices/platform/ast-i2c.1/i2c-1/1-0050/eeprom"
#define EEPROM_NIC      "/sys/devices/platform/ast-i2c.3/i2c-3/3-0054/eeprom"
#define EEPROM_MB      "/sys/devices/platform/ast-i2c.6/i2c-6/6-0054/eeprom"

#define BIN_MB         "/tmp/fruid_mb.bin"
#define BIN_NIC         "/tmp/fruid_nic.bin"
#define BIN_RISER        "/tmp/fruid_riser_slot%d.bin"

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
  
  if (access(eeprom_file, F_OK) != -1) {

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
      return errno;
    }

    bytes_rd = read(eeprom, tmp, FRUID_SIZE);
    if (bytes_rd > FRUID_SIZE) {
      syslog(LOG_ERR, "%s: read %s file failed: %s",
          __func__, eeprom_file, strerror(errno));
      syslog(LOG_ERR, "%s: Need to less than or equal to %d bytes", __func__, FRUID_SIZE);
      return errno;
    }

    bytes_wr = write(bin, tmp, bytes_rd);
    if (bytes_wr != bytes_rd) {
      syslog(LOG_ERR, "%s: write to %s file failed: %s",
          __func__, bin_file, strerror(errno));
      return errno;
    }

    close(bin);
    close(eeprom);
  }
  
  return 0;
}

/* Populate the platform specific eeprom for fruid info */
int plat_fruid_init(void) {

  int riser_slot;
  int ret;
  uint8_t slot_cfg = 0;
  uint8_t total_riser_slot = 0;
  char device_name[16]={0};
  uint8_t bus = 0;
  uint8_t device_addr = 0;
  uint8_t device_type = 0;
  char path[64]={0};

  enum
  {
    TWO_SLOT_RISER = 0x2,
    THREE_SLOT_RISER = 0x3,
  };

  enum
  {
    SMBUS1 = 0x1,
    SMBUS3 = 0x3,
  };

  ret = pal_get_slot_cfg_id(&slot_cfg);
  if ( (PAL_EOK == ret) && (SLOT_CFG_EMPTY != slot_cfg) )
  {
    total_riser_slot =  ( SLOT_CFG_SS_3x8 != slot_cfg )?TWO_SLOT_RISER:THREE_SLOT_RISER;

    for ( riser_slot = 0; riser_slot < total_riser_slot; riser_slot++ )
    {
      //identify the device which contains fru or not
      ret = pal_is_fru_on_riser_card( riser_slot, &device_type );
      if ( ret < 0 )
      {
        continue;
      }

      if ( FOUND_AVA_DEVICE == device_type )
      {
        bus = 0x1;
        sprintf(device_name, "24c64");
        device_addr = 0x50;
      }
      else if ( FOUND_RETIMER_DEVICE == device_type )
      {
        bus = 0x3;
        sprintf(device_name, "24c02");
        device_addr = 0x55;
      }

      sprintf(path, BIN_RISER, (riser_slot+2) );//the output file name. The riser slot id is start from 2

      //add the device
      pal_add_i2c_device(bus, device_name, device_addr);

      if ( bus == SMBUS1 )
      {
        if ( pal_riser_mux_switch( riser_slot ) )
          syslog(LOG_WARNING, "[%s] Switch Riser Card MUX Failed",__func__);

        if ( copy_eeprom_to_bin( EEPROM_RISER, path) )
          syslog(LOG_WARNING, "[%s] Copy Riser Card EEPROM Failed",__func__);

        if ( pal_riser_mux_release() )
          syslog(LOG_WARNING, "[%s] Release Riser Card MUX Failed",__func__);
      }
      else if ( bus == SMBUS3 )
      {
        ret = pal_control_mux_to_target_ch( riser_slot, bus/*retimer fru is on bus3*/, 0xe2/*mux address*/);
        if ( ret < 0 )
        {
          syslog(LOG_WARNING,"[%s] Cannot Switch MUX on bus %d. ", __func__, bus);
        }

        if ( copy_eeprom_to_bin( EEPROM_RETIMER, path) )
        {
          syslog(LOG_WARNING, "[%s] Copy Retimer Card EEPROM Failed",__func__);
        }
      }
      else
      {
        syslog(LOG_WARNING, "[%s] unexpected error. bus#%d, device#%s, device_addr#%x",
                            __func__, bus, device_name, device_addr);
      }

      //del the device
      pal_del_i2c_device(bus, device_addr);
    }
  }
#ifdef DEBUG
  else
  {
    syslog(LOG_WARNING, "[%s]No Riser Card Detected",__func__);
  }
#endif

  if ( copy_eeprom_to_bin(EEPROM_MB, BIN_MB))
    syslog(LOG_WARNING, "[%s]Copy MB EEPROM Failed",__func__);

  if ( copy_eeprom_to_bin(EEPROM_NIC, BIN_NIC))
    syslog(LOG_WARNING, "[%s]Copy NIC EEPROM Failed",__func__);

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

  // open file for read purpose
  fd = open(BIN_MB, O_RDONLY);
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
