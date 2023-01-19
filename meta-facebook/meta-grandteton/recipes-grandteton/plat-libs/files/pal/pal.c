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
#include <stdbool.h>
#include <ctype.h>
#include <errno.h>
#include <syslog.h>
#include <string.h>
#include <unistd.h>
#include <time.h>
#include <sys/mman.h>
#include <sys/time.h>
#include <dirent.h>
#include <openbmc/kv.h>
#include <openbmc/libgpio.h>
#include <openbmc/nm.h>
#include <openbmc/obmc-i2c.h>
#include <openbmc/ipmb.h>
#include <openbmc/ncsi.h>
#include <openbmc/nl-wrapper.h>
#include <openbmc/hal_fruid.h>
#include "pal.h"
#include <libpldm/pldm.h>
#include <libpldm/platform.h>
#include <libpldm-oem/pldm.h>
#include "pal_common.h"


#ifndef PLATFORM_NAME
#define PLATFORM_NAME "grandteton"
#endif

#define GUID_SIZE 16
#define OFFSET_SYS_GUID 0x17F0
#define OFFSET_DEV_GUID 0x1800


#define NUM_SERVER_FRU  1
#define NUM_NIC_FRU     2
#define NUM_BMC_FRU     1

const char pal_fru_list[] = \
"all, mb, nic0, nic1, swb, hgx, bmc, scm, vpdb, hpdb, fan_bp0, fan_bp1, fio, hsc, swb_hsc, " \
// Artemis fru list
"acb, meb, acb_accl1, acb_accl2, acb_accl3, acb_accl4, acb_accl5, acb_accl6, acb_accl7, acb_accl8, " \
"acb_accl9, acb_accl10, acb_accl11, acb_accl12";

const char pal_server_list[] = "mb";


#define ALL_CAPABILITY  FRU_CAPABILITY_SENSOR_READ | FRU_CAPABILITY_SENSOR_HISTORY

#define MB_CAPABILITY   FRU_CAPABILITY_FRUID_ALL | FRU_CAPABILITY_SERVER |  \
                        FRU_CAPABILITY_SENSOR_ALL | FRU_CAPABILITY_POWER_ALL

#define SWB_CAPABILITY  FRU_CAPABILITY_FRUID_ALL | FRU_CAPABILITY_SENSOR_ALL

#define HGX_CAPABILITY  FRU_CAPABILITY_FRUID_ALL | FRU_CAPABILITY_SENSOR_ALL

#define NIC_CAPABILITY  FRU_CAPABILITY_FRUID_ALL | FRU_CAPABILITY_SENSOR_ALL | \
                        FRU_CAPABILITY_NETWORK_CARD

#define BMC_CAPABILITY  FRU_CAPABILITY_FRUID_ALL | FRU_CAPABILITY_MANAGEMENT_CONTROLLER

#define SCM_CAPABILITY  FRU_CAPABILITY_FRUID_ALL | FRU_CAPABILITY_SENSOR_ALL

#define PDB_CAPABILITY  FRU_CAPABILITY_FRUID_ALL | FRU_CAPABILITY_SENSOR_ALL

#define BP_CAPABILITY   FRU_CAPABILITY_FRUID_ALL | FRU_CAPABILITY_SENSOR_ALL

#define FIO_CAPABILITY  FRU_CAPABILITY_FRUID_ALL

#define HSC_CAPABILITY  FRU_CAPABILITY_FRUID_ALL | FRU_CAPABILITY_SENSOR_ALL

// Artemis fru capability
#define ACB_CAPABILITY  FRU_CAPABILITY_FRUID_ALL | FRU_CAPABILITY_SENSOR_ALL
#define MEB_CAPABILITY  FRU_CAPABILITY_FRUID_ALL | FRU_CAPABILITY_SENSOR_ALL
#define ACB_ACCL_CAPABILITY FRU_CAPABILITY_FRUID_ALL

struct fru_dev_info {
  uint8_t fru_id;
  char *name;
  char *desc;
  uint8_t bus;
  uint8_t addr;
  uint32_t cap;
  uint8_t path;
  bool (*check_presence) (uint8_t fru_id);
  int8_t pldm_fru_id;
};

typedef struct {
  uint8_t target;
  uint8_t data[MAX_IPMB_REQ_LEN];
} bypass_cmd;

typedef struct {
  uint8_t netdev;
  uint8_t channel;
  uint8_t cmd;
  unsigned char data[NCSI_MAX_PAYLOAD];
} bypass_ncsi_req;

typedef struct {
  uint8_t payload_id;
  uint8_t netfn;
  uint8_t cmd;
  uint8_t target;
  uint8_t netdev;
  uint8_t channel;
  uint8_t ncsi_cmd;
} bypass_ncsi_header;

