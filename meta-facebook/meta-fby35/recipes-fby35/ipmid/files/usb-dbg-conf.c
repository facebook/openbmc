#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include <stdio.h>
#include <openbmc/kv.h>
#include <openbmc/pal.h>
#include <openbmc/sdr.h>
#include <openbmc/obmc-i2c.h>
#include <openbmc/obmc_pal_sensors.h>
#include <facebook/bic.h>
#include <facebook/fby35_common.h>
#include <syslog.h>

#include "usb-dbg-conf.h"
#if defined CONFIG_POSTCODE_AMD
#include "postcode-amd.h"
#endif

#define ESCAPE "\x1B"
#define ESC_ALT ESCAPE"[5;7m"
#define ESC_RST ESCAPE"[m"

#define MAX_SENSOR_NAME 31
static sensor_desc_t critical_sensor_list[MAX_SENSOR_NUM + 1] = {0};

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
  { 0xE0, "BMC" },
  { 0xE1, "SLOT1" },
  { 0xE4, "SLOT4" },
  { 0xE3, "SLOT3" },
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

  // slot2
  { 0xE2, "SLOT2" },

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
  { 0xDD, "DEX_SELTEST_FAILs" },
  /*--------------------- DXE Phase - End--------------------- */

  /*--------------------- SLOT Info Start--------------------- */
  { 0xE0, "BMC" },
  { 0xE1, "SLOT1" },
  { 0xE2, "SLOT2" },
  { 0xE3, "SLOT3" },
  { 0xE4, "SLOT4" }
  /*--------------------- SLOT Info End--------------------- */
};

static post_phase_desc_t post_phase_desc[] = {
  {1, pdesc_phase1, sizeof(pdesc_phase1)/sizeof(pdesc_phase1[0])},
  {2, pdesc_phase2, sizeof(pdesc_phase2)/sizeof(pdesc_phase2[0])},
};

static dbg_gpio_desc_t gdesc[] = {
 { 0x10, 0, 1, "RST_BTN_N" },
 { 0x11, 0, 1, "PWR_BTN_N" },
 { 0x12, 0, 0, "SYS_PWROK" },
 { 0x13, 0, 0, "RST_PLTRST" },
 { 0x14, 0, 0, "DSW_PWROK" },
 { 0x15, 0, 0, "FM_CATERR_MSMI" },
 { 0x16, 0, 0, "FM_SLPS3" },
 { 0x17, 0, 1, "FM_UART_SWITCH" },
};

bool plat_supported(void)
{
  return true;
}

int plat_get_post_phase(uint8_t fru, post_phase_desc_t **desc, size_t *desc_count)
{
  if (!desc || !desc_count) {
    return -1;
  }
  *desc = post_phase_desc;
  *desc_count = sizeof(post_phase_desc) / sizeof(post_phase_desc[0]);

  return 0;
}

int plat_get_gdesc(uint8_t fru, dbg_gpio_desc_t **desc, size_t *desc_count)
{
  if (!desc || !desc_count) {
    return -1;
  }
  *desc = gdesc;
  *desc_count = sizeof(gdesc) / sizeof(gdesc[0]);
  return 0;
}

