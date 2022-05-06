#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include <stdio.h>
#include <openbmc/pal.h>
#include <openbmc/sdr.h>
#include <openbmc/obmc-i2c.h>
#include <openbmc/obmc_pal_sensors.h>
#include <facebook/bic.h>
#include <facebook/fby35_common.h>
#include <syslog.h>

#include "usb-dbg-conf.h"

#define ESCAPE "\x1B"
#define ESC_ALT ESCAPE"[5;7m"
#define ESC_RST ESCAPE"[m"

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

static sensor_desc_t cri_sensor_sb[] =
{
  {"MB Inlet Temp:"        , BIC_SENSOR_INLET_TEMP         , "C"    , FRU_ALL, 0},
  {"MB Outlet Temp:"       , BIC_SENSOR_OUTLET_TEMP        , "C"    , FRU_ALL, 0},
  {"FIO Temp:"             , BIC_SENSOR_FIO_TEMP           , "C"    , FRU_ALL, 0},
  {"PCH Temp:"             , BIC_SENSOR_PCH_TEMP           , "C"    , FRU_ALL, 0},
  {"CPU Temp:"             , BIC_SENSOR_CPU_TEMP           , "C"    , FRU_ALL, 0},
  {"Therm Margin:"         , BIC_SENSOR_CPU_THERM_MARGIN   , "C"    , FRU_ALL, 0},
  {"CPU TjMax:"            , BIC_SENSOR_CPU_TJMAX          , "C"    , FRU_ALL, 0},
  {"DIMMA TEMP:"           , BIC_SENSOR_DIMMA0_TEMP        , "C"    , FRU_ALL, 0},
  {"DIMMC TEMP:"           , BIC_SENSOR_DIMMC0_TEMP        , "C"    , FRU_ALL, 0},
  {"DIMMD TEMP:"           , BIC_SENSOR_DIMMD0_TEMP        , "C"    , FRU_ALL, 0},
  {"DIMME TEMP:"           , BIC_SENSOR_DIMME0_TEMP        , "C"    , FRU_ALL, 0},
  {"DIMMG TEMP:"           , BIC_SENSOR_DIMMG0_TEMP        , "C"    , FRU_ALL, 0},
  {"DIMMH TEMP:"           , BIC_SENSOR_DIMMH0_TEMP        , "C"    , FRU_ALL, 0},
  {"SSD0 Temp:"            , BIC_SENSOR_M2A_TEMP           , "C"    , FRU_ALL, 0},
  {"HSC Temp:"             , BIC_SENSOR_HSC_TEMP           , "C"    , FRU_ALL, 0},
  {"VCCIN VR Temp:"        , BIC_SENSOR_VCCIN_VR_TEMP      , "C"    , FRU_ALL, 0},
  {"FIVRA VR Temp:"        , BIC_SENSOR_FIVRA_VR_TEMP      , "C"    , FRU_ALL, 0},
  {"EHV VR Temp:"          , BIC_SENSOR_EHV_VR_TEMP        , "C"    , FRU_ALL, 0},
  {"VCCD VR Temp:"         , BIC_SENSOR_VCCD_VR_TEMP       , "C"    , FRU_ALL, 0},
  {"FAON VR Temp:"         , BIC_SENSOR_FAON_VR_TEMP       , "C"    , FRU_ALL, 0},
  {"P12V_STBY Vol:"        , BIC_SENSOR_P12V_STBY_VOL      , "V"    , FRU_ALL, 2},
  {"P3V_BAT Vol:"          , BIC_SENSOR_P3V_BAT_VOL        , "V"    , FRU_ALL, 2},
  {"P3V3_STBY Vol:"        , BIC_SENSOR_P3V3_STBY_VOL      , "V"    , FRU_ALL, 2},
  {"P1V05_PCH Vol:"        , BIC_SENSOR_P1V05_PCH_STBY_VOL , "V"    , FRU_ALL, 2},
  {"P1V8_STBY Vol:"        , BIC_SENSOR_P1V8_STBY_VOL      , "V"    , FRU_ALL, 2},
  {"HSC Input Vol:"        , BIC_SENSOR_HSC_INPUT_VOL      , "V"    , FRU_ALL, 2},
  {"VCCIN VR Vol:"         , BIC_SENSOR_VCCIN_VR_VOL       , "V"    , FRU_ALL, 2},
  {"FIVRA VR Vol:"         , BIC_SENSOR_FIVRA_VR_VOL       , "V"    , FRU_ALL, 2},
  {"EHV VR Vol:"           , BIC_SENSOR_EHV_VR_VOL         , "V"    , FRU_ALL, 2},
  {"VCCD VR Vol:"          , BIC_SENSOR_VCCD_VR_VOL        , "V"    , FRU_ALL, 2},
  {"FAON VR Vol:"          , BIC_SENSOR_FAON_VR_VOL        , "V"    , FRU_ALL, 2},
  {"HSC OutputCur:"        , BIC_SENSOR_HSC_OUTPUT_CUR     , "A"    , FRU_ALL, 2},
  {"VCCIN VR Cur:"         , BIC_SENSOR_VCCIN_VR_CUR       , "A"    , FRU_ALL, 2},
  {"FIVRA VR Cur:"         , BIC_SENSOR_FIVRA_VR_CUR       , "A"    , FRU_ALL, 2},
  {"EHV VR Cur:"           , BIC_SENSOR_EHV_VR_CUR         , "A"    , FRU_ALL, 2},
  {"VCCD VR Cur:"          , BIC_SENSOR_VCCD_VR_CUR        , "A"    , FRU_ALL, 2},
  {"FAON VR Cur:"          , BIC_SENSOR_FAON_VR_CUR        , "A"    , FRU_ALL, 2},
  {"HSC PwrIn:"            , BIC_SENSOR_HSC_INPUT_PWR      , "W"    , FRU_ALL, 2},
  {"VCCIN VR Pout:"        , BIC_SENSOR_VCCIN_VR_POUT      , "W"    , FRU_ALL, 2},
  {"FIVRA VR Pout:"        , BIC_SENSOR_FIVRA_VR_POUT      , "W"    , FRU_ALL, 2},
  {"EHV VR Pout:"          , BIC_SENSOR_EHV_VR_POUT        , "W"    , FRU_ALL, 2},
  {"VCCD VR Pout"          , BIC_SENSOR_VCCD_VR_POUT       , "W"    , FRU_ALL, 2},
  {"FAON VR Pout:"         , BIC_SENSOR_FAON_VR_POUT       , "W"    , FRU_ALL, 2},
  {"DIMMA PMIC_Pout:"      , BIC_SENSOR_DIMMA_PMIC_Pout    , "W"    , FRU_ALL, 2},
  {"DIMMC PMIC_Pout:"      , BIC_SENSOR_DIMMC_PMIC_Pout    , "W"    , FRU_ALL, 2},
  {"DIMMD PMIC_Pout:"      , BIC_SENSOR_DIMMD_PMIC_Pout    , "W"    , FRU_ALL, 2},
  {"DIMME PMIC_Pout:"      , BIC_SENSOR_DIMME_PMIC_Pout    , "W"    , FRU_ALL, 2},
  {"DIMMG PMIC_Pout:"      , BIC_SENSOR_DIMMG_PMIC_Pout    , "W"    , FRU_ALL, 2},
  {"DIMMH PMIC_Pout:"      , BIC_SENSOR_DIMMH_PMIC_Pout    , "W"    , FRU_ALL, 2},
};