struct fru_dev_info fru_dev_data[] = {
  {FRU_ALL,   "all",     NULL,            0,  0,    ALL_CAPABILITY, FRU_PATH_NONE,   NULL, PLDM_FRU_NOT_SUPPORT},
  {FRU_MB,    "mb",      "Mother Board",  33, 0x51, MB_CAPABILITY,  FRU_PATH_EEPROM, fru_presence, PLDM_FRU_NOT_SUPPORT},
  {FRU_SWB,   "swb",     "Switch Board",  3,  0x20, SWB_CAPABILITY, FRU_PATH_PLDM,   fru_presence,  PLDM_FRU_SWB},
  {FRU_HGX,   "hgx",     "HGX Board"  ,   9,  0x53,  HGX_CAPABILITY, FRU_PATH_EEPROM,fru_presence, PLDM_FRU_NOT_SUPPORT},
  {FRU_NIC0,  "nic0",    "Mezz Card 0",   13, 0x50, NIC_CAPABILITY, FRU_PATH_EEPROM, fru_presence, PLDM_FRU_NOT_SUPPORT},
  {FRU_NIC1,  "nic1",    "Mezz Card 1",   4,  0x52, NIC_CAPABILITY, FRU_PATH_EEPROM, fru_presence, PLDM_FRU_NOT_SUPPORT},
  {FRU_OCPDBG,   "ocpdbg",  "Debug Board",   14, 0,    0,              FRU_PATH_NONE,   NULL,         PLDM_FRU_NOT_SUPPORT},
  {FRU_BMC,   "bmc",     "BSM Board",     15, 0x56, BMC_CAPABILITY, FRU_PATH_EEPROM, fru_presence, PLDM_FRU_NOT_SUPPORT},
  {FRU_SCM,   "scm",     "SCM Board",     15, 0x50, SCM_CAPABILITY, FRU_PATH_EEPROM, fru_presence, PLDM_FRU_NOT_SUPPORT},
  {FRU_VPDB,  "vpdb",    "VPDB Board",    36, 0x52, PDB_CAPABILITY, FRU_PATH_EEPROM, fru_presence, PLDM_FRU_NOT_SUPPORT},
  {FRU_HPDB,  "hpdb",    "HPDB Board",    37, 0x51, PDB_CAPABILITY, FRU_PATH_EEPROM, fru_presence, PLDM_FRU_NOT_SUPPORT},
  {FRU_FAN_BP0,   "fan_bp0",     "FAN_BP0 Board",     40, 0x56, BP_CAPABILITY,  FRU_PATH_EEPROM, fru_presence, PLDM_FRU_NOT_SUPPORT},
  {FRU_FAN_BP1,   "fan_bp1",     "FAN_BP1 Board",     41, 0x56, BP_CAPABILITY,  FRU_PATH_EEPROM, fru_presence, PLDM_FRU_NOT_SUPPORT},
  {FRU_FIO,   "fio",     "FIO Board",     3,  0x20, FIO_CAPABILITY, FRU_PATH_PLDM,   fru_presence, PLDM_FRU_FIO},
  {FRU_HSC,   "hsc",     "HSC Board",     2,  0x51, 0,              FRU_PATH_EEPROM, fru_presence, PLDM_FRU_NOT_SUPPORT},
  {FRU_SHSC,  "swb_hsc", "SWB HSC Board", 3,  0x20, 0,              FRU_PATH_PLDM,   fru_presence, PLDM_FRU_SHSC},
  // Artemis FRU dev data
  {FRU_ACB,        "acb",        "Carrier Board",     ACB_BIC_BUS,   ACB_BIC_ADDR,   ACB_CAPABILITY,      FRU_PATH_PLDM,   fru_presence,  PLDM_FRU_ACB},
  {FRU_MEB,        "meb",        "Memory Exp Board",  MEB_BIC_BUS,   MEB_BIC_ADDR,   MEB_CAPABILITY,      FRU_PATH_PLDM,   fru_presence,  PLDM_FRU_NOT_SUPPORT},
  {FRU_ACB_ACCL1,  "acb_accl1",  "ACB ACCL1 Card",    ACB_BIC_BUS,   ACB_BIC_ADDR,   ACB_ACCL_CAPABILITY, FRU_PATH_PLDM,   pldm_fru_prsnt,  PLDM_FRU_ACB_ACCL1},
  {FRU_ACB_ACCL2,  "acb_accl2",  "ACB ACCL2 Card",    ACB_BIC_BUS,   ACB_BIC_ADDR,   ACB_ACCL_CAPABILITY, FRU_PATH_PLDM,   pldm_fru_prsnt,  PLDM_FRU_ACB_ACCL2},
  {FRU_ACB_ACCL3,  "acb_accl3",  "ACB ACCL3 Card",    ACB_BIC_BUS,   ACB_BIC_ADDR,   ACB_ACCL_CAPABILITY, FRU_PATH_PLDM,   pldm_fru_prsnt,  PLDM_FRU_ACB_ACCL3},
  {FRU_ACB_ACCL4,  "acb_accl4",  "ACB ACCL4 Card",    ACB_BIC_BUS,   ACB_BIC_ADDR,   ACB_ACCL_CAPABILITY, FRU_PATH_PLDM,   pldm_fru_prsnt,  PLDM_FRU_ACB_ACCL4},
  {FRU_ACB_ACCL5,  "acb_accl5",  "ACB ACCL5 Card",    ACB_BIC_BUS,   ACB_BIC_ADDR,   ACB_ACCL_CAPABILITY, FRU_PATH_PLDM,   pldm_fru_prsnt,  PLDM_FRU_ACB_ACCL5},
  {FRU_ACB_ACCL6,  "acb_accl6",  "ACB ACCL6 Card",    ACB_BIC_BUS,   ACB_BIC_ADDR,   ACB_ACCL_CAPABILITY, FRU_PATH_PLDM,   pldm_fru_prsnt,  PLDM_FRU_ACB_ACCL6},
  {FRU_ACB_ACCL7,  "acb_accl7",  "ACB ACCL7 Card",    ACB_BIC_BUS,   ACB_BIC_ADDR,   ACB_ACCL_CAPABILITY, FRU_PATH_PLDM,   pldm_fru_prsnt,  PLDM_FRU_ACB_ACCL7},
  {FRU_ACB_ACCL8,  "acb_accl8",  "ACB ACCL8 Card",    ACB_BIC_BUS,   ACB_BIC_ADDR,   ACB_ACCL_CAPABILITY, FRU_PATH_PLDM,   pldm_fru_prsnt,  PLDM_FRU_ACB_ACCL8},
  {FRU_ACB_ACCL9,  "acb_accl9",  "ACB ACCL9 Card",    ACB_BIC_BUS,   ACB_BIC_ADDR,   ACB_ACCL_CAPABILITY, FRU_PATH_PLDM,   pldm_fru_prsnt,  PLDM_FRU_ACB_ACCL9},
  {FRU_ACB_ACCL10, "acb_accl10", "ACB ACCL10 Card",   ACB_BIC_BUS,   ACB_BIC_ADDR,   ACB_ACCL_CAPABILITY, FRU_PATH_PLDM,   pldm_fru_prsnt,  PLDM_FRU_ACB_ACCL10},
  {FRU_ACB_ACCL11, "acb_accl11", "ACB ACCL11 Card",   ACB_BIC_BUS,   ACB_BIC_ADDR,   ACB_ACCL_CAPABILITY, FRU_PATH_PLDM,   pldm_fru_prsnt,  PLDM_FRU_ACB_ACCL11},
  {FRU_ACB_ACCL12, "acb_accl12", "ACB ACCL12 Card",   ACB_BIC_BUS,   ACB_BIC_ADDR,   ACB_ACCL_CAPABILITY, FRU_PATH_PLDM,   pldm_fru_prsnt,  PLDM_FRU_ACB_ACCL12},
};

