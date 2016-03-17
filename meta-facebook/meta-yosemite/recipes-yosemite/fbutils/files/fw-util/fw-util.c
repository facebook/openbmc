/*
 * fw-util
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
#include <errno.h>
#include <syslog.h>
#include <stdint.h>
#include <pthread.h>
#include <facebook/bic.h>
#include <openbmc/pal.h>
#include <openbmc/ipmi.h>

static void
print_usage_help(void) {
  printf("Usage: fw_util <slot1|slot2|slot3|slot4> <--version>\n");
  printf("       fw_util <slot1|slot2|slot3|slot4> <--update> <--cpld|--bios|--bic|--bicbl> <path>\n");
}

// TODO: Need to confirm the interpretation of firmware version for print
// Right now using decimal to print the versions
static void
print_fw_ver(uint8_t slot_id) {
  int i;
  uint8_t ver[32] = {0};

  // Print CPLD Version
  if (bic_get_fw_ver(slot_id, FW_CPLD, ver)) {
    return;
  }

  printf("CPLD Version: 0x%02x%02x%02x%02x\n", ver[0], ver[1], ver[2], ver[3]);

  // Print Bridge-IC Version
  if (bic_get_fw_ver(slot_id, FW_BIC, ver)) {
    return;
  }

  printf("Bridge-IC Version: v%x.%02x\n", ver[0], ver[1]);

  // Print Bridge-IC Bootloader Version
  if (bic_get_fw_ver(slot_id, FW_BIC_BOOTLOADER, ver)) {
    return;
  }

  printf("Bridge-IC Bootloader Version: v%x.%02x\n", ver[0], ver[1]);

  // Print ME Version
  if (bic_get_fw_ver(slot_id, FW_ME, ver)){
    return;
  }

  printf("ME Version: %x.%x.%x.%x%x\n", ver[0], ver[1], ver[2], ver[3], ver[4]);

  // Print PVCCIN VR Version
  if (bic_get_fw_ver(slot_id, FW_PVCCIN_VR, ver)){
    return;
  }

  printf("PVCCIN VR Version: 0x%02x%02x, 0x%02x%02x\n", ver[0], ver[1], ver[2], ver[3]);

  // Print DDRAB VR Version
  if (bic_get_fw_ver(slot_id, FW_DDRAB_VR, ver)){
    return;
  }

  printf("DDRAB VR Version: 0x%02x%02x, 0x%02x%02x\n", ver[0], ver[1], ver[2], ver[3]);

  // Print P1V05 VR Version
  if (bic_get_fw_ver(slot_id, FW_P1V05_VR, ver)){
    return;
  }

  printf("P1V05 VR Version: 0x%02x%02x, 0x%02x%02x\n", ver[0], ver[1], ver[2], ver[3]);

  // Print PVCCGBE VR Version
  if (bic_get_fw_ver(slot_id, FW_PVCCGBE_VR, ver)){
    return;
  }

  printf("PVCCGBE VR Version: 0x%02x%02x, 0x%02x%02x\n", ver[0], ver[1], ver[2], ver[3]);

  // Print PVCCSCSUS VR Version
  if (bic_get_fw_ver(slot_id, FW_PVCCSCSUS_VR, ver)){
    return;
  }

  printf("PVCCSCSUS VR Version: 0x%02x%02x, 0x%02x%02x\n", ver[0], ver[1], ver[2], ver[3]);

  // Print BIOS version
  if (pal_get_sysfw_ver(slot_id, ver)) {
    return;
  }

  // BIOS version response contains the length at offset 2 followed by ascii string
  printf("BIOS Version: ");
  for (i = 3; i < 3+ver[2]; i++) {
    printf("%c", ver[i]);
  }
    printf("\n");
}

int
main(int argc, char **argv) {

  uint8_t slot_id;

  // Check for border conditions
  if ((argc != 3) && (argc != 5)) {
    goto err_exit;
  }

  // Derive slot_id from first parameter
  if (!strcmp(argv[1], "slot1")) {
    slot_id = 1;
  } else if (!strcmp(argv[1] , "slot2")) {
    slot_id =2;
  } else if (!strcmp(argv[1] , "slot3")) {
    slot_id =3;
  } else if (!strcmp(argv[1] , "slot4")) {
    slot_id =4;
  } else {
      goto err_exit;
  }

  // check operation to perform
  if (!strcmp(argv[2], "--version")) {
    // handle printing versions of f/w components
    print_fw_ver(slot_id);
  } else if (!strcmp(argv[2], "--update")) {
    // handle firmware update
    if (argc != 5) {
      goto err_exit;
    }

    if (!strcmp(argv[3], "--cpld")) {
      return bic_update_fw(slot_id, UPDATE_CPLD, argv[4]);
    } else if (!strcmp(argv[3], "--bios")) {
      return bic_update_fw(slot_id, UPDATE_BIOS, argv[4]);
    } else if (!strcmp(argv[3], "--bic")) {
      return bic_update_fw(slot_id, UPDATE_BIC, argv[4]);
    } else if (!strcmp(argv[3], "--bicbl")) {
      return bic_update_fw(slot_id, UPDATE_BIC_BOOTLOADER, argv[4]);
    } else {
      goto err_exit;
    }
  } else {
    goto err_exit;
  }

  return 0;

err_exit:
  print_usage_help();
  return -1;
}