int plat_get_sensor_desc(uint8_t fru, sensor_desc_t **desc, size_t *desc_count)
{
  int ret = 0;
  uint8_t *sensor_nums;
  int i = 0;
  int count = 0;
  thresh_sensor_t sensor_info;

  if (!desc) {
    syslog(LOG_ERR, "%s() Variable: desc NULL pointer ERROR", __func__);
    return -1;
  }

  if (!desc_count) {
    syslog(LOG_ERR, "%s() Variable: desc_count NULL pointer ERROR", __func__);
    return -1;
  }
  if (fru == UART_SELECT_BMC) {
    fru = FRU_BMC;
  }
  if (pal_get_fru_sensor_list(fru, &sensor_nums, &count) < 0) {
    return -1;
  }
  *desc_count = (size_t)count;
  for (i = 0; i < count; i++) {
    ret = sdr_get_snr_thresh(fru, sensor_nums[i], &sensor_info);
    if (ret < 0) {
      // skip this sensor if get SDR failed
      (*desc_count)--;
      continue;
    }
    memcpy(critical_sensor_list[i].name, sensor_info.name, MAX_SENSOR_NAME);
    strcat(critical_sensor_list[i].name, ":");
    critical_sensor_list[i].sensor_num = sensor_nums[i];
    if (strncmp(sensor_info.units, "Volts", sizeof(sensor_info.units)) == 0) {
      strncpy(critical_sensor_list[i].unit, "V", sizeof(critical_sensor_list[i].unit));
      critical_sensor_list[i].disp_prec = 2;
    } else if (strncmp(sensor_info.units, "Amps", sizeof(sensor_info.units)) == 0) {
      strncpy(critical_sensor_list[i].unit, "A", sizeof(critical_sensor_list[i].unit));
      critical_sensor_list[i].disp_prec = 2;
    } else if (strncmp(sensor_info.units, "Watts", sizeof(sensor_info.units)) == 0) {
      strncpy(critical_sensor_list[i].unit, "W", sizeof(critical_sensor_list[i].unit));
      critical_sensor_list[i].disp_prec = 2;
    } else {
      memcpy(critical_sensor_list[i].unit, sensor_info.units, sizeof(critical_sensor_list[i].unit));
      critical_sensor_list[i].disp_prec = 0;
    }
    critical_sensor_list[i].fru = (fru == FRU_BMC)? FRU_BMC: FRU_ALL;
  }
  *desc = critical_sensor_list;

  return ret;
}

uint8_t plat_get_fru_sel(void)
{
  uint8_t pos;

  if (pal_get_uart_select(&pos)) {
    return 0;
  }
  return pos;
}

int plat_get_me_status(uint8_t fru, char *status)
{
  char tbuf[256];
  char rbuf[256];
  unsigned char rlen;
  int ret;

  // ME only on Crater Lake system
  if (fby35_common_get_slot_type(fru) != SERVER_TYPE_CL) {
    return -1;
  }

  tbuf[0] = NETFN_APP_REQ << 2;
  tbuf[1] = CMD_APP_GET_DEVICE_ID;
  ret = bic_me_xmit(fru, (uint8_t *)tbuf, 2, (uint8_t *)rbuf, &rlen);
  if (ret || rbuf[0]) {
    return -1;
  }
  strcpy(status, (rbuf[3] & 0x80) ? "recovert_mode" : "operation mode");
  return 0;
}

#if defined CONFIG_HALFDOME
int plat_dword_postcode_buf(uint8_t fru, char *status) {
  int ret = 0;
  uint32_t len;
  uint32_t intput_len = 0;
  int i;
  uint32_t * dw_postcode_buf = malloc( 30 * sizeof(uint32_t));
  char temp_str[128]  = {0};
  if (dw_postcode_buf) {
    intput_len = 30;
  } else {
    syslog(LOG_ERR, "%s Error, failed to allocate dw_postcode buffer", __func__);
    intput_len = 0;
    return -1;
  }

  ret = bic_request_post_buffer_dword_data(fru, dw_postcode_buf, intput_len, &len);
  if (ret) {
    syslog(LOG_WARNING, "plat_dword_postcode_buf, FRU: %d, bic_request_post_buffer_dword_data ret: %d\n", fru, ret);
    free(dw_postcode_buf);
    return ret;
  }

  for (i = 0; i < len; i++) {
    pal_parse_amd_post_code_helper(dw_postcode_buf[i],temp_str);
    strcat(status, temp_str);
  }
  if(dw_postcode_buf)
    free(dw_postcode_buf);

  return ret;

}
#endif

int plat_get_mrc_desc(uint8_t fru, uint16_t major, uint16_t minor, char *desc) {
#ifdef CONFIG_HALFDOME
  uint8_t server_type;

  server_type = fby35_common_get_slot_type(fru);
  if (server_type == SERVER_TYPE_HD) {
    return plat_get_amd_mrc_desc(major, minor, desc);
  }
#endif
  return pal_get_mrc_desc(major, minor, desc);
}

int plat_get_board_id(char *id)
{
  uint8_t pos = plat_get_fru_sel();
  uint8_t buf[256] = {0x00};
  uint8_t rlen;

  if (pal_get_board_id(pos, buf, 0, buf, &rlen)) {
    return -1;
  }

  sprintf(id, "%02d", buf[2]);
  return 0;
}