uint8_t
pal_get_pldm_fru_id(uint8_t fru) {
  return fru_dev_data[fru].pldm_fru_id;
}

uint8_t
pal_get_fru_path_type(uint8_t fru) {
  return fru_dev_data[fru].path;
}

bool
pal_is_artemis() { // TODO: Use MB CPLD Offest to check the platform
#ifdef CONFIG_ARTEMIS
  return true;
#else
  return false;
#endif
}

int
pal_get_platform_name(char *name) {
  strcpy(name, PLATFORM_NAME);
  return 0;
}

int
pal_get_num_slots(uint8_t *num) {
  *num = 1;
  return 0;
}

int
pal_is_fru_prsnt(uint8_t fru, uint8_t *status) {
  if (!status) {
    syslog(LOG_WARNING, "%s() Status pointer is NULL.", __func__);
    return -1;
  }

  if (fru >= FRU_CNT || fru_dev_data[fru].check_presence == NULL) {
    *status = FRU_NOT_PRSNT;
  } else {
    if (pal_is_artemis() && ((fru >= FRU_ACB_ACCL1 && fru <= FRU_ACB_ACCL12) || fru == FRU_FIO)) {
      if (fru == FRU_FIO) {
        *status = pldm_fru_prsnt(fru_dev_data[fru].pldm_fru_id);
      } else {
        *status = fru_dev_data[fru].check_presence(fru_dev_data[fru].pldm_fru_id);
      }
    } else {
      *status = fru_dev_data[fru].check_presence(fru);
    }
  }
  return 0;
}

int
pal_is_slot_server(uint8_t fru) {
  if (fru == FRU_MB)
    return 1;
  return 0;
}

int
pal_get_fru_id(char *str, uint8_t *fru) {
  if (!str || !fru) {
    syslog(LOG_WARNING, "%s() Input pointer is NULL.", __func__);
    return -1;
  }

  for(int i=0; i < FRU_CNT; i++) {
    if (!strcmp(str, fru_dev_data[i].name)) {
       *fru = fru_dev_data[i].fru_id;
       return 0;
    }
  }
  syslog(LOG_WARNING, "pal_get_fru_id: Wrong fru#%s", str);
  return -1;
}

