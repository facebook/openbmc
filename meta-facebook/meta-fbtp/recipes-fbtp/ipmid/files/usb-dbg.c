/*
 *
 * Copyright 2016-present Facebook. All Rights Reserved.
 *
 * This file represents platform specific implementation for storing
 * SDR record entries and acts as back-end for IPMI stack
 *
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
#include <stdint.h>
#include <unistd.h>
#include <errno.h>
#include <syslog.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <openbmc/pal.h>

typedef struct _post_desc {
  uint8_t code;
  uint8_t desc[32];
} post_desc_t;

typedef struct _gpio_desc {
  uint8_t pin;
  uint8_t level;
  uint8_t def;
  uint8_t desc[32];
} gpio_desc_t;

typedef struct _sensor_desc {
  char name[16];
  int sensor_num;
  char unit[5];
} sensor_desc_c;

//These postcodes are defined in document "F08 BIOS Specification" Revision: 2A
static post_desc_t pdesc_phase1[] = {
  { 0x00, "Not used" },
  { 0x01, "POWER_ON" },
  { 0x02, "MICROCODE" },
  { 0x03, "CACHE_ENABLED" },
  { 0x04, "SECSB_INIT" },
  { 0x05, "OEM_INIT_ENTRY" },
  { 0x06, "CPU_EARLY_INIT" },
  { 0x1D, "OEM_pMEM_INIT" },
  { 0x19, "PM_SB_INITS" },
  { 0xA1, "STS_COLLECT_INFO" },
  { 0xA3, "STS_SETUP_PATH" },
  { 0xA7, "STS_TOPOLOGY_DISC" },
  { 0xA8, "STS_FINAL_ROUTE" },
  { 0xA9, "STS_FINAL_IO_SAD" },
  { 0xAA, "STS_PROTOCOL_SET" },
  { 0xAE, "STS_COHERNCY_SETUP" },
  { 0xAF, "STS_KTI_COMPLETE" },
  { 0xE0, "S3_RESUME_START" },
  { 0xE1, "S3_BOOT_SCRIPT_EXE" },
  { 0xE4, "AMI_PROG_CODE" },
  { 0xE3, "S3_OS_WAKE" },
  { 0xE5, "AMI_PROG_CODE" },
  { 0xB0, "STS_DIMM_DETECT" },
  { 0xB1, "STS_CHECK_INIT" },
  { 0xB4, "STS_RAKE_DETECT" },
  { 0xB2, "STS_SPD_DATA" },
  { 0xB3, "STS_GLOBAL_EARILY" },
  { 0xB6, "STS_DDRIO_INIT" },
  { 0xB7, "STS_TRAIN_DRR" },
  { 0xBE, "STS_GET_MARGINS" },
  { 0xB8, "STS_INIT_THROTTLING" },
  { 0xB9, "STS_MEMBIST" },
  { 0xBA, "STS_MEMINIT" },
  { 0xBB, "STS_DDR_M_INIT" },
  { 0xBC, "STS_RAS_MEMMAP" },
  { 0xBF, "STS_MRC_DONE" },
  { 0xE6, "AMI_PROG_CODE" },
  { 0xE7, "AMI_PROG_CODE" },
  { 0xE8, "S3_RESUME_FAIL" },
  { 0xE9, "S3_PPI_NOT_FOUND" },
  { 0xEB, "S3_OS_WAKE_ERR" },
  { 0xEC, "AMI_ERR_CODE" },
  { 0xED, "AMI_ERR_CODE" },
  { 0xEE, "AMI_ERR_CODE" },

  /*--------------------- UPI Phase - Start--------------------- */
  { 0xA0, "STS_DATA_INIT" },
  { 0xA6, "STS_PBSP_SYNC" },
  { 0xAB, "STS_FULL_SPEED" },
  /*--------------------- UPI Phase - End--------------------- */

  /*--------------------- SEC Phase - Start--------------------- */
  { 0x07, "AP_INIT" },
  { 0x08, "NB_INIT" },
  { 0x09, "SB_INIT" },
  { 0x0A, "OEM_INIT_END" },
  { 0x0B, "CASHE_INIT" },
  { 0x0C, "SEC_ERR" },
  { 0x0D, "SEC_ERR" },
  { 0x0E, "MICROC_N_FOUND" },
  { 0x0F, "MICROC_N_LOAD" },
  /*--------------------- SEC Phase - End----------------------- */

  /*--------------------- PEI Phase - Start--------------------- */
  { 0x10, "PEI_CORE_START" },
  { 0x11, "PM_CPU_INITS" },
  { 0x12, "PM_CPU_INIT1" },
  { 0x13, "PM_CPU_INIT2" },
  { 0x14, "PM_CPU_INIT3" },
  { 0x15, "PM_NB_INITS" },
  { 0x16, "PM_NB_INIT1" },
  { 0x17, "PM_NB_INIT2" },
  { 0x18, "PM_NB_INIT3" },
  { 0x1A, "PM_SB_INIT1" },
  { 0x1B, "PM_SB_INIT2" },
  { 0x1C, "PM_SB_INIT3" },
  { 0x1E, "OEM_pMEM_INIT" },
  { 0x1F, "OEM_pMEM_INIT" },

  { 0x20, "OEM_pMEM_INIT" },
  { 0x21, "OEM_pMEM_INIT" },
  { 0x22, "OEM_pMEM_INIT" },
  { 0x23, "OEM_pMEM_INIT" },
  { 0x24, "OEM_pMEM_INIT" },
  { 0x25, "OEM_pMEM_INIT" },
  { 0x26, "OEM_pMEM_INIT" },
  { 0x27, "OEM_pMEM_INIT" },
  { 0x28, "OEM_pMEM_INIT" },
  { 0x29, "OEM_pMEM_INIT" },
  { 0x2A, "OEM_pMEM_INIT" },
  { 0x2B, "MEM_INIT_SPD" },
  { 0x2C, "MEM_INIT_PRS" },
  { 0x2D, "MEM_INIT_PROG" },
  { 0x2E, "MEM_INIT_CFG" },
  { 0x2F, "MEM_INIT" },

  { 0x30, "ASL_STATUS" },
  { 0x31, "MEM_INSTALLED" },
  { 0x32, "CPU_INITS" },
  { 0x33, "CPU_CACHE_INIT" },
  { 0x34, "CPU_AP_INIT" },
  { 0x35, "CPU_BSP_INIT" },
  { 0x36, "CPU_SMM_INIT" },
  { 0x37, "NB_INITS" },
  { 0x38, "NB_INIT1" },
  { 0x39, "NB_INIT2" },
  { 0x3A, "NB_INIT3" },
  { 0x3B, "SB_INITS" },
  { 0x3C, "SB_INIT1" },
  { 0x3D, "SB_INIT2" },
  { 0x3E, "SB_INIT3" },
  { 0x3F, "OEM_pMEM_INIT" },

  { 0x41, "OEM_pMEM_INIT" },
  { 0x42, "OEM_pMEM_INIT" },
  { 0x43, "OEM_pMEM_INIT" },
  { 0x44, "OEM_pMEM_INIT" },
  { 0x45, "OEM_pMEM_INIT" },
  { 0x46, "OEM_pMEM_INIT" },
  { 0x47, "OEM_pMEM_INIT" },
  { 0x48, "OEM_pMEM_INIT" },
  { 0x49, "OEM_pMEM_INIT" },
  { 0x4A, "OEM_pMEM_INIT" },
  { 0x4B, "OEM_pMEM_INIT" },
  { 0x4C, "OEM_pMEM_INIT" },
  { 0x4D, "OEM_pMEM_INIT" },
  { 0x4E, "OEM_pMEM_INIT" },

  { 0x50, "INVALID_MEM" },
  { 0x51, "SPD_READ_FAIL" },
  { 0x52, "INVALID_MEM_SIZE" },
  { 0x53, "NO_USABLE_MEMORY" },
  { 0x54, "MEM_INIT_ERROR" },
  { 0x55, "MEM_NOT_INSTALLED" },
  { 0x56, "INVALID_CPU" },
  { 0x57, "CPU_MISMATCH" },
  { 0x58, "CPU_SELFTEST_FAIL" },
  { 0x59, "MICROCODE_FAIL" },
  { 0x5A, "CPU_INT_ERR" },
  { 0x5B, "RESET_PPI_ERR" },
  { 0x5C, "BMC_SELF_TEST_F" },
  { 0x5D, "AMI_ERR_CODE" },
  { 0x5E, "AMI_ERR_CODE" },
  { 0x5F, "AMI_ERR_CODE" },

  // S3 Resume Progress Code
  { 0xE2, "S3_VIDEO_REPOST" },

  // S3 Resume Error Code
  { 0xEA, "S3_BOOT_SCRIPT_ERR" },
  { 0xEF, "AMI_ERR_CODE" },

  // Recovery Progress Code
  { 0xF0, "REC_BY_FW" },
  { 0xF1, "REC_BY_USER" },
  { 0xF2, "REC_STARTED" },
  { 0xF3, "REC_FW_FOUNDED" },
  { 0xF4, "REC_FW_LOADED" },
  { 0xF5, "AMI_PROG_CODE" },
  { 0xF6, "AMI_PROG_CODE" },
  { 0xF7, "AMI_PROG_CODE" },

  // Recovery Error code
  { 0xF8, "RECOVERY_PPI_FAIL" },
  { 0xFA, "RECOVERY_CAP_ERR" },
  { 0xFB, "AMI_ERR_CODE" },
  { 0xFC, "AMI_ERR_CODE" },
  { 0xFD, "AMI_ERR_CODE" },
  { 0xFE, "AMI_ERR_CODE" },
  { 0xFF, "AMI_ERR_CODE" },
  /*--------------------- PEI Phase - End----------------------- */

  /*--------------------- MRC Phase - Start--------------------- */
  { 0xB5, "STS_CHAN_EARILY" },
  { 0xBD, "STS_RAS_CONF" },
  /*--------------------- MRC Phase - End----------------------- */

  { 0x4F, "DXE_IPL_START" }
};

