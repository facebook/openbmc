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
#define _XOPEN_SOURCE 500
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
#include <openbmc/cpld.h>
#include <openbmc/vr.h>
#include <openbmc/bios.h>
#include <openbmc/gpio.h>
#include <openbmc/ocp-dbg-lcd.h>

#define POST_CODE_FILE       "/sys/devices/platform/ast-snoop-dma.0/data_history"
#define FSC_CONFIG           "/etc/fsc-config.json"

#define NCSI_DATA_PAYLOAD 64
#define NIC_FW_VER_PATH "/tmp/cache_store/nic_fw_ver"

static uint8_t g_board_rev_id = BOARD_REV_EVT;
static uint8_t g_vr_cpu0_vddq_abc;
static uint8_t g_vr_cpu0_vddq_def;
static uint8_t g_vr_cpu1_vddq_ghj;
static uint8_t g_vr_cpu1_vddq_klm;

static int get_mtd_name(const char* name, char* dev);

typedef struct
{
  char  mfg_name[10];//manufacture name
  uint32_t mfg_id;//manufacture id
  void (*get_nic_fw)(uint8_t *, char *);
} nic_info_st;

static void
print_usage_help(void) {
  printf("Usage: fw-util <all|mb|nic> <--version>\n");
  printf("       fw-util <mb|nic> <--update> <--cpld|--bios|--nic|--vr|--rom|");
  printf("--bmc|--usbdbgfw|--usbdbgbl> <path>\n");
  printf("       fw-util <mb> <--postcode>\n");
}

