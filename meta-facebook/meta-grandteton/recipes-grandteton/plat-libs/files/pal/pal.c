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
#include "pal.h"

#define GT_PLATFORM_NAME "grandteton"

#define GUID_SIZE 16
#define OFFSET_SYS_GUID 0x17F0
#define OFFSET_DEV_GUID 0x1800


#define NUM_SERVER_FRU  1
#define NUM_NIC_FRU     2
#define NUM_BMC_FRU     1

const char pal_fru_list[] = \
"all, mb, nic0, nic1, swb, bmc, scm, pdbv, pdbh, bp0, bp1, fio,\
 fan0, fan1, fan2, fan3, fan4, fan5, fan6, fan7, \
 fan8, fan9, fan10, fan11, fan12, fan13, fan14, fan15";

const char pal_server_list[] = "mb";


#define MB_CAPABILITY   FRU_CAPABILITY_FRUID_ALL | FRU_CAPABILITY_SERVER |  \
                        FRU_CAPABILITY_SENSOR_ALL | FRU_CAPABILITY_POWER_ALL

#define SWB_CAPABILITY  FRU_CAPABILITY_FRUID_ALL | FRU_CAPABILITY_SENSOR_ALL

#define NIC_CAPABILITY  FRU_CAPABILITY_FRUID_ALL | FRU_CAPABILITY_SENSOR_ALL | \
                        FRU_CAPABILITY_NETWORK_CARD

#define OCP_CAPABILITY  FRU_CAPABILITY_SENSOR_ALL

#define BMC_CAPABILITY  FRU_CAPABILITY_FRUID_ALL | FRU_CAPABILITY_SENSOR_ALL | \
                        FRU_CAPABILITY_MANAGEMENT_CONTROLLER

#define SCM_CAPABILITY  FRU_CAPABILITY_FRUID_ALL | FRU_CAPABILITY_MANAGEMENT_CONTROLLER

#define PDB_CAPABILITY  FRU_CAPABILITY_FRUID_ALL | FRU_CAPABILITY_SENSOR_ALL

#define BP_CAPABILITY	FRU_CAPABILITY_FRUID_ALL | FRU_CAPABILITY_SENSOR_ALL

#define FIO_CAPABILITY  FRU_CAPABILITY_FRUID_ALL | FRU_CAPABILITY_SENSOR_ALL

#define FAN_CAPABILITY  FRU_CAPABILITY_FRUID_ALL | FRU_CAPABILITY_SENSOR_ALL


struct fru_dev_info {
  uint8_t fru_id;
  char *name;
  char *desc;
  uint8_t bus;
  uint8_t addr;
  uint32_t cap;
};

struct fru_dev_info fru_dev_data[] = {
  {FRU_ALL,   "all",    NULL,           0,  0, 0},
  {FRU_MB,    "mb",     "Mother Board", 33, 0x51, MB_CAPABILITY},
  {FRU_SWB,   "swb",    "Switch Board", 3,  0x20, SWB_CAPABILITY},
  {FRU_NIC0,  "nic0",   "Mezz Card 0",  13, 0x50, NIC_CAPABILITY},
  {FRU_NIC1,  "nic1",   "Mezz Card 1",  4,  0x52, NIC_CAPABILITY},
  {FRU_DBG,   "ocpdbg", "Debug Board",  14, 0,    OCP_CAPABILITY},
  {FRU_BMC,   "bmc",    "BSM Board",    15, 0x56, BMC_CAPABILITY},
  {FRU_SCM,   "scm",    "SCM Board",    15, 0x50, SCM_CAPABILITY},
  {FRU_PDBV,  "pdbv",   "PDBV Board",   36, 0x52, PDB_CAPABILITY},
  {FRU_PDBH,  "pdbh",   "PDBH Board",   37, 0x51, PDB_CAPABILITY},
  {FRU_BP0,   "bp0",    "BP0 Board",    40, 0x56, BP_CAPABILITY},
  {FRU_BP1,   "bp1",    "BP1 Board",    41, 0x56, BP_CAPABILITY},
  {FRU_FIO,   "fio",    "FIO Board",    3,  0x20, FIO_CAPABILITY},
  {FRU_FAN0,  "fan0",   "FAN0 Board",   42, 0x50, FAN_CAPABILITY},
  {FRU_FAN1,  "fan1",   "FAN1 Board",   43, 0x50, FAN_CAPABILITY},
  {FRU_FAN2,  "fan2",   "FAN2 Board",   44, 0x50, FAN_CAPABILITY},
  {FRU_FAN3,  "fan3",   "FAN3 Board",   45, 0x50, FAN_CAPABILITY},
  {FRU_FAN4,  "fan4",   "FAN4 Board",   46, 0x50, FAN_CAPABILITY},
  {FRU_FAN5,  "fan5",   "FAN5 Board",   47, 0x50, FAN_CAPABILITY},
  {FRU_FAN6,  "fan6",   "FAN6 Board",   48, 0x50, FAN_CAPABILITY},
  {FRU_FAN7,  "fan7",   "FAN7 Board",   49, 0x50, FAN_CAPABILITY},
  {FRU_FAN8,  "fan8",   "FAN8 Board",   50, 0x50, FAN_CAPABILITY},
  {FRU_FAN9,  "fan9",   "FAN9 Board",   51, 0x50, FAN_CAPABILITY},
  {FRU_FAN10, "fan10",  "FAN10 Board",  52, 0x50, FAN_CAPABILITY},
  {FRU_FAN11, "fan11",  "FAN11 Board",  53, 0x50, FAN_CAPABILITY},
  {FRU_FAN12, "fan12",  "FAN12 Board",  54, 0x50, FAN_CAPABILITY},
  {FRU_FAN13, "fan13",  "FAN13 Board",  55, 0x50, FAN_CAPABILITY},
  {FRU_FAN14, "fan14",  "FAN14 Board",  56, 0x50, FAN_CAPABILITY},
  {FRU_FAN15, "fan15",  "FAN15 Board",  57, 0x50, FAN_CAPABILITY},
};