static post_desc_t pdesc_phase2[] = {
  { 0x61, "NVRAM_INIT" },
  { 0x9A, "USB_INIT" },
  { 0x78, "ACPI_INIT" },
  { 0x68, "PCI_BRIDEGE_INIT" },
  { 0x70, "SB_DXE_START" },
  { 0x79, "CSM_INIT" },
  { 0xD1, "NB_INIT_ERR" },
  { 0xD2, "SB_INIT_ERR" },
  { 0xD4, "PCI_ALLOC_ERR" },
  { 0x92, "PCIB_INIT" },
  { 0x94, "PCIB_ENUMERATION" },
  { 0x95, "PCIB_REQ_RESOURCE" },
  { 0x96, "PCIB_ASS_RESOURCE" },
  { 0xEF, "PCIB_INIT" },
  { 0x99, "SUPER_IO_INIT" },
  { 0x91, "DRIVER_CONN_START" },
  { 0xD5, "NO_SPACE_ROM" },
  { 0x97, "CONSOLE_INPUT_CONN" },
  { 0xB2, "LEGACY_ROM_INIT" },
  { 0xAA, "ACPI_ACPI_MODE" },
  { 0xC0, "OEM_BDS_INIT" },
  { 0xBB, "AMI_CODE" },
  { 0xC1, "OEM_BDS_INIT" },
  { 0x98, "CONSOLE_OUTPUT_CONN" },
  { 0x9D, "USB_ENABLE" },
  { 0x9C, "USB_DETECT" },
  { 0xB4, "USB_HOT_PLUG" },
  { 0xA0, "IDE_INIT" },
  { 0xA2, "IDE_DETECT" },
  { 0xA9, "START_OF_SETUP" },
  { 0xAB, "SETUP_INIT_WAIT" },

  /*--------------------- ACPI/ASL Phase - Start--------------------- */
  { 0x01, "S1_SLEEP_STATE" },
  { 0x02, "S2_SLEEP_STATE" },
  { 0x03, "S3_SLEEP_STATE" },
  { 0x04, "S4_SLEEP_STATE" },
  { 0x05, "S5_SLEEP_STATE" },
  { 0x06, "S6_SLEEP_STATE" },
  { 0x10, "WEAK_FROM_S1" },
  { 0x20, "WEAK_FROM_S2" },
  { 0x30, "WEAK_FROM_S3" },
  { 0x40, "WEAK_FROM_S4" },
  { 0xAC, "ACPI_PIC_MODE" },
  /*--------------------- ACPI/ASL Phase - Start--------------------- */

  /*--------------------- DXE Phase - Start--------------------- */
  { 0x60, "DXE_CORE_START" },
  { 0x62, "INSTALL_SB_SERVICE" },
  { 0x63, "CPU_DXE_STARTED" },
  { 0x64, "CPU_DXE_INIT" },
  { 0x65, "CPU_DXE_INIT" },
  { 0x66, "CPU_DXE_INIT" },
  { 0x67, "CPU_DXE_INIT" },
  { 0x69, "NB_DEX_INIT" },
  { 0x6A, "NB_DEX_SMM_INIT" },
  { 0x6B, "NB_DEX_BRI_START" },
  { 0x6C, "NB_DEX_BRI_START" },
  { 0x6D, "NB_DEX_BRI_START" },
  { 0x6E, "NB_DEX_BRI_START" },
  { 0x6F, "NB_DEX_BRI_START" },

  { 0x71, "SB_DEX_SMM_INIT" },
  { 0x72, "SB_DEX_DEV_START" },
  { 0x73, "SB_DEX_BRI_START" },
  { 0x74, "SB_DEX_BRI_START" },
  { 0x75, "SB_DEX_BRI_START" },
  { 0x76, "SB_DEX_BRI_START" },
  { 0x77, "SB_DEX_BRI_START" },
  { 0x7A, "AMI_DXE_CODE" },
  { 0x7B, "AMI_DXE_CODE" },
  { 0x7C, "AMI_DXE_CODE" },
  { 0x7D, "AMI_DXE_CODE" },
  { 0x7E, "AMI_DXE_CODE" },
  { 0x7F, "AMI_DXE_CODE" },

  //OEM DXE INIT CODE
  { 0x80, "OEM_DEX_INIT" },
  { 0x81, "OEM_DEX_INIT" },
  { 0x82, "OEM_DEX_INIT" },
  { 0x83, "OEM_DEX_INIT" },
  { 0x84, "OEM_DEX_INIT" },
  { 0x85, "OEM_DEX_INIT" },
  { 0x86, "OEM_DEX_INIT" },
  { 0x87, "OEM_DEX_INIT" },
  { 0x88, "OEM_DEX_INIT" },
  { 0x89, "OEM_DEX_INIT" },
  { 0x8A, "OEM_DEX_INIT" },
  { 0x8B, "OEM_DEX_INIT" },
  { 0x8C, "OEM_DEX_INIT" },
  { 0x8D, "OEM_DEX_INIT" },
  { 0x8E, "OEM_DEX_INIT" },
  { 0x8F, "OEM_DEX_INIT" },

  //BDS EXECUTION
  { 0x90, "BDS_START" },
  { 0x93, "PCIB_HOT_PLUG_INIT" },
  { 0x9B, "USB_RESET" },
  { 0x9E, "AMI_CODE" },
  { 0x9F, "AMI_CODE" },

  { 0xA1, "IDE_RESET" },
  { 0xA3, "IDE_ENABLE" },
  { 0xA4, "SCSI_INIT" },
  { 0xA5, "SCSI_RESET" },
  { 0xA6, "SCSI_DETECT" },
  { 0xA7, "SCSI_ENABLE" },
  { 0xA8, "SETUP_VERIFY_PW" },
  { 0xAD, "READY_TO_BOOT" },
  { 0xAE, "LEGACY_BOOT_EVE" },
  { 0xAF, "EXIT_BOOT_EVE" },

  { 0xB0, "SET_VIR_ADDR_START" },
  { 0xB1, "SET_VIR_ADDR_END" },
  { 0xB3, "SYS_RESET" },
  { 0xB5, "PCIB_HOT_PLUG" },
  { 0xB6, "CLEAN_NVRAM" },
  { 0xB7, "CONFIG_RESET" },
  { 0xB8, "AMI_CODE" },
  { 0xB9, "AMI_CODE" },
  { 0xBA, "ASL_CODE" },
  { 0xBC, "AMI_CODE" },
  { 0xBD, "AMI_CODE" },
  { 0xBE, "AMI_CODE" },
  { 0xBF, "AMI_CODE" },

  //OEM BDS Initialization Code
  { 0xC2, "OEM_BDS_INIT" },
  { 0xC3, "OEM_BDS_INIT" },
  { 0xC4, "OEM_BDS_INIT" },
  { 0xC5, "OEM_BDS_INIT" },
  { 0xC6, "OEM_BDS_INIT" },
  { 0xC7, "OEM_BDS_INIT" },
  { 0xC8, "OEM_BDS_INIT" },
  { 0xC9, "OEM_BDS_INIT" },
  { 0xCA, "OEM_BDS_INIT" },
  { 0xCB, "OEM_BDS_INIT" },
  { 0xCC, "OEM_BDS_INIT" },
  { 0xCD, "OEM_BDS_INIT" },
  { 0xCE, "OEM_BDS_INIT" },
  { 0xCF, "OEM_BDS_INIT" },

  //DXE Phase
  { 0xD0, "CPU_INIT_ERR" },
  { 0xD3, "ARCH_PROT_ERR" },
  { 0xD6, "NO_CONSOLE_OUT" },
  { 0xD7, "NO_CONSOLE_IN" },
  { 0xD8, "INVALID_PW" },
  { 0xD9, "BOOT_OPT_ERR" },
  { 0xDA, "BOOT_OPT_FAIL" },
  { 0xDB, "FLASH_UPD_FAIL" },
  { 0xDC, "RST_PROT_NA" },
  { 0xDD, "DEX_SELTEST_FAILs" }
  /*--------------------- DXE Phase - End--------------------- */

};