static sensor_desc_t cri_sensor_bmc[] =
{
  {"FAN0_TACH:"            , BMC_SENSOR_FAN0_TACH          , "RPM"  , FRU_BMC, 0},
  {"FAN1_TACH:"            , BMC_SENSOR_FAN1_TACH          , "RPM"  , FRU_BMC, 0},
  {"FAN2_TACH:"            , BMC_SENSOR_FAN2_TACH          , "RPM"  , FRU_BMC, 0},
  {"FAN3_TACH:"            , BMC_SENSOR_FAN3_TACH          , "RPM"  , FRU_BMC, 0},
  {"FAN4_TACH:"            , BMC_SENSOR_FAN4_TACH          , "RPM"  , FRU_BMC, 0},
  {"FAN5_TACH:"            , BMC_SENSOR_FAN5_TACH          , "RPM"  , FRU_BMC, 0},
  {"FAN6_TACH:"            , BMC_SENSOR_FAN6_TACH          , "RPM"  , FRU_BMC, 0},
  {"FAN7_TACH:"            , BMC_SENSOR_FAN7_TACH          , "RPM"  , FRU_BMC, 0},
  {"BMC_PWM0:"             , BMC_SENSOR_PWM0               , "%"    , FRU_BMC, 0},
  {"BMC_PWM1:"             , BMC_SENSOR_PWM1               , "%"    , FRU_BMC, 0},
  {"BMC_PWM2:"             , BMC_SENSOR_PWM2               , "%"    , FRU_BMC, 0},
  {"BMC_PWM3:"             , BMC_SENSOR_PWM3               , "%"    , FRU_BMC, 0},
  {"OUTLET_TEMP:"          , BMC_SENSOR_OUTLET_TEMP        , "C"    , FRU_BMC, 2},
  {"INLET_TEMP:"           , BMC_SENSOR_INLET_TEMP         , "C"    , FRU_BMC, 2},
  {"BMC_P5V:"              , BMC_SENSOR_P5V                , "V"    , FRU_BMC, 2},
  {"BMC_P12V:"             , BMC_SENSOR_P12V               , "V"    , FRU_BMC, 2},
  {"P3V3_STBY:"            , BMC_SENSOR_P3V3_STBY          , "V"    , FRU_BMC, 2},
  {"P1V8_STBY:"            , BMC_SENSOR_P1V8_STBY          , "V"    , FRU_BMC, 2},
  {"P1V2_STBY:"            , BMC_SENSOR_P1V2_BMC_STBY      , "V"    , FRU_BMC, 2},
  {"P2V5_STBY:"            , BMC_SENSOR_P2V5_BMC_STBY      , "V"    , FRU_BMC, 2},
  {"HSC_TEMP:"             , BMC_SENSOR_HSC_TEMP           , "C"    , FRU_BMC, 2},
  {"HSC_VIN:"              , BMC_SENSOR_HSC_VIN            , "V"    , FRU_BMC, 2},
  {"HSC_PIN:"              , BMC_SENSOR_HSC_PIN            , "W"    , FRU_BMC, 2},
  {"HSC_EIN:"              , BMC_SENSOR_HSC_EIN            , "W"    , FRU_BMC, 2},
  {"HSC_IOUT:"             , BMC_SENSOR_HSC_IOUT           , "A"    , FRU_BMC, 2},
  {"HSC_PEAK_IOUT:"        , BMC_SENSOR_HSC_PEAK_IOUT      , "A"    , FRU_BMC, 2},
  {"HSC_PEAK_PIN:"         , BMC_SENSOR_HSC_PEAK_PIN       , "W"    , FRU_BMC, 2},
  {"MEDUSA_VOUT:"          , BMC_SENSOR_MEDUSA_VOUT        , "V"    , FRU_BMC, 2},
  {"MEDUSA_VIN:"           , BMC_SENSOR_MEDUSA_VIN         , "V"    , FRU_BMC, 2},
  {"MEDUSA_CURR:"          , BMC_SENSOR_MEDUSA_CURR        , "A"    , FRU_BMC, 2},
  {"MEDUSA_PWR:"           , BMC_SENSOR_MEDUSA_PWR         , "W"    , FRU_BMC, 2},
  {"MEDUSA_VDELTA:"        , BMC_SENSOR_MEDUSA_VDELTA      , "V"    , FRU_BMC, 2},
  {"PDB_CL_VDELTA:"        , BMC_SENSOR_PDB_CL_VDELTA      , "V"    , FRU_BMC, 2},
  {"PDB_BB_VDELTA:"        , BMC_SENSOR_PDB_BB_VDELTA      , "V"    , FRU_BMC, 2},
  {"CURR_LEAKAGE:"         , BMC_SENSOR_CURR_LEAKAGE       , "%"    , FRU_BMC, 2},
  {"FAN_IOUT:"             , BMC_SENSOR_FAN_IOUT           , "A"    , FRU_BMC, 2},
  {"FAN_PWR:"              , BMC_SENSOR_FAN_PWR            , "W"    , FRU_BMC, 2},
  {"NIC_P12V:"             , BMC_SENSOR_NIC_P12V           , "V"    , FRU_BMC, 2},
  {"NIC_IOUT:"             , BMC_SENSOR_NIC_IOUT           , "A"    , FRU_BMC, 2},
  {"NIC_PWR:"              , BMC_SENSOR_NIC_PWR            , "W"    , FRU_BMC, 2},
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
  if (!desc || !desc_count) {
    return -1;
  }
  if(fru == FRU_ALL){ //uart select BMC position
    *desc = cri_sensor_bmc;
    *desc_count = sizeof(cri_sensor_bmc) / sizeof(cri_sensor_bmc[0]);
  }
  else{
    *desc = cri_sensor_sb;
    *desc_count = sizeof(cri_sensor_sb) / sizeof(cri_sensor_sb[0]);
  }
  return 0;
}