int
pal_get_platform_name(char *name) {
  strcpy(name, GT_PLATFORM_NAME);

  return 0;
}

int
pal_get_num_slots(uint8_t *num) {
  *num = 1;
  return 0;
}

int
pal_is_fru_prsnt(uint8_t fru, uint8_t *status) {
  switch (fru) {
    case FRU_MB:
    case FRU_SWB:
    case FRU_NIC0:
    case FRU_NIC1:
    case FRU_DBG:
    case FRU_BMC:
    case FRU_SCM:
    case FRU_PDBV:
    case FRU_PDBH:
    case FRU_BP0:
    case FRU_BP1:
    case FRU_FIO:
    case FRU_FAN0:
    case FRU_FAN1:
    case FRU_FAN2:
    case FRU_FAN3:
    case FRU_FAN4:
    case FRU_FAN5:
    case FRU_FAN6:
    case FRU_FAN7:
    case FRU_FAN8:
    case FRU_FAN9:
    case FRU_FAN10:
    case FRU_FAN11:
    case FRU_FAN12:
    case FRU_FAN13:
    case FRU_FAN14:
    case FRU_FAN15:
      *status = 1;
      break;
    default:
      *status = 0;
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
  if (fru > MAX_NUM_FRUS)
    return -1;

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
  printf("%s\n", path);
  return 0;
}

int
pal_get_fruid_eeprom_path(uint8_t fru, char *path) {

  if (fru > MAX_NUM_FRUS || fru == FRU_ALL || fru == FRU_SWB || fru == FRU_FIO || fru == FRU_DBG)
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

  switch (fru) {
    case FRU_SWB:
   //  return gt_write_swb_fruid(0, path);

    case FRU_FIO:
   //  return gt_write_swb_fruid(1, path);

    default:
    break;
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
      return I2C_BUS_2; // ME
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

int
pal_set_sysfw_ver(uint8_t slot, uint8_t *ver) {
  int i;
  char str[MAX_VALUE_LEN] = {0};
  char tstr[8] = {0};

  for (i = 0; i < SIZE_SYSFW_VER; i++) {
    sprintf(tstr, "%02x", ver[i]);
    strcat(str, tstr);
  }

  return pal_set_key_value("sysfw_ver_server", str);
}

int
pal_get_sysfw_ver(uint8_t slot, uint8_t *ver) {
  int ret = -1;
  int i, j;
  char str[MAX_VALUE_LEN] = {0};
  char tstr[8] = {0};

  ret = pal_get_key_value("sysfw_ver_server", str);
  if (ret) {
    return ret;
  }

  for (i = 0, j = 0; i < 2*SIZE_SYSFW_VER; i += 2) {
    sprintf(tstr, "%c%c", str[i], str[i+1]);
    ver[j++] = strtol(tstr, NULL, 16);
  }

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

void
pal_set_post_end(uint8_t slot, uint8_t *req_data, uint8_t *res_data, uint8_t *res_len)
{
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
  if (fru > MAX_NUM_FRUS || fru == FRU_ALL)
    return -1;

  *caps = fru_dev_data[fru].cap;
  return 0;
}

int pal_get_dev_capability(uint8_t fru, uint8_t dev, unsigned int *caps)
{
  return -1;
}