static gpio_desc_t gdesc[] = {
  { 0x10, 0, 2, "FM_DBG_RST_BTN" },
  { 0x11, 0, 1, "FM_PWR_BTN" },
  { 0x12, 0, 0, "SYS_PWROK" },
  { 0x13, 0, 0, "RST_PLTRST" },
  { 0x14, 0, 0, "DSW_PWROK" },
  { 0x15, 0, 0, "FM_CPU_CATERR" },
  { 0x16, 0, 0, "FM_SLPS3" },
  { 0x17, 0, 0, "FM_CPU_MSMI" },
};

static int gdesc_count = sizeof(gdesc) / sizeof (gpio_desc_t);

static sensor_desc_c cri_sensor[]  =
{
    {"CPU0_TEMP:"      , MB_SENSOR_CPU0_TEMP        ,"C"},
    {"CPU1_TEMP:"      , MB_SENSOR_CPU1_TEMP        ,"C"},
    {"HSC_PWR:"        , MB_SENSOR_HSC_IN_POWER     ,"W"},
    {"HSC_VOL:"        , MB_SENSOR_HSC_IN_VOLT      ,"V"},
    {"FAN0:"           , MB_SENSOR_FAN0_TACH        ,"RPM"},
    {"FAN1:"           , MB_SENSOR_FAN1_TACH        ,"RPM"},
    {"LAST_KEY",' ' ," " },
};

