/*
 * psb-util
 *
 * Copyright 2015-present Facebook. All Rights Reserved.
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
#include <syslog.h>
#include <stdint.h>
#include <string.h>
#include <getopt.h>
#include <stdbool.h>
#include <fcntl.h>
#include <iostream>
#include <nlohmann/json.hpp>
#include <openbmc/kv.h>
#include <openbmc/pal.h>
#include <CLI/CLI.hpp>

#define PSB_ENABLE_READ_CACHE  0

using nlohmann::json;

extern int plat_read_psb_eeprom(uint8_t fru, uint8_t cpu, uint8_t *rbuf, size_t rsize);

struct psb_util_args_t {
  uint8_t fruid;
  bool debug;
  bool print_help;
  bool json_fmt;
};

static struct psb_util_args_t psb_util_args = {FRU_ALL, 0, false, false};

struct psb_config_info {
  uint8_t platform_vendor_id;                 // Byte[0] : 0-7

  uint8_t platform_model_id : 4;              // Byte[1] : 0-3
  uint8_t bios_key_revision_id : 4;           // Byte[1] : 4-7

  uint8_t root_key_select : 4;                // Byte[2] : 0-3
  uint8_t reserved_byte2_4_7 : 4;             // Byte[2] : 4-7

  uint8_t platform_secure_boot_en : 1;        // Byte[3] : 0
  uint8_t disable_bios_key_antirollback : 1;  // Byte[3] : 1
  uint8_t disable_amd_key_usage : 1;          // Byte[3] : 2
  uint8_t disable_secure_debug_unlock : 1;    // Byte[3] : 3
  uint8_t customer_key_lock : 1;              // Byte[3] : 4
  uint8_t reserved_byte3_5_7 : 3;             // Byte[3] : 5-7

  uint8_t psb_status;                         // Byte[4] : 0-7

  uint8_t psb_fusing_readiness : 1;           // Byte[5] : 0
  uint8_t reserved_byte5_1_7 : 7;             // Byte[5] : 1-7

  uint8_t reserved_byte6_0_7;                 // Byte[6] : 0-7

  uint8_t reserved_byte7_0_3 : 4;               // Byte[7] : 0-3
  uint8_t hstistate_psp_secure_en : 1;          // Byte[7] : 4
  uint8_t hstistate_psp_platform_secure_en : 1; // Byte[7] : 5
  uint8_t hstistate_psp_debug_lock_on : 1;      // Byte[7] : 6
  uint8_t reserved_byte7_7_7 : 1;               // Byte[7] : 7
} __attribute__((packed));

struct psb_eeprom {
  uint16_t                bios_debug_data;
  struct psb_config_info  psb_config;
  uint8_t                 checksum;
} __attribute__((packed));

enum {
  PSB_SUCCESS = 0,
  PSB_ERROR_GET_SRV_PRST,
  PSB_ERROR_GET_SRV_PWR,
  PSB_ERROR_SRV_NOT_PRST,
  PSB_ERROR_SRV_NOT_READY,
  PSB_ERROR_KV_NOT_AVAIL,
  PSB_ERROR_READ_EEPROM_FAIL,
  PSB_ERROR_INVAL_DATA,
  PSB_ERROR_UNKNOW,
};

const char* err_msg[] = {
  "success",                            // PSB_SUCCESS
  "failed to get server present",       // PSB_ERROR_GET_SRV_PRST
  "failed to get server power status",  // PSB_ERROR_GET_SRV_PWR
  "server not present",                 // PSB_ERROR_SRV_NOT_PRST
  "server not ready (12V-off)",         // PSB_ERROR_SRV_NOT_READY
  "kv file not found",                  // PSB_ERROR_KV_NOT_AVAIL
  "read eeprom from slot failed",       // PSB_ERROR_READ_EEPROM_FAIL
  "invalid PSB info",                   // PSB_ERROR_INVAL_DATA
  "unknow error",                       // PSB_ERROR_UNKNOW
};

static const char*
get_psb_err_msg(const int psb_err_code) {
  if (psb_err_code >= PSB_SUCCESS &&
      psb_err_code <= PSB_ERROR_UNKNOW) {
    return err_msg[psb_err_code];
  } else {
    return err_msg[PSB_ERROR_UNKNOW];
  }
}

#if PSB_ENABLE_READ_CACHE
static void
del_psb_cache(uint8_t slot) {
  char key[MAX_KEY_LEN] = {0};

  snprintf(key,MAX_KEY_LEN, "slot%d_psb_config_raw", slot);
  if (kv_del(key, 0) < 0) {
    syslog(LOG_ERR, "%s(): failed to delete PSB cache file: %s", __func__, key);
  }
}

static int
read_psb_from_cache(uint8_t slot, struct psb_config_info *pc_info) {
  int ret;
  char key[MAX_KEY_LEN] = {0};
  char value[MAX_VALUE_LEN] = {0};
  size_t len;

  snprintf(key,MAX_KEY_LEN, "slot%d_psb_config_raw", slot);
  ret = kv_get(key, value, &len, 0);
  if (ret != 0) {
    return PSB_ERROR_KV_NOT_AVAIL;
  }

  memcpy(pc_info, value, sizeof(struct psb_config_info));
  return PSB_SUCCESS;
}

static void
store_psb_to_cache(uint8_t slot, struct psb_config_info *pc_info) {
  char key[MAX_KEY_LEN] = {0};
  size_t len = sizeof(struct psb_config_info);

  snprintf(key,MAX_KEY_LEN, "slot%d_psb_config_raw", slot);

  if (kv_set(key, (uint8_t*)pc_info, len, 0)) {
    syslog(LOG_ERR, "%s(): failed to store psb info to cache file: %s",
      __func__, key);
  }
}
#endif

static uint8_t
cal_checksum8(const uint8_t* data, size_t length) {
  size_t i;
  uint8_t cks = 0;
  for(i=0; i<length; i++) {
    cks -= data[i];
  }
  return cks;
}

static int
read_psb_from_eeprom(uint8_t slot, struct psb_config_info *pc_info) {
  uint8_t rbuf[16] = {0};
  const struct psb_eeprom* eeprom = (const struct psb_eeprom*) rbuf;
  uint8_t cks;

  if (plat_read_psb_eeprom(slot, 0, rbuf, sizeof(struct psb_eeprom))) {
    return PSB_ERROR_READ_EEPROM_FAIL;
  }

  cks = cal_checksum8((const uint8_t*)&eeprom->psb_config, sizeof(struct psb_config_info));
  if (cks != eeprom->checksum) {
    syslog(LOG_ERR, "%s(): Invalid data from eeprom, checksum mismatch! calculate: 0x%02x, get: %02x",
      __func__, cks, eeprom->checksum);
    return PSB_ERROR_INVAL_DATA;
  }

  memcpy(pc_info, (rbuf+2), sizeof(struct psb_config_info));
  return PSB_SUCCESS;
}

static int
read_psb_config(uint8_t slot, struct psb_config_info *pc_info) {
  int  ret;
  uint8_t status = 0;

  if (pal_is_fru_prsnt(slot, &status) != 0) {
    return PSB_ERROR_GET_SRV_PRST;
  } else if(!status) {
    return PSB_ERROR_SRV_NOT_PRST;
  }

  if (pal_get_server_power(slot, &status) != 0) {
    return PSB_ERROR_GET_SRV_PWR;
  } else if( status == SERVER_12V_OFF) {
    return PSB_ERROR_SRV_NOT_READY;
  }

#if PSB_ENABLE_READ_CACHE
  ret = read_psb_from_cache(slot, pc_info);
  if (ret == PSB_SUCCESS) {
    return ret;
  }
#endif

  ret = read_psb_from_eeprom(slot, pc_info);
  return ret;
}

static void
print_psb_config(const struct psb_config_info *psb_info) {
  
  printf("PlatformVendorID:             0x%02X\n", psb_info->platform_vendor_id);
  printf("PlatformModelID:              0x%02X\n", psb_info->platform_model_id);
  printf("BiosKeyRevisionID:            0x%02X\n", psb_info->bios_key_revision_id);
  printf("RootKeySelect:                0x%02X\n", psb_info->root_key_select);
  printf("PlatformSecureBootEn:         0x%02X\n", psb_info->platform_secure_boot_en);
  printf("DisableBiosKeyAntiRollback:   0x%02X\n", psb_info->disable_bios_key_antirollback);
  printf("DisableAmdKeyUsage:           0x%02X\n", psb_info->disable_amd_key_usage);
  printf("DisableSecureDebugUnlock:     0x%02X\n", psb_info->disable_secure_debug_unlock);
  printf("CustomerKeyLock:              0x%02X\n", psb_info->customer_key_lock);
  printf("PSBStatus:                    0x%02X\n", psb_info->psb_status);
  printf("PSBFusingReadiness:           0x%02X\n", psb_info->psb_fusing_readiness);
  printf("HSTIStatePSPSecureEn:         0x%02X\n", psb_info->hstistate_psp_secure_en);
  printf("HSTIStatePSPPlatformSecureEn: 0x%02X\n", psb_info->hstistate_psp_platform_secure_en);
  printf("HSTIStatePSPDebugLockOn:      0x%02X\n", psb_info->hstistate_psp_debug_lock_on);
}

static json
get_psb_config_json(const uint8_t fru_id, const char* fru_name, const struct psb_config_info *psb_info) {
  json obj = json::object();
  obj["fru id"] = int(fru_id);
  obj["fru name"] = fru_name;
  obj["PlatformVendorID"] = psb_info->platform_vendor_id;
  obj["PlatformModelID"] = psb_info->platform_model_id;
  obj["BiosKeyRevisionID"] = psb_info->bios_key_revision_id;
  obj["RootKeySelect"] = psb_info->root_key_select;
  obj["PlatformSecureBootEn"] = psb_info->platform_secure_boot_en;
  obj["DisableBiosKeyAntiRollback"] = psb_info->disable_bios_key_antirollback;
  obj["DisableAmdKeyUsage"] = psb_info->disable_amd_key_usage;
  obj["DisableSecureDebugUnlock"] = psb_info->disable_secure_debug_unlock;
  obj["CustomerKeyLock"] = psb_info->customer_key_lock;
  obj["PSBStatus"] = psb_info->psb_status;
  obj["PSBFusingReadiness"] = psb_info->psb_fusing_readiness;
  obj["HSTIStatePSPSecureEn"] = psb_info->hstistate_psp_secure_en;
  obj["HSTIStatePSPPlatformSecureEn"] = psb_info->hstistate_psp_platform_secure_en;
  obj["HSTIStatePSPDebugLockOn"] = psb_info->hstistate_psp_debug_lock_on;
  return obj;
}

int
do_action(uint8_t fru_id, json *jarray) {
  int ret;
  struct psb_config_info psb_info;
  char fru_name[32];

  if (pal_get_fru_name(fru_id, fru_name)) {
    sprintf(fru_name, "fru%d", fru_id);
  }

  if (!jarray) {
    printf("==================================\n");
    printf("%s\n", fru_name);
    printf("==================================\n");
  }

  ret = read_psb_config(fru_id, &psb_info);
  if (ret != PSB_SUCCESS) {
    if (!jarray) {
      printf("error msg: %s\n", get_psb_err_msg(ret));
    } else {
      json obj = json::object();
      obj["fru id"] = +fru_id;
      obj["fru name"] = fru_name;
      obj["error msg"] = get_psb_err_msg(ret);
      (*jarray).push_back(obj);
    }
  } else {
    if (!jarray) {
      print_psb_config(&psb_info);
    } else {
      (*jarray).push_back(get_psb_config_json(fru_id, fru_name, &psb_info));
    }
  }

  return ret;
}

std::set<std::string> get_allowed_frus() {
  std::set<std::string> ret{"all"};
  for (uint8_t fru = 1; fru <= pal_get_fru_count(); fru++) {
    unsigned int caps;
    if (pal_get_fru_capability(fru, &caps)) {
      continue;
    }
    if (!(caps & FRU_CAPABILITY_SERVER)) {
      continue;
    }
    char name[128];
    if (pal_get_fru_name(fru, name)) {
      continue;
    }
    ret.insert(std::string(name));
  }
  return ret;
}

int
main(int argc, char *argv[]) {
  int ret = 0;
  json array = json::array();
  std::set<std::string> allowed_frus = get_allowed_frus();
  CLI::App app("PSB Utility");
  app.failure_message(CLI::FailureMessage::help);
  std::string fru{};
  app.add_option("fru", fru, "FRU to show PSB")->check(CLI::IsMember(allowed_frus))->required();
  app.add_flag("--json", psb_util_args.json_fmt, "Print output as JSON formatted");
  app.add_flag("-d,--debug", psb_util_args.debug, "Enable Debug");

  CLI11_PARSE(app, argc, argv);
  if (fru == "all") {
    psb_util_args.fruid = FRU_ALL;
  } else if (pal_get_fru_id((char *)fru.c_str(), &psb_util_args.fruid)) {
    std::cerr << "Could not get FRUID For " << fru << std::endl;
    return -1;
  }

  if (psb_util_args.fruid == FRU_ALL) {
    for (uint8_t i = 1; i <= pal_get_fru_count(); i++) {
      unsigned int caps;
      if (pal_get_fru_capability(i, &caps) || 
          !(caps & FRU_CAPABILITY_SERVER)) {
        continue;
      }
      ret |= do_action(i, (psb_util_args.json_fmt)?&array:NULL);
      if (!psb_util_args.json_fmt) {
        std::cout << std::endl;
      }
    }
  } else {
    ret = do_action(psb_util_args.fruid, (psb_util_args.json_fmt)?&array:NULL);
  }

  if (psb_util_args.json_fmt) {
    std::cout << array.dump(4) << std::endl;
  }
  return ret;
}
