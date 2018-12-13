#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include <stdio.h>
#include <openbmc/pal.h>
#include <openbmc/sdr.h>
#include <openbmc/gpio.h>
#include <openbmc/obmc-sensor.h>
#include <facebook/bic.h>

#include "usb-dbg-conf.h"

#define ESCAPE "\x1B"
#define ESC_ALT ESCAPE"[5;7m"
#define ESC_RST ESCAPE"[m"

#define GPIO_UART_SEL 145

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

//Expander Error Code Conversion
//BMC send Hexadecimal Error Code to Debug Card
//Debug Card will send the same value to BMC to query String
//Expander Error String is in Decimal 0~99, so needs the conversion
//Expmale: 0x99 Convert to 99 literally
//TODO We need to modify this table to accommodate the above issue.
static post_desc_t pdesc_error[] = {
  {0, "No error"}, //Error Code 0
  {0x1, "Expander I2C bus 0 crash"},
  {0x2, "Expander I2C bus 1 crash"},
  {0x3, "Expander I2C bus 2 crash"},
  {0x4, "Expander I2C bus 3 crash"},
  {0x5, "Expander I2C bus 4 crash"},
  {0x6, "Expander I2C bus 5 crash"},
  {0x7, "Expander I2C bus 6 crash"},
  {0x8, "Expander I2C bus 7 crash"},
  {0x9, "Expander I2C bus 8 crash"},
  {0x10, "Expander I2C bus 9 crash"},
  {0x11, "Expander I2C bus 10 crash"},
  {0x12, "Expander I2C bus 11 crash"},
  {0x13, "Fan 1 front fault"},
  {0x14, "Fan 1 rear fault"},
  {0x15, "Fan 2 front fault"},
  {0x16, "Fan 2 rear fault"},
  {0x17, "Fan 3 front fault"},
  {0x18, "Fan 3 rear fault"},
  {0x19, "Fan 4 front fault"},
  {0x20, "Fan 4 rear fault"},
  {0x21, "SCC voltage warning"},
  {0x22, "DPB voltage warning"},
  {0x25, "SCC current warning"},
  {0x26, "DPB current warning"},
  {0x27, "SCC_I2C_device_loss"},
  {0x28, "DPB_I2C_device_loss"},
  {0x29, "DPB_Temp1"},
  {0x30, "DPB_Temp2"},
  {0x31, "SCC_Expander_Temp"},
  {0x32, "SCC_IOC_Temp"},
  {0x33, "HDD X SMART temp. warning"},
  {0x34, "Front_Panel_I2C_device_loss"},
  {0x36, "HDD0 fault"},
  {0x37, "HDD1 fault"},
  {0x38, "HDD2 fault"},
  {0x39, "HDD3 fault"},
  {0x40, "HDD4 fault"},
  {0x41, "HDD5 fault"},
  {0x42, "HDD6 fault"},
  {0x43, "HDD7 fault"},
  {0x44, "HDD8 fault"},
  {0x45, "HDD9 fault"},
  {0x46, "HDD10 fault"},
  {0x47, "HDD11 fault"},
  {0x48, "HDD12 fault"},
  {0x49, "HDD13 fault"},
  {0x50, "HDD14 fault"},
  {0x51, "HDD15 fault"},
  {0x52, "HDD16 fault"},
  {0x53, "HDD17 fault"},
  {0x54, "HDD18 fault"},
  {0x55, "HDD19 fault"},
  {0x56, "HDD20 fault"},
  {0x57, "HDD21 fault"},
  {0x58, "HDD22 fault"},
  {0x59, "HDD23 fault"},
  {0x60, "HDD24 fault"},
  {0x61, "HDD25 fault"},
  {0x62, "HDD26 fault"},
  {0x63, "HDD27 fault"},
  {0x64, "HDD28 fault"},
  {0x65, "HDD29 fault"},
  {0x66, "HDD30 fault"},
  {0x67, "HDD31 fault"},
  {0x68, "HDD32 fault"},
  {0x69, "HDD33 fault"},
  {0x70, "HDD34 fault"},
  {0x71, "HDD35 fault"},
  {0x72, "HDD X fault sensed"},
  {0x86, "Fan1 Plug Out"},
  {0x87, "Fan2 Plug Out"},
  {0x88, "Fan3 Plug Out"},
  {0x89, "Fan4 Plug Out"},
  {0x90, "Internal Mini-SAS link error"},
  {0x91, "Internal Mini-SAS link error"},
  {0x92, "Drawer be pulled out"},
  {0x93, "Peer SCC be plug out"},
  {0x94, "IOMA be plug out"},
  {0x95, "IOMB be plug out"},
  {0x99, "HW Configuration_Type Not Match"}, //Error Code 99
  {0xE0, "BMC CPU utilization exceeded"},// BMC error code
  {0xE1, "BMC Memory utilization exceeded"},
  {0xE2, "ECC Recoverable Error"},
  {0xE3, "ECC Un-recoverable Error"},
  {0xE4, "Server board Missing"},
  {0xE7, "SCC Missing"},
  {0xE8, "NIC is plugged out"},
  {0xE9, "I2C bus 0 hang"},
  {0xEA, "I2C bus 1 hang"},
  {0xEB, "I2C bus 2 hang"},
  {0xEC, "I2C bus 3 hang"},
  {0xED, "I2C bus 4 hang"},
  {0xEE, "I2C bus 5 hang"},
  {0xEF, "I2C bus 6 hang"},
  {0xF0, "I2C bus 7 hang"},
  {0xF1, "I2C bus 8 hang"},
  {0xF2, "I2C bus 9 hang"},
  {0xF3, "I2C bus 10 hang"},
  {0xF4, "I2C bus 11 hang"},
  {0xF5, "I2C bus 12 hang"},
  {0xF6, "I2C bus 13 hang"},
  {0xF7, "Server health ERR"},
  {0xF8, "IOM health ERR"},
  {0xF9, "DPB health ERR"},
  {0xFA, "SCC health ERR"},
  {0xFB, "NIC health ERR"},
  {0xFC, "Remote BMC health ERR"},
  {0xFD, "Local SCC health ERR"},
  {0xFE, "Remote SCC health ERR"},
};