static void
get_mlx_fw_version(uint8_t *buf, char *version)
{
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

static void
get_bcm_fw_version(uint8_t *buf, char *version)
{
  int ver_start_index = 8;      //the index is defined in the NC-SI spec
  const int ver_end_index = 19;
  char ver[32]={0};
  int i = 0;

  for( ; ver_start_index <= ver_end_index; ver_start_index++, i++)
  {
    if ( 0 == buf[ver_start_index] )
    {
      break;
    }
    ver[i] = buf[ver_start_index];
  }

  strcat(version, ver);
}

static  nic_info_st support_nic_list[] =
{
  {"Mellanox", 0x19810000, get_mlx_fw_version},
  {"Broadcom", 0x3d110000, get_bcm_fw_version},
};

int nic_list_size = sizeof(support_nic_list) / sizeof(nic_info_st);

static void
print_nic_fw_version(void)
{
  char display_nic_str[128]={0};
  char version[32]={0};
  char vendor[32]={0};
  uint8_t buf[NCSI_DATA_PAYLOAD]={0};
  uint32_t nic_mfg_id=0;
  FILE *file = NULL;
  bool is_unknown_mfg_id = true;
  int current_nic;

  file = fopen(NIC_FW_VER_PATH, "rb");
  if ( NULL != file )
  {
    fread(buf, sizeof(uint8_t), NCSI_DATA_PAYLOAD, file);
    fclose(file);
  }
  else
  {
    syslog(LOG_WARNING, "[%s]Cannot open the file at %s",__func__, NIC_FW_VER_PATH);
  }

  //get the manufcture id
  nic_mfg_id = (buf[35]<<24) + (buf[34]<<16) + (buf[33]<<8) + buf[32];

  for ( current_nic=0; current_nic <nic_list_size; current_nic++)
  {
    //check the nic on the system is supported or not
    if ( support_nic_list[current_nic].mfg_id == nic_mfg_id )
    {
      sprintf(vendor, support_nic_list[current_nic].mfg_name);
      support_nic_list[current_nic].get_nic_fw(buf, version);
      is_unknown_mfg_id = false;
      break;
    }
    else
    {
      is_unknown_mfg_id = true;
    }
  }

  if ( is_unknown_mfg_id )
  {
    sprintf(display_nic_str ,"NIC firmware version: NA (Unknown Manufacture ID: 0x%04x)", nic_mfg_id);
  }
  else
  {
    sprintf(display_nic_str ,"%s NIC firmware version: %s", vendor, version);
  }

  printf("%s\n", display_nic_str);
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

static void
print_bmc_version(void) {
  char vers[128] = "NA";
  FILE *fp = fopen("/etc/issue", "r");
  if (fp) {
    fscanf(fp, "OpenBMC Release %s\n", vers);
    fclose(fp);
  }
  printf("BMC Version: %s\n", vers);
}

static void
print_rom_version(void) {
  char vers[128] = "NA";
  char mtd[32];

  if (get_mtd_name("\"rom\"", mtd)) {
    char cmd[128];
    FILE *fp;
    sprintf(cmd, "strings %s | grep 'U-Boot 2016.07'", mtd);
    fp = popen(cmd, "r");
    if (fp) {
      char line[256];
      char min[32];
      while (fgets(line, sizeof(line), fp)) {
        int ret;
        ret = sscanf(line, "U-Boot 2016.07%*[ ]fbtp-%[^ \n]%*[ ](%*[^)])\n", min);
        if (ret == 1) {
          sprintf(vers, "fbtp-%s", min);
          break;
        }
      }
      pclose(fp);
    }
  }
  printf("ROM Version: %s\n", vers);
}

// TODO: Need to confirm the interpretation of firmware version for print
// Right now using decimal to print the versions
static void
print_fw_ver(uint8_t fru_id) {
  char ver[128] = {0};
  uint8_t cpld_var[4] = {0};

  if ( FRU_NIC == fru_id )
  {
    print_nic_fw_version();
    return;
  }

  if (fru_id != FRU_MB ) {
    printf("Not Supported Operation\n");
    return;
  }

  init_board_sensors();

  print_bmc_version();

  print_rom_version();

  // Print ME Version
  if (me_get_fw_ver((uint8_t *)ver)){
    printf("ME Version: NA\n");
  } else {
    printf("ME Version: SPS_%02X.%02X.%02X.%02X%X.%X\n", ver[0], ver[1]>>4, ver[1] & 0x0F,
                    ver[3], ver[4]>>4, ver[4] & 0x0F);
  }

  // Print BIOS version
  if (bios_get_ver(fru_id, ver)) {
    printf("BIOS Version: NA\n");
  } else {
    printf("BIOS Version: %s\n", (char *)ver);
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
  if (vr_fw_version(VR_PCH_PVNN, ver)) {
    printf("VR_PCH_[PVNN, PV1V05] Version: NA Checksum: NA, DeviceID: NA\n");
  } else {
    printf("VR_PCH_[PVNN, PV1V05] %s\n", ver);
  }

  if (vr_fw_version(VR_CPU0_VCCIN, ver)) {
    printf("VR_CPU0_[VCCIN, VSA] Version: NA, Checksum: NA, DeviceID: NA\n");
  } else {
    printf("VR_CPU0_[VCCIN, VSA] %s\n", ver);
  }

  if (vr_fw_version(VR_CPU0_VCCIO, ver)) {
    printf("VR_CPU0_VCCIO Version: NA, Checksum: NA, DeviceID: NA\n");
  } else {
    printf("VR_CPU0_VCCIO %s\n", ver);
  }

  if (vr_fw_version(g_vr_cpu0_vddq_abc, ver)) {
    printf("VR_CPU0_VDDQ_ABC Version: NA, Checksum: NA, DeviceID: NA\n");
  } else {
    printf("VR_CPU0_VDDQ_ABC %s\n", ver);
  }

  if (vr_fw_version(g_vr_cpu0_vddq_def, ver)) {
    printf("VR_CPU0_VDDQ_DEF Version: NA, Checksum: NA, DeviceID: NA\n");
  } else {
    printf("VR_CPU0_VDDQ_DEF %s\n", ver);
  }

  if (vr_fw_version(VR_CPU1_VCCIN, ver)) {
    printf("VR_CPU1_[VCCIN, VSA] Version: NA, Checksum: NA, DeviceID: NA\n");
  } else {
    printf("VR_CPU1_[VCCIN, VSA] %s\n", ver);
  }

  if (vr_fw_version(VR_CPU1_VCCIO, ver)) {
    printf("VR_CPU1_VCCIO Version: NA, Checksum: NA, DeviceID: NA\n");
  } else {
    printf("VR_CPU1_VCCIO %s\n", ver);
  }

  if (vr_fw_version(g_vr_cpu1_vddq_ghj, ver)) {
    printf("VR_CPU1_VDDQ_GHJ Version: NA, Checksum: NA, DeviceID: NA\n");
  } else {
    printf("VR_CPU1_VDDQ_GHJ %s\n", ver);
  }

  if (vr_fw_version(g_vr_cpu1_vddq_klm, ver)) {
    printf("VR_CPU1_VDDQ_KLM Version: NA, Checksum: NA, DeviceID: NA\n");
  } else {
    printf("VR_CPU1_VDDQ_KLM %s\n", ver);
  }

  print_fsc_version();
}

int
get_mtd_name(const char* name, char* dev)
{
  FILE* partitions = fopen("/proc/mtd", "r");
  char line[256], mnt_name[32];
  unsigned int mtdno;
  int found = 0;

  dev[0] = '\0';
  while (fgets(line, sizeof(line), partitions)) {
    if(sscanf(line, "mtd%d: %*x %*x %s",
                 &mtdno, mnt_name) == 2) {
      if(!strcmp(name, mnt_name)) {
        sprintf(dev, "/dev/mtd%d", mtdno);
        found = 1;
        break;
      }
    }
  }
  fclose(partitions);
  return found;
}

int
fw_update_fru(char **argv, uint8_t slot_id) {
  uint8_t status;
  int ret;
  char cmd[80];
  char dev[12];

  ret = pal_is_fru_prsnt(slot_id, &status);
  if (ret < 0) {
     printf("pal_is_fru_prsnt failed for fru: %d\n", slot_id);
     goto err_exit;
  }
  if (status == 0) {
    printf("slot%d is empty!\n", slot_id);
    goto err_exit;
  }

  pal_set_fw_update_ongoing(FRU_MB, 60*10);

  if (!strcmp(argv[3], "--rom")) {
    if (!get_mtd_name("\"flash0\"", dev)) {
      printf("Error: Cannot find flash0 MTD partition in /proc/mtd\n");
      goto fail_exit;
    }

    printf("Flashing to device: %s\n", dev);
    snprintf(cmd, sizeof(cmd), "flashcp -v %s %s", argv[4], dev);
    ret = run_command(cmd);
  } else if (!strcmp(argv[3], "--bmc")) {
    if (!get_mtd_name("\"flash1\"", dev)) {
      printf("Note: Cannot find flash1 MTD partition in /proc/mtd\n");
      if (!get_mtd_name("\"flash0\"", dev)) {
        printf("Error: Cannot find flash0 MTD partition in /proc/mtd\n");
        goto fail_exit;
      }
    }

    printf("Flashing to device: %s\n", dev);
    snprintf(cmd, sizeof(cmd), "flashcp -v %s %s", argv[4], dev);
    ret = run_command(cmd);
  } else if (!strcmp(argv[3], "--cpld")) {
    if ( !cpld_intf_open() ) {
      ret = cpld_program(argv[4]);
      cpld_intf_close();
      if ( ret < 0 ) {
        printf("Error Occur at updating CPLD FW!\n");
      }
    } else {
      printf("Cannot open JTAG!\n");
      ret = -1;
    }
  } else if (!strcmp(argv[3], "--bios")) {
    ret = bios_program(slot_id, argv[4]);
    if (ret < 0) {
      printf("ERROR: BIOS Program failed\n");
    }
  } else if ( !strcmp(argv[3], "--vr") ) {
    uint8_t board_info;
    if (pal_get_platform_id(&board_info) < 0) {
      printf("Get PlatformID failed!\n");
      ret = -1;
    } else {
      //call vr function
      ret = vr_fw_update(slot_id, board_info, argv[4]);
      if (ret < 0) {
        printf("ERROR: VR Firmware update failed!\n");
      }
    }
  } else if (!strcmp(argv[3], "--usbdbgfw")) {
    ret = usb_dbg_update_fw(argv[4]);
    if ( ret < 0 )
    {
      printf("Error Occur at updating USB DBG FW!\n");
    }
  } else if (!strcmp(argv[3], "--usbdbgbl")) {
    ret = usb_dbg_update_boot_loader(argv[4]);
    if ( ret < 0 )
    {
      printf("Error Occur at updating USB DBG bootloader!\n");
    }
  } else {
    printf("Uknown option: %s\n", argv[3]);
    goto err_exit;
  }
fail_exit:
  pal_set_fw_update_ongoing(FRU_MB, 0);
  return ret;
err_exit:
  print_usage_help();
  pal_set_fw_update_ongoing(FRU_MB, 0);
  return ret;
}

static int
print_postcodes(uint8_t fru_id) {
  int i, rc, len;
  unsigned char postcodes[256];

  if (fru_id != 1) {
    fprintf(stderr, "Not Supported Operation\n");
    return -1;
  }

  len = 0; // clear higher bits
  rc = pal_get_80port_record(fru_id, NULL, 0, postcodes, (uint8_t *)&len);
  if (rc != PAL_EOK) {
    fprintf(stderr, "Error while get 80 port: %d\n", rc);
    return -1;
  }

  for (i=0; i<len; i++) {
    printf("%02X ", postcodes[i]);
    if (i%16 == 15)
      printf("\n");
  }
  if (i%16 != 0)
    printf("\n");

  return 0;
}

int
main(int argc, char **argv) {
  uint8_t fru_id;
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
        print_fw_ver(fru_id);
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
