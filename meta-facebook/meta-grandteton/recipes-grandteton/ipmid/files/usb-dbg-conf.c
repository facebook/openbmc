#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include <stdio.h>
#include <openbmc/pal.h>
#include <openbmc/ipmb.h>
#include "usb-dbg-conf.h"
#include <syslog.h>
#if defined CONFIG_POSTCODE_AMD
#include "postcode-amd.h"
#endif

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

static post_phase_desc_t post_phase_desc[] = {
  {1, pdesc_phase1, sizeof(pdesc_phase1)/sizeof(pdesc_phase1[0])},
  {2, pdesc_phase2, sizeof(pdesc_phase2)/sizeof(pdesc_phase2[0])},
};

static dbg_gpio_desc_t gdesc[] = {
  { 0x10, 0, 2, "FM_DBG_RST_BTN" },
  { 0x11, 0, 1, "FM_PWR_BTN" },
  { 0x12, 1, 0, "SYS_PWROK" },
  { 0x13, 0, 0, "RST_PLTRST" },
  { 0x14, 1, 0, "DSW_PWROK" },
  { 0x15, 0, 0, "FM_CATERR_MSMI" },
  { 0x16, 0, 0, "FM_SLPS3" },
  { 0x17, 0, 3, "FM_UART_SWITCH" },
};

static sensor_desc_t hsc_cri_sensor[] = {
    {"HSC_PWR:",  MB_SNR_HSC_PIN, "W", FRU_MB, 1},
    {"HSC_VOL:",  MB_SNR_HSC_VIN, "V", FRU_MB, 2},
};