#define LINE_PER_PAGE 7
#define LEN_PER_LINE 16
#define LEN_PER_PAGE (LEN_PER_LINE*LINE_PER_PAGE)
#define MAX_PAGE 20
#define MAX_LINE (MAX_PAGE*LINE_PER_PAGE)

static int
read_device(const char *device,  char *value) {
  FILE *fp;
  int rc;

  fp = fopen(device, "r");
  if (!fp) {
    int err = errno;
#ifdef DEBUG
    syslog(LOG_INFO, "failed to open device in usb-dbg.c %s", device);
#endif
    return err;
  }

  rc = fscanf(fp, "%s", value);
  fclose(fp);
  if (rc != 1) {
#ifdef DEBUG
    syslog(LOG_INFO, "failed to read device in usb-dbg.c %s", device);
#endif
    return ENOENT;
  } else {
    return 0;
  }
}

static int
plat_chk_cri_sel_update(uint8_t *cri_sel_up) {
  static time_t mtime;
  FILE *fp;
  struct stat file_stat;

  fp = fopen("/mnt/data/cri_sel", "r");
  if (fp) {
    if ((stat("/mnt/data/cri_sel", &file_stat) == 0) && (file_stat.st_mtime > mtime)) {
      mtime = file_stat.st_mtime;
      *cri_sel_up = 1;
    } else {
      *cri_sel_up = 0;
    }
    fclose(fp);
  }
  return 0;
}

