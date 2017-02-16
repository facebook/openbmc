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
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <syslog.h>
#include <stdint.h>
#include <pthread.h>
#include <jansson.h>

#include <sys/wait.h>

#include <openbmc/me.h>
#include <openbmc/pal.h>
#include <openbmc/ipmi.h>
#include <openbmc/cpld.h>
#include <openbmc/gpio.h>

#define GPIO_BMC_CTRL        109
#define POST_CODE_FILE       "/sys/devices/platform/ast-snoop-dma.0/data_history"
#define FSC_CONFIG           "/etc/fsc-config.json"

static uint8_t g_board_rev_id = BOARD_REV_EVT;
static uint8_t g_vr_cpu0_vddq_abc;
static uint8_t g_vr_cpu0_vddq_def;
static uint8_t g_vr_cpu1_vddq_ghj;
static uint8_t g_vr_cpu1_vddq_klm;


static void
print_usage_help(void) {
  printf("Usage: fw-util <all|mb|nic> <--version>\n");
  printf("       fw-util <mb|nic> <--update> <--cpld|--bios|--nic|--vr|--rom|--bmc> <path>\n");
  printf("       fw-util <mb> <--postcode>\n");
}

static void
init_board_sensors(void) {
  pal_get_board_rev_id(&g_board_rev_id);

  if (g_board_rev_id == BOARD_REV_POWERON ||
      g_board_rev_id == BOARD_REV_EVT ) {
    g_vr_cpu0_vddq_abc = VR_CPU0_VDDQ_ABC_EVT;
    g_vr_cpu0_vddq_def = VR_CPU0_VDDQ_DEF_EVT;
    g_vr_cpu1_vddq_ghj = VR_CPU1_VDDQ_GHJ_EVT;
    g_vr_cpu1_vddq_klm = VR_CPU1_VDDQ_KLM_EVT;
  } else {
    g_vr_cpu0_vddq_abc = VR_CPU0_VDDQ_ABC;
    g_vr_cpu0_vddq_def = VR_CPU0_VDDQ_DEF;
    g_vr_cpu1_vddq_ghj = VR_CPU1_VDDQ_GHJ;
    g_vr_cpu1_vddq_klm = VR_CPU1_VDDQ_KLM;
  }
}

static void
print_fsc_version(void) {
  json_error_t error;
  json_t *conf, *vers;

  conf = json_load_file(FSC_CONFIG, 0, &error);
  if(!conf) {
    printf("Fan Speed Controller Version: NA\n");
    return;
  }
  vers = json_object_get(conf, "version");
  if(!vers || !json_is_string(vers)) {
    printf("Fan Speed Controller Version: NA\n");
  }
  printf("Fan Speed Controller Version: %s\n", json_string_value(vers));
  json_decref(conf);
}