static sensor_desc_t cri_sensor[] = {
    {"P0_TEMP:", MB_SNR_CPU0_TEMP, "C", FRU_MB, 0},
    {"P1_TEMP:", MB_SNR_CPU1_TEMP, "C", FRU_MB, 0},
    {"FAN0_INLET_SP:",   FAN_BP1_SNR_FAN0_INLET_SPEED,   "RPM", FRU_FAN_BP1, 0},
    {"FAN0_OUTLET_SP:",  FAN_BP1_SNR_FAN0_OUTLET_SPEED,  "RPM", FRU_FAN_BP1, 0},
    {"FAN1_INLET_SP:",   FAN_BP1_SNR_FAN1_INLET_SPEED,   "RPM", FRU_FAN_BP1, 0},
    {"FAN1_OUTLET_SP:",  FAN_BP1_SNR_FAN1_OUTLET_SPEED,  "RPM", FRU_FAN_BP1, 0},
    {"FAN2_INLET_SP:",   FAN_BP2_SNR_FAN2_INLET_SPEED,   "RPM", FRU_FAN_BP2, 0},
    {"FAN2_OUTLET_SP:",  FAN_BP2_SNR_FAN2_OUTLET_SPEED,  "RPM", FRU_FAN_BP2, 0},
    {"FAN3_INLET_SP:",   FAN_BP2_SNR_FAN3_INLET_SPEED,   "RPM", FRU_FAN_BP2, 0},
    {"FAN3_OUTLET_SP:",  FAN_BP2_SNR_FAN3_OUTLET_SPEED,  "RPM", FRU_FAN_BP2, 0},
    {"FAN4_INLET_SP:",   FAN_BP1_SNR_FAN4_INLET_SPEED,   "RPM", FRU_FAN_BP1, 0},
    {"FAN4_OUTLET_SP:",  FAN_BP1_SNR_FAN4_OUTLET_SPEED,  "RPM", FRU_FAN_BP1, 0},
    {"FAN5_INLET_SP:",   FAN_BP1_SNR_FAN5_INLET_SPEED,   "RPM", FRU_FAN_BP1, 0},
    {"FAN5_OUTLET_SP:",  FAN_BP1_SNR_FAN5_OUTLET_SPEED,  "RPM", FRU_FAN_BP1, 0},
    {"FAN6_INLET_SP:",   FAN_BP2_SNR_FAN6_INLET_SPEED,   "RPM", FRU_FAN_BP2, 0},
    {"FAN6_OUTLET_SP:",  FAN_BP2_SNR_FAN6_OUTLET_SPEED,  "RPM", FRU_FAN_BP2, 0},
    {"FAN7_INLET_SP:",   FAN_BP2_SNR_FAN7_INLET_SPEED,   "RPM", FRU_FAN_BP2, 0},
    {"FAN7_OUTLET_SP:",  FAN_BP2_SNR_FAN7_OUTLET_SPEED,  "RPM", FRU_FAN_BP2, 0},
    {"FAN8_INLET_SP:",   FAN_BP1_SNR_FAN8_INLET_SPEED,   "RPM", FRU_FAN_BP1, 0},
    {"FAN8_OUTLET_SP:",  FAN_BP1_SNR_FAN8_OUTLET_SPEED,  "RPM", FRU_FAN_BP1, 0},
    {"FAN9_INLET_SP:",   FAN_BP1_SNR_FAN9_INLET_SPEED,   "RPM", FRU_FAN_BP1, 0},
    {"FAN9_OUTLET_SP:",  FAN_BP1_SNR_FAN9_OUTLET_SPEED,  "RPM", FRU_FAN_BP1, 0},
    {"FAN10_INLET_SP:",  FAN_BP2_SNR_FAN10_INLET_SPEED,  "RPM", FRU_FAN_BP2, 0},
    {"FAN10_OUTLET_SP:", FAN_BP2_SNR_FAN10_OUTLET_SPEED, "RPM", FRU_FAN_BP2, 0},
    {"FAN11_INLET_SP:",  FAN_BP2_SNR_FAN11_INLET_SPEED,  "RPM", FRU_FAN_BP2, 0},
    {"FAN11_OUTLET_SP:", FAN_BP2_SNR_FAN11_OUTLET_SPEED, "RPM", FRU_FAN_BP2, 0},
    {"FAN12_INLET_SP:",  FAN_BP1_SNR_FAN12_INLET_SPEED,  "RPM", FRU_FAN_BP1, 0},
    {"FAN12_OUTLET_SP:", FAN_BP1_SNR_FAN12_OUTLET_SPEED, "RPM", FRU_FAN_BP1, 0},
    {"FAN13_INLET_SP:",  FAN_BP1_SNR_FAN13_INLET_SPEED,  "RPM", FRU_FAN_BP1, 0},
    {"FAN13_OUTLET_SP:", FAN_BP1_SNR_FAN13_OUTLET_SPEED, "RPM", FRU_FAN_BP1, 0},
    {"FAN14_OUTLET_SP:", FAN_BP2_SNR_FAN14_OUTLET_SPEED, "RPM", FRU_FAN_BP2, 0},
    {"FAN14_INLET_SP:",  FAN_BP2_SNR_FAN14_INLET_SPEED,  "RPM", FRU_FAN_BP2, 0},
    {"FAN15_INLET_SP:",  FAN_BP2_SNR_FAN15_INLET_SPEED,  "RPM", FRU_FAN_BP2, 0},
    {"FAN15_OUTLET_SP:", FAN_BP2_SNR_FAN15_OUTLET_SPEED, "RPM", FRU_FAN_BP2, 0},
    {"INLET_TEMP_R:", MB_SNR_INLET_TEMP_R, "C", FRU_MB, 0},
    {"INLET_TEMP_L:", MB_SNR_INLET_TEMP_L, "C", FRU_MB, 0},
    {"P0_VR_TEMP:",  MB_SNR_VR_CPU0_VCCIN_TEMP, "C", FRU_MB, 0},
    {"P1_VR_TEMP:",  MB_SNR_VR_CPU1_VCCIN_TEMP, "C", FRU_MB, 0},
    {"P0_VR_POWER:", MB_SNR_VR_CPU0_VCCIN_POWER, "W", FRU_MB, 0},
    {"P1_VR_POWER:", MB_SNR_VR_CPU1_VCCIN_POWER, "W", FRU_MB, 1},
    {"P0_DIMMA_TEMP:", MB_SNR_DIMM_CPU0_GRPA_TEMP, "C", FRU_MB, 0},
    {"P0_DIMMB_TEMP:", MB_SNR_DIMM_CPU0_GRPB_TEMP, "C", FRU_MB, 0},
    {"P0_DIMMC_TEMP:", MB_SNR_DIMM_CPU0_GRPC_TEMP, "C", FRU_MB, 0},
    {"P0_DIMMD_TEMP:", MB_SNR_DIMM_CPU0_GRPD_TEMP, "C", FRU_MB, 0},
    {"P0_DIMME_TEMP:", MB_SNR_DIMM_CPU0_GRPE_TEMP, "C", FRU_MB, 0},
    {"P0_DIMMF_TEMP:", MB_SNR_DIMM_CPU0_GRPF_TEMP, "C", FRU_MB, 0},
    {"P0_DIMMG_TEMP:", MB_SNR_DIMM_CPU0_GRPG_TEMP, "C", FRU_MB, 0},
    {"P0_DIMMH_TEMP:", MB_SNR_DIMM_CPU0_GRPH_TEMP, "C", FRU_MB, 0},
    {"P1_DIMMA_TEMP:", MB_SNR_DIMM_CPU1_GRPA_TEMP, "C", FRU_MB, 0},
    {"P1_DIMMB_TEMP:", MB_SNR_DIMM_CPU1_GRPB_TEMP, "C", FRU_MB, 0},
    {"P1_DIMMC_TEMP:", MB_SNR_DIMM_CPU1_GRPC_TEMP, "C", FRU_MB, 0},
    {"P1_DIMMD_TEMP:", MB_SNR_DIMM_CPU1_GRPD_TEMP, "C", FRU_MB, 0},
    {"P1_DIMME_TEMP:", MB_SNR_DIMM_CPU1_GRPE_TEMP, "C", FRU_MB, 0},
    {"P1_DIMMF_TEMP:", MB_SNR_DIMM_CPU1_GRPF_TEMP, "C", FRU_MB, 0},
    {"P1_DIMMG_TEMP:", MB_SNR_DIMM_CPU1_GRPG_TEMP, "C", FRU_MB, 0},
    {"P1_DIMMH_TEMP:", MB_SNR_DIMM_CPU1_GRPH_TEMP, "C", FRU_MB, 0},
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
  static sensor_desc_t sensors[255];
  bool module = is_mb_hsc_module();
  uint8_t hsc_cnt = sizeof(hsc_cri_sensor) / sizeof(hsc_cri_sensor[0]);
  uint8_t cri_cnt = sizeof(cri_sensor) / sizeof(cri_sensor[0]);

  if (!desc || !desc_count) {
    return -1;
  }

  if(module) {
    for (int i=0; i<hsc_cnt; i++)
      hsc_cri_sensor[i].fru= FRU_HSC;
  }

  memcpy(sensors, &cri_sensor, sizeof(cri_sensor));
  memcpy(&sensors[cri_cnt], &hsc_cri_sensor, sizeof(hsc_cri_sensor));

  *desc = sensors;
  *desc_count = cri_cnt + hsc_cnt;

  return 0;
}