int
pal_get_fru_name(uint8_t fru, char *name) {
  if (!name) {
    syslog(LOG_WARNING, "%s() Name pointer is NULL.", __func__);
    return -1;
  }

  if (fru >= FRU_CNT) {
    syslog(LOG_WARNING, "%s(): Input fruid %d is invalid.", __func__, fru);
    return -1;
  }

  strcpy(name, fru_dev_data[fru].name);

  return 0;
}

void
pal_update_ts_sled() {
  char key[MAX_KEY_LEN] = {0};
  char tstr[MAX_VALUE_LEN] = {0};
  struct timespec ts;

  clock_gettime(CLOCK_REALTIME, &ts);
  sprintf(tstr, "%ld", ts.tv_sec);

  sprintf(key, "timestamp_sled");

  pal_set_key_value(key, tstr);
}

int
pal_get_fruid_path(uint8_t fru, char *path) {
  if (fru > MAX_NUM_FRUS || fru == FRU_ALL)
    return -1;

  sprintf(path, "/tmp/fruid_%s.bin", fru_dev_data[fru].name);
  return 0;
}

int
pal_get_fruid_eeprom_path(uint8_t fru, char *path) {

  if (fru > MAX_NUM_FRUS || fru_dev_data[fru].path != FRU_PATH_EEPROM)
    return -1;

  sprintf(path, FRU_EEPROM, fru_dev_data[fru].bus, fru_dev_data[fru].bus, fru_dev_data[fru].addr);
  return 0;
}

int
pal_get_fruid_name(uint8_t fru, char *name) {
  if (fru > MAX_NUM_FRUS || fru == FRU_ALL)
    return -1;

  sprintf(name, "%s", fru_dev_data[fru].desc);
  return 0;
}

int
pal_fruid_write(uint8_t fru, char *path) {
  uint8_t status = 0;
  unsigned int caps = 0;

  if (!path) {
    syslog(LOG_WARNING, "%s() path pointer is NULL.", __func__);
    return -1;
  }

  if (pal_get_fru_capability(fru, &caps) == 0 && (caps & FRU_CAPABILITY_FRUID_WRITE) && pal_is_fru_prsnt(fru, &status) == 0) {
    if (status == FRU_PRSNT) {
      return hal_write_pldm_fruid(fru_dev_data[fru].pldm_fru_id, path);
    } else {
      syslog(LOG_WARNING, "%s: FRU:%d Not Present", __func__, fru);
      return -1;
    }
  }

  return -1;
}

int
pal_is_fru_ready(uint8_t fru, uint8_t *status) {
  *status = 1;

  return 0;
}

int
pal_devnum_to_fruid(int devnum) {
  return FRU_MB;
}

int
pal_channel_to_bus(int channel) {
  switch (channel) {
    case IPMI_CHANNEL_0:
      return I2C_BUS_14; // USB (LCD Debug Board)

    case IPMI_CHANNEL_6:
      return I2C_BUS_6; // ME
  }

  // Debug purpose, map to real bus number
  if (channel & 0x80) {
    return (channel & 0x7f);
  }

  return channel;
}

int
pal_ipmb_processing(int bus, void *buf, uint16_t size) {
  char key[MAX_KEY_LEN];
  char value[MAX_VALUE_LEN];
  struct timespec ts;
  static time_t last_time = 0;

  switch (bus) {
    case I2C_BUS_14:
      if (((uint8_t *)buf)[0] == 0x20) {
        clock_gettime(CLOCK_MONOTONIC, &ts);
        if (ts.tv_sec >= (last_time + 5)) {
          last_time = ts.tv_sec;
          ts.tv_sec += 20;

          sprintf(key, "ocpdbg_lcd");
          sprintf(value, "%ld", ts.tv_sec);
          if (kv_set(key, value, 0, 0) < 0) {
            return -1;
          }
        }
      }
      break;
  }

  return 0;
}

int
pal_is_mcu_ready(uint8_t bus) {
  char key[MAX_KEY_LEN];
  char value[MAX_VALUE_LEN] = {0};
  struct timespec ts;

  switch (bus) {
    case I2C_BUS_14:
      sprintf(key, "ocpdbg_lcd");
      if (kv_get(key, value, NULL, 0)) {
        return false;
      }

      clock_gettime(CLOCK_MONOTONIC, &ts);
      if (strtoul(value, NULL, 10) > ts.tv_sec) {
         return true;
      }
      break;

    case I2C_BUS_3:
      return true;
  }

  return false;
}

int
pal_get_platform_id(uint8_t *id) {
  char value[MAX_VALUE_LEN] = {0};

  if (kv_get("mb_sku", value, NULL, 0)) {
    return -1;
  }

  *id = (uint8_t)atoi(value);
  return 0;
}