static gpio_desc_t gdesc[] = {
  { 0x10, 0, 2, "DBG_RST_BTN_N" },
  { 0x11, 0, 1, "DBG_PWR_BTN_N" },
  { 0x12, 0, 0, "DBG_GPIO_BMC2" },
  { 0x13, 0, 0, "DBG_GPIO_BMC3" },
  { 0x14, 0, 0, "DBG_GPIO_BMC4" },
  { 0x15, 0, 0, "DBG_GPIO_BMC5" },
  { 0x16, 0, 0, "DBG_GPIO_BMC6" },
  { 0x17, 0, 3, "DBG_HDR_UART_SEL" },
};

static sensor_desc_t cri_sensor[] =
{
    {"SOC_TEMP:"    , BIC_SENSOR_SOC_TEMP        , "C"   , FRU_SLOT1, 0},
    {"DIMMA0_TEMP:" , BIC_SENSOR_SOC_DIMMA0_TEMP , "C"   , FRU_SLOT1, 0},
    {"DIMMA1_TEMP:" , BIC_SENSOR_SOC_DIMMA1_TEMP , "C"   , FRU_SLOT1, 0},
    {"DIMMB0_TEMP:" , BIC_SENSOR_SOC_DIMMB0_TEMP , "C"   , FRU_SLOT1, 0},
    {"DIMMB1_TEMP:" , BIC_SENSOR_SOC_DIMMB1_TEMP , "C"   , FRU_SLOT1, 0},
    {"HSC_PWR:"     , DPB_SENSOR_HSC_POWER       , "W"   , FRU_DPB, 1},
    {"HSC_VOL:"     , DPB_SENSOR_HSC_VOLT        , "V"   , FRU_DPB, 2},
    {"HSC_CUR:"     , DPB_SENSOR_HSC_CURR        , "A"   , FRU_DPB, 2},
    {"FAN1_F:"      , DPB_SENSOR_FAN1_FRONT      , "RPM" , FRU_DPB, 0},
    {"FAN1_R:"      , DPB_SENSOR_FAN1_REAR       , "RPM" , FRU_DPB, 0},
    {"FAN2_F:"      , DPB_SENSOR_FAN2_FRONT      , "RPM" , FRU_DPB, 0},
    {"FAN2_R:"      , DPB_SENSOR_FAN2_REAR       , "RPM" , FRU_DPB, 0},
    {"FAN3_F:"      , DPB_SENSOR_FAN3_FRONT      , "RPM" , FRU_DPB, 0},
    {"FAN3_R:"      , DPB_SENSOR_FAN3_REAR       , "RPM" , FRU_DPB, 0},
    {"FAN4_F:"      , DPB_SENSOR_FAN4_FRONT      , "RPM" , FRU_DPB, 0},
    {"FAN4_R:"      , DPB_SENSOR_FAN4_REAR       , "RPM" , FRU_DPB, 0},
    {"NIC Temp:"    , MEZZ_SENSOR_TEMP           , "C"   , FRU_NIC, 0},
};

static post_phase_desc_t post_phase_desc[] = {
  {1, pdesc_phase1, sizeof(pdesc_phase1)/sizeof(pdesc_phase1[0])},
  {2, pdesc_phase2, sizeof(pdesc_phase2)/sizeof(pdesc_phase2[0])},
};

static post_phase_desc_t post_phase_desc_bmc[] = {
  {PHASE_ANY, pdesc_error, sizeof(pdesc_error)/sizeof(pdesc_error[0])}
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

  if (gpio_get(GPIO_UART_SEL) == GPIO_VALUE_LOW) {
    *desc = post_phase_desc;
    *desc_count = sizeof(post_phase_desc) / sizeof(post_phase_desc[0]);
  } else {
    *desc = post_phase_desc_bmc;
    *desc_count = sizeof(post_phase_desc_bmc) / sizeof(post_phase_desc_bmc[0]);
  }

  return 0;
}

int plat_get_gdesc(uint8_t fru, gpio_desc_t **desc, size_t *desc_count)
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
  *desc = cri_sensor;
  *desc_count = sizeof(cri_sensor) / sizeof(cri_sensor[0]);
  return 0;
}

uint8_t plat_get_fru_sel(void)
{
  return FRU_SLOT1;
}

int plat_get_me_status(uint8_t fru, char *status)
{
  char buf[256];
  unsigned char rlen;
  int ret;
 
  buf[0] = NETFN_APP_REQ << 2;
  buf[1] = CMD_APP_GET_DEVICE_ID;
  ret = bic_me_xmit(fru, (uint8_t *)buf, 2, (uint8_t *)buf, &rlen);
  if (ret || buf[0]) {
    return -1;
  }
  strcpy(status, (buf[3] & 0x80) ? "recovert_mode" : "operation mode");
  return 0;
}

int plat_get_board_id(char *id)
{
  uint8_t boardid = pal_get_iom_board_id();
  sprintf(id, "%d%d%d",
    (boardid & (1 << 2)) ? 1 : 0,
    (boardid & (1 << 1)) ? 1 : 0,
    (boardid & (1 << 0)) ? 1 : 0);
  return 0;
}

int plat_get_syscfg_text(uint8_t slot, char *text)
{
  return -1;
}

int plat_get_extra_sysinfo(uint8_t slot, char *info)
{
  return -1;
}