uint8_t plat_get_fru_sel(void)
{
  return FRU_MB;
}

int plat_get_me_status(uint8_t fru, char *status)
{
  char buf[256];
  ipmb_req_t *req;
  ipmb_res_t *res;
  unsigned char len;

  req = (ipmb_req_t*)buf;
  res = (ipmb_res_t*)buf;
  req->res_slave_addr = NM_SLAVE_ADDR;
  req->netfn_lun = NETFN_APP_REQ<<2;
  req->hdr_cksum = req->res_slave_addr + req->netfn_lun;
  req->hdr_cksum = ZERO_CKSUM_CONST - req->hdr_cksum;

  req->req_slave_addr = 0x20;
  req->seq_lun = 0x00;

  req->cmd = CMD_APP_GET_DEVICE_ID;
  status[0] = '\0';
  // Invoke IPMB library handler
  len = 0;
  lib_ipmb_handle(NM_IPMB_BUS_ID, (uint8_t *)req, 7, (uint8_t *)res, &len);
  if (len > 7 && res->cc == 0) {
    if (res->data[2] & 0x80)
      strcpy(status, "recovery mode");
    else
      strcpy(status, "operation mode");
    return 0;
  } else {
    strcpy(status, "unknown");
  }
  return -1;
}

int plat_get_board_id(char *id)
{
  uint8_t byte;

  if (!pal_get_platform_id(&byte)){
    sprintf(id, "%d%d%d%d%d",
      (byte & (1<<4))?1:0,
      (byte & (1<<3))?1:0,
      (byte & (1<<2))?1:0,
      (byte & (1<<1))?1:0,
      (byte & (1<<0))?1:0);
    return 0;
  }
  return -1;
}

int plat_dword_postcode_buf(uint8_t fru, char *status) {
  uint32_t dw_postcodes[30];  // to display the latest 30 postcodes
  size_t i, len = 0, total_len = 0;
  char temp_str[128];

  if (pal_get_80port_record(fru, (uint8_t *)dw_postcodes,
                            sizeof(dw_postcodes), &len)) {
    return -1;
  }

  len /= sizeof(uint32_t);
  for (i = 0; i < len; ++i) {
    pal_parse_amd_post_code_helper(dw_postcodes[i], temp_str);
    if ((total_len += strlen(temp_str)) >= 2048) {  // limit in 2KB
      break;
    }
    strcat(status, temp_str);
  }

  return 0;
}

int plat_get_syscfg_text(uint8_t fru, char *syscfg)
{
  pal_get_syscfg_text(syscfg);
  return 0;
}

int plat_get_etra_fw_version(uint8_t slot_id, char *fw_text)
{
  return -1;
}

int plat_get_extra_sysinfo(uint8_t fru, char *info)
{
  return -1;
}
