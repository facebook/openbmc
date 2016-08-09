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
#include <openbmc/me.h>
#include <openbmc/pal.h>
#include <openbmc/ipmi.h>
#include <openbmc/cpld.h>

static void
print_usage_help(void) {
  printf("Usage: fw-util <all|mb|nic> <--version>\n");
  printf("       fw-util <mb|nic> <--update> <--cpld|--bios|--nic> <path>\n");
}

// TODO: Need to confirm the interpretation of firmware version for print
// Right now using decimal to print the versions
static void
print_fw_ver(uint8_t fru_id) {
  int i;
  uint8_t ver[32] = {0};
  uint8_t cpld_ver[4] = {0};

  if (fru_id != 1) {
    printf("Not Supported Operation\n");
    return;
  }

  // Print CPLD Version
  if (cpld_get_ver((unsigned int *)&cpld_ver)) {
    printf("CPLD Version: NA\n");
  } else {
    printf("CPLD Version: %02X%02X%02X%02X\n", cpld_ver[3], cpld_ver[2],
		    cpld_ver[1], cpld_ver[0]);
  }

  // Print ME Version
  if (me_get_fw_ver(ver)){
    printf("ME Version: NA\n");
  } else {
    printf("ME Version: SPS_%02X.%02X.%02X.%02X%X.%X\n", ver[0], ver[1]>>4, ver[1] & 0x0F,
                    ver[3], ver[4]>>4, ver[4] & 0x0F);
  }

  // Print BIOS version
  if (pal_get_sysfw_ver(fru_id, ver)) {
    printf("BIOS Version: ");
  } else {

    // BIOS version response contains the length at offset 2 followed by ascii string
    printf("BIOS Version: ");
    for (i = 3; i < 3+ver[2]; i++) {
      printf("%c", ver[i]);
    }
      printf("\n");
  }

  // Print VR Version
  if (pal_get_vr_ver(VR_PCH_PVNN, ver)) {
    printf("VR_PCH_[PVNN, PV1V05] Version: NA");
  } else {
    printf("VR_PCH_[PVNN, PV1V05] Version: %02X%02X%02X%02X", ver[0], ver[1], ver[2], ver[3]);
  }

  if (pal_get_vr_checksum(VR_PCH_PVNN, ver)) {
    printf(", Checksum: NA");
  } else {
    printf(", Checksum: %02X%02X%02X%02X", ver[0], ver[1], ver[2], ver[3]);
  }

  if (pal_get_vr_deviceId(VR_PCH_PVNN, ver)) {
    printf(", DeviceID: NA\n");
  } else {
    printf(", DeviceID: %02X%02X\n", ver[0], ver[1]);
  }

  if (pal_get_vr_ver(VR_CPU0_VCCIN, ver)) {
    printf("VR_CPU0_[VCCIN, VSA] Version: NA");
  } else {
    printf("VR_CPU0_[VCCIN, VSA] Version: %02X%02X%02X%02X", ver[0], ver[1], ver[2], ver[3]);
  }

  if (pal_get_vr_checksum(VR_CPU0_VCCIN, ver)) {
    printf(", Checksum: NA");
  } else {
    printf(", Checksum: %02X%02X%02X%02X", ver[0], ver[1], ver[2], ver[3]);
  }

  if (pal_get_vr_deviceId(VR_CPU0_VCCIN, ver)) {
    printf(", DeviceID: NA\n");
  } else {
    printf(", DeviceID: %02X%02X\n", ver[0], ver[1]);
  }

  if (pal_get_vr_ver(VR_CPU0_VCCIO, ver)) {
    printf("VR_CPU0_VCCIO Version: NA");
  } else {
    printf("VR_CPU0_VCCIO Version: %02X%02X%02X%02X", ver[0], ver[1], ver[2], ver[3]);
  }

  if (pal_get_vr_checksum(VR_CPU0_VCCIO, ver)) {
    printf(", Checksum: NA");
  } else {
    printf(", Checksum: %02X%02X%02X%02X", ver[0], ver[1], ver[2], ver[3]);
  }

  if (pal_get_vr_deviceId(VR_CPU0_VCCIO, ver)) {
    printf(", DeviceID: NA\n");
  } else {
    printf(", DeviceID: %02X%02X\n", ver[0], ver[1]);
  }

  if (pal_get_vr_ver(VR_CPU0_VDDQ_ABC, ver)) {
    printf("VR_CPU0_VDDQ_ABC Version: NA");
  } else {
    printf("VR_CPU0_VDDQ_ABC Version: %02X%02X%02X%02X", ver[0], ver[1], ver[2], ver[3]);
  }

  if (pal_get_vr_checksum(VR_CPU0_VDDQ_ABC, ver)) {
    printf(", Checksum: NA");
  } else {
    printf(", Checksum: %02X%02X%02X%02X", ver[0], ver[1], ver[2], ver[3]);
  }

  if (pal_get_vr_deviceId(VR_CPU0_VDDQ_ABC, ver)) {
    printf(", DeviceID: NA\n");
  } else {
    printf(", DeviceID: %02X%02X\n", ver[0], ver[1]);
  }

  if (pal_get_vr_ver(VR_CPU0_VDDQ_DEF, ver)) {
    printf("VR_CPU0_VDDQ_DEF Version: NA");
  } else {
    printf("VR_CPU0_VDDQ_DEF Version: %02X%02X%02X%02X", ver[0], ver[1], ver[2], ver[3]);
  }

  if (pal_get_vr_checksum(VR_CPU0_VDDQ_DEF, ver)) {
    printf(", Checksum: NA");
  } else {
    printf(", Checksum: %02X%02X%02X%02X", ver[0], ver[1], ver[2], ver[3]);
  }

  if (pal_get_vr_deviceId(VR_CPU0_VDDQ_DEF, ver)) {
    printf(", DeviceID: NA\n");
  } else {
    printf(", DeviceID: %02X%02X\n", ver[0], ver[1]);
  }

  if (pal_get_vr_ver(VR_CPU1_VCCIN, ver)) {
    printf("VR_CPU1_[VCCIN, VSA] Version: NA");
  } else {
    printf("VR_CPU1_[VCCIN, VSA] Version: %02X%02X%02X%02X", ver[0], ver[1], ver[2], ver[3]);
  }

  if (pal_get_vr_checksum(VR_CPU1_VCCIN, ver)) {
    printf(", Checksum: NA");
  } else {
    printf(", Checksum: %02X%02X%02X%02X", ver[0], ver[1], ver[2], ver[3]);
  }

  if (pal_get_vr_deviceId(VR_CPU1_VCCIN, ver)) {
    printf(", DeviceID: NA\n");
  } else {
    printf(", DeviceID: %02X%02X\n", ver[0], ver[1]);
  }

  if (pal_get_vr_ver(VR_CPU1_VCCIO, ver)) {
    printf("VR_CPU1_VCCIO Version: NA");
  } else {
    printf("VR_CPU1_VCCIO Version: %02X%02X%02X%02X", ver[0], ver[1], ver[2], ver[3]);
  }

  if (pal_get_vr_checksum(VR_CPU1_VCCIO, ver)) {
    printf(", Checksum: NA");
  } else {
    printf(", Checksum: %02X%02X%02X%02X", ver[0], ver[1], ver[2], ver[3]);
  }

  if (pal_get_vr_deviceId(VR_CPU1_VCCIO, ver)) {
    printf(", DeviceID: NA\n");
  } else {
    printf(", DeviceID: %02X%02X\n", ver[0], ver[1]);
  }

  if (pal_get_vr_ver(VR_CPU1_VDDQ_GHJ, ver)) {
    printf("VR_CPU1_VDDQ_GHJ Version: NA");
  } else {
    printf("VR_CPU1_VDDQ_GHJ Version: %02X%02X%02X%02X", ver[0], ver[1], ver[2], ver[3]);
  }

  if (pal_get_vr_checksum(VR_CPU1_VDDQ_GHJ, ver)) {
    printf(", Checksum: NA");
  } else {
    printf(", Checksum: %02X%02X%02X%02X", ver[0], ver[1], ver[2], ver[3]);
  }

  if (pal_get_vr_deviceId(VR_CPU1_VDDQ_GHJ, ver)) {
    printf(", DeviceID: NA\n");
  } else {
    printf(", DeviceID: %02X%02X\n", ver[0], ver[1]);
  }

  if (pal_get_vr_ver(VR_CPU1_VDDQ_KLM, ver)) {
    printf("VR_CPU1_VDDQ_KLM Version: NA");
  } else {
    printf("VR_CPU1_VDDQ_KLM Version: %02X%02X%02X%02X", ver[0], ver[1], ver[2], ver[3]);
  }

  if (pal_get_vr_checksum(VR_CPU1_VDDQ_KLM, ver)) {
    printf(", Checksum: NA");
  } else {
    printf(", Checksum: %02X%02X%02X%02X", ver[0], ver[1], ver[2], ver[3]);
  }

  if (pal_get_vr_deviceId(VR_CPU1_VDDQ_KLM, ver)) {
    printf(", DeviceID: NA\n");
  } else {
    printf(", DeviceID: %02X%02X\n", ver[0], ver[1]);
  }

}