bool
pal_check_boot_device_is_vaild(uint8_t device) {
  bool vaild = false;

  switch (device)
  {
    case BOOT_DEVICE_USB:
    case BOOT_DEVICE_IPV4:
    case BOOT_DEVICE_HDD:
    case BOOT_DEVICE_CDROM:
    case BOOT_DEVICE_OTHERS:
    case BOOT_DEVICE_IPV6:
    case BOOT_DEVICE_RESERVED:
      vaild = true;
      break;
    default:
      break;
  }

  return vaild;
}

int
pal_set_boot_order(uint8_t slot, uint8_t *boot, uint8_t *res_data, uint8_t *res_len) {
  int i, j, network_dev = 0;
  char key[MAX_KEY_LEN] = {0};
  char str[MAX_VALUE_LEN] = {0};
  char tstr[10] = {0};
  *res_len = 0;
  sprintf(key, "server_boot_order");

  for (i = 0; i < SIZE_BOOT_ORDER; i++) {
    //Byte 0 is boot mode, Byte 1~5 is boot order
    if ((i > 0) && (boot[i] != 0xFF)) {
      if(!pal_check_boot_device_is_vaild(boot[i]))
        return CC_INVALID_PARAM;
      for (j = i+1; j < SIZE_BOOT_ORDER; j++) {
        if ( boot[i] == boot[j])
          return CC_INVALID_PARAM;
      }

      //If Bit 2:0 is 001b (Network), Bit3 is IPv4/IPv6 order
      //Bit3=0b: IPv4 first
      //Bit3=1b: IPv6 first
      if ( boot[i] == BOOT_DEVICE_IPV4 || boot[i] == BOOT_DEVICE_IPV6)
        network_dev++;
    }

    snprintf(tstr, sizeof(tstr), "%02x", boot[i]);
    strncat(str, tstr, sizeof(str) - 1);
  }

  //Not allow having more than 1 network boot device in the boot order.
  if (network_dev > 1)
    return CC_INVALID_PARAM;

  return pal_set_key_value(key, str);
}

int
pal_get_boot_order(uint8_t slot, uint8_t *req_data, uint8_t *boot, uint8_t *res_len) {
  int i;
  int j = 0;
  int ret;
  int msb, lsb;
  char key[MAX_KEY_LEN] = {0};
  char str[MAX_VALUE_LEN] = {0};
  char tstr[4] = {0};

  sprintf(key, "server_boot_order");

  ret = pal_get_key_value(key, str);
  if (ret) {
    *res_len = 0;
    return ret;
  }

  for (i = 0; i < 2*SIZE_BOOT_ORDER; i += 2) {
    sprintf(tstr, "%c\n", str[i]);
    msb = strtol(tstr, NULL, 16);

    sprintf(tstr, "%c\n", str[i+1]);
    lsb = strtol(tstr, NULL, 16);
    boot[j++] = (msb << 4) | lsb;
  }
  *res_len = SIZE_BOOT_ORDER;
  return 0;
}

// GUID for System and Device
static int
pal_get_guid(uint16_t offset, char *guid) {
  int fd;
  ssize_t bytes_rd;
  errno = 0;
  char mb_path[128] = {0};

  sprintf(mb_path, FRU_EEPROM, fru_dev_data[FRU_MB].bus, fru_dev_data[FRU_MB].bus, fru_dev_data[FRU_MB].addr);

  // check for file presence
  if (access(mb_path, F_OK)) {
    syslog(LOG_ERR, "pal_get_guid: unable to access %s: %s", mb_path, strerror(errno));
    return errno;
  }

  fd = open(mb_path, O_RDONLY);
  if (fd < 0) {
    syslog(LOG_ERR, "pal_get_guid: unable to open %s: %s", mb_path, strerror(errno));
    return errno;
  }

  lseek(fd, offset, SEEK_SET);

  bytes_rd = read(fd, guid, GUID_SIZE);
  if (bytes_rd != GUID_SIZE) {
    syslog(LOG_ERR, "pal_get_guid: read from %s failed: %s", mb_path, strerror(errno));
  }

  close(fd);
  return errno;
}

static int
pal_set_guid(uint16_t offset, char *guid) {
  int fd;
  ssize_t bytes_wr;
  errno = 0;
  char mb_path[128] = {0};

  sprintf(mb_path, FRU_EEPROM, fru_dev_data[FRU_MB].bus, fru_dev_data[FRU_MB].bus, fru_dev_data[FRU_MB].addr);

  // check for file presence
  if (access(mb_path, F_OK)) {
    syslog(LOG_ERR, "pal_set_guid: unable to access %s: %s", mb_path, strerror(errno));
    return errno;
  }

  fd = open(mb_path, O_WRONLY);
  if (fd < 0) {
    syslog(LOG_ERR, "pal_set_guid: unable to open %s: %s", mb_path, strerror(errno));
    return errno;
  }

  lseek(fd, offset, SEEK_SET);

  bytes_wr = write(fd, guid, GUID_SIZE);
  if (bytes_wr != GUID_SIZE) {
    syslog(LOG_ERR, "pal_set_guid: write to %s failed: %s", mb_path, strerror(errno));
  }

  close(fd);
  return errno;
}