uint8_t plat_get_fru_sel(void)
{
  uint8_t pos;
  if (pal_get_uart_select_from_kv(&pos)) {
    return  0;
  }
  return pos;
}


int plat_get_me_status(uint8_t fru, char *status)
{
  char tbuf[256];
  char rbuf[256];
  unsigned char rlen;
  int ret;
  tbuf[0] = NETFN_APP_REQ << 2;
  tbuf[1] = CMD_APP_GET_DEVICE_ID;
  ret = bic_me_xmit(fru, (uint8_t *)tbuf, 2, (uint8_t *)rbuf, &rlen);
  if (ret || rbuf[0]) {
    return -1;
  }
  strcpy(status, (rbuf[3] & 0x80) ? "recovert_mode" : "operation mode");
  return 0;
}

int plat_get_board_id(char *id)
{
  uint8_t pos;
  uint8_t buf[256] = {0x00};
  uint8_t rlen;
  if (pal_get_uart_select_from_kv(&pos)) {
    return -1;
  }
  if(pal_get_board_id(pos, buf, 0, buf, &rlen)) {
    return -1;
  }

  sprintf(id, "%02d", buf[2]);
  return 0;
}

int plat_get_syscfg_text(uint8_t fru, char *syscfg)
{
  return -1;
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

  if(fru == FRU_ALL) {
    fru = FRU_BMC;
  }

  if (!pal_get_fru_name( fru, fru_name)) {

    sprintf(info, "FRU:%s", fru_name);
  }
  return 0;
}
