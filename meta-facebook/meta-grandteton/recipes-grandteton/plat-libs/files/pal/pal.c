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
#include <pthread.h>


#ifndef PLATFORM_NAME
#define PLATFORM_NAME "grandteton"
#endif

#define GUID_SIZE 16
#define OFFSET_SYS_GUID 0x17F0
#define OFFSET_DEV_GUID 0x1800


#define NUM_SERVER_FRU  1
#define NUM_NIC_FRU     2
#define NUM_BMC_FRU     1

#define PSB_CONFIG_RAW "slot%d_psb_config_raw"

const char pal_fru_list[] = \
"all, mb, nic0, nic1, swb, hgx, bmc, scm, vpdb, hpdb, fan_bp1, fan_bp2, fio, hsc, swb_hsc, " \
// Artemis fru list
"cb, mc, cb_accl1, cb_accl2, cb_accl3, cb_accl4, cb_accl5, cb_accl6, cb_accl7, cb_accl8, " \
"cb_accl9, cb_accl10, cb_accl11, cb_accl12";

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

#define HSC_CAPABILITY  FRU_CAPABILITY_FRUID_ALL

// Artemis fru capability
#define ACB_CAPABILITY  FRU_CAPABILITY_FRUID_ALL | FRU_CAPABILITY_SENSOR_ALL | FRU_CAPABILITY_POWER_ALL
#define MEB_CAPABILITY  FRU_CAPABILITY_FRUID_ALL | FRU_CAPABILITY_SENSOR_ALL | FRU_CAPABILITY_POWER_ALL
#define MEB_CXL_CAPABILITY  FRU_CAPABILITY_FRUID_ALL | FRU_CAPABILITY_SENSOR_ALL | FRU_CAPABILITY_POWER_ALL
#define ACB_ACCL_CAPABILITY FRU_CAPABILITY_FRUID_ALL | FRU_CAPABILITY_POWER_ALL | FRU_CAPABILITY_HAS_DEVICE

static bool is_fio_handler_running = false;

enum {
  FIO_BUTTON_ACB_12V_OFF = 1,
};

enum {
  ACB_PU10_HSC_ADDR = 0x20,
  ACB_PU11_HSC_ADDR = 0x26,
};

