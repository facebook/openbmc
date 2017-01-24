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
  uint8_t sku = 0;
  sku = pal_get_iom_type();

  //SKU : 2 type7
  if (sku == 2)
  printf("Usage: fw-util <all|slot1|scc|ioc2> <--version>\n");
  else
  printf("Usage: fw-util <all|slot1|scc> <--version>\n");

  printf("       fw-util <all|slot1> <--update> <--cpld|--bios|--bic|--bicbl> <path>\n");
}

static void
print_fw_scc_ver(void) {

  uint8_t ver[32] = {0};
  int ret = 0;

  // Read Firwmare Versions of Expander via IPMB
  // ID: exp is 0, ioc is 1
  ret = exp_get_fw_ver(ver);
  if( !ret )
    printf("Expander Version: 0x%02x%02x%02x%02x\n", ver[0], ver[1], ver[2], ver[3]);
  else
    printf("Get Expander FW Verion Fail...\n");

  ret = exp_get_ioc_fw_ver(ver);
  if( !ret )
    printf("SCC IOC  Version: %x.%x.%x.%x\n", ver[3], ver[2], ver[1], ver[0]);
  else
    printf("Get Expander FW Verion Fail...\n");

  return;
}

static void
print_fw_ioc_ver(void) {
  uint8_t ver[32] = {0};
  int ret = 0;

  // Read Firwmare Versions of IOM IOC viacd MCTP
  ret = pal_get_iom_ioc_ver(ver);
  if(!ret)
    printf("IOM IOC Version: %x.%x.%x.%x\n", ver[3], ver[2], ver[1], ver[0]);
  else
    printf("Get IOM IOC FW Verion Fail...\n");

  return;
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

int
main(int argc, char **argv) {
  uint8_t fru;
  int ret = 0;
  char cmd[80];
  uint8_t sku = 0;

  sku = pal_get_iom_type();

  // Check for border conditions
  if ((argc != 3) && (argc != 5)) {
    goto err_exit;
  }

  // Derive fru from first parameter
  if (!strcmp(argv[1], "slot1")) {
    fru = FRU_SLOT1;
  } else if (!strcmp(argv[1] , "scc")) {
    fru = FRU_SCC;
  }else if (!strcmp(argv[1] , "ioc2")) {
    fru = FRU_IOM_IOC;
  }else if (!strcmp(argv[1] , "all")) {
    fru = FRU_ALL;
  } else {
      goto err_exit;
  }
  // check operation to perform
  if (!strcmp(argv[2], "--version")) {
    switch(fru) {
      case FRU_SLOT1:
        print_fw_ver(FRU_SLOT1);
        break;

      case FRU_SCC:
        print_fw_scc_ver();
        break;

      case FRU_IOM_IOC:
        print_fw_ioc_ver();
        break;

      case FRU_ALL:
        print_fw_ver(FRU_SLOT1);
        print_fw_scc_ver();
        if (sku == 2)
        print_fw_ioc_ver();
        break;
    }
    return 0;
  }
  if (!strcmp(argv[2], "--update")) {
    if (argc != 5) {
      goto err_exit;
    }
    if (fru < 2) {
      return fw_update_slot(argv, fru);
    }
    printf("Updating all slots....\n");
    for (fru = 1; fru < 2; fru++) {
       if (fw_update_slot(argv, fru)) {
         printf("fw_util:  updating %s on slot %d failed!\n", argv[3], fru);
         ret++;
       }
    }
    if (ret) {
      printf("fw_util:  updating all slots failed!\n");
      return -1;
    }
    printf("fw_util: updated all slots successfully!\n");
    return 0;
  }
err_exit:
  print_usage_help();
  return -1;
}