#if 0
int
fw_update_slot(char **argv, uint8_t slot_id) {

  uint8_t status;
  int ret;
  char cmd[80];

  ret = pal_is_fru_prsnt(slot_id, &status);
  if (ret < 0) {
     printf("pal_is_fru_prsnt failed for fru: %d\n", slot_id);
     goto err_exit;
  }
  if (status == 0) {
    printf("slot%d is empty!\n", slot_id);
    goto err_exit;
  }
  if (!strcmp(argv[3], "--cpld")) {
     return bic_update_fw(slot_id, UPDATE_CPLD, argv[4]);
  }
  if (!strcmp(argv[3], "--bios")) {
    sprintf(cmd, "power-util slot%u off", slot_id);
    system(cmd);
    ret = bic_update_fw(slot_id, UPDATE_BIOS, argv[4]);
    sprintf(cmd, "power-util slot%u on", slot_id);
    system(cmd);
    return ret;
  }
  if (!strcmp(argv[3], "--bic")) {
    return bic_update_fw(slot_id, UPDATE_BIC, argv[4]);
  }
  if (!strcmp(argv[3], "--bicbl")) {
    return bic_update_fw(slot_id, UPDATE_BIC_BOOTLOADER, argv[4]);
  }

err_exit:
  print_usage_help();
  return -1;
}

#endif

int
main(int argc, char **argv) {

  uint8_t fru_id;
  int ret = 0;
  char cmd[80];
  // Check for border conditions
  if ((argc != 3) && (argc != 5)) {
    goto err_exit;
  }

  // Derive slot_id from first parameter
  if (!strcmp(argv[1], "mb")) {
    fru_id = 1;
  } else if (!strcmp(argv[1] , "nic")) {
    fru_id =2;
  } else if (!strcmp(argv[1] , "all")) {
    fru_id =3;
  } else {
      goto err_exit;
  }
  // check operation to perform
  if (!strcmp(argv[2], "--version")) {
     if (fru_id < 3) {
       print_fw_ver(fru_id);
       return 0;
     }

     for (fru_id = 1; fru_id < 3; fru_id++) {
        printf("Get version info for fru%d\n", fru_id);
        print_fw_ver(fru_id);;
        printf("\n");
     }

     return 0;
  }
#if 0
  if (!strcmp(argv[2], "--update")) {
    if (argc != 5) {
      goto err_exit;
    }

    if (fru_id > 2) {
      goto err_exit;
    }
    return fw_update_fru(argv, fru_id);
  }
#endif

err_exit:
  print_usage_help();
  return -1;
}