int
plat_udbg_get_frame_info(uint8_t *num) {
  *num = 3;
  syslog(LOG_WARNING, "mini test");
  return 0;
}

int
plat_udbg_get_updated_frames(uint8_t *count, uint8_t *buffer) {
  uint8_t cri_sel_up;
  uint8_t info_page_up = 0;

  syslog(LOG_WARNING, "plat_udbg_get_updated_frames");
  *count = 0;

  //cri sel update
  plat_chk_cri_sel_update(&cri_sel_up);
  if(cri_sel_up == 1) {
    *count += 1;
    buffer[*count-1] = 1;
  }
  syslog(LOG_WARNING, "mini cri_sel_up:%d, *count:%d, buffer[*count-1]:%d", cri_sel_up,*count,buffer[*count-1]);

  //cri sensor update
  *count += 1;
  buffer[*count-1] = 2;
  syslog(LOG_WARNING, "mini  *count:%d, buffer[*count-1]:%d", *count,buffer[*count-1]);

  //info page update
  pal_post_end_chk(&info_page_up);
  if(info_page_up == 1) {
    *count += 1;
    buffer[*count-1] = 3;
  }
  syslog(LOG_WARNING, "udbg_get_updated_frames: *count %d, buffer:%d %d", *count,buffer[0],buffer[1]);

  return 0;
}