int
pal_get_sys_guid(uint8_t fru, char *guid) {
  pal_get_guid(OFFSET_SYS_GUID, guid);
  return 0;
}

int
pal_set_sys_guid(uint8_t fru, char *str) {
  char guid[GUID_SIZE] = {0};

  pal_populate_guid(guid, str);
  return pal_set_guid(OFFSET_SYS_GUID, guid);
}

int
pal_get_dev_guid(uint8_t fru, char *guid) {
  pal_get_guid(OFFSET_DEV_GUID, guid);
  return 0;
}

int
pal_set_dev_guid(uint8_t fru, char *str) {
  char guid[GUID_SIZE] = {0};

  pal_populate_guid(guid, str);
  return pal_set_guid(OFFSET_DEV_GUID, guid);
}

int
pal_get_fru_list(char *list) {

  strcpy(list, pal_fru_list);
  return 0;
}

int
pal_get_board_rev_id(uint8_t *id) {
  char value[MAX_VALUE_LEN] = {0};

  if (kv_get("mb_rev", value, NULL, 0)) {
    return -1;
  }

  *id = (uint8_t)atoi(value);
  return 0;
}

int
pal_get_board_id(uint8_t slot, uint8_t *req_data, uint8_t req_len, uint8_t *res_data, uint8_t *res_len) {
  int ret;
  uint8_t platform_id  = 0x00;
  uint8_t board_rev_id = 0x00;
  int completion_code=CC_UNSPECIFIED_ERROR;

  ret = pal_get_platform_id(&platform_id);
  if (ret) {
    *res_len = 0x00;
    return completion_code;
  }

  ret = pal_get_board_rev_id(&board_rev_id);
  if (ret) {
    *res_len = 0x00;
    return completion_code;
  }

  // Prepare response buffer
  completion_code = CC_SUCCESS;
  res_data[0] = platform_id;
  res_data[1] = board_rev_id;
  *res_len = 0x02;

  return completion_code;
}

int
pal_convert_to_dimm_str(uint8_t cpu, uint8_t channel, uint8_t slot, char *str) {
  uint8_t idx;
  uint8_t cpu_num;
  char label[] = {'A','C','B','D'};

  cpu_num = cpu%2;

  if ((idx = cpu_num*2+slot) < sizeof(label)) {
    sprintf(str, "%c%d", label[idx], channel);
  } else {
    sprintf(str, "NA");
  }

  return 0;
}

int
pal_handle_dcmi(uint8_t fru, uint8_t *request, uint8_t req_len, uint8_t *response, uint8_t *rlen) {
  NM_RW_INFO info;

  info.bus = NM_IPMB_BUS_ID;
  info.nm_addr = NM_SLAVE_ADDR;

  return lib_dcmi_wrapper(&info, request, req_len, response, rlen);
}

static void
update_bios_version_active()
{
  uint8_t tmp[64] = {0};
  uint8_t ver[64] = {0};
  uint8_t fruId;
  char *fru_name = "mb";
  int end;

  pal_get_fru_id(fru_name, &fruId);
  if (!pal_get_sysfw_ver(fruId, tmp)) {
    if ((end = 3+tmp[2]) > (int)sizeof(tmp)) {
      end = sizeof(tmp);
    }
    memcpy(ver, tmp+3, end-3);
    kv_set("cur_mb_bios_ver", (char *)ver, 0, KV_FPERSIST);
  }
}

void
pal_set_post_end(uint8_t slot, uint8_t *req_data, uint8_t *res_data, uint8_t *res_len)
{
  // update bios version active
  update_bios_version_active();

  // log the post end event
  syslog (LOG_INFO, "POST End Event for Payload#%d\n", slot);

  // Sync time with system
  if (system("/etc/init.d/sync_date.sh &") != 0) {
    syslog(LOG_ERR, "Sync date failed!\n");
  } else {
    syslog(LOG_INFO, "Sync date success!\n");
  }
}

int
pal_get_sensor_util_timeout(uint8_t fru) {

  if ( fru == FRU_MB ) {
    return 10;
  } else {
    return 4;
  }
}