struct fru_dev_info {
  uint8_t fru_id;
  char *name;
  char *desc;
  uint8_t bus;
  uint8_t addr;
  uint32_t cap;
  uint8_t path;
  bool (*check_presence) (uint8_t fru, uint8_t *status);
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
  {FRU_FAN_BP1,   "fan_bp1",     "FAN_BP1 Board",     40, 0x56, BP_CAPABILITY,  FRU_PATH_EEPROM, fru_presence, PLDM_FRU_NOT_SUPPORT},
  {FRU_FAN_BP2,   "fan_bp2",     "FAN_BP2 Board",     41, 0x56, BP_CAPABILITY,  FRU_PATH_EEPROM, fru_presence, PLDM_FRU_NOT_SUPPORT},
  {FRU_FIO,   "fio",     "FIO Board",     3,  0x20, FIO_CAPABILITY, FRU_PATH_PLDM,   fru_presence, PLDM_FRU_FIO},
  {FRU_HSC,   "hsc",     "HSC Board",     HSC_BUS_NUM,  0x51, 0,              FRU_PATH_EEPROM, fru_presence, PLDM_FRU_NOT_SUPPORT},
  {FRU_SHSC,  "swb_hsc", "SWB HSC Board", 3,  0x20, 0,              FRU_PATH_PLDM,   fru_presence, PLDM_FRU_SHSC},
  // Artemis FRU dev data
  {FRU_ACB,        "cb",        "Carrier Board",     ACB_BIC_BUS,   ACB_BIC_ADDR,   ACB_CAPABILITY,       FRU_PATH_PLDM,   fru_presence,    PLDM_FRU_NOT_SUPPORT},
  {FRU_MEB,        "mc",        "Memory Exp Board",  MEB_BIC_BUS,   MEB_BIC_ADDR,   MEB_CAPABILITY,       FRU_PATH_PLDM,   fru_presence,    PLDM_FRU_NOT_SUPPORT},
  {FRU_ACB_ACCL1,  "cb_accl1",  "CB ACCL1 Card",    ACB_BIC_BUS,   ACB_BIC_ADDR,   ACB_ACCL_CAPABILITY,  FRU_PATH_PLDM,   pal_is_pldm_fru_prsnt,  PLDM_FRU_NOT_SUPPORT},
  {FRU_ACB_ACCL2,  "cb_accl2",  "CB ACCL2 Card",    ACB_BIC_BUS,   ACB_BIC_ADDR,   ACB_ACCL_CAPABILITY,  FRU_PATH_PLDM,   pal_is_pldm_fru_prsnt,  PLDM_FRU_NOT_SUPPORT},
  {FRU_ACB_ACCL3,  "cb_accl3",  "CB ACCL3 Card",    ACB_BIC_BUS,   ACB_BIC_ADDR,   ACB_ACCL_CAPABILITY,  FRU_PATH_PLDM,   pal_is_pldm_fru_prsnt,  PLDM_FRU_NOT_SUPPORT},
  {FRU_ACB_ACCL4,  "cb_accl4",  "CB ACCL4 Card",    ACB_BIC_BUS,   ACB_BIC_ADDR,   ACB_ACCL_CAPABILITY,  FRU_PATH_PLDM,   pal_is_pldm_fru_prsnt,  PLDM_FRU_NOT_SUPPORT},
  {FRU_ACB_ACCL5,  "cb_accl5",  "CB ACCL5 Card",    ACB_BIC_BUS,   ACB_BIC_ADDR,   ACB_ACCL_CAPABILITY,  FRU_PATH_PLDM,   pal_is_pldm_fru_prsnt,  PLDM_FRU_NOT_SUPPORT},
  {FRU_ACB_ACCL6,  "cb_accl6",  "CB ACCL6 Card",    ACB_BIC_BUS,   ACB_BIC_ADDR,   ACB_ACCL_CAPABILITY,  FRU_PATH_PLDM,   pal_is_pldm_fru_prsnt,  PLDM_FRU_NOT_SUPPORT},
  {FRU_ACB_ACCL7,  "cb_accl7",  "CB ACCL7 Card",    ACB_BIC_BUS,   ACB_BIC_ADDR,   ACB_ACCL_CAPABILITY,  FRU_PATH_PLDM,   pal_is_pldm_fru_prsnt,  PLDM_FRU_NOT_SUPPORT},
  {FRU_ACB_ACCL8,  "cb_accl8",  "CB ACCL8 Card",    ACB_BIC_BUS,   ACB_BIC_ADDR,   ACB_ACCL_CAPABILITY,  FRU_PATH_PLDM,   pal_is_pldm_fru_prsnt,  PLDM_FRU_NOT_SUPPORT},
  {FRU_ACB_ACCL9,  "cb_accl9",  "CB ACCL9 Card",    ACB_BIC_BUS,   ACB_BIC_ADDR,   ACB_ACCL_CAPABILITY,  FRU_PATH_PLDM,   pal_is_pldm_fru_prsnt,  PLDM_FRU_NOT_SUPPORT},
  {FRU_ACB_ACCL10, "cb_accl10", "CB ACCL10 Card",   ACB_BIC_BUS,   ACB_BIC_ADDR,   ACB_ACCL_CAPABILITY,  FRU_PATH_PLDM,   pal_is_pldm_fru_prsnt,  PLDM_FRU_NOT_SUPPORT},
  {FRU_ACB_ACCL11, "cb_accl11", "CB ACCL11 Card",   ACB_BIC_BUS,   ACB_BIC_ADDR,   ACB_ACCL_CAPABILITY,  FRU_PATH_PLDM,   pal_is_pldm_fru_prsnt,  PLDM_FRU_NOT_SUPPORT},
  {FRU_ACB_ACCL12, "cb_accl12", "CB ACCL12 Card",   ACB_BIC_BUS,   ACB_BIC_ADDR,   ACB_ACCL_CAPABILITY,  FRU_PATH_PLDM,   pal_is_pldm_fru_prsnt,  PLDM_FRU_NOT_SUPPORT},
  {FRU_MEB_JCN1,   "mc_cxl8",   "MC CXL8",          MEB_BIC_BUS,   MEB_BIC_ADDR,   MEB_CXL_CAPABILITY, FRU_PATH_PLDM,   pal_is_pldm_fru_prsnt,  PLDM_FRU_NOT_SUPPORT},
  {FRU_MEB_JCN2,   "mc_cxl7",   "MC CXL7",          MEB_BIC_BUS,   MEB_BIC_ADDR,   MEB_CXL_CAPABILITY, FRU_PATH_PLDM,   pal_is_pldm_fru_prsnt,  PLDM_FRU_NOT_SUPPORT},
  {FRU_MEB_JCN3,   "mc_cxl6",   "MC CXL6",          MEB_BIC_BUS,   MEB_BIC_ADDR,   MEB_CXL_CAPABILITY, FRU_PATH_PLDM,   pal_is_pldm_fru_prsnt,  PLDM_FRU_NOT_SUPPORT},
  {FRU_MEB_JCN4,   "mc_cxl5",   "MC CXL5",          MEB_BIC_BUS,   MEB_BIC_ADDR,   MEB_CXL_CAPABILITY, FRU_PATH_PLDM,   pal_is_pldm_fru_prsnt,  PLDM_FRU_NOT_SUPPORT},
  {FRU_MEB_JCN5,   "mc_ssd4",   "MC SSD4",          MEB_BIC_BUS,   MEB_BIC_ADDR,   FRU_CAPABILITY_POWER_ALL, FRU_PATH_NONE,   pal_is_pldm_fru_prsnt,  PLDM_FRU_NOT_SUPPORT},
  {FRU_MEB_JCN6,   "mc_ssd3",   "MC SSD3",          MEB_BIC_BUS,   MEB_BIC_ADDR,   FRU_CAPABILITY_POWER_ALL, FRU_PATH_NONE,   pal_is_pldm_fru_prsnt,  PLDM_FRU_NOT_SUPPORT},
  {FRU_MEB_JCN7,   "mc_ssd2",   "MC SSD2",          MEB_BIC_BUS,   MEB_BIC_ADDR,   FRU_CAPABILITY_POWER_ALL, FRU_PATH_NONE,   pal_is_pldm_fru_prsnt,  PLDM_FRU_NOT_SUPPORT},
  {FRU_MEB_JCN8,   "mc_ssd1",   "MC SSD1",          MEB_BIC_BUS,   MEB_BIC_ADDR,   FRU_CAPABILITY_POWER_ALL, FRU_PATH_NONE,   pal_is_pldm_fru_prsnt,  PLDM_FRU_NOT_SUPPORT},
  {FRU_MEB_JCN9,   "mc_cxl3",   "MC CXL3",          MEB_BIC_BUS,   MEB_BIC_ADDR,   MEB_CXL_CAPABILITY, FRU_PATH_PLDM,   pal_is_pldm_fru_prsnt,  PLDM_FRU_NOT_SUPPORT},
  {FRU_MEB_JCN10,  "mc_cxl4",   "MC CXL4",          MEB_BIC_BUS,   MEB_BIC_ADDR,   MEB_CXL_CAPABILITY, FRU_PATH_PLDM,   pal_is_pldm_fru_prsnt,  PLDM_FRU_NOT_SUPPORT},
  {FRU_MEB_JCN11,  "mc_cxl1",   "MC CXL1",          MEB_BIC_BUS,   MEB_BIC_ADDR,   MEB_CXL_CAPABILITY, FRU_PATH_PLDM,   pal_is_pldm_fru_prsnt,  PLDM_FRU_NOT_SUPPORT},
  {FRU_MEB_JCN12,  "mc_cxl2",   "MC CXL2",          MEB_BIC_BUS,   MEB_BIC_ADDR,   MEB_CXL_CAPABILITY, FRU_PATH_PLDM,   pal_is_pldm_fru_prsnt,  PLDM_FRU_NOT_SUPPORT},
  {FRU_MEB_JCN13,  "mc_jcn13",  "MC JCN13",         MEB_BIC_BUS,   MEB_BIC_ADDR,   0, FRU_PATH_PLDM,   pal_is_pldm_fru_prsnt,  PLDM_FRU_NOT_SUPPORT},
  {FRU_MEB_JCN14,  "mc_jcn14",  "MC JCN14",         MEB_BIC_BUS,   MEB_BIC_ADDR,   0, FRU_PATH_PLDM,   pal_is_pldm_fru_prsnt,  PLDM_FRU_NOT_SUPPORT},
};

