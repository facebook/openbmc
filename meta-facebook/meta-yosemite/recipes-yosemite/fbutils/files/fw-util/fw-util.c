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
#include <sys/file.h>
#include <facebook/bic.h>
#include <openbmc/pal.h>


#define NCSI_DATA_PAYLOAD 64
#define NIC_FW_VER_PATH "/tmp/cache_store/nic_fw_ver"

#define MAX_NUM_OPTIONS 6
enum {
  OPT_SLOT1 = 1,
  OPT_SLOT2 = 2,
  OPT_SLOT3 = 3,
  OPT_SLOT4 = 4,
  OPT_NIC   = 5,
  OPT_ALL   = 6,
};

typedef struct
{
  char  mfg_name[10];  //manufacture name
  uint32_t mfg_id;     //manufacture id
  void (*get_nic_fw)(uint8_t *, char *);
} nic_info_st;

static void
print_usage_help(void) {
  printf("Usage: fw-util <all|slot1|slot2|slot3|slot4|nic> <--version>\n");
  printf("       fw-util <all|slot1|slot2|slot3|slot4> <--update> <--cpld|--bios|--bic|--bicbl> <path>\n");
}

static void
get_nic_fw_version(uint8_t *buf, char *version) {
  int ver_index_based = 20;
  int major = 0;
  int minor = 0;
  int revision = 0;

  major = buf[ver_index_based++];
  minor = buf[ver_index_based++];
  revision = buf[ver_index_based++] << 8;
  revision += buf[ver_index_based++];

  sprintf(version, "%d.%d.%d", major, minor, revision);
}

static nic_info_st support_nic_list[] =
{
  {"Mellanox", 0x19810000, get_nic_fw_version},
  {"Broadcom", 0x3D110000, get_nic_fw_version}
};

int nic_list_size = sizeof(support_nic_list) / sizeof(nic_info_st);


static void
print_slot_version(uint8_t slot_id) {
  int i;
  uint8_t ver[32] = {0};
  uint8_t status;
  int ret;

  if (slot_id < OPT_SLOT1 || slot_id > OPT_SLOT4) {
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


static void
print_nic_version(uint8_t slot_id) {
  // Need to read nic card firmware version for print
  // There are 1 nic card vendor,MEZZ
  char display_nic_str[128]={0};
  char version[32]={0};
  char vendor[32]={0};
  uint8_t buf[NCSI_DATA_PAYLOAD]={0};
  uint32_t nic_mfg_id=0;
  FILE *file = NULL;
  bool is_unknown_mfg_id = true;
  int current_nic;

  file = fopen(NIC_FW_VER_PATH, "rb");
  if ( NULL != file ) {
    fread(buf, sizeof(uint8_t), NCSI_DATA_PAYLOAD, file);
    fclose(file);
  }
  else {
    syslog(LOG_WARNING, "[%s]Cannot open the file at %s",__func__, NIC_FW_VER_PATH);
  }

  //get the manufcture id
  nic_mfg_id = (buf[35]<<24) + (buf[34]<<16) + (buf[33]<<8) + buf[32];

  for ( current_nic=0; current_nic <nic_list_size; current_nic++) {
    //check the nic on the system is supported or not
    if ( support_nic_list[current_nic].mfg_id == nic_mfg_id ) {
      sprintf(vendor, support_nic_list[current_nic].mfg_name);
      support_nic_list[current_nic].get_nic_fw(buf, version);
      is_unknown_mfg_id = false;
      break;
    }
    else {
      is_unknown_mfg_id = true;
    }
  }

  if ( is_unknown_mfg_id ) {
    sprintf(display_nic_str ,"NIC firmware version: NA (Unknown Manufacture ID: 0x%04x)", nic_mfg_id);
  }
  else {
    sprintf(display_nic_str ,"%s NIC firmware version: %s", vendor, version);
  }
  printf("%s\n", display_nic_str);
}


// TODO: Need to confirm the interpretation of firmware version for print
// Right now using decimal to print the versions
static void
print_fw_ver(uint8_t slot_id) {
  int i;
  uint8_t ver[32] = {0};

  switch(slot_id) {
    case OPT_SLOT1:
    case OPT_SLOT2:
    case OPT_SLOT3:
    case OPT_SLOT4:
      printf("Get version info for slot%d\n", slot_id);
      print_slot_version(slot_id);
      break;
    case OPT_NIC:
      print_nic_version(slot_id);
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
    sprintf(cmd, "/usr/local/bin/power-util slot%u off", slot_id);
    system(cmd);
    ret = bic_update_fw(slot_id, UPDATE_BIOS, argv[4]);
    sleep(1);
    sprintf(cmd, "/usr/local/bin/power-util slot%u 12V-cycle", slot_id);
    system(cmd);
    sleep(5);
    sprintf(cmd, "/usr/local/bin/power-util slot%u on", slot_id);
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

  uint8_t slot_id;
  int ret = 0;
  char cmd[80];
  // Check for border conditions
  if ((argc != 3) && (argc != 5)) {
    goto err_exit;
  }

  // Derive slot_id from first parameter
  if (!strcmp(argv[1], "slot1")) {
    slot_id = OPT_SLOT1;
  } else if (!strcmp(argv[1] , "slot2")) {
    slot_id =OPT_SLOT2;
  } else if (!strcmp(argv[1] , "slot3")) {
    slot_id =OPT_SLOT3;
  } else if (!strcmp(argv[1] , "slot4")) {
    slot_id =OPT_SLOT4;
  } else if (!strcmp(argv[1] , "nic")) {
    slot_id = OPT_NIC;
  } else if (!strcmp(argv[1] , "all")) {
    slot_id =OPT_ALL;
  } else {
      goto err_exit;
  }
  // check operation to perform
  if (!strcmp(argv[2], "--version")) {
     if (slot_id < OPT_ALL) {
       print_fw_ver(slot_id);
       return 0;
     }
     for (slot_id = 1; slot_id < MAX_NUM_OPTIONS; slot_id++) {
        print_fw_ver(slot_id);;
        printf("\n");
     }
     return 0;
  }
  if (!strcmp(argv[2], "--update")) {
    if (argc != 5) {
      goto err_exit;
    }
    if ((slot_id == OPT_NIC) || !strcmp(argv[3], "--nic") )
    {
        printf("fw_util: NIC update is not supported\n");
        return -1;
    }
    // single slot update
    if (slot_id <= OPT_SLOT4) {
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

    // update ALL slots (minus NIC)
    for (slot_id = OPT_SLOT1; slot_id <= OPT_SLOT4; slot_id++) {
      if (check_dup_process(slot_id) < 0) {
        printf("fw_util: another instance is running...\n");
        return -1;
      }
    }

    printf("Updating all slots....\n");
    for (slot_id = OPT_SLOT1; slot_id <= OPT_SLOT4; slot_id++) {
       if (fw_update_slot(argv, slot_id)) {
         printf("fw_util: updating %s on slot %d failed!\n", argv[3], slot_id);
         ret++;
       }
    }
    if (ret) {
      printf("fw_util: updating all slots failed!\n");
      return -1;
    }
    printf("fw_util: updated all slots successfully!\n");
    return 0;
  }
err_exit:
  print_usage_help();
  return -1;
}
