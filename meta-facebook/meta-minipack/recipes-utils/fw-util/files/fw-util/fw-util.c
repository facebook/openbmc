/*
 * fw-util
 *
 * Copyright 2018-present Facebook. All Rights Reserved.
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
#include <sys/file.h>
#include <facebook/bic.h>
#include <openbmc/pal.h>
#include <openbmc/ipmi.h>

enum {
  OPT_SCM = 0,
};

static void
print_usage_help(void) {
  printf("Usage: fw-util <scm> <--version>\n");
  printf("       fw-util <scm> <--update> <--cpld|--bios|--bic|--bicbl|--vr> <path>\n");
}

static void
print_slot_version(uint8_t slot_id) {
  int i;
  uint8_t ver[32] = {0};
  uint8_t status;
  int ret;

  if (slot_id != OPT_SCM) {
    printf("Not Supported Operation\n");
    return;
  }

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

  // Print ME Version
  if (bic_get_fw_ver(slot_id, FW_ME, ver)){
    return;
  }

  printf("ME Version: %x.%x.%x.%x%x\n", ver[0], ver[1], ver[2], ver[3], ver[4]);

  // Print Bridge-IC Bootloader Version
  if (bic_get_fw_ver(slot_id, FW_BIC_BOOTLOADER, ver)) {
    return;
  }

  printf("Bridge-IC Bootloader Version: v%x.%02x\n", ver[0], ver[1]);

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

  // Print BIOS version
  if (pal_get_sysfw_ver(1, ver)) { //slot1
    printf("Can't get key\n");
    return;
  }

  // BIOS version response contains the length at offset 2 followed by ascii string
  printf("BIOS Version: ");
  for (i = 3; i < 3+ver[2]; i++) {
    printf("%c", ver[i]);
  }
  printf("\n");
}


// TODO: Need to confirm the interpretation of firmware version for print
// Right now using decimal to print the versions
static void
print_fw_ver(uint8_t slot_id) {
  int i;
  uint8_t ver[32] = {0};

  switch(slot_id) {
    case OPT_SCM:
      printf("Get version info for slot%d\n", slot_id);
      print_slot_version(slot_id);
      break;
  }

}


static int
check_dup_process(uint8_t slot_id) {
  int pid_file;
  char path[64];

  sprintf(path, "/var/run/fw-util_%d.lock", slot_id);
  pid_file = open(path, O_CREAT | O_RDWR, 0666);
  if (flock(pid_file, LOCK_EX | LOCK_NB) && (errno == EWOULDBLOCK)) {
    return -1;
  }

  return 0;
}

int
fw_update_slot(char **argv, uint8_t slot_id) {

  uint8_t status;
  int ret;
  char cmd[80];

  ret = pal_is_fru_prsnt(FRU_SCM, &status);
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
    ret = pal_set_com_pwr_btn_n("0");//set COM_PWR_BTN_N to low
    if(ret){
      printf("Cannot power off COM-e.\n");
      return -1;
    }
    sleep(6);
    ret = pal_set_com_pwr_btn_n("1");//set COM_PWR_BTN_N to high
    if(ret){
      printf("Cannot power on COM-e.\n");
      return -1;
    }
    ret = bic_update_fw(slot_id, UPDATE_BIOS, argv[4]);
    if (ret) {
      printf("Update BIOS failed (%d).\n", ret);
      return -1;
    }
    sleep(3);
    ret = pal_set_com_pwr_btn_n("0");//set COM_PWR_BTN_N to low
    if(ret){
      printf("Cannot power off COM-e.\n");
      return -1;
    }
    sleep(1);
    ret = pal_set_com_pwr_btn_n("1");//set COM_PWR_BTN_N to high
    if(ret){
      printf("Cannot power on COM-e.\n");
      return -1;
    }
    return ret;
  }
  if (!strcmp(argv[3], "--bic")) {
    return bic_update_fw(slot_id, UPDATE_BIC, argv[4]);
  }
  if (!strcmp(argv[3], "--bicbl")) {
    return bic_update_fw(slot_id, UPDATE_BIC_BOOTLOADER, argv[4]);
  }
  if (!strcmp(argv[3], "--vr")) {
    return bic_update_fw(slot_id, UPDATE_VR, argv[4]);
  }

err_exit:
  print_usage_help();
  return -1;
}

int
main(int argc, char **argv) {

  uint8_t slot_id;
  int ret = 0;
  char cmd[80];
  // Check for border conditions
  if ((argc != 3) && (argc != 5)) {
    goto err_exit;
  }

  // Derive slot_id from first parameter
  if (!strcmp(argv[1], "scm")) {
    slot_id = OPT_SCM;
  }else {
      goto err_exit;
  }
  // check operation to perform
  if (!strcmp(argv[2], "--version")) {
     print_fw_ver(slot_id);
     return 0;
  }
  if (!strcmp(argv[2], "--update")) {
    if (argc != 5) {
      goto err_exit;
    }
    // minilake update
    if (check_dup_process(slot_id) < 0) {
      printf("fw_util: another instance is running...\n");
      return -1;
    }

    if (fw_update_slot(argv, slot_id)) {
      printf("fw_util: updating %s on slot %d failed!\n", argv[3], slot_id);
      return -1;
    }
    return 0;
  }
err_exit:
  print_usage_help();
  return -1;
}