int pal_get_fru_capability(uint8_t fru, unsigned int *caps)
{
  if (!caps) {
    syslog(LOG_WARNING, "%s() Capability pointer is NULL.", __func__);
    return -1;
  }

  if (fru >= FRU_CNT) {
    return -1;
  }

  if (pal_is_artemis()) {
    switch (fru) {
      case FRU_SWB:
      case FRU_HGX:
      case FRU_OCPDBG:
      case FRU_HSC:
      case FRU_SHSC:
        *caps = 0; // Not in Artemis
        break;
      default:
        *caps = fru_dev_data[fru].cap;
        break;
    }
  } else {
    switch (fru) {
      case FRU_ACB:
      case FRU_MEB:
      case FRU_ACB_ACCL1:
      case FRU_ACB_ACCL2:
      case FRU_ACB_ACCL3:
      case FRU_ACB_ACCL4:
      case FRU_ACB_ACCL5:
      case FRU_ACB_ACCL6:
      case FRU_ACB_ACCL7:
      case FRU_ACB_ACCL8:
      case FRU_ACB_ACCL9:
      case FRU_ACB_ACCL10:
      case FRU_ACB_ACCL11:
      case FRU_ACB_ACCL12:
        *caps = 0;
        break;
      case FRU_SHSC:
        if (is_swb_hsc_module()) {
          *caps = HSC_CAPABILITY;
        } else {
          *caps = 0;
        }
        break;
      case FRU_HSC:
        if (is_mb_hsc_module()) {
          *caps = HSC_CAPABILITY;
        } else {
          *caps = 0;
        }
        break;
      default:
        *caps = fru_dev_data[fru].cap;
        break;
    }
  }
  return 0;
}

int pal_get_dev_capability(uint8_t fru, uint8_t dev, unsigned int *caps)
{
  return -1;
}

int
pal_get_nm_selftest_result(uint8_t fruid, uint8_t *data)
{
  NM_RW_INFO info;
  uint8_t rbuf[8];
  uint8_t rlen;
  int ret;

  info.bus = NM_IPMB_BUS_ID;
  info.nm_addr = NM_SLAVE_ADDR;
  info.bmc_addr = BMC_DEF_SLAVE_ADDR;

  ret = cmd_NM_get_self_test_result(&info, rbuf, &rlen);
  if (ret != 0) {
    return PAL_ENOTSUP;
  }

  memcpy(data, rbuf, rlen);
#ifdef DEBUG
  syslog(LOG_WARNING, "rbuf[0] =%x rbuf[1] =%x\n", rbuf[0], rbuf[1]);
#endif
  return ret;
}

int pal_get_cpld_version(uint8_t addr, uint8_t bus, uint8_t* rbuf)
{
  int fd = 0, ret = -1;
  uint8_t tlen, rlen;
  uint8_t tbuf[16] = {0};

  fd = i2c_cdev_slave_open(bus, addr >> 1, I2C_SLAVE_FORCE_CLAIM);
  if (fd < 0) {
    syslog(LOG_WARNING, "%s() Failed to open %d", __func__, bus);
    return ret;
  }

  tbuf[0] = 0xC0;
  tbuf[1] = 0x00;
  tbuf[2] = 0x00;
  tbuf[3] = 0x00;

  tlen = 4;
  rlen = 4;

  ret = i2c_rdwr_msg_transfer(fd, addr, tbuf, tlen, rbuf, rlen);
  i2c_cdev_slave_close(fd);

  if (ret == -1) {
    syslog(LOG_WARNING, "%s bus=%x slavaddr=%x \n", __func__, bus, addr >> 1);
    return ret;
  }

  return 0;
}

int
pal_get_fw_info(uint8_t fru, unsigned char target, unsigned char* res, unsigned char* res_len) {
  int ret = -1;
  uint8_t rbuf[16] = {0};

  if( fru != FRU_MB )
    return -1;

  switch (target) {
    case CMD_GET_MAIN_CPLD_VER:
      ret = pal_get_cpld_version(MAIN_CPLD_SLV_ADDR, MAIN_CPLD_BUS_NUM, rbuf);
    break;
    default:
      return -1;
  }

  if( ret == 0 ) {
    memcpy(res, rbuf, 4);
    *res_len = 4;
  }

  return ret;
}