// TODO: Need to confirm the interpretation of firmware version for print
// Right now using decimal to print the versions
static void
print_fw_ver(uint8_t fru_id) {
  int i;
  uint8_t ver[32] = {0};
  uint8_t cpld_var[4] = {0};

  if (fru_id != 1) {
    printf("Not Supported Operation\n");
    return;
  }

  init_board_sensors();

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

  if (!cpld_intf_open()) {
    // Print CPLD Version
    if (cpld_get_ver((unsigned int *)&cpld_var)) {
      printf("CPLD Version: NA, ");
    } else {
      printf("CPLD Version: %02X%02X%02X%02X, ", cpld_var[3], cpld_var[2],
		cpld_var[1], cpld_var[0]);
    }

    // Print CPLD Device ID
    if (cpld_get_device_id((unsigned int *)&cpld_var)) {
      printf("CPLD DeviceID: NA\n");
    } else {
      printf("CPLD DeviceID: %02X%02X%02X%02X\n", cpld_var[3], cpld_var[2],
		cpld_var[1], cpld_var[0]);
    }

    cpld_intf_close();
  }

  //Disable JTAG Engine after CPLD access
  system("devmem 0x1e6e4008 32 0");

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

  if (pal_get_vr_ver(g_vr_cpu0_vddq_abc, ver)) {
    printf("VR_CPU0_VDDQ_ABC Version: NA");
  } else {
    printf("VR_CPU0_VDDQ_ABC Version: %02X%02X%02X%02X", ver[0], ver[1], ver[2], ver[3]);
  }

  if (pal_get_vr_checksum(g_vr_cpu0_vddq_abc, ver)) {
    printf(", Checksum: NA");
  } else {
    printf(", Checksum: %02X%02X%02X%02X", ver[0], ver[1], ver[2], ver[3]);
  }

  if (pal_get_vr_deviceId(g_vr_cpu0_vddq_abc, ver)) {
    printf(", DeviceID: NA\n");
  } else {
    printf(", DeviceID: %02X%02X\n", ver[0], ver[1]);
  }

  if (pal_get_vr_ver(g_vr_cpu0_vddq_def, ver)) {
    printf("VR_CPU0_VDDQ_DEF Version: NA");
  } else {
    printf("VR_CPU0_VDDQ_DEF Version: %02X%02X%02X%02X", ver[0], ver[1], ver[2], ver[3]);
  }

  if (pal_get_vr_checksum(g_vr_cpu0_vddq_def, ver)) {
    printf(", Checksum: NA");
  } else {
    printf(", Checksum: %02X%02X%02X%02X", ver[0], ver[1], ver[2], ver[3]);
  }

  if (pal_get_vr_deviceId(g_vr_cpu0_vddq_def, ver)) {
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

  if (pal_get_vr_ver(g_vr_cpu1_vddq_ghj, ver)) {
    printf("VR_CPU1_VDDQ_GHJ Version: NA");
  } else {
    printf("VR_CPU1_VDDQ_GHJ Version: %02X%02X%02X%02X", ver[0], ver[1], ver[2], ver[3]);
  }

  if (pal_get_vr_checksum(g_vr_cpu1_vddq_ghj, ver)) {
    printf(", Checksum: NA");
  } else {
    printf(", Checksum: %02X%02X%02X%02X", ver[0], ver[1], ver[2], ver[3]);
  }

  if (pal_get_vr_deviceId(g_vr_cpu1_vddq_ghj, ver)) {
    printf(", DeviceID: NA\n");
  } else {
    printf(", DeviceID: %02X%02X\n", ver[0], ver[1]);
  }

  if (pal_get_vr_ver(g_vr_cpu1_vddq_klm, ver)) {
    printf("VR_CPU1_VDDQ_KLM Version: NA");
  } else {
    printf("VR_CPU1_VDDQ_KLM Version: %02X%02X%02X%02X", ver[0], ver[1], ver[2], ver[3]);
  }

  if (pal_get_vr_checksum(g_vr_cpu1_vddq_klm, ver)) {
    printf(", Checksum: NA");
  } else {
    printf(", Checksum: %02X%02X%02X%02X", ver[0], ver[1], ver[2], ver[3]);
  }

  if (pal_get_vr_deviceId(g_vr_cpu1_vddq_klm, ver)) {
    printf(", DeviceID: NA\n");
  } else {
    printf(", DeviceID: %02X%02X\n", ver[0], ver[1]);
  }

  print_fsc_version();
}

int
get_mtd_name(const char* name, char* dev)
{
  FILE* partitions = fopen("/proc/mtd", "r");
  char line[256];

  memset((void*)dev, 0, 5);
  while (fgets(line, sizeof(line), partitions)) {
    if (!strncmp(&line[24], name, strlen(name))) {
      strncpy(dev, line, 4);
      fclose(partitions);
      return 1;
    }
  }

  fclose(partitions);
  return 0;
}

int
run_command(const char* cmd) {
  int status = system(cmd);
  if (status == -1) {
    return 127;
  }

  return WEXITSTATUS(status);
}

int
fw_update_fru(char **argv, uint8_t slot_id) {
  uint8_t status;
  int ret;
  char cmd[80];
  char dev[5];
  gpio_st bmc_ctrl_pin;

  ret = pal_is_fru_prsnt(slot_id, &status);
  if (ret < 0) {
     printf("pal_is_fru_prsnt failed for fru: %d\n", slot_id);
     goto err_exit;
  }
  if (status == 0) {
    printf("slot%d is empty!\n", slot_id);
    goto err_exit;
  }

  if (!strcmp(argv[3], "--rom")) {
    if (!get_mtd_name("\"flash0\"", dev)) {
      printf("Error: Cannot find flash0 MTD partition in /proc/mtd\n");
      goto err_exit;
    }

    snprintf(cmd, sizeof(cmd), "flashcp -v %s /dev/%s", argv[4], dev);
    return run_command(cmd);
  }

  if (!strcmp(argv[3], "--bmc")) {
    if (!get_mtd_name("\"flash1\"", dev)) {
      printf("Note: Cannot find flash1 MTD partition in /proc/mtd\n");
      if (!get_mtd_name("\"flash0\"", dev)) {
        printf("Error: Cannot find flash0 MTD partition in /proc/mtd\n");
        goto err_exit;
      }
    }

    snprintf(cmd, sizeof(cmd), "flashcp -v %s /dev/%s", argv[4], dev);
    return run_command(cmd);
  }

  if (!strcmp(argv[3], "--cpld")) {
    if ( !cpld_intf_open() ) {
      ret = cpld_program(argv[4]);
      cpld_intf_close();
      if ( ret < 0 ) {
        printf("Error Occur at updating CPLD FW!\n");
        goto err_exit;
      }
    } else {
      printf("Cannot open JTAG!\n");
    }
    return 0;
  }

  if (!strcmp(argv[3], "--bios")) {
    int exit_code;

    system("/usr/local/bin/power-util mb off");
    sleep(10);
    system("/usr/local/bin/me-util 0xB8 0xDF 0x57 0x01 0x00 0x01");
    sleep(1);
    gpio_export(GPIO_BMC_CTRL);
    gpio_open(&bmc_ctrl_pin,  GPIO_BMC_CTRL);
    gpio_change_direction(&bmc_ctrl_pin, GPIO_DIRECTION_OUT);
    gpio_write(&bmc_ctrl_pin, GPIO_VALUE_HIGH);

    system("echo -n \"spi1.0\" > /sys/bus/spi/drivers/m25p80/bind");
    exit_code = 0;
    if (!get_mtd_name("\"bios0\"", dev)) {
      printf("Error: Cannot find bios0 MTD partition in /proc/mtd\n");
    } else {
      snprintf(cmd, sizeof(cmd), "flashcp -v %s /dev/%s", argv[4], dev);
      exit_code = run_command(cmd);
    }

    system("echo -n \"spi1.0\" > /sys/bus/spi/drivers/m25p80/unbind");
    gpio_write(&bmc_ctrl_pin, GPIO_VALUE_LOW);
    gpio_close(&bmc_ctrl_pin);
    gpio_unexport(GPIO_BMC_CTRL);
    sleep(1);
    pal_PBO();
    sleep(10);
    system("/usr/local/bin/power-util mb on");
    return exit_code;
  }

  if ( !strcmp(argv[3], "--vr") ) {
    //call vr function
    uint8_t *BinData=NULL;

    ret = pal_get_vr_update_data(argv[4], &BinData);
    if ( ret < 0 )
    {
      printf("Cannot locate the file at: %s!\n", argv[4]);

      //check data
      goto err_exit;
    }

    ret = pal_vr_fw_update(BinData);
    if ( ret < 0 )
    {
      printf("Error Occur at updating VR FW!\n");
        goto err_exit;
    }

    return 0;
  }

err_exit:
  print_usage_help();
  return ret;
}

static int
print_postcodes(uint8_t fru_id) {
  FILE *fp=NULL;
  int i;
  unsigned char postcode;

  if (fru_id != 1) {
    fprintf(stderr, "Not Supported Operation\n");
    return -1;
  }

  fp = fopen(POST_CODE_FILE, "r");
  if (fp == NULL) {
    fprintf(stderr, "Cannot open %s\n", POST_CODE_FILE);
    return -1;
  }

  for (i=0; i<256; i++) {
    // %hhx: unsigned char*
    if (fscanf(fp, "%hhx", &postcode) == 1) {
      printf("%02X ", postcode);
    } else {
      if (i%16 != 0)
        printf("\n");
      break;
    }
    if (i%16 == 15)
      printf("\n");
  }

  fclose(fp);

  return 0;
}

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
  if (!strcmp(argv[2], "--update")) {
    if (argc != 5) {
      goto err_exit;
    }

    if (fru_id >= 2) {
      goto err_exit;
    }
    return fw_update_fru(argv, fru_id);
  }

  if (!strcmp(argv[2], "--postcode")) {
     return print_postcodes(fru_id);
  }

err_exit:
  print_usage_help();
  return -1;
}
