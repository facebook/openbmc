/* Copyright 2015-present Facebook. All Rights Reserved.
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
#include <stdlib.h>
#include <stdint.h>
#include <syslog.h>
#include <facebook/fby3_common.h>
#include "fby3_fruid.h"

#define NIC_FW_VER_PATH "/tmp/cache_store/nic_fw_ver"

uint32_t
fby3_get_nic_mfgid(void) {
  FILE *fp;
  uint8_t buf[16];

  fp = fopen(NIC_FW_VER_PATH, "rb");
  if (!fp) {
    return MFG_UNKNOWN;
  }

  fseek(fp, 32 , SEEK_SET);
  if (fread(buf, 1, 4, fp)) {
    fclose(fp);
    return -1;
  }
  fclose(fp);

  return ((buf[3]<<24)|(buf[2]<<16)|(buf[1]<<8)|buf[0]);
}

#if 0
static int
fby3_fruid_init_local_fru() {
  int ret;
  char cmd[128] = {0};
  int cmd_len = sizeof(cmd);
  
  uint8_t bmc_location = 0;

  ret = get_bmc_location(&bmc_location);
  if ( ret < 0 ) {
    syslog(LOG_WARNING, "%s() Cannot get the location of BMC", __func__);
    return ret;;
  }

  if ( bmc_location == BB_BMC ) {
    snprintf(cmd, cmd_len, "echo \"24c128 0x54\" > /sys/class/i2c-dev/i2c-11/device/new_device");
    ret = system(cmd);
    snprintf(cmd, cmd_len, "echo \"24c128 0x51\" > /sys/class/i2c-dev/i2c-11/device/new_device");
    ret = system(cmd);
  } else {
    snprintf(cmd, cmd_len, "echo \"24c128 0x54\" > /sys/class/i2c-dev/i2c-10/device/new_device");
    ret = system(cmd);
    snprintf(cmd, cmd_len, "echo \"24c128 0x51\" > /sys/class/i2c-dev/i2c-10/device/new_device");
    ret = system(cmd);
  }

  return ret;
}
#endif

#if 0
static int
fby3_fruid_init_server_fru() {

}
#endif

#if 0
int
fby3_fruid_init_file(void) {
  int ret;
  //intialize the local frus that is connected to BMC directly.
  ret = fby3_fruid_init_local_fru();
  return ret;
}
#endif

/* Populate char path[] with the path to the fru's fruid binary dump */
int
fby3_get_fruid_path(uint8_t fru, uint8_t dev_id, char *path) {
  return 0;
}

/* Populate char name[] with the path to the fru's name */
int
fby3_get_fruid_name(uint8_t fru, char *name) {
  return 0;
}