uint8_t
pal_get_pldm_fru_id(uint8_t fru) {
  if (pal_is_artemis()) {
    return fru;
  } else {
    return fru_dev_data[fru].pldm_fru_id;
  }
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
    if (pal_is_artemis() && fru == FRU_FIO) {
      if (!pal_is_pldm_fru_prsnt(fru, status)) {
        syslog(LOG_WARNING, "%s() get FRU:%d present failed", __func__, fru);
        return -1;
      }
    } else {
      if (!fru_dev_data[fru].check_presence(fru, status)) {
        syslog(LOG_WARNING, "%s() get FRU: %d present failed", __func__, fru);
      }
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

  if (fru == FRU_AGGREGATE) {
    return 0; //it's the virtual FRU.
  }

  if (fru >= FRU_CNT) {
    syslog(LOG_WARNING, "%s(): Input fruid %d is invalid.", __func__, fru);
    return -1;
  }

  strcpy(name, fru_dev_data[fru].name);

  return 0;
}

bool
pal_is_aggregate_snr_valid(uint8_t snr_num) {
  char key[MAX_KEY_LEN] = {0};
  char value[MAX_VALUE_LEN] = {0};
  snprintf(key, sizeof(key), "fan_fail_boost");

  switch(snr_num) {
    case AGGREGATE_SENSOR_SYSTEM_AIRFLOW:
      //Check if there are any fan fail
      if (kv_get(key, value, NULL, 0) == 0) {
        return false;
      }
      break;
    default:
      return true;
  }

  return true;
}

void
pal_update_ts_sled() {
  char key[MAX_KEY_LEN] = {0};
  char tstr[MAX_VALUE_LEN] = {0};
  struct timespec ts;

  clock_gettime(CLOCK_REALTIME, &ts);
  sprintf(tstr, "%lld", ts.tv_sec);

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

  bic_intf fru_bic_info = {0};

  fru_bic_info.fru_id = fru;
  pal_get_bic_intf(&fru_bic_info);

  if (!pal_is_artemis()) {
    fru_bic_info.fru_id = pal_get_pldm_fru_id(fru);
  }

  if (pal_get_fru_capability(fru, &caps) == 0 && (caps & FRU_CAPABILITY_FRUID_WRITE) && pal_is_fru_prsnt(fru, &status) == 0) {
    if (status == FRU_PRSNT) {
      return hal_write_pldm_fruid(fru_bic_info, path);
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
          sprintf(value, "%lld", ts.tv_sec);
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

  ret = pal_get_board_rev_id(FRU_MB, &board_rev_id);
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

int
pal_get_fru_capability(uint8_t fru, unsigned int *caps) {
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
      case FRU_SHSC:
        *caps = 0; // Not in Artemis
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
      case FRU_MEB_JCN1:
      case FRU_MEB_JCN2:
      case FRU_MEB_JCN3:
      case FRU_MEB_JCN4:
      case FRU_MEB_JCN5:
      case FRU_MEB_JCN6:
      case FRU_MEB_JCN7:
      case FRU_MEB_JCN8:
      case FRU_MEB_JCN9:
      case FRU_MEB_JCN10:
      case FRU_MEB_JCN11:
      case FRU_MEB_JCN12:
      case FRU_MEB_JCN13:
      case FRU_MEB_JCN14:
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

int pal_get_dev_capability(uint8_t fru, uint8_t dev, unsigned int *caps) {
  if (pal_is_artemis()) {
    if ( fru >= FRU_ACB_ACCL1 && fru <= FRU_ACB_ACCL12 ) {
      if ((dev >= DEV_ID1 && dev <= DEV_ID2)) {
        *caps = FRU_CAPABILITY_FRUID_ALL | FRU_CAPABILITY_SENSOR_ALL;
      } else {
        *caps = 0;
      }
    } else {
      *caps = 0;
    }
    return 0;
  }
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

uint8_t save_psb_config_info_to_kvstore(uint8_t slot, uint8_t* req_data, uint8_t req_len) {
  char key[MAX_KEY_LEN] = {0};
  uint8_t completion_code = CC_UNSPECIFIED_ERROR;

  snprintf(key,MAX_KEY_LEN, PSB_CONFIG_RAW, slot);
  if (kv_set(key, (const char*)req_data, req_len, KV_FPERSIST) == 0) {
    completion_code = CC_SUCCESS;
  }
  return completion_code;
}


uint8_t
pal_set_psb_info(uint8_t slot, uint8_t *req_data, uint8_t req_len, uint8_t *res_data, uint8_t *res_len) {
  uint8_t completion_code = CC_UNSPECIFIED_ERROR;

  if ((slot > 0) && (slot <= MAX_NODES)) {
    completion_code = save_psb_config_info_to_kvstore(
        slot,
        req_data,
        req_len - IPMI_MN_REQ_HDR_SIZE);
    return completion_code;
  } else {
    return CC_PARAM_OUT_OF_RANGE;
  }
}

bool
pal_is_pldm_fru_prsnt(uint8_t fru, uint8_t *status) {
  char key[MAX_KEY_LEN] = {0};
  char value[MAX_VALUE_LEN] = {0};


  if (!status) {
    syslog(LOG_WARNING, "%s() Status pointer is NULL.", __func__);
    return false;
  }

  int ret = 0;
  fru_status pldm_fru_status = {0};

  if (!pal_is_artemis()) {
    *status = FRU_NOT_PRSNT;
    return true;
  }

  snprintf(key, sizeof(key), "fru%d_prsnt", fru);
  if (kv_get(key, value, NULL, 0) == 0) {
    *status = atoi(value);
    return true;
  }

  // FRU on JCN is dual type
  ret = pal_get_pldm_fru_status(fru, JCN_0_1, &pldm_fru_status);

  if (ret == 0) {
    *status = pldm_fru_status.fru_prsnt;
    snprintf(value, sizeof(value), "%d", pldm_fru_status.fru_prsnt);
    kv_set(key, value, 0, 0);
    return true;
  } else {
    return false;
  }
}

int
pal_get_root_fru(uint8_t fru, uint8_t *root) {
  if (!pal_is_artemis()) {
    return PAL_ENOTSUP;
  }
  switch (fru) {
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
      *root = FRU_ACB;
      break;
    case FRU_MEB_JCN1:
    case FRU_MEB_JCN2:
    case FRU_MEB_JCN3:
    case FRU_MEB_JCN4:
    case FRU_MEB_JCN5:
    case FRU_MEB_JCN6:
    case FRU_MEB_JCN7:
    case FRU_MEB_JCN8:
    case FRU_MEB_JCN9:
    case FRU_MEB_JCN10:
    case FRU_MEB_JCN11:
    case FRU_MEB_JCN12:
    case FRU_MEB_JCN13:
    case FRU_MEB_JCN14:
      *root = FRU_MEB;
      break;
    default:
      *root = fru;
      break;
  }
  return PAL_EOK;
}

bool
pal_is_fw_update_ongoing(uint8_t fruid) {
  uint8_t root_fruid = fruid;
  char key[MAX_KEY_LEN];
  char value[MAX_VALUE_LEN] = {0};
  int ret = 0;
  struct timespec ts;

  pal_get_root_fru(fruid, &root_fruid);
  snprintf(key, sizeof(key), "fru%d_fwupd", root_fruid);
  ret = kv_get(key, value, NULL, 0);
  if (ret < 0) {
    return false;
  }

  clock_gettime(CLOCK_MONOTONIC, &ts);
  if (strtoul(value, NULL, 10) > ts.tv_sec) {
    return true;
  }

  return false;
}

int
_pal_kv_set_fw_update_ongoing(uint8_t fruid, uint16_t tmout) {
  char key[MAX_KEY_LEN] = {0};
  char value[MAX_VALUE_LEN] = {0};
  struct timespec ts;

  snprintf(key, sizeof(key), "fru%d_fwupd", fruid);
  clock_gettime(CLOCK_MONOTONIC, &ts);
  ts.tv_sec += tmout;
  snprintf(value, sizeof(value), "%lld", ts.tv_sec);
  if (kv_set(key, value, 0, 0) < 0) {
    return -1;
  }
  return 0;
}

int
pal_set_fw_update_ongoing(uint8_t fruid, uint16_t tmout) {
  uint8_t root_fruid = fruid;

  switch (fruid) {
    case FRU_ACB:
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
      if ( _pal_kv_set_fw_update_ongoing(FRU_ACB, tmout) < 0 ) {
        return -1;
      }
      for (int fru_id = FRU_ACB_ACCL1; fru_id <= FRU_ACB_ACCL12; fru_id ++) {
        if ( _pal_kv_set_fw_update_ongoing(fru_id, tmout) < 0 ) {
          return -1;
        }
      }
      break;
    case FRU_MEB:
    case FRU_MEB_JCN1:
    case FRU_MEB_JCN2:
    case FRU_MEB_JCN3:
    case FRU_MEB_JCN4:
    case FRU_MEB_JCN5:
    case FRU_MEB_JCN6:
    case FRU_MEB_JCN7:
    case FRU_MEB_JCN8:
    case FRU_MEB_JCN9:
    case FRU_MEB_JCN10:
    case FRU_MEB_JCN11:
    case FRU_MEB_JCN12:
    case FRU_MEB_JCN13:
    case FRU_MEB_JCN14:
      if ( _pal_kv_set_fw_update_ongoing(FRU_MEB, tmout) < 0 ) {
        return -1;
      }
      for (int fru_id = FRU_MEB_JCN1; fru_id <= FRU_MEB_JCN14; fru_id ++) {
        if ( _pal_kv_set_fw_update_ongoing(fru_id, tmout) < 0 ) {
          return -1;
        }
      }
      break;
    default:
      pal_get_root_fru(fruid, &root_fruid);
      return _pal_kv_set_fw_update_ongoing(root_fruid, tmout);
  }
  return 0;
}

int
pal_get_pldm_fru_status(uint8_t fru, uint8_t dev_id, fru_status *status) {
  if (!status) {
    syslog(LOG_WARNING, "%s() Status pointer is NULL.", __func__);
    return -1;
  }

  int ret = 0;
  uint8_t txbuf[MAX_TXBUF_SIZE] = {0};
  uint8_t rxbuf[MAX_RXBUF_SIZE] = {0};
  uint8_t txlen = 0;
  size_t  rxlen = 0;
  bic_intf fru_bic_info = {0};
  uint8_t parent_fru_status = 0;

  fru_bic_info.fru_id = fru;
  pal_get_bic_intf(&fru_bic_info);

  // check if parent fru is present
  if ( !fru_presence(fru_bic_info.root_fru, &parent_fru_status) || (parent_fru_status == FRU_NOT_PRSNT)) {
    syslog(LOG_INFO, "%s: FRU: %d, Root FRU: %d is not present", __func__, fru_bic_info.fru_id, fru_bic_info.root_fru);
    return false;
  }

  txbuf[txlen++] = fru_bic_info.fru_id;
  txbuf[txlen++] = dev_id;
  ret = oem_pldm_ipmi_send_recv(fru_bic_info.bus_id, fru_bic_info.bic_eid, NETFN_OEM_1S_REQ,
                                CMD_OEM_1S_GET_ASIC_CARD_STATUS, txbuf, txlen,
                                rxbuf, &rxlen, true);

  if ((rxlen != sizeof(fru_status)/sizeof(uint8_t)) || (ret != 0)) {
    return -1;
  }

  memcpy(status, rxbuf, sizeof(fru_status));
  return ret;
}

void
pal_get_bic_intf(bic_intf *fru_bic_info) {
  switch (fru_bic_info->fru_id) {
    case FRU_SWB:
    case FRU_SHSC:
      fru_bic_info->root_fru = FRU_SWB;
      fru_bic_info->bic_eid = SWB_BIC_EID;
      fru_bic_info->bus_id  = SWB_BUS_ID;
      break;
    case FRU_FIO:
      if (pal_is_artemis()) {
        fru_bic_info->root_fru = FRU_ACB;
        fru_bic_info->bic_eid = ACB_BIC_EID;
        fru_bic_info->bus_id  = ACB_BIC_BUS;
      } else {
        fru_bic_info->root_fru = FRU_SWB;
        fru_bic_info->bic_eid = SWB_BIC_EID;
        fru_bic_info->bus_id  = SWB_BUS_ID;
      }
      break;
    case FRU_MEB:
    case FRU_MEB_JCN1:
    case FRU_MEB_JCN2:
    case FRU_MEB_JCN3:
    case FRU_MEB_JCN4:
    case FRU_MEB_JCN5:
    case FRU_MEB_JCN6:
    case FRU_MEB_JCN7:
    case FRU_MEB_JCN8:
    case FRU_MEB_JCN9:
    case FRU_MEB_JCN10:
    case FRU_MEB_JCN11:
    case FRU_MEB_JCN12:
    case FRU_MEB_JCN13:
    case FRU_MEB_JCN14:
      fru_bic_info->root_fru = FRU_MEB;
      fru_bic_info->bic_eid = MEB_BIC_EID;
      fru_bic_info->bus_id  = MEB_BIC_BUS;
      break;
    case FRU_ACB:
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
      fru_bic_info->root_fru = FRU_ACB;
      fru_bic_info->bic_eid = ACB_BIC_EID;
      fru_bic_info->bus_id  = ACB_BIC_BUS;
      break;
    default:
      break;
  }
  return;
}

static void *
fio_button_handler(void *arg) {
  int ret = 0;
  uint8_t txbuf[MAX_TXBUF_SIZE] = {0};
  uint8_t rxbuf[MAX_RXBUF_SIZE] = {0};
  uint8_t txlen = 0;
  size_t  rxlen = 0;
  uint32_t operation = (uint32_t)arg;

  pthread_detach(pthread_self());

  syslog(LOG_CRIT, "%s Powering off host...", __func__);
  if (pal_set_server_power(FRU_MB, SERVER_POWER_OFF) < 0) {
    syslog(LOG_ERR, "%s Fail to power off host", __func__);
    goto exit;
  }

  switch(operation) {
  case FIO_BUTTON_ACB_12V_OFF:
    txbuf[1] = ACB_PU11_HSC_ADDR;
    txbuf[3] = 0x1;
    txbuf[4] = 0x0;
    txlen = 5;
    syslog(LOG_CRIT, "%s 12V off ACB", __func__);
    ret = oem_pldm_ipmi_send_recv(ACB_BIC_BUS, ACB_BIC_EID, NETFN_APP_REQ,
            CMD_APP_MASTER_WRITE_READ, txbuf, txlen, rxbuf, &rxlen, true);
    break;
  default:
    syslog(LOG_ERR, "%s invalid data %u", __func__, operation);
    goto exit;
  }

  if (ret) {
    syslog(LOG_ERR, "%s Fail to power control ACB ret: %d", __func__, ret);
  }

exit:
  is_fio_handler_running = false;
  pthread_exit(0);
}

int
pal_handle_oem_1s_intr(uint8_t fru, uint8_t *data)
{
  pthread_t tid;
  uint32_t operation = 0;

  if (!pal_is_artemis()) {
    return 0;
  }
  if (data == NULL) {
    syslog(LOG_ERR, "%s null parameter", __func__);
    return -1;
  }

  operation = (uint32_t)(data[2]);
  if (operation != FIO_BUTTON_ACB_12V_OFF) {
    return -1;
  }

  if (is_fio_handler_running) {
    syslog(LOG_WARNING, "%s() another instance is running", __func__);
    return -1;
  }

  is_fio_handler_running = true;

  if (pthread_create(&tid, NULL, fio_button_handler, (void *)operation) < 0) {
    syslog(LOG_WARNING, "%s() pthread create failed", __func__);
    is_fio_handler_running = false;
    return -1;
  }

  return 0;
}

int
pal_handle_oem_1s_dev_power(uint8_t fru, uint8_t *req_data, uint8_t req_len, uint8_t *res_data, uint8_t *res_len) {
  int ret = 0;

  if (!pal_is_artemis()) {
    return CC_NOT_SUPP_IN_CURR_STATE;
  }

  if ((req_data == NULL) || (res_data == NULL) || (res_len == NULL)) {
    syslog(LOG_WARNING, "%s: Failed to handle device power due to parameters are NULL.", __func__);
    return CC_INVALID_PARAM;
  }

  *res_len = 0;

  if ((req_len < 2) || (req_len > 3)) {
    return CC_INVALID_LENGTH;
  }

  // get device power
  if ((req_data[1] == GET_DEV_POWER) && (req_len == 2)) {
    ret = pal_get_fru_power(req_data[0], &res_data[0]);
    if (ret < 0) {
      syslog(LOG_WARNING, "%s: Failed to get device %u power, ret %d", __func__, req_data[0], ret);
      return CC_UNSPECIFIED_ERROR;
    }
    *res_len = 1;

  // set device power
  } else if ((req_data[1] == SET_DEV_POWER) && (req_len == 3)) {
    if ((req_data[2] != DEVICE_POWER_ON) && (req_data[2] != DEVICE_POWER_OFF)) {
      syslog(LOG_WARNING, "%s: Failed to set device power by invalid action: 0x%02X.", __func__, req_data[2]);
      return CC_UNSPECIFIED_ERROR;
    }
    ret = pal_set_fru_power(req_data[0], req_data[2]);
    if (ret < 0) {
      syslog(LOG_WARNING, "%s: Failed to set device %u power, ret %d", __func__, req_data[0], ret);
      return CC_UNSPECIFIED_ERROR;
    }
  } else {
      return CC_UNSPECIFIED_ERROR;
  }

  return CC_SUCCESS;
}

int
pal_get_num_devs(uint8_t fru, uint8_t *num_devs) {
  switch (fru) {
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
      *num_devs = ACCL_DEV_NUM;
      break;
    default:
      *num_devs = 0;
  }
  return PAL_EOK;
}

int
pal_get_dev_name(uint8_t fru, uint8_t dev, char *name) {
  switch (dev) {
    case FRU_ALL:
      strncpy(name, "all", MAX_DEV_NAME);
      break;
    case DEV_ID1:
      strncpy(name, "dev1", MAX_DEV_NAME);
      break;
    case DEV_ID2:
      strncpy(name, "dev2", MAX_DEV_NAME);
      break;
    case FRU_MEB_JCN5:
      strncpy(name, "ssd4", MAX_DEV_NAME);
      break;
    case FRU_MEB_JCN6:
      strncpy(name, "ssd3", MAX_DEV_NAME);
      break;
    case FRU_MEB_JCN7:
      strncpy(name, "ssd2", MAX_DEV_NAME);
      break;
    case FRU_MEB_JCN8:
      strncpy(name, "ssd1", MAX_DEV_NAME);
      break;
    default:
      syslog(LOG_WARNING, "%s() FRU: %u wrong device id: %u", __func__,fru, dev);
      return -1;
  }
  return 0;
}

int
pal_get_dev_id(char *dev_str, uint8_t *dev) {
  if (!strncmp(dev_str, "all", MAX_DEV_NAME)) {
    *dev = FRU_ALL;
  } else if (!strncmp(dev_str, "dev1", MAX_DEV_NAME)) {
    *dev = DEV_ID1;
  } else if (!strncmp(dev_str, "dev2", MAX_DEV_NAME)) {
    *dev = DEV_ID2;
  } else if (!strncmp(dev_str, "ssd4", MAX_DEV_NAME)) {
    *dev = FRU_MEB_JCN5;
  } else if (!strncmp(dev_str, "ssd3", MAX_DEV_NAME)) {
    *dev = FRU_MEB_JCN6;
  } else if (!strncmp(dev_str, "ssd2", MAX_DEV_NAME)) {
    *dev = FRU_MEB_JCN7;
  } else if (!strncmp(dev_str, "ssd1", MAX_DEV_NAME)) {
    *dev = FRU_MEB_JCN8;
  } else {
    syslog(LOG_WARNING, "%s() Wrong device name: %s", __func__, dev_str);
    return -1;
  }
  return 0;
}

int
pal_get_dev_fruid_name(uint8_t fru, uint8_t dev, char *name) {
  char temp_name[MAX_FRUID_NAME] = {0};
  char dev_name[MAX_DEV_NAME] = {0};

  if (pal_get_fruid_name(fru, name) < 0) {
    syslog(LOG_WARNING, "%s() Wrong FRU: %u", __func__, fru);
    return -1;
  }

  if (pal_get_dev_name(fru, dev, dev_name) < 0) {
    syslog(LOG_WARNING, "%s() Wrong device ID: %u", __func__, dev);
    return -1;
  }

  snprintf(temp_name, MAX_FRUID_NAME, "%s %s", name, dev_name);
  strncpy(name, temp_name, MAX_FRUID_NAME);
  return 0;
}

int
pal_get_dev_fruid_path(uint8_t fru, uint8_t dev_id, char *path) {
  char fru_name[MAX_FRUID_NAME] = {0};

  if (pal_get_fru_name(fru, fru_name) < 0) {
    syslog(LOG_WARNING, "%s() Wrong FRU: %u", __func__, fru);
    return -1;
  }
  snprintf(path, MAX_FRUID_PATH_LEN, GTA_FRUID_DEV_PATH, fru_name, dev_id);
  return 0;
}

int
pal_is_dev_prsnt(uint8_t fru, uint8_t dev, uint8_t *status) {
  fru_status dev_status = {0};

  if (!status) {
    syslog(LOG_WARNING, "%s() Pointer is NULL.", __func__);
    return -1;
  }
  if (pal_get_pldm_fru_status(fru, dev, &dev_status) < 0) {
    syslog(LOG_WARNING, "%s() FRU: %u get device id: %u present failed", __func__, fru, dev);
    return -1;
  }
  if (dev_status.fru_prsnt == FRU_PRSNT) {
    *status = FRU_PRSNT;
  } else {
    *status = FRU_NOT_PRSNT;
  }
  return 0;
}