int plat_get_syscfg_text(uint8_t fru, char *syscfg)
{
  static const uint8_t cl_dimm_cache_id[6] = { 0, 4, 6, 8, 12, 14, };
  static const uint8_t hd_dimm_cache_id[8] = { 0, 1, 2, 4, 6, 7, 8, 10};
  static const uint8_t gl_dimm_cache_id[8] = { 0, 1, 2, 3, 4, 5, 6, 7};
  static const uint8_t* dimm_cache_id = cl_dimm_cache_id;
  static const char *cl_dimm_label[6] = { "A0", "A2", "A3", "A4", "A6", "A7"};
  static const char *hd_dimm_label[8] = { "A0", "A1", "A2", "A4", "A6", "A7", "A8", "A10"};
  static const char *gl_dimm_label[8] = { "A0", "A1", "A2", "A3", "A4", "A5", "A6", "A7"};
  const char **dimm_label = cl_dimm_label;
  char *key_prefix = "sys_config/";
  char key[MAX_KEY_LEN], value[MAX_VALUE_LEN], entry[MAX_VALUE_LEN];
  char *token;
  uint8_t cpu_name_pos = 0;
  uint8_t dimm_num = 0;
  uint8_t index, pos;
  uint8_t cpu_core;
  uint16_t dimm_speed;
  uint32_t dimm_capacity;
  float cpu_speed;
  int slen;
  int server_type = SERVER_TYPE_NONE;
  size_t ret;

  if (fru == FRU_ALL)
    return -1;

  if (syscfg == NULL)
    return -1;

  // Clear string buffer
  syscfg[0] = '\0';

  // CPU information
  slen = snprintf(entry, sizeof(entry), "CPU:");

  server_type = fby35_common_get_slot_type(fru);
  if (server_type == SERVER_TYPE_CL) {
    cpu_name_pos = 3;  //Intel(R) Xeon(R) Gold "6450C"
    dimm_cache_id = cl_dimm_cache_id;
    dimm_label = cl_dimm_label;
    dimm_num = ARRAY_SIZE(cl_dimm_cache_id);
  } else if (server_type == SERVER_TYPE_HD) {
    cpu_name_pos = 2;  // AMD EPYC "9D64" 88-Core Processor
    dimm_cache_id = hd_dimm_cache_id;
    dimm_label = hd_dimm_label;
    dimm_num = ARRAY_SIZE(hd_dimm_cache_id);
  } else if (server_type == SERVER_TYPE_GL) {
    cpu_name_pos = 3;  // The CPU is RD sample now, so there is no model number
    dimm_cache_id = gl_dimm_cache_id;
    dimm_label = gl_dimm_label;
    dimm_num = ARRAY_SIZE(gl_dimm_cache_id);
  } else {
    syslog(LOG_WARNING, "Unknown server type: %d", server_type);
    return -1;
  }

  // Processor#
  snprintf(key, sizeof(key), "%sfru%d_cpu0_product_name", key_prefix, fru);
  if (kv_get(key, value, &ret, KV_FPERSIST) == 0) {
    token = strtok(value, " ");
    for (pos = 0; token && pos < cpu_name_pos; pos++) {
      token = strtok(NULL, " ");
    }
    if (token) {
      slen += snprintf(&entry[slen], sizeof(entry) - slen, "%s", token);
    }
  }

  // Frequency & Core Number
  snprintf(key, sizeof(key), "%sfru%d_cpu0_basic_info", key_prefix, fru);
  if (kv_get(key, value, &ret, KV_FPERSIST) == 0 && ret >= 5) {
    cpu_speed = (float)(value[4] << 8 | value[3])/1000;
    cpu_core = value[0];
    slen += snprintf(&entry[slen], sizeof(entry) - slen, "/%.1fG/%dc", cpu_speed, cpu_core);
  }

  snprintf(&entry[slen], sizeof(entry) - slen, "\n");
  strcat(syscfg, entry);

 // DIMM information
  for (index = 0; index < dimm_num; index++) {
    slen = snprintf(entry, sizeof(entry), "MEM%s:", dimm_label[index]);

    // Check Present
    snprintf(key, sizeof(key), "%sfru%d_dimm%d_location", key_prefix, fru, dimm_cache_id[index]);
    if (kv_get(key, value, &ret, KV_FPERSIST) == 0 && ret >= 1) {
      // Skip if not present
      if (value[0] != 0x01)
        continue;
    }

    // Module Manufacturer ID
    snprintf(key, sizeof(key), "%sfru%d_dimm%d_manufacturer_id", key_prefix, fru, dimm_cache_id[index]);
    if (kv_get(key, value, &ret, KV_FPERSIST) == 0 && ret >= 2) {
      switch (value[1]) {
        case 0xce:
          slen += snprintf(&entry[slen], sizeof(entry) - slen, "Samsung");
          break;
        case 0xad:
          slen += snprintf(&entry[slen], sizeof(entry) - slen, "Hynix");
          break;
        case 0x2c:
          slen += snprintf(&entry[slen], sizeof(entry) - slen, "Micron");
          break;
        default:
          slen += snprintf(&entry[slen], sizeof(entry) - slen, "unknown");
          break;
      }
    }

    // Speed & Capacity
    snprintf(key, sizeof(key), "%sfru%d_dimm%d_speed", key_prefix, fru, dimm_cache_id[index]);
    if (kv_get(key, value, &ret, KV_FPERSIST) == 0 && ret >= 6) {
      dimm_speed = value[1]<<8 | value[0];
      dimm_capacity = (value[5]<<24 | value[4]<<16 | value[3]<<8 | value[2])/1024;
      slen += snprintf(&entry[slen], sizeof(entry) - slen, "/%uMhz/%uGB", dimm_speed, dimm_capacity);
    }

    snprintf(&entry[slen], sizeof(entry) - slen, "\n");
    strcat(syscfg, entry);
  }

  return 0;
}

