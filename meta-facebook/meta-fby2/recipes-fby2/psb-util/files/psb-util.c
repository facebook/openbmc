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
#include <openbmc/kv.h>
#include <openbmc/pal.h>

#define CTRL_FLAG_DEBUG  0x01

struct psb_util_args_t {
  int fruid;
  uint8_t ctrl_flags;
  uint8_t print_help;
};

static struct psb_util_args_t psb_util_args;

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

static void
print_usage_help(void) {
  printf("Usage: psb-util <all|slot1|slot2|slot3|slot4>\n");
}

static int
print_psb_config(uint8_t slot) {
  struct psb_config_info *pc_info;
  char key[MAX_KEY_LEN] = {0};
  char value[MAX_VALUE_LEN] = {0};
  int  ret;
  size_t len;
  size_t i;

  memset(value, 0, MAX_VALUE_LEN);
  snprintf(key,MAX_KEY_LEN, "slot%d_psb_config_raw", slot);
  ret = kv_get(key, value, &len, 0);
  if (ret != 0) {
    printf("Slot%d psb config cache file not exist: %s\n", slot, key);
    return -1;
  }

  if (psb_util_args.ctrl_flags & CTRL_FLAG_DEBUG) {
    printf("RAW: ");
    for (i=0; i<len; i++) {
      printf("[%02X]", value[i]);
    }
    printf("\n");
  }

  pc_info = (struct psb_config_info*)value;
  printf("PlatformVendorID:             0x%02X\n", pc_info->platform_vendor_id);
  printf("PlatformModelID:              0x%02X\n", pc_info->platform_model_id);
  printf("BiosKeyRevisionID:            0x%02X\n", pc_info->bios_key_revision_id);
  printf("RootKeySelect:                0x%02X\n", pc_info->root_key_select);
  printf("PlatformSecureBootEn:         0x%02X\n", pc_info->platform_secure_boot_en);
  printf("DisableBiosKeyAntiRollback:   0x%02X\n", pc_info->disable_bios_key_antirollback);
  printf("DissableAmdKeyUsage:          0x%02X\n", pc_info->disable_amd_key_usage);
  printf("DissableSecureDebugUnlock:    0x%02X\n", pc_info->disable_secure_debug_unlock);
  printf("CustomerKeyLock:              0x%02X\n", pc_info->customer_key_lock);
  printf("PSBStatus:                    0x%02X\n", pc_info->psb_status);
  printf("PSBFusingReadiness:           0x%02X\n", pc_info->psb_fusing_readiness);
  printf("HSTIStatePSPSecureEn:         0x%02X\n", pc_info->hstistate_psp_secure_en);
  printf("HSTIStatePSPPlatformSecureEn: 0x%02X\n", pc_info->hstistate_psp_platform_secure_en);
  printf("HSTIStatePSPDebugLockOn:      0x%02X\n", pc_info->hstistate_psp_debug_lock_on);

  return 0;
}


int
parse_args(int argc, char **argv) {
  int ret;
  uint8_t fru;
  char fruname[32];
  static struct option long_opts[] = {
    {"help", no_argument, 0, 'h'},
    {"debug", no_argument, 0, 'd'},
    {0,0,0,0},
  };

  psb_util_args.fruid = -1;
  psb_util_args.ctrl_flags = 0;
  psb_util_args.print_help = 0;

  if (argc < 2) {
    printf("Not enough argument\n");
    goto err_exit;
  } else {
    strcpy(fruname, argv[1]);
  }

  while(1) {
    ret = getopt_long(argc, argv, "hd", long_opts, NULL);
    if (ret == -1) {
      break;
    }

    switch (ret) {
      case 'h':
        psb_util_args.print_help = 1;
        return 0;
      case 'd':
        psb_util_args.ctrl_flags |= CTRL_FLAG_DEBUG;
        break;
      default:
        goto err_exit;
    }
  }

  if(pal_get_fru_id(fruname, &fru) != 0 ) {
    psb_util_args.print_help = 1;
    printf("Wrong fru: %s\n", fruname);
    goto err_exit;
  } else if (fru > MAX_NODES) {   // fru > 4
    printf("Wrong fru: %s\n", fruname);
    psb_util_args.print_help = 1;
    goto err_exit;
  } else {
    psb_util_args.fruid = fru;
  }

  return 0;

err_exit:
  return -1;
}


int
do_action(int slot_id) {
  uint8_t status = 0;

  printf("==================================\n");
  printf("Slot%d\n", slot_id);
  printf("==================================\n");

  if (pal_get_server_power(slot_id, &status) != 0) {
    printf("Can not get Slot%d power status\n", slot_id);
    goto err_exit;
  } else if(status != SERVER_POWER_ON) {
    printf("Slot%d is not POWER ON\n", slot_id);
    goto err_exit;
  } 
    
  return print_psb_config(slot_id);

err_exit:
  return -1;
}


int
main(int argc, char **argv) {
  int ret = 0;
  uint8_t fru;

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
      ret |= do_action(i);
      printf("\n");
    }
  } else {
    ret = do_action(psb_util_args.fruid);
  }

  return ret;
}