int
plat_udbg_get_post_desc(uint8_t index, uint8_t *next, uint8_t phase,  uint8_t *end, uint8_t *length, uint8_t *buffer) {
  int target, pdesc_size;
  post_desc_t *ptr;
  post_desc_t arr[200];

  switch (phase){
    case 1:
      ptr = pdesc_phase1;
      pdesc_size  =  sizeof(pdesc_phase1) /sizeof(post_desc_t);
      break;
    case 2:
      ptr = pdesc_phase2;
      pdesc_size  =  sizeof(pdesc_phase2) /sizeof(post_desc_t);
      break;
    default:
      return -1;
      break;
  }

  for (target = 0; target < pdesc_size; target++) {
    if (index==ptr->code) {
      *length = strlen(ptr->desc);
      memcpy(buffer, ptr->desc, *length);
      buffer[*length] = '\0';

      if (index == 0x4f) {
        *next = pdesc_phase2[0].code;
        *end = 0x00;
      }
      else if (target == pdesc_size - 1) {
        *next = 0xFF;
        *end = 0x01;
      }
      else {
        *next = (ptr+1)->code;
        *end = 0x00;
      }
      return 0;
    }
  ptr=ptr+1;
  }
  return -1;
}

int
plat_udbg_get_gpio_desc(uint8_t index, uint8_t *next, uint8_t *level, uint8_t *def,
                            uint8_t *count, uint8_t *buffer) {
  int i = 0;

  // If the index is 0x00: populate the next pointer with the first
  if (index == 0x00) {
    *next = gdesc[0].pin;
    *count = 0;
    return 0;
  }

  // Look for the index
  for (i = 0; i < gdesc_count; i++) {
    if (index == gdesc[i].pin) {
      break;
    }
  }

  // Check for not found
  if (i == gdesc_count) {
    return -1;
  }

  // Populate the description and next index
  *level = gdesc[i].level;
  *def = gdesc[i].def;
  *count = strlen(gdesc[i].desc);
  memcpy(buffer, gdesc[i].desc, *count);
  buffer[*count] = '\0';

  // Populate the next index
  if (i == gdesc_count-1) { // last entry
    *next = 0xFF;
  } else {
    *next = gdesc[i+1].pin;
  }

  return 0;
}

