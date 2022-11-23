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


#if !defined(PLATFORM_NAME)
  #define PLATFORM_NAME "grandteton"
#endif

#define GUID_SIZE 16
#define OFFSET_SYS_GUID 0x17F0
#define OFFSET_DEV_GUID 0x1800


#define NUM_SERVER_FRU  1
#define NUM_NIC_FRU     2
#define NUM_BMC_FRU     1

const char pal_fru_list[] = \
"all, mb, nic0, nic1, swb, hmc, bmc, scm, vpdb, hpdb, fan_bp0, fan_bp1, fio, hsc, swb_hsc";

const char pal_server_list[] = "mb";


#define ALL_CAPABILITY  FRU_CAPABILITY_SENSOR_READ | FRU_CAPABILITY_SENSOR_HISTORY

#define MB_CAPABILITY   FRU_CAPABILITY_FRUID_ALL | FRU_CAPABILITY_SERVER |  \
                        FRU_CAPABILITY_SENSOR_ALL | FRU_CAPABILITY_POWER_ALL

#define SWB_CAPABILITY  FRU_CAPABILITY_FRUID_ALL | FRU_CAPABILITY_SENSOR_ALL

#define HMC_CAPABILITY  FRU_CAPABILITY_FRUID_ALL | FRU_CAPABILITY_SENSOR_ALL

#define NIC_CAPABILITY  FRU_CAPABILITY_FRUID_ALL | FRU_CAPABILITY_SENSOR_ALL | \
                        FRU_CAPABILITY_NETWORK_CARD

#define BMC_CAPABILITY  FRU_CAPABILITY_FRUID_ALL | FRU_CAPABILITY_MANAGEMENT_CONTROLLER

#define SCM_CAPABILITY  FRU_CAPABILITY_FRUID_ALL | FRU_CAPABILITY_SENSOR_ALL

#define PDB_CAPABILITY  FRU_CAPABILITY_FRUID_ALL | FRU_CAPABILITY_SENSOR_ALL

#define BP_CAPABILITY   FRU_CAPABILITY_FRUID_ALL | FRU_CAPABILITY_SENSOR_ALL

#define FIO_CAPABILITY  FRU_CAPABILITY_FRUID_ALL

#define HSC_CAPABILITY  FRU_CAPABILITY_FRUID_ALL | FRU_CAPABILITY_SENSOR_ALL


struct fru_dev_info {
  uint8_t fru_id;
  char *name;
  char *desc;
  uint8_t bus;
  uint8_t addr;
  uint32_t cap;
  uint8_t path;
  bool (*check_presence) (void);
};

enum {
  FRU_PATH_NONE,
  FRU_PATH_EEPROM,
  FRU_PATH_PLDM,
};

struct fru_dev_info fru_dev_data[] = {
  {FRU_ALL,   "all",     NULL,            0,  0,    ALL_CAPABILITY, FRU_PATH_NONE,   NULL },
  {FRU_MB,    "mb",      "Mother Board",  33, 0x51, MB_CAPABILITY,  FRU_PATH_EEPROM, fru_presence},
  {FRU_SWB,   "swb",     "Switch Board",  3,  0x20, SWB_CAPABILITY, FRU_PATH_PLDM,   swb_presence},
  {FRU_HMC,   "hmc",     "HMC Board"  ,   9,  0x53,  HMC_CAPABILITY, FRU_PATH_EEPROM,   hgx_presence},
  {FRU_NIC0,  "nic0",    "Mezz Card 0",   13, 0x50, NIC_CAPABILITY, FRU_PATH_EEPROM, fru_presence},
  {FRU_NIC1,  "nic1",    "Mezz Card 1",   4,  0x52, NIC_CAPABILITY, FRU_PATH_EEPROM, fru_presence},
  {FRU_DBG,   "ocpdbg",  "Debug Board",   14, 0,    0,              FRU_PATH_NONE,   NULL},
  {FRU_BMC,   "bmc",     "BSM Board",     15, 0x56, BMC_CAPABILITY, FRU_PATH_EEPROM, fru_presence},
  {FRU_SCM,   "scm",     "SCM Board",     15, 0x50, SCM_CAPABILITY, FRU_PATH_EEPROM, fru_presence},
  {FRU_VPDB,  "vpdb",    "VPDB Board",    36, 0x52, PDB_CAPABILITY, FRU_PATH_EEPROM, fru_presence},
  {FRU_HPDB,  "hpdb",    "HPDB Board",    37, 0x51, PDB_CAPABILITY, FRU_PATH_EEPROM, fru_presence},
  {FRU_FAN_BP0,   "fan_bp0",     "FAN_BP0 Board",     40, 0x56, BP_CAPABILITY,  FRU_PATH_EEPROM, fru_presence},
  {FRU_FAN_BP1,   "fan_bp1",     "FAN_BP1 Board",     41, 0x56, BP_CAPABILITY,  FRU_PATH_EEPROM, fru_presence},
  {FRU_FIO,   "fio",     "FIO Board",     3,  0x20, FIO_CAPABILITY, FRU_PATH_PLDM,   swb_presence},
  {FRU_HSC,   "hsc",     "HSC Board",     2,  0x51, 0,              FRU_PATH_EEPROM, fru_presence},
  {FRU_SHSC,  "swb_hsc", "SWB HSC Board", 3,  0x20, 0,              FRU_PATH_PLDM,   swb_presence},
};

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

  if (fru >= FRU_CNT || fru_dev_data[fru].check_presence == NULL) {
    *status = 0;
  } else {
    *status = fru_dev_data[fru].check_presence();
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

  switch (fru) {
    case FRU_SWB:
     return hal_write_pldm_fruid(0, path);

    case FRU_FIO:
     return hal_write_pldm_fruid(1, path);

    case FRU_SHSC:
     return hal_write_pldm_fruid(2, path);

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
  if (fru > MAX_NUM_FRUS)
    return -1;

  if(fru == FRU_SHSC && is_swb_hsc_module()) {
    *caps = HSC_CAPABILITY;
  } else if(fru == FRU_HSC && is_mb_hsc_module()) {
    *caps = HSC_CAPABILITY;
  } else {
    *caps = fru_dev_data[fru].cap;
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
