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

static void
print_usage_help(void) {
  printf("Usage: psb-util <slot1|slot2|slot3|slot4> \n");
}

static void
print_psb_config(uint8_t slot) {
  char key[MAX_KEY_LEN] = {0};
  char value[MAX_VALUE_LEN] = {0};
  int  ret;

  memset(value, 0, MAX_VALUE_LEN);
  snprintf(key,MAX_KEY_LEN, "slot%d_platform_vendor_id", slot);
  ret = kv_get(key, value, NULL, 0);
  if (!ret) {
    printf("PlatformVendorID: %s\n", value);
  }

  memset(value, 0, MAX_VALUE_LEN);
  snprintf(key,MAX_KEY_LEN, "slot%d_platform_model_id", slot);
  ret = kv_get(key, value, NULL, 0);
  if (!ret) {
    printf("PlatformModelID: %s\n", value);
  }

  memset(value, 0, MAX_VALUE_LEN);
  snprintf(key,MAX_KEY_LEN, "slot%d_bios_key_revision_id", slot);
  ret = kv_get(key, value, NULL, 0);
  if (!ret) {
    printf("BiosKeyRevisionID: %s\n", value);
  }

  memset(value, 0, MAX_VALUE_LEN);
  snprintf(key,MAX_KEY_LEN, "slot%d_root_key_select", slot);
  ret = kv_get(key, value, NULL, 0);
  if (!ret) {
    printf("RootKeySelect: %s\n", value);
  }

  memset(value, 0, MAX_VALUE_LEN);
  snprintf(key,MAX_KEY_LEN, "slot%d_platform_secure_boot_en", slot);
  ret = kv_get(key, value, NULL, 0);
  if (!ret) {
    printf("PlatformSecureBootEn: %s\n", value);
  }

  memset(value, 0, MAX_VALUE_LEN);
  snprintf(key,MAX_KEY_LEN, "slot%d_disable_bios_key_antirollback", slot);
  ret = kv_get(key, value, NULL, 0);
  if (!ret) {
    printf("DisableBiosKeyAntiRollback: %s\n", value);
  }

  memset(value, 0, MAX_VALUE_LEN);
  snprintf(key,MAX_KEY_LEN, "slot%d_disable_amd_key_usage", slot);
  ret = kv_get(key, value, NULL, 0);
  if (!ret) {
    printf("DissableAmdKeyUsage: %s\n", value);
  }

  memset(value, 0, MAX_VALUE_LEN);
  snprintf(key,MAX_KEY_LEN, "slot%d_disable_secure_debug_unlock", slot);
  ret = kv_get(key, value, NULL, 0);
  if (!ret) {
    printf("DissableSecureDebugUnlock: %s\n", value);
  }

  memset(value, 0, MAX_VALUE_LEN);
  snprintf(key,MAX_KEY_LEN, "slot%d_customer_key_lock", slot);
  ret = kv_get(key, value, NULL, 0);
  if (!ret) {
    printf("CustomerKeyLock: %s\n", value);
  }
}

int
main(int argc, char **argv) {
  int ret = 0;
  uint8_t fru;

  if (argc == 2) {
    ret = pal_get_fru_id(argv[1], &fru);
    if (ret < 0) {
      printf("Wrong fru: %s\n", argv[1]);
      print_usage_help();
      exit(-1);
    }
  } else {
    print_usage_help();
    fru = -1;
  }

  if ((fru > 0) && (fru <= MAX_NODES)) {
      uint8_t status = 0;
      pal_get_server_power(fru, &status);
      if(status == SERVER_POWER_ON) {
        print_psb_config(fru);
      } else {
        printf("%s is not POWER ON \n", argv[1]);
        print_usage_help();
      }
  }

  return ret;
}