static int
plat_udbg_get_cri_sel(uint8_t frame, uint8_t page, uint8_t *next, uint8_t *count, uint8_t *buffer) {
  static char frame_buff[MAX_PAGE * LEN_PER_PAGE];
  static time_t mtime;
  static int page_num = 1, line_num = 0;
  int i, len, msg_line;
  char line_buff[256], *ptr;
  FILE *fp;
  struct stat file_stat;

  fp = fopen("/mnt/data/cri_sel", "r");
  if (fp) {
    if ((stat("/mnt/data/cri_sel", &file_stat) == 0) && (file_stat.st_mtime > mtime)) {
      mtime = file_stat.st_mtime;
      memset(frame_buff, ' ', sizeof(frame_buff));

      line_num = 0;
      while (fgets(line_buff, 256, fp)) {
        // Remove newline
        line_buff[strlen(line_buff)-1] = '\0';
        ptr = line_buff;
        // Find message
        ptr = strstr(ptr, "local0.err");
        if(ptr)
          ptr = strstr(ptr, ":");
        len = (ptr)?strlen(ptr):0;
        if (len > 2) {
          ptr+=2;
          len-=2;
        } else {
          continue;
        }

        // line number of this 1 message
        msg_line = (len/LEN_PER_LINE) + ((len%LEN_PER_LINE)?1:0);

        // total line number after this message
        if ((line_num + msg_line) < MAX_LINE)
          line_num += msg_line;
        else
          line_num = MAX_LINE;

        // Scroll message down
        for(i = (line_num-1); (i-msg_line)>=0; i--)
          memcpy(&frame_buff[LEN_PER_LINE*i], &frame_buff[LEN_PER_LINE*(i-msg_line)], LEN_PER_LINE);

        // Write new message
        memcpy(&frame_buff[0], ptr, len);
        memset(&frame_buff[len], ' ', msg_line*LEN_PER_LINE-len);
      }
    }
    fclose(fp);
    page_num = line_num/LINE_PER_PAGE + ((line_num%LINE_PER_PAGE)?1:0);
  } else {
    memset(frame_buff, ' ', sizeof(frame_buff));
    page_num = 1;
    line_num = 0;
  }

  if (page > page_num) {
    return -1;
  }

  // Frame Head
  snprintf(line_buff, 17, "Cri SEL   P%2d/%2d", page, page_num);
  memcpy(&buffer[0], line_buff, 16); // First line

  // Frame Body
  memcpy(&buffer[16], &frame_buff[(page-1)*LEN_PER_PAGE], LEN_PER_PAGE);


  *count = 128;
  if (page == page_num) {
    // Set next to 0xFF to indicate this is last page
    *next = 0xFF;
  } else {
    *next = page+1;
  }

  return 0;
}

