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
#include <jansson.h>
#include <openbmc/kv.h>
#include <openbmc/pal.h>

#define CTRL_FLAG_DEBUG     0x01
#define CTRL_FLAG_JSON_FMT  0x02

#define PSB_ENABLE_READ_CACHE  0

#ifndef PSB_EEPROM_BUS
#define PSB_EEPROM_BUS 0x05
#endif

struct psb_util_args_t {
  int fruid;
  uint8_t ctrl_flags;
  bool print_help;
  bool json_fmt;
};

static struct psb_util_args_t psb_util_args = {-1, 0, false, false};

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

static void
print_usage_help(void) {
  printf("Usage: psb-util <all|slot1|slot2|slot3|slot4> [Options]\n");
  printf("Options:\n");
  printf("    -h, --help      Print this help message and exit\n");
  printf("    -j, --json      Print PSB info in JSON format\n");
}

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
  int ret;
  uint8_t tbuf[16] = {0};
  uint8_t rbuf[16] = {0};
  uint8_t tlen = 0;
  uint8_t rlen = 0;
  const struct psb_eeprom* eeprom = (const struct psb_eeprom*) rbuf;
  uint8_t cks;
  tbuf[0] = PSB_EEPROM_BUS;             // bus
  tbuf[1] = 0xA0;                       // dev addr
  tbuf[2] = sizeof(struct psb_eeprom);  // read count
  tbuf[3] = 0x00;                       // write data addr (MSB)
  tbuf[4] = 0x00;                       // write data addr (LSB)
  tlen = 5;

  ret = bic_ipmb_wrapper(slot, NETFN_APP_REQ, CMD_APP_MASTER_WRITE_READ, tbuf, tlen, rbuf, &rlen);
  if (ret < 0) {
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

static json_t *
get_psb_config_json(const uint8_t fru_id, const char* fru_name, const struct psb_config_info *psb_info) {
  json_t *obj = json_object();
  json_object_set_new(obj, "fru id", json_integer(fru_id));
  json_object_set_new(obj, "fru name", json_string(fru_name));
  json_object_set_new(obj, "PlatformVendorID", json_integer(psb_info->platform_vendor_id));
  json_object_set_new(obj, "PlatformModelID", json_integer(psb_info->platform_model_id));
  json_object_set_new(obj, "BiosKeyRevisionID", json_integer(psb_info->bios_key_revision_id));
  json_object_set_new(obj, "RootKeySelect", json_integer(psb_info->root_key_select));
  json_object_set_new(obj, "PlatformSecureBootEn", json_integer(psb_info->platform_secure_boot_en));
  json_object_set_new(obj, "DisableBiosKeyAntiRollback", json_integer(psb_info->disable_bios_key_antirollback));
  json_object_set_new(obj, "DisableAmdKeyUsage", json_integer(psb_info->disable_amd_key_usage));
  json_object_set_new(obj, "DisableSecureDebugUnlock", json_integer(psb_info->disable_secure_debug_unlock));
  json_object_set_new(obj, "CustomerKeyLock", json_integer(psb_info->customer_key_lock));
  json_object_set_new(obj, "PSBStatus", json_integer(psb_info->psb_status));
  json_object_set_new(obj, "PSBFusingReadiness", json_integer(psb_info->psb_fusing_readiness));
  json_object_set_new(obj, "HSTIStatePSPSecureEn", json_integer(psb_info->hstistate_psp_secure_en));
  json_object_set_new(obj, "HSTIStatePSPPlatformSecureEn", json_integer(psb_info->hstistate_psp_platform_secure_en));
  json_object_set_new(obj, "HSTIStatePSPDebugLockOn", json_integer(psb_info->hstistate_psp_debug_lock_on));
  return obj;
}


int
parse_args(int argc, char **argv) {
  int ret;
  uint8_t fru;
  char fruname[32];
  static struct option long_opts[] = {
    {"help", no_argument, 0, 'h'},
    {"debug", no_argument, 0, 'd'},
    {"json", no_argument, 0, 'j'},
    {0,0,0,0},
  };

  psb_util_args.fruid = -1;
  psb_util_args.ctrl_flags = 0;
  psb_util_args.print_help = false;
  psb_util_args.json_fmt = false;

  if (argc < 2) {
    printf("Not enough argument\n");
    goto err_exit;
  } else {
    strcpy(fruname, argv[1]);
  }

  while(1) {
    ret = getopt_long(argc, argv, "hdj", long_opts, NULL);
    if (ret == -1) {
      break;
    }

    switch (ret) {
      case 'h':
        psb_util_args.print_help = true;
        return 0;
      case 'd':
        psb_util_args.ctrl_flags |= CTRL_FLAG_DEBUG;
        break;
      case 'j':
        psb_util_args.json_fmt = true;
        break;
      default:
        goto err_exit;
    }
  }

  if(pal_get_fru_id(fruname, &fru) != 0 ) {
    psb_util_args.print_help = true;
    printf("Wrong fru: %s\n", fruname);
    goto err_exit;
  } else if (fru > MAX_NODES) {   // fru > 4
    printf("Wrong fru: %s\n", fruname);
    psb_util_args.print_help = true;
    goto err_exit;
  } else {
    psb_util_args.fruid = fru;
  }

  return 0;

err_exit:
  return -1;
}


int
do_action(uint8_t fru_id, json_t *jarray) {
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
      json_t *err_obj = json_object();
      json_object_set_new(err_obj, "fru id", json_integer(fru_id));
      json_object_set_new(err_obj, "fru name", json_string(fru_name));
      json_object_set_new(err_obj, "error msg", json_string(get_psb_err_msg(ret)));
      json_array_append_new(jarray, err_obj);
    }
  } else {
    if (!jarray) {
      print_psb_config(&psb_info);
    } else {
      json_array_append_new(jarray, get_psb_config_json(fru_id, fru_name, &psb_info));
    }
  }

  return ret;
}

int
main(int argc, char **argv) {
  int ret = 0;
  json_t *array = json_array();

  if (parse_args(argc, argv) != 0) {
    print_usage_help();
    return -1;
  }

  if (psb_util_args.print_help) {
    print_usage_help();
    return 0;
  }

  if (psb_util_args.fruid == FRU_ALL) {
    size_t i;
    for (i=1; i<=MAX_NODES; i++) {
      ret |= do_action(i, (psb_util_args.json_fmt)?array:NULL);
      if (!psb_util_args.json_fmt) {
        printf("\n");
      }
    }
  } else {
    ret = do_action(psb_util_args.fruid, (psb_util_args.json_fmt)?array:NULL);
  }

  if (psb_util_args.json_fmt) {
    json_dumpf(array, stdout, 4);
    printf("\n");
  }

  json_decref(array);
  return ret;
}