int
plat_get_etra_fw_version(uint8_t slot_id, char *fw_text)
{
  char entry[256];
  uint8_t ver[32] = {0};
  uint8_t rlen = 4;
  uint8_t* plat_name = NULL;

  if (fw_text == NULL)
    return -1;

  // Clear string buffer
  fw_text[0] = '\0';

  //CPLD Version
  if (slot_id == FRU_ALL) { //uart select BMC position
    if (!pal_get_cpld_ver(FRU_BMC, ver)) {
      sprintf(entry, "CPLD_ver:\n%02X%02X%02X%02X\n", ver[3], ver[2], ver[1], ver[0]);
      strcat(fw_text, entry);
    }
  } else {
    //Bridge-IC Version
    if (bic_get_fw_ver(slot_id, FW_SB_BIC, ver)) {
      strcat(fw_text,"BIC_ver:\nNA\n");
    } else {
      if (strlen((char*)ver) == 2) { // old version format
        snprintf(entry, sizeof(entry), "BIC_ver:\nv%x.%02x\n", ver[0], ver[1]);
      } else if (strlen((char*)ver) >= 4){ // new version format
        plat_name = ver + 4;
        snprintf(entry, sizeof(entry), "BIC_ver:\noby35-%s-v%02x%02x.%02x.%02x\n", (char*)plat_name, ver[0], ver[1], ver[2], ver[3]);
      } else {
        snprintf(entry, sizeof(entry), "BIC_ver:\nFormat not supported\n");
      }

      strcat(fw_text, entry);
    }

    if (pal_get_fw_info(slot_id, FW_CPLD, ver, &rlen)) {
      strcat(fw_text,"CPLD_ver:\nNA\n");
    } else {
      sprintf(entry,"CPLD_ver:\n%02X%02X%02X%02X\n", ver[0], ver[1], ver[2], ver[3]);
      strcat(fw_text, entry);
    }
  }

  return 0;
}

int plat_get_extra_sysinfo(uint8_t fru, char *info)
{
  char fru_name[16];
  char post_code_info[32] = {0};
  char postcode[8] = {0};

  if (fru == FRU_ALL) {
    fru = FRU_BMC;
  }

  if (!pal_get_fru_name(fru, fru_name)) {
    sprintf(info, "FRU:%s", fru_name);
  }

  if ((fru != FRU_BMC) && (fby35_common_get_slot_type(fru) == SERVER_TYPE_HD)) {
    if (pal_get_last_postcode(fru, postcode)) {
      sprintf(post_code_info, "\nPOST: NA\n");
    } else {
      sprintf(post_code_info, "\nPOST: %s\n", postcode);
    }
    strcat(info, post_code_info);
  }
  return 0;
}