static int
plat_udbg_get_cri_sensor (uint8_t frame, uint8_t page, uint8_t *next, uint8_t *count, uint8_t *buffer) {
  int page_num;
  char page_buff[128], val[50] = {0}, str[16] = {0};
  int LineOffset = 0 ;
  char FilePath [] = "/tmp/cache_store/mb_sensor", SensorFilePath [30];

  //Each Page has seven sensors
  int SensorPerPage = 7;
  int SensorIndex = (page-1) * SensorPerPage;
  memset(page_buff, ' ', sizeof(page_buff));

  //Total page = (Total sensor - "LAST KEY") / sensor per page
  if((sizeof(cri_sensor)-sizeof(cri_sensor[0])) % (sizeof(cri_sensor[0])*SensorPerPage))
    page_num = (sizeof(cri_sensor)-sizeof(cri_sensor[0])) / (sizeof(cri_sensor[0])*SensorPerPage) + 1;
  else
    page_num = (sizeof(cri_sensor)-sizeof(cri_sensor[0])) / (sizeof(cri_sensor[0])*SensorPerPage) ;

  // Frame Head
  snprintf(page_buff, 17, "CriSensor  P%d/%d", page, page_num);
  memcpy(&buffer[LineOffset], page_buff, 16);

  // Frame Body
  while(strcmp(cri_sensor[SensorIndex].name, "LAST_KEY")){
    LineOffset  += LEN_PER_LINE;
    memset(str,' ', sizeof(str));
    memcpy(&buffer[LineOffset], cri_sensor[SensorIndex].name, strlen(cri_sensor[SensorIndex].name));
    sprintf(SensorFilePath, "%s%d", FilePath, cri_sensor[SensorIndex].sensor_num);
    if (read_device(SensorFilePath, val))
      snprintf(str,LEN_PER_LINE, "Read failed");
    else if(!strcmp(val, "NA") || strlen(val) == 0)
      snprintf(str,LEN_PER_LINE, "NA");
    else
      snprintf(str, LEN_PER_LINE, "%s%s", val, cri_sensor[SensorIndex].unit);
    memcpy(&buffer[LineOffset + strlen(cri_sensor[SensorIndex].name)], str, strlen(str));
    memset(&buffer[LineOffset + strlen(cri_sensor[SensorIndex].name) + strlen(str) ], ' ', LEN_PER_LINE - strlen(cri_sensor[SensorIndex].name) - strlen(str));
    SensorIndex++;
  }
  *count = 128;

  int k;
  for (k = 0; k < *count; k++)
    if(buffer[k] == 0)
      memset(&buffer[k], ' ', 1);

  if (page == page_num)
    *next = 0xFF;    // Set the value of next to 0xFF to indicate this is the last page
  else
    *next = page+1;
  return 0;
}


static int
plat_udbg_get_info_page (uint8_t frame, uint8_t page, uint8_t *next, uint8_t *count, uint8_t *buffer) {
  syslog(LOG_WARNING, "mini udbg_get_info_page");
  char page_buff[128];
  int page_num;
  int LineOffset = 0 ;

  page_num = 1;
  memset(page_buff, ' ', sizeof(page_buff));
  // Frame Head
  snprintf(page_buff, 17, "info  P%d/%d", page, page_num);
  memcpy(&buffer[LineOffset], page_buff, 16);
  *count = 16;
  if (page == page_num)
    *next = 0xFF;    // Set the value of next to 0xFF to indicate this is the last page
  else
    *next = page+1;

  return 0;
}

int
plat_udbg_get_frame_data(uint8_t frame, uint8_t page, uint8_t *next, uint8_t *count, uint8_t *buffer) {

  switch (frame) {
    case 1: // critical SEL
      return plat_udbg_get_cri_sel(frame, page, next, count, buffer);
    case 2: //critical Sensor
      return plat_udbg_get_cri_sensor(frame, page, next, count, buffer);
    case 3: //info_page
      return plat_udbg_get_info_page(frame, page, next, count, buffer);
    default:
      return -1;
  }
}