// IPMI OEM Command
// netfn: NETFN_OEM_1S_REQ (0x30)
// command code: CMD_OEM_BYPASS_CMD (0x34)
int
pal_bypass_cmd(uint8_t slot, uint8_t *req_data, uint8_t req_len, uint8_t *res_data, uint8_t *res_len) {
  int completion_code = CC_SUCCESS;
  uint8_t tlen = 0;
  uint8_t tbuf[MAX_IPMB_REQ_LEN] = {0};
  uint8_t rbuf[MAX_IPMB_RES_LEN] = {0};
  NCSI_NL_MSG_T *msg = NULL;
  NCSI_NL_RSP_T *rsp = NULL;
  bypass_ncsi_req ncsi_req = {0};

  if (req_data == NULL) {
    syslog(LOG_WARNING, "%s(): NULL request data, can not bypass the command", __func__);
    return CC_INVALID_PARAM;
  }
  if (res_data == NULL) {
    syslog(LOG_WARNING, "%s(): NULL response data, can not bypass the command", __func__);
    return CC_INVALID_PARAM;
  }
  if (res_len == NULL) {
    syslog(LOG_WARNING, "%s(): NULL response length, can not bypass the command", __func__);
    return CC_INVALID_PARAM;
  }
  *res_len = 0;

  memset(tbuf, 0, sizeof(tbuf));
  memset(rbuf, 0, sizeof(rbuf));

  switch (((bypass_cmd*)req_data)->target) {
    case BYPASS_NCSI:
      if ((req_len < sizeof(bypass_ncsi_header)) || (req_len > sizeof(NCSI_NL_MSG_T))) {
        completion_code = CC_INVALID_LENGTH;
        break;
      }
      tlen = req_len - sizeof(bypass_ncsi_header);
      msg = calloc(1, sizeof(NCSI_NL_MSG_T));
      if (msg == NULL) {
        syslog(LOG_ERR, "%s(): failed msg buffer allocation", __func__);
        completion_code = CC_UNSPECIFIED_ERROR;
        break;
      }
      memset(&ncsi_req, 0, sizeof(ncsi_req));
      memcpy(&ncsi_req, &((bypass_cmd*)req_data)->data[0], sizeof(ncsi_req));
      memset(msg, 0, sizeof(*msg));
      sprintf(msg->dev_name, "eth%d", ncsi_req.netdev);

      msg->channel_id = ncsi_req.channel;
      msg->cmd = ncsi_req.cmd;
      msg->payload_length = tlen;
      memcpy(&msg->msg_payload[0], &ncsi_req.data[0], NCSI_MAX_PAYLOAD);

      rsp = send_nl_msg_libnl(msg);
      if (rsp != NULL) {
        memcpy(&res_data[0], &rsp->msg_payload[0], rsp->hdr.payload_length);
        *res_len = rsp->hdr.payload_length;
      } else {
        completion_code = CC_UNSPECIFIED_ERROR;
      }
      break;

    default:
      completion_code = CC_NOT_SUPP_IN_CURR_STATE;
      break;
  }

  if (msg != NULL) {
    free(msg);
  }
  if (rsp != NULL) {
    free(rsp);
  }

  return completion_code;
}

int
pal_parse_oem_sel(uint8_t fru, uint8_t *sel, char *error_log)
{
  uint8_t index = sel[11] & 0x01;
  char* yellow_track[] = {
    "Corrected Cache Error",
    "Persistent Cache Fault"
  };

  // error_log size 256
  snprintf(error_log, 256, "%s", yellow_track[index]);

  return 0;
}
bool
pal_is_support_vr_delay_activate(void){
  return true;
}
int
pal_postcode_select(int option) {
  int mmap_fd;
  uint32_t ctrl;
  void *reg_base;
  void *reg_offset;

  mmap_fd = open("/dev/mem", O_RDWR | O_SYNC);
  if (mmap_fd < 0) {
    return -1;
  }

  reg_base = mmap(NULL, PAGE_SIZE, PROT_READ|PROT_WRITE, MAP_SHARED, mmap_fd, AST_GPIO_BASE);
  reg_offset = (uint8_t *)reg_base + GPION_COMMON_SOURCE_OFFSET;
  ctrl = *(volatile uint32_t *)reg_offset;

  switch(option) {
    case POSTCODE_BY_BMC:   // POST code LED controlled by BMC
      ctrl &= ~0x00000100;
      break;
    case POSTCODE_BY_HOST:  // POST code LED controlled by LPC
      ctrl |= 0x00000100;
      break;
    default:
      syslog(LOG_WARNING, "pal_mmap: unknown option");
      break;
  }
  *(volatile uint32_t *)reg_offset = ctrl;

  munmap(reg_base, PAGE_SIZE);
  close(mmap_fd);

  return 0;
}

int
pal_uart_select_led_set(void) {
  static uint32_t pre_channel = 0xFF;
  uint32_t channel;
  uint32_t vals;
  const char *uartsw_pins[] = {
    "FM_UARTSW_MSB_N",
    "FM_UARTSW_LSB_N",
  };
  const char *postcode_pins[] = {
    "LED_POSTCODE_0",
    "LED_POSTCODE_1",
    "LED_POSTCODE_2",
    "LED_POSTCODE_3",
    "LED_POSTCODE_4",
    "LED_POSTCODE_5",
    "LED_POSTCODE_6",
    "LED_POSTCODE_7"
  };

  pal_postcode_select(POSTCODE_BY_BMC);

  if (gpio_get_value_by_shadow_list(uartsw_pins, ARRAY_SIZE(uartsw_pins), &vals)) {
    return -1;
  }
  channel = ~vals & 0x3;  // the GPIOs are active-low, so invert it

  if (channel != pre_channel) {
    // show channel on 7-segment display
    if (gpio_set_value_by_shadow_list(postcode_pins, ARRAY_SIZE(postcode_pins), channel)) {
      return -1;
    }
    pre_channel = channel;
  }

  return 0;
}
