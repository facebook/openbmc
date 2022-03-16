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
#include <facebook/fby3_common.h>
#include <syslog.h>

#include "usb-dbg-conf.h"

#define ESCAPE "\x1B"
#define ESC_ALT ESCAPE"[5;7m"
#define ESC_RST ESCAPE"[m"

static sensor_desc_t dynamic_cri_sensor[MAX_SENSOR_NUM + 1] = {0};

//These postcodes are defined in document "F08 BIOS Specification" Revision: 2A
static post_desc_t pdesc_phase1[] = {
  { 0x00, "Not used" },
  { 0x01, "POWER_ON" },
  { 0x02, "MICROCODE" },
  { 0x03, "CACHE_ENABLED" },
  { 0x04, "SECSB_INIT" },
  { 0x05, "OEM_INIT_ENTRY" },
  { 0x06, "CPU_EARLY_INIT" },
  { 0x1D, "OEM_PMEM_INIT" },
  { 0xA1, "STS_COLLECT_INFO" },
  { 0xA3, "STS_SETUP_PATH" },
  { 0xA7, "STS_TOPOLOGY_DISC" },
  { 0xA8, "STS_FINAL_ROUTE" },
  { 0xA9, "STS_FINAL_IO_SAD" },
  { 0xAA, "STS_PROTOCOL_SET" },
  { 0xAE, "STS_COHERENCY_SETUP" },
  { 0xAF, "STS_KTI_COMPLETE" },
  { 0xE0, "BMC" },
  { 0xE1, "SLOT1" },
  { 0xE4, "SLOT4" },
  { 0xE3, "SLOT3" },
  { 0xE5, "AMI_PROG_CODE" },
  { 0xB0, "STS_DIMM_DETECT" },
  { 0xB1, "STS_CLOCK_INIT" },
  { 0xB4, "STS_RANK_DETECT" },
  { 0xB2, "STS_SPD_DATA" },
  { 0xB3, "STS_GLOBAL_EARLY" },
  { 0xB6, "STS_DDRIO_INIT" },
  { 0xB7, "STS_CHANNEL_TRAINING" },
  { 0xBE, "STS_SSA_API_INIT" },
  { 0xB8, "STS_INIT_THROTTLING" },
  { 0xB9, "STS_MEMBIST" },
  { 0xBA, "STS_MEMINIT" },
  { 0xBB, "STS_DDR_MEMMAP" },
  { 0xBC, "STS_RAS_CONFIG" },
  { 0xBF, "STS_MRC_DONE" },
  { 0xE6, "AMI_PROG_CODE" },
  { 0xE7, "AMI_PROG_CODE" },
  { 0xE8, "S3_RESUME_FAIL" },
  { 0xE9, "S3_PPI_NOT_FOUND" },
  { 0xEB, "S3_OS_WAKE_ERR" },
  { 0xEC, "AMI_PROG_CODE" },
  { 0xED, "AMI_PROG_CODE" },
  { 0xEE, "AMI_PROG_CODE" },

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
  { 0x0B, "CACHE_INIT" },
  { 0x0C, "SEC_ERR" },
  { 0x0D, "SEC_ERR" },
  { 0x0E, "MICROCODE_N_FOUND" },
  { 0x0F, "MICROCODE_N_LOAD" },
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
  { 0x19, "PM_SB_INITS" },
  { 0x1A, "PM_SB_INIT1" },
  { 0x1B, "PM_SB_INIT2" },
  { 0x1C, "PM_SB_INIT3" },
  { 0x1E, "OEM_PMEM_INIT" },
  { 0x1F, "OEM_PMEM_INIT" },

  { 0x20, "OEM_PMEM_INIT" },
  { 0x21, "OEM_PMEM_INIT" },
  { 0x22, "OEM_PMEM_INIT" },
  { 0x23, "OEM_PMEM_INIT" },
  { 0x24, "OEM_PMEM_INIT" },
  { 0x25, "OEM_PMEM_INIT" },
  { 0x26, "OEM_PMEM_INIT" },
  { 0x27, "OEM_PMEM_INIT" },
  { 0x28, "OEM_PMEM_INIT" },
  { 0x29, "OEM_PMEM_INIT" },
  { 0x2A, "OEM_PMEM_INIT" },
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
  { 0x3F, "OEM_POST_MEM_INIT" },

  { 0x41, "OEM_POST_MEM_INIT" },
  { 0x42, "OEM_POST_MEM_INIT" },
  { 0x43, "OEM_POST_MEM_INIT" },
  { 0x44, "OEM_POST_MEM_INIT" },
  { 0x45, "OEM_POST_MEM_INIT" },
  { 0x46, "OEM_POST_MEM_INIT" },
  { 0x47, "OEM_POST_MEM_INIT" },
  { 0x48, "OEM_POST_MEM_INIT" },
  { 0x49, "OEM_POST_MEM_INIT" },
  { 0x4A, "OEM_POST_MEM_INIT" },
  { 0x4B, "OEM_POST_MEM_INIT" },
  { 0x4C, "OEM_POST_MEM_INIT" },
  { 0x4D, "OEM_POST_MEM_INIT" },
  { 0x4E, "OEM_POST_MEM_INIT" },

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
  { 0x5A, "CPU_INTERNAL_ERR" },
  { 0x5B, "RESET_PPI_ERR" },
  { 0x5C, "AMI_PROG_CODE" },
  { 0x5D, "AMI_PROG_CODE" },
  { 0x5E, "AMI_PROG_CODE" },
  { 0x5F, "AMI_PROG_CODE" },

  // slot2
  { 0xE2, "SLOT2" },

  // S3 Resume Error Code
  { 0xEA, "S3_BOOT_SCRIPT_ERR" },
  { 0xEF, "AMI_PROG_CODE" },

  // Recovery Progress Code
  { 0xF0, "REC_BY_FW" },
  { 0xF1, "REC_BY_USER" },
  { 0xF2, "REC_STARTED" },
  { 0xF3, "REC_FW_FOUND" },
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
  { 0xBD, "STS_GET_MARGINS" },
  /*--------------------- MRC Phase - End----------------------- */

  { 0x4F, "DXE_IPL_START" },

  { 0xF9, "RECOVERY_CAP_NOT_FOUND" },
  { 0x70, "STS_HBM" },
  { 0x71, "STS_HBM_DEBUG" },
  { 0x72, "STS_HBM_INTERNAL" },
  { 0x7E, "STS_PIPE_SYNC" },
  { 0xC1, "STS_CHECK_POR" },
  { 0xC2, "STS_UNLOCK_MEM_REGS" },
  { 0xC3, "STS_CHECK_STATUS" },
  { 0xC4, "STS_CONFIG_XMP" },
  { 0xC5, "STS_EARLY_INIT_MEM" },
  { 0xC6, "STS_DIMM_INFO" },
  { 0xC7, "STS_NVDIMM" },
  { 0xC9, "STS_SVL_SCRAMBLE" }

};

static post_desc_t pdesc_phase2[] = {
  { 0x61, "NVRAM_INIT" },
  { 0x9A, "USB_INIT" },
  { 0x78, "ACPI_INIT" },
  { 0x68, "PCI_BRIDGE_INIT" },
  { 0x70, "SB_DXE_START" },
  { 0x79, "CSM_INIT" },
  { 0xD1, "NB_INIT_ERR" },
  { 0xD2, "SB_INIT_ERR" },
  { 0xD4, "PCI_ALLOC_ERR" },
  { 0x92, "PCIB_INIT" },
  { 0x94, "PCIB_ENUMERATION" },
  { 0x95, "PCIB_REQ_RESOURCE" },
  { 0x96, "PCIB_ASSIGN_RESOURCE" },
  { 0xEF, "PCIB_INIT" },
  { 0x99, "SUPER_IO_INIT" },
  { 0x91, "DRIVER_CONN_START" },
  { 0xD5, "NO_SPACE_ROM" },
  { 0x97, "CONSOLE_INPUT_CONN" },
  { 0xB2, "LEGACY_ROM_INIT" },
  { 0xAA, "ACPI_ACPI_MODE" },
  { 0xC0, "OEM_DXE_OPROM_ENTER" },
  { 0xBB, "AMI_CODE" },
  { 0xC1, "OEM_DXE_OPROM_EXIT" },
  { 0x98, "CONSOLE_OUTPUT_CONN" },
  { 0x9D, "USB_ENABLE" },
  { 0x9C, "USB_DETECT" },
  { 0xB4, "USB_HOT_PLUG" },
  { 0xA0, "IDE_INIT" },
  { 0xA2, "IDE_DETECT" },
  { 0xA9, "START_OF_SETUP" },
  { 0xAB, "SETUP_INIT_WAIT" },

  /*--------------------- DXE Phase - Start--------------------- */
  { 0x60, "DXE_CORE_START" },
  { 0x62, "INSTALL_SB_SERVICE" },
  { 0x63, "CPU_DXE_STARTED" },
  { 0x64, "CPU_DXE_INIT" },
  { 0x65, "CPU_DXE_INIT" },
  { 0x66, "CPU_DXE_INIT" },
  { 0x67, "CPU_DXE_INIT" },
  { 0x69, "NB_DXE_INIT" },
  { 0x6A, "NB_DXE_SMM_INIT" },
  { 0x6B, "NB_DXE_BRI_START" },
  { 0x6C, "NB_DXE_BRI_START" },
  { 0x6D, "NB_DXE_BRI_START" },
  { 0x6E, "NB_DXE_BRI_START" },
  { 0x6F, "NB_DXE_BRI_START" },

  { 0x71, "SB_DXE_SMM_INIT" },
  { 0x72, "SB_DXE_DEV_START" },
  { 0x73, "SB_DXE_BRI_START" },
  { 0x74, "SB_DXE_BRI_START" },
  { 0x75, "SB_DXE_BRI_START" },
  { 0x76, "SB_DXE_BRI_START" },
  { 0x77, "SB_DXE_BRI_START" },
  { 0x7A, "AMI_DXE_CODE" },
  { 0x7B, "AMI_DXE_CODE" },
  { 0x7C, "AMI_DXE_CODE" },
  { 0x7D, "AMI_DXE_CODE" },
  { 0x7E, "AMI_DXE_CODE" },
  { 0x7F, "AMI_DXE_CODE" },

  //OEM DXE INIT CODE
  { 0x80, "OEM_DXE_INIT" },
  { 0x81, "OEM_DXE_INIT" },
  { 0x82, "OEM_DXE_INIT" },
  { 0x83, "OEM_DXE_INIT" },
  { 0x84, "OEM_DXE_INIT" },
  { 0x85, "OEM_DXE_INIT" },
  { 0x86, "OEM_DXE_INIT" },
  { 0x87, "OEM_DXE_INIT" },
  { 0x88, "OEM_DXE_INIT" },
  { 0x89, "OEM_DXE_INIT" },
  { 0x8A, "OEM_DXE_INIT" },
  { 0x8B, "OEM_DXE_INIT" },
  { 0x8C, "OEM_DXE_INIT" },
  { 0x8D, "OEM_DXE_INIT" },
  { 0x8E, "OEM_DXE_INIT" },
  { 0x8F, "OEM_DXE_INIT" },

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
  { 0xB6, "CLEAN_UP_NVRAM" },
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
  /*--------------------- DXE Phase - End--------------------- */

  /*--------------------- SLOT Info Start--------------------- */
  { 0xE0, "BMC" },
  { 0xE1, "SLOT1" },
  { 0xE2, "SLOT2" },
  { 0xE3, "SLOT3" },
  { 0xE4, "SLOT4" },
  /*--------------------- SLOT Info End--------------------- */
  { 0x58, "CPU_SELFTEST_FAIL" }
};

static post_desc_t pdesc_phase1_class2[] = {
  { 0x00, "Not used" },
  { 0x01, "POWER_ON" },
  { 0x02, "MICROCODE" },
  { 0x03, "CACHE_ENABLED" },
  { 0x04, "SECSB_INIT" },
  { 0x05, "OEM_INIT_ENTRY" },
  { 0x06, "CPU_EARLY_INIT" },
  { 0x1D, "OEM_PMEM_INIT" },
  { 0xA1, "STS_COLLECT_INFO" },
  { 0xA3, "STS_SETUP_PATH" },
  { 0xA7, "STS_TOPOLOGY_DISC" },
  { 0xA8, "STS_FINAL_ROUTE" },
  { 0xA9, "STS_FINAL_IO_SAD" },
  { 0xAA, "STS_PROTOCOL_SET" },
  { 0xAE, "STS_COHERENCY_SETUP" },
  { 0xAF, "STS_KTI_COMPLETE" },
  { 0xE0, "BB_BIC" },
  { 0xE1, "SLOT1_BMC" },
  { 0xE4, "SLOT3_BMC" },
  { 0xE3, "SLOT1_BIC" },
  { 0xE5, "SLOT3_HOST" },
  { 0xB0, "STS_DIMM_DETECT" },
  { 0xB1, "STS_CLOCK_INIT" },
  { 0xB4, "STS_RANK_DETECT" },
  { 0xB2, "STS_SPD_DATA" },
  { 0xB3, "STS_GLOBAL_EARLY" },
  { 0xB6, "STS_DDRIO_INIT" },
  { 0xB7, "STS_CHANNEL_TRAINING" },
  { 0xBE, "STS_SSA_API_INIT" },
  { 0xB8, "STS_INIT_THROTTLING" },
  { 0xB9, "STS_MEMBIST" },
  { 0xBA, "STS_MEMINIT" },
  { 0xBB, "STS_DDR_MEMMAP" },
  { 0xBC, "STS_RAS_CONFIG" },
  { 0xBF, "STS_MRC_DONE" },
  { 0xE6, "SLOT3_BIC" },
  { 0xE7, "AMI_PROG_CODE" },
  { 0xE8, "S3_RESUME_FAIL" },
  { 0xE9, "S3_PPI_NOT_FOUND" },
  { 0xEB, "S3_OS_WAKE_ERR" },
  { 0xEC, "AMI_PROG_CODE" },
  { 0xED, "AMI_PROG_CODE" },
  { 0xEE, "AMI_PROG_CODE" },

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
  { 0x0B, "CACHE_INIT" },
  { 0x0C, "SEC_ERR" },
  { 0x0D, "SEC_ERR" },
  { 0x0E, "MICROCODE_N_FOUND" },
  { 0x0F, "MICROCODE_N_LOAD" },
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
  { 0x19, "PM_SB_INITS" },
  { 0x1A, "PM_SB_INIT1" },
  { 0x1B, "PM_SB_INIT2" },
  { 0x1C, "PM_SB_INIT3" },
  { 0x1E, "OEM_PMEM_INIT" },
  { 0x1F, "OEM_PMEM_INIT" },

  { 0x20, "OEM_PMEM_INIT" },
  { 0x21, "OEM_PMEM_INIT" },
  { 0x22, "OEM_PMEM_INIT" },
  { 0x23, "OEM_PMEM_INIT" },
  { 0x24, "OEM_PMEM_INIT" },
  { 0x25, "OEM_PMEM_INIT" },
  { 0x26, "OEM_PMEM_INIT" },
  { 0x27, "OEM_PMEM_INIT" },
  { 0x28, "OEM_PMEM_INIT" },
  { 0x29, "OEM_PMEM_INIT" },
  { 0x2A, "OEM_PMEM_INIT" },
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
  { 0x3F, "OEM_POST_MEM_INIT" },

  { 0x41, "OEM_POST_MEM_INIT" },
  { 0x42, "OEM_POST_MEM_INIT" },
  { 0x43, "OEM_POST_MEM_INIT" },
  { 0x44, "OEM_POST_MEM_INIT" },
  { 0x45, "OEM_POST_MEM_INIT" },
  { 0x46, "OEM_POST_MEM_INIT" },
  { 0x47, "OEM_POST_MEM_INIT" },
  { 0x48, "OEM_POST_MEM_INIT" },
  { 0x49, "OEM_POST_MEM_INIT" },
  { 0x4A, "OEM_POST_MEM_INIT" },
  { 0x4B, "OEM_POST_MEM_INIT" },
  { 0x4C, "OEM_POST_MEM_INIT" },
  { 0x4D, "OEM_POST_MEM_INIT" },
  { 0x4E, "OEM_POST_MEM_INIT" },

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
  { 0x5A, "CPU_INTERNAL_ERR" },
  { 0x5B, "RESET_PPI_ERR" },
  { 0x5C, "AMI_PROG_CODE" },
  { 0x5D, "AMI_PROG_CODE" },
  { 0x5E, "AMI_PROG_CODE" },
  { 0x5F, "AMI_PROG_CODE" },

  // slots host
  { 0xE2, "SLOT1_HOST" },

  // S3 Resume Error Code
  { 0xEA, "S3_BOOT_SCRIPT_ERR" },
  { 0xEF, "AMI_PROG_CODE" },

  // Recovery Progress Code
  { 0xF0, "REC_BY_FW" },
  { 0xF1, "REC_BY_USER" },
  { 0xF2, "REC_STARTED" },
  { 0xF3, "REC_FW_FOUND" },
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
  { 0xBD, "STS_GET_MARGINS" },
  /*--------------------- MRC Phase - End----------------------- */

  { 0x4F, "DXE_IPL_START" },

  { 0xF9, "RECOVERY_CAP_NOT_FOUND" },
  { 0x70, "STS_HBM" },
  { 0x71, "STS_HBM_DEBUG" },
  { 0x72, "STS_HBM_INTERNAL" },
  { 0x7E, "STS_PIPE_SYNC" },
  { 0xC1, "STS_CHECK_POR" },
  { 0xC2, "STS_UNLOCK_MEM_REGS" },
  { 0xC3, "STS_CHECK_STATUS" },
  { 0xC4, "STS_CONFIG_XMP" },
  { 0xC5, "STS_EARLY_INIT_MEM" },
  { 0xC6, "STS_DIMM_INFO" },
  { 0xC7, "STS_NVDIMM" },
  { 0xC9, "STS_SVL_SCRAMBLE" }

};

static post_desc_t pdesc_phase2_class2[] = {
  { 0x61, "NVRAM_INIT" },
  { 0x9A, "USB_INIT" },
  { 0x78, "ACPI_INIT" },
  { 0x68, "PCI_BRIDGE_INIT" },
  { 0x70, "SB_DXE_START" },
  { 0x79, "CSM_INIT" },
  { 0xD1, "NB_INIT_ERR" },
  { 0xD2, "SB_INIT_ERR" },
  { 0xD4, "PCI_ALLOC_ERR" },
  { 0x92, "PCIB_INIT" },
  { 0x94, "PCIB_ENUMERATION" },
  { 0x95, "PCIB_REQ_RESOURCE" },
  { 0x96, "PCIB_ASSIGN_RESOURCE" },
  { 0xEF, "PCIB_INIT" },
  { 0x99, "SUPER_IO_INIT" },
  { 0x91, "DRIVER_CONN_START" },
  { 0xD5, "NO_SPACE_ROM" },
  { 0x97, "CONSOLE_INPUT_CONN" },
  { 0xB2, "LEGACY_ROM_INIT" },
  { 0xAA, "ACPI_ACPI_MODE" },
  { 0xC0, "OEM_DXE_OPROM_ENTER" },
  { 0xBB, "AMI_CODE" },
  { 0xC1, "OEM_DXE_OPROM_EXIT" },
  { 0x98, "CONSOLE_OUTPUT_CONN" },
  { 0x9D, "USB_ENABLE" },
  { 0x9C, "USB_DETECT" },
  { 0xB4, "USB_HOT_PLUG" },
  { 0xA0, "IDE_INIT" },
  { 0xA2, "IDE_DETECT" },
  { 0xA9, "START_OF_SETUP" },
  { 0xAB, "SETUP_INIT_WAIT" },

  /*--------------------- DXE Phase - Start--------------------- */
  { 0x60, "DXE_CORE_START" },
  { 0x62, "INSTALL_SB_SERVICE" },
  { 0x63, "CPU_DXE_STARTED" },
  { 0x64, "CPU_DXE_INIT" },
  { 0x65, "CPU_DXE_INIT" },
  { 0x66, "CPU_DXE_INIT" },
  { 0x67, "CPU_DXE_INIT" },
  { 0x69, "NB_DXE_INIT" },
  { 0x6A, "NB_DXE_SMM_INIT" },
  { 0x6B, "NB_DXE_BRI_START" },
  { 0x6C, "NB_DXE_BRI_START" },
  { 0x6D, "NB_DXE_BRI_START" },
  { 0x6E, "NB_DXE_BRI_START" },
  { 0x6F, "NB_DXE_BRI_START" },

  { 0x71, "SB_DXE_SMM_INIT" },
  { 0x72, "SB_DXE_DEV_START" },
  { 0x73, "SB_DXE_BRI_START" },
  { 0x74, "SB_DXE_BRI_START" },
  { 0x75, "SB_DXE_BRI_START" },
  { 0x76, "SB_DXE_BRI_START" },
  { 0x77, "SB_DXE_BRI_START" },
  { 0x7A, "AMI_DXE_CODE" },
  { 0x7B, "AMI_DXE_CODE" },
  { 0x7C, "AMI_DXE_CODE" },
  { 0x7D, "AMI_DXE_CODE" },
  { 0x7E, "AMI_DXE_CODE" },
  { 0x7F, "AMI_DXE_CODE" },

  //OEM DXE INIT CODE
  { 0x80, "OEM_DXE_INIT" },
  { 0x81, "OEM_DXE_INIT" },
  { 0x82, "OEM_DXE_INIT" },
  { 0x83, "OEM_DXE_INIT" },
  { 0x84, "OEM_DXE_INIT" },
  { 0x85, "OEM_DXE_INIT" },
  { 0x86, "OEM_DXE_INIT" },
  { 0x87, "OEM_DXE_INIT" },
  { 0x88, "OEM_DXE_INIT" },
  { 0x89, "OEM_DXE_INIT" },
  { 0x8A, "OEM_DXE_INIT" },
  { 0x8B, "OEM_DXE_INIT" },
  { 0x8C, "OEM_DXE_INIT" },
  { 0x8D, "OEM_DXE_INIT" },
  { 0x8E, "OEM_DXE_INIT" },
  { 0x8F, "OEM_DXE_INIT" },

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
  { 0xB6, "CLEAN_UP_NVRAM" },
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
  /*--------------------- DXE Phase - End--------------------- */

  /*--------------------- SLOT Info Start--------------------- */
  { 0xE0, "BB_BIC" },
  { 0xE1, "SLOT1_BMC" },
  { 0xE2, "SLOT1_HOST" },
  { 0xE3, "SLOT1_BIC" },
  { 0xE4, "SLOT3_BMC" },
  { 0xE5, "SLOT3_HOST"},
  { 0xE6, "SLOT3_BIC"},
  /*--------------------- SLOT Info End--------------------- */
  { 0x58, "CPU_SELFTEST_FAIL" }
};

static post_phase_desc_t post_phase_desc[] = {
  {1, pdesc_phase1, sizeof(pdesc_phase1)/sizeof(pdesc_phase1[0])},
  {2, pdesc_phase2, sizeof(pdesc_phase2)/sizeof(pdesc_phase2[0])},
};

static post_phase_desc_t post_phase_desc_class2[] = {
  {1, pdesc_phase1_class2, sizeof(pdesc_phase1_class2)/sizeof(pdesc_phase1_class2[0])},
  {2, pdesc_phase2_class2, sizeof(pdesc_phase2_class2)/sizeof(pdesc_phase2_class2[0])},
};

static dbg_gpio_desc_t gdesc[] = {
 { 0x10, 0, 1, "RST_BTN_N" },
 { 0x11, 0, 2, "PWR_BTN_N" },
 { 0x12, 0, 0, "SYS_PWROK" },
 { 0x13, 0, 0, "RST_PLTRST" },
 { 0x14, 0, 0, "RST_RSMRST" },
 { 0x15, 0, 0, "FM_MSMI_CATERR" },
 { 0x16, 0, 0, "FM_SLPS3" },
 { 0x17, 0, 3, "FM_UART_SWITCH" },
};

static sensor_desc_t cri_sensor_sb[] =
{
  {"MB Inlet Temp:"        , BIC_SENSOR_INLET_TEMP         , "C"    , FRU_ALL, 0},
  {"MB Outlet Temp:"       , BIC_SENSOR_OUTLET_TEMP        , "C"    , FRU_ALL, 0},
  {"FRONT IO Temp:"        , BIC_SENSOR_FIO_TEMP           , "C"    , FRU_ALL, 2},
  {"PCH Temp:"             , BIC_SENSOR_PCH_TEMP           , "C"    , FRU_ALL, 0},
  {"SOC CPU Temp:"         , BIC_SENSOR_CPU_TEMP           , "C"    , FRU_ALL, 0},
  {"Therm Margin:"         , BIC_SENSOR_CPU_THERM_MARGIN   , "C"    , FRU_ALL, 0},
  {"SOC CPU TjMax:"        , BIC_SENSOR_CPU_TJMAX          , "C"    , FRU_ALL, 0},
  {"CPU Pkg Pwr:"          , BIC_SENSOR_SOC_PKG_PWR        , "W"    , FRU_ALL, 0},
  {"DIMMA TEMP:"           , BIC_SENSOR_DIMMA0_TEMP        , "C"    , FRU_ALL, 0},
  {"DIMMB TEMP:"           , BIC_SENSOR_DIMMB0_TEMP        , "C"    , FRU_ALL, 0},
  {"DIMMC TEMP:"           , BIC_SENSOR_DIMMC0_TEMP        , "C"    , FRU_ALL, 0},
  {"DIMMD TEMP:"           , BIC_SENSOR_DIMMD0_TEMP        , "C"    , FRU_ALL, 0},
  {"DIMME TEMP:"           , BIC_SENSOR_DIMME0_TEMP        , "C"    , FRU_ALL, 0},
  {"DIMMF TEMP:"           , BIC_SENSOR_DIMMF0_TEMP        , "C"    , FRU_ALL, 0},
  {"SSD1 Temp:"            , BIC_SENSOR_M2B_TEMP           , "C"    , FRU_ALL, 0},
  {"HSC Temp:"             , BIC_SENSOR_HSC_TEMP           , "C"    , FRU_ALL, 0},
  {"VCCIN VR Temp:"        , BIC_SENSOR_VCCIN_VR_TEMP      , "C"    , FRU_ALL, 0},
  {"VCCSA VR Temp:"        , BIC_SENSOR_VCCSA_VR_TEMP      , "C"    , FRU_ALL, 0},
  {"VCCIO VR Temp:"        , BIC_SENSOR_VCCIO_VR_Temp      , "C"    , FRU_ALL, 0},
  {"3V3STBY VRTemp:"       , BIC_SENSOR_P3V3_STBY_VR_TEMP  , "C"    , FRU_ALL, 0},
  {"VDDQABC VRTemp:"       , BIC_SENSOR_PVDDQ_ABC_VR_TEMP  , "C"    , FRU_ALL, 0},
  {"VDDQDEF VRTemp:"       , BIC_SENSOR_PVDDQ_DEF_VR_TEMP  , "C"    , FRU_ALL, 0},
  {"P12V_STBY Vol:"        , BIC_SENSOR_P12V_STBY_VOL      , "V"    , FRU_ALL, 2},
  {"P3V_BAT Vol:"          , BIC_SENSOR_P3V_BAT_VOL        , "V"    , FRU_ALL, 2},
  {"P3V3_STBY Vol:"        , BIC_SENSOR_P3V3_STBY_VOL      , "V"    , FRU_ALL, 2},
  {"P1V05_PCH Vol:"        , BIC_SENSOR_P1V05_PCH_STBY_VOL , "V"    , FRU_ALL, 2},
  {"PVNN_PCH Vol:"         , BIC_SENSOR_PVNN_PCH_STBY_VOL  , "V"    , FRU_ALL, 2},
  {"HSC Input Vol:"        , BIC_SENSOR_HSC_INPUT_VOL      , "V"    , FRU_ALL, 2},
  {"VCCIN VR Vol:"         , BIC_SENSOR_VCCIN_VR_VOL       , "V"    , FRU_ALL, 2},
  {"VCCSA VR Vol:"         , BIC_SENSOR_VCCSA_VR_VOL       , "V"    , FRU_ALL, 2},
  {"VCCIO VR Vol:"         , BIC_SENSOR_VCCIO_VR_VOL       , "V"    , FRU_ALL, 2},
  {"3V3STBY VRVol:"        , BIC_SENSOR_P3V3_STBY_VR_VOL   , "V"    , FRU_ALL, 2},
  {"VDDQABC VRVol:"        , BIC_PVDDQ_ABC_VR_VOL          , "V"    , FRU_ALL, 2},
  {"VDDQDEF VRVol:"        , BIC_PVDDQ_DEF_VR_VOL          , "V"    , FRU_ALL, 2},
  {"HSC OutputCur:"        , BIC_SENSOR_HSC_OUTPUT_CUR     , "Amps" , FRU_ALL, 2},
  {"VCCIN VR Cur:"         , BIC_SENSOR_VCCIN_VR_CUR       , "Amps" , FRU_ALL, 2},
  {"VCCSA VR Cur:"         , BIC_SENSOR_VCCSA_VR_CUR       , "Amps" , FRU_ALL, 2},
  {"VCCIO VR Cur:"         , BIC_SENSOR_VCCIO_VR_CUR       , "Amps" , FRU_ALL, 2},
  {"3V3STBY VRCur:"        , BIC_SENSOR_P3V3_STBY_VR_CUR   , "Amps" , FRU_ALL, 2},
  {"VDDQABC VRCur:"        , BIC_SENSOR_PVDDQ_ABC_VR_CUR   , "Amps" , FRU_ALL, 2},
  {"VDDQDEF VRCur:"        , BIC_SENSOR_PVDDQ_DEF_VR_CUR   , "Amps" , FRU_ALL, 2},
  {"HSC PwrIn:"            , BIC_SENSOR_HSC_INPUT_PWR      , "W"    , FRU_ALL, 0},
  {"HSC AvgPwrIn:"         , BIC_SENSOR_HSC_INPUT_AVGPWR   , "W"    , FRU_ALL, 0},
  {"VCCIN VR Pout:"        , BIC_SENSOR_VCCIN_VR_POUT      , "W"    , FRU_ALL, 0},
  {"VCCSA VR Pout:"        , BIC_SENSOR_VCCSA_VR_POUT      , "W"    , FRU_ALL, 0},
  {"VCCIO VR Pout:"        , BIC_SENSOR_VCCIO_VR_POUT      , "W"    , FRU_ALL, 0},
  {"3V3STBY VRPout:"       , BIC_SENSOR_P3V3_STBY_VR_POUT  , "W"    , FRU_ALL, 0},
  {"VDDQABC VRPout"        , BIC_SENSOR_PVDDQ_ABC_VR_POUT  , "W"    , FRU_ALL, 2},
  {"VDDQDEF VRPout:"       , BIC_SENSOR_PVDDQ_DEF_VR_POUT  , "W"    , FRU_ALL, 2},
 
};

static sensor_desc_t cri_sensor_1ou_edsff[] =
{
  {"E1S Outlet Temp:"     , BIC_1OU_EDSFF_SENSOR_NUM_T_MB_OUTLET_TEMP_T , "C"     , FRU_ALL, 2},
  {"E1S P12V_AUX:"        , BIC_1OU_EDSFF_SENSOR_NUM_V_12_AUX           , "V"     , FRU_ALL, 2},
  {"E1S P12V_EDGE:"       , BIC_1OU_EDSFF_SENSOR_NUM_V_12_EDGE          , "V"     , FRU_ALL, 2},
  {"E1S P3V3_AUX:"        , BIC_1OU_EDSFF_SENSOR_NUM_V_3_3_AUX          , "V"     , FRU_ALL, 2},
  {"E1S HSC VIN:"         , BIC_1OU_EDSFF_SENSOR_NUM_V_HSC_IN           , "V"     , FRU_ALL, 2},
  {"E1S HSC IOUT:"        , BIC_1OU_EDSFF_SENSOR_NUM_I_HSC_OUT          , "Amps"  , FRU_ALL, 2},
  {"E1S HSC PIN:"         , BIC_1OU_EDSFF_SENSOR_NUM_P_HSC_IN           , "W"     , FRU_ALL, 2},
  {"E1S HSC Temp:"        , BIC_1OU_EDSFF_SENSOR_NUM_T_HSC              , "C"     , FRU_ALL, 2},
  {"E1S DEV0 Power:"      , BIC_1OU_EDSFF_SENSOR_NUM_INA231_PWR_M2A     , "W"     , FRU_ALL, 2},
  {"E1S DEV0 Voltage:"    , BIC_1OU_EDSFF_SENSOR_NUM_INA231_VOL_M2A     , "V"     , FRU_ALL, 2},
  {"E1S DEV0 Temp:"       , BIC_1OU_EDSFF_SENSOR_NUM_NVME_TEMP_M2A      , "C"     , FRU_ALL, 2},
  {"E1S 3V3_ADC_DEV0:"    , BIC_1OU_EDSFF_SENSOR_NUM_ADC_3V3_VOL_M2A    , "V"     , FRU_ALL, 2},
  {"E1S 12V_ADC_DEV0:"    , BIC_1OU_EDSFF_SENSOR_NUM_ADC_12V_VOL_M2A    , "V"     , FRU_ALL, 2},
  {"E1S DEV1 Power:"      , BIC_1OU_EDSFF_SENSOR_NUM_INA231_PWR_M2B     , "W"     , FRU_ALL, 2},
  {"E1S DEV1 Voltage:"    , BIC_1OU_EDSFF_SENSOR_NUM_INA231_VOL_M2B     , "V"     , FRU_ALL, 2},
  {"E1S DEV1 Temp:"       , BIC_1OU_EDSFF_SENSOR_NUM_NVME_TEMP_M2B      , "C"     , FRU_ALL, 2},
  {"E1S 3V3_ADC_DEV1:"    , BIC_1OU_EDSFF_SENSOR_NUM_ADC_3V3_VOL_M2B    , "V"     , FRU_ALL, 2},
  {"E1S 12V_ADC_DEV1:"    , BIC_1OU_EDSFF_SENSOR_NUM_ADC_12V_VOL_M2B    , "V"     , FRU_ALL, 2},
  {"E1S DEV2 Power:"      , BIC_1OU_EDSFF_SENSOR_NUM_INA231_PWR_M2C     , "W"     , FRU_ALL, 2},
  {"E1S DEV2 Voltage:"    , BIC_1OU_EDSFF_SENSOR_NUM_INA231_VOL_M2C     , "V"     , FRU_ALL, 2},
  {"E1S DEV2 Temp:"       , BIC_1OU_EDSFF_SENSOR_NUM_NVME_TEMP_M2C      , "C"     , FRU_ALL, 2},
  {"E1S 3V3_ADC_DEV2:"    , BIC_1OU_EDSFF_SENSOR_NUM_ADC_3V3_VOL_M2C    , "V"     , FRU_ALL, 2},
  {"E1S 12V_ADC_DEV2:"    , BIC_1OU_EDSFF_SENSOR_NUM_ADC_12V_VOL_M2C    , "V"     , FRU_ALL, 2},
  {"E1S DEV3 Power:"      , BIC_1OU_EDSFF_SENSOR_NUM_INA231_PWR_M2D     , "W"     , FRU_ALL, 2},
  {"E1S DEV3 Voltage:"    , BIC_1OU_EDSFF_SENSOR_NUM_INA231_VOL_M2D     , "V"     , FRU_ALL, 2},
  {"E1S DEV3 Temp :"      , BIC_1OU_EDSFF_SENSOR_NUM_NVME_TEMP_M2D      , "C"     , FRU_ALL, 2},
  {"E1S 3V3_ADC_DEV3:"    , BIC_1OU_EDSFF_SENSOR_NUM_ADC_3V3_VOL_M2D    , "V"     , FRU_ALL, 2},
  {"E1S 12V_ADC_DEV3:"    , BIC_1OU_EDSFF_SENSOR_NUM_ADC_12V_VOL_M2D    , "V"     , FRU_ALL, 2},
};

static sensor_desc_t cri_sensor_gpv3[] =
{
// GPv3 sensors
  {"GP3 Outlet Temp:"      , BIC_GPV3_OUTLET_TEMP          , "C"    , FRU_ALL, 0},
  {"GP3 PESW Temp:"        , BIC_GPV3_PCIE_SW_TEMP         , "C"    , FRU_ALL, 0},
  {"GP3 3V3S1 Temp:"       , BIC_GPV3_P3V3_STBY1_TEMP      , "C"    , FRU_ALL, 0},
  {"GP3 3V3S2 Temp:"       , BIC_GPV3_P3V3_STBY2_TEMP      , "C"    , FRU_ALL, 0},
  {"GP3 3V3S3 Temp:"       , BIC_GPV3_P3V3_STBY3_TEMP      , "C"    , FRU_ALL, 0},
  {"GP3 PESW VR P:"        , BIC_GPV3_VR_P0V84_POWER       , "W"    , FRU_ALL, 0},
  {"GP3 PESW VR T:"        , BIC_GPV3_VR_P0V84_TEMP        , "C"    , FRU_ALL, 0},
  {"GP3 P1V8 Pwr:"         , BIC_GPV3_VR_P1V8_POWER        , "W"    , FRU_ALL, 0},
  {"GP3 P1V8 Temp:"        , BIC_GPV3_VR_P1V8_TEMP         , "C"    , FRU_ALL, 0},
  {"GP3 E1S0 Temp:"        , BIC_GPV3_E1S_1_TEMP           , "C"    , FRU_ALL, 0},
  {"GP3 E1S1 Temp:"        , BIC_GPV3_E1S_2_TEMP           , "C"    , FRU_ALL, 0},
  {"GP3 M2 0 Temp:"        , BIC_GPV3_NVME_TEMP_DEV0       , "C"    , FRU_ALL, 0},
  {"GP3 M2 1 Temp:"        , BIC_GPV3_NVME_TEMP_DEV1       , "C"    , FRU_ALL, 0},
  {"GP3 M2 2 Temp:"        , BIC_GPV3_NVME_TEMP_DEV2       , "C"    , FRU_ALL, 0},
  {"GP3 M2 3 Temp:"        , BIC_GPV3_NVME_TEMP_DEV3       , "C"    , FRU_ALL, 0},
  {"GP3 M2 4 Temp:"        , BIC_GPV3_NVME_TEMP_DEV4       , "C"    , FRU_ALL, 0},
  {"GP3 M2 5 Temp:"        , BIC_GPV3_NVME_TEMP_DEV5       , "C"    , FRU_ALL, 0},
  {"GP3 M2 6 Temp:"        , BIC_GPV3_NVME_TEMP_DEV6       , "C"    , FRU_ALL, 0},
  {"GP3 M2 7 Temp:"        , BIC_GPV3_NVME_TEMP_DEV7       , "C"    , FRU_ALL, 0},
  {"GP3 M2 8 Temp:"        , BIC_GPV3_NVME_TEMP_DEV8       , "C"    , FRU_ALL, 0},
  {"GP3 M2 9 Temp:"        , BIC_GPV3_NVME_TEMP_DEV9       , "C"    , FRU_ALL, 0},
  {"GP3 M2 10 Temp:"       , BIC_GPV3_NVME_TEMP_DEV10      , "C"    , FRU_ALL, 0},
  {"GP3 M2 11 Temp:"       , BIC_GPV3_NVME_TEMP_DEV11      , "C"    , FRU_ALL, 0},
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
  {"P1V15_STBY:"           , BMC_SENSOR_P1V15_BMC_STBY     , "V"    , FRU_BMC, 2},
  {"P1V2_STBY:"            , BMC_SENSOR_P1V2_BMC_STBY      , "V"    , FRU_BMC, 2},
  {"P2V5_STBY:"            , BMC_SENSOR_P2V5_BMC_STBY      , "V"    , FRU_BMC, 2},
  {"HSC_TEMP:"             , BMC_SENSOR_HSC_TEMP           , "C"    , FRU_BMC, 2},
  {"HSC_VIN:"              , BMC_SENSOR_HSC_VIN            , "V"    , FRU_BMC, 2},
  {"HSC_PIN:"              , BMC_SENSOR_HSC_PIN            , "W"    , FRU_BMC, 2},
  {"HSC_EIN:"              , BMC_SENSOR_HSC_EIN            , "W"    , FRU_BMC, 2},
  {"HSC_IOUT:"             , BMC_SENSOR_HSC_IOUT           , "Amps" , FRU_BMC, 2},
  {"HSCP_EAK_IOUT:"        , BMC_SENSOR_HSC_PEAK_IOUT      , "Amps" , FRU_BMC, 2},
  {"HSCPEAKPIN:"           , BMC_SENSOR_HSC_PEAK_PIN       , "W"    , FRU_BMC, 2},
  {"MEDUSA_VOUT:"          , BMC_SENSOR_MEDUSA_VOUT        , "V"    , FRU_BMC, 2},
  {"MEDUSA_VIN:"           , BMC_SENSOR_MEDUSA_VIN         , "V"    , FRU_BMC, 2},
  {"MEDUSA_CURR:"          , BMC_SENSOR_MEDUSA_CURR        , "Amps" , FRU_BMC, 2},
  {"MEDUSA_PWR:"           , BMC_SENSOR_MEDUSA_PWR         , "W"    , FRU_BMC, 2},
  {"MEDUSA_VDELTA:"        , BMC_SENSOR_MEDUSA_VDELTA      , "V"    , FRU_BMC, 2},
  {"PDB_DL_VDELTA:"        , BMC_SENSOR_PDB_DL_VDELTA      , "V"    , FRU_BMC, 2},
  {"PDB_BB_VDELTA:"        , BMC_SENSOR_PDB_BB_VDELTA      , "V"    , FRU_BMC, 2},
  {"CURR_LEAKAGE:"         , BMC_SENSOR_CURR_LEAKAGE       , "%"    , FRU_BMC, 2},
  {"FAN_IOUT:"             , BMC_SENSOR_FAN_IOUT           , "Amps" , FRU_BMC, 2},
  {"FAN_PWR:"              , BMC_SENSOR_FAN_PWR            , "W"    , FRU_BMC, 2},
  {"NIC_P12V:"             , BMC_SENSOR_NIC_P12V           , "V"    , FRU_BMC, 2},
  {"NIC_IOUT:"             , BMC_SENSOR_NIC_IOUT           , "Amps" , FRU_BMC, 2},
  {"NIC_PWR:"              , BMC_SENSOR_NIC_PWR            , "W"    , FRU_BMC, 2},
};

static sensor_desc_t cri_sensor_bmc_class2[] =
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
  {"BMC_P12V:"             , BMC_SENSOR_P12V               , "V"    , FRU_BMC, 2},
  {"P3V3_STBY:"            , BMC_SENSOR_P3V3_STBY          , "V"    , FRU_BMC, 2},
  {"P1V15_STBY:"           , BMC_SENSOR_P1V15_BMC_STBY     , "V"    , FRU_BMC, 2},
  {"P1V2_STBY:"            , BMC_SENSOR_P1V2_BMC_STBY      , "V"    , FRU_BMC, 2},
  {"P2V5_STBY:"            , BMC_SENSOR_P2V5_BMC_STBY      , "V"    , FRU_BMC, 2},
  {"NIC_P12V:"             , BMC_SENSOR_NIC_P12V           , "V"    , FRU_BMC, 2},
  {"NIC_IOUT:"             , BMC_SENSOR_NIC_IOUT           , "Amps" , FRU_BMC, 2},
  {"NIC_PWR:"              , BMC_SENSOR_NIC_PWR            , "W"    , FRU_BMC, 2},
};

static sensor_desc_t cri_sensor_cwc[] =
// CWC sensors
{
  {"CWC Outlet Temp:"      , BIC_CWC_SENSOR_OUTLET_TEMP_T  , "C"    , FRU_ALL, 2},
  {"CWC PESW Temp:"        , BIC_CWC_SENSOR_PCIE_SWITCH_T  , "C"    , FRU_ALL, 2},
  {"CWC P12V STBY:"        , BIC_CWC_SENSOR_NUM_V_12       , "V"    , FRU_ALL, 2},
  {"CWC P3V3 STBY:"        , BIC_CWC_SENSOR_NUM_V_3_3_S    , "V"    , FRU_ALL, 2},
  {"CWC P1V8 Vol:"         , BIC_CWC_SENSOR_NUM_V_1_8      , "V"    , FRU_ALL, 2},
  {"CWC P5V Vol:"          , BIC_CWC_SENSOR_NUM_V_5        , "V"    , FRU_ALL, 2},
  {"CWC P1V8 VR Pwr:"      , BIC_CWC_SENSOR_NUM_P_P1V8_VR  , "W"    , FRU_ALL, 2},
  {"CWC P1V8 VR Vol:"      , BIC_CWC_SENSOR_NUM_V_P1V8_VR  , "V"    , FRU_ALL, 2},
  {"CWC P1V8 VR Cur:"      , BIC_CWC_SENSOR_NUM_C_P1V8_VR  , "Amps" , FRU_ALL, 2},
  {"CWC P1V8 VR Temp:"     , BIC_CWC_SENSOR_NUM_T_P1V8_VR  , "C"    , FRU_ALL, 2},
  {"CWC P0V84 VR Pwr:"     , BIC_CWC_SENSOR_NUM_P_P0V84_VR , "W"    , FRU_ALL, 2},
  {"CWC P0V84 VR Vol:"     , BIC_CWC_SENSOR_NUM_V_P0V84_VR , "V"    , FRU_ALL, 2},
  {"CWC P0V84 VR Cur:"     , BIC_CWC_SENSOR_NUM_C_P0V84_VR , "Amps" , FRU_ALL, 2},
  {"CWC P0V84VR Temp:"     , BIC_CWC_SENSOR_NUM_T_P0V84_VR , "C"    , FRU_ALL, 2},
  {"CWC P3V3 AUX Pwr:"     , BIC_CWC_SENSOR_NUM_P_3V3_AUX  , "W"    , FRU_ALL, 2},
  {"CWC P3V3 AUX Vol:"     , BIC_CWC_SENSOR_NUM_V_3V3_AUX  , "V"    , FRU_ALL, 2},
  {"CWC P3V3 AUX Cur:"     , BIC_CWC_SENSOR_NUM_C_3V3_AUX  , "Amps" , FRU_ALL, 2},
  {"CWC HSC CWC Pwr:"      , BIC_CWC_SENSOR_NUM_P_HSC_CWC  , "W"    , FRU_ALL, 2},
  {"CWC HSC CWC Vol:"      , BIC_CWC_SENSOR_NUM_V_HSC_CWC  , "V"    , FRU_ALL, 2},
  {"CWC HSC CWC Cur:"      , BIC_CWC_SENSOR_NUM_C_HSC_CWC  , "Amps" , FRU_ALL, 2},
  {"CWC HSC CWC Temp:"     , BIC_CWC_SENSOR_NUM_T_HSC_CWC  , "C"    , FRU_ALL, 2},
  {"CWC HSC BOT Pwr:"      , BIC_CWC_SENSOR_NUM_P_HSC_BOT  , "W"    , FRU_ALL, 2},
  {"CWC HSC BOT Vol:"      , BIC_CWC_SENSOR_NUM_V_HSC_BOT  , "V"    , FRU_ALL, 2},
  {"CWC HSC BOT Cur:"      , BIC_CWC_SENSOR_NUM_C_HSC_BOT  , "Amps" , FRU_ALL, 2},
  {"CWC HSC BOT Temp:"     , BIC_CWC_SENSOR_NUM_T_HSC_BOT  , "C"    , FRU_ALL, 2},
  {"CWC HSC TOP Pwr:"      , BIC_CWC_SENSOR_NUM_P_HSC_TOP  , "W"    , FRU_ALL, 2},
  {"CWC HSC TOP Vol:"      , BIC_CWC_SENSOR_NUM_V_HSC_TOP  , "V"    , FRU_ALL, 2},
  {"CWC HSC TOP Cur:"      , BIC_CWC_SENSOR_NUM_C_HSC_TOP  , "Amps" , FRU_ALL, 2},
  {"CWC HSC TOP Temp:"     , BIC_CWC_SENSOR_NUM_T_HSC_TOP  , "C"    , FRU_ALL, 2},
  {"CWC PESW Pwr:"         , BIC_CWC_SENSOR_NUM_P_PESW     , "W"    , FRU_ALL, 2},
};

static sensor_desc_t cri_sensor_cwc_top_gpv3[] =
{
// GPv3 sensors
  {"TOP GP3 Outlet Temp:"      , BIC_GPV3_OUTLET_TEMP          , "C"    , FRU_2U_TOP, 2},
  {"TOP GP3 PESW Temp:"        , BIC_GPV3_PCIE_SW_TEMP         , "C"    , FRU_2U_TOP, 2},
  {"TOP GP3 3V3S1 Temp:"       , BIC_GPV3_P3V3_STBY1_TEMP      , "C"    , FRU_2U_TOP, 2},
  {"TOP GP3 3V3S2 Temp:"       , BIC_GPV3_P3V3_STBY2_TEMP      , "C"    , FRU_2U_TOP, 2},
  {"TOP GP3 3V3S3 Temp:"       , BIC_GPV3_P3V3_STBY3_TEMP      , "C"    , FRU_2U_TOP, 2},
  {"TOP GP3 PESW VR P:"        , BIC_GPV3_VR_P0V84_POWER       , "W"    , FRU_2U_TOP, 2},
  {"TOP GP3 PESW VR T:"        , BIC_GPV3_VR_P0V84_TEMP        , "C"    , FRU_2U_TOP, 2},
  {"TOP GP3 P1V8 Pwr:"         , BIC_GPV3_VR_P1V8_POWER        , "W"    , FRU_2U_TOP, 2},
  {"TOP GP3 P1V8 Temp:"        , BIC_GPV3_VR_P1V8_TEMP         , "C"    , FRU_2U_TOP, 2},
  {"TOP GP3 E1S0 Temp:"        , BIC_GPV3_E1S_1_TEMP           , "C"    , FRU_2U_TOP, 2},
  {"TOP GP3 E1S1 Temp:"        , BIC_GPV3_E1S_2_TEMP           , "C"    , FRU_2U_TOP, 2},
  {"TOP GP3 M2 0 Temp:"        , BIC_GPV3_NVME_TEMP_DEV0       , "C"    , FRU_2U_TOP, 2},
  {"TOP GP3 M2 1 Temp:"        , BIC_GPV3_NVME_TEMP_DEV1       , "C"    , FRU_2U_TOP, 2},
  {"TOP GP3 M2 2 Temp:"        , BIC_GPV3_NVME_TEMP_DEV2       , "C"    , FRU_2U_TOP, 2},
  {"TOP GP3 M2 3 Temp:"        , BIC_GPV3_NVME_TEMP_DEV3       , "C"    , FRU_2U_TOP, 2},
  {"TOP GP3 M2 4 Temp:"        , BIC_GPV3_NVME_TEMP_DEV4       , "C"    , FRU_2U_TOP, 2},
  {"TOP GP3 M2 5 Temp:"        , BIC_GPV3_NVME_TEMP_DEV5       , "C"    , FRU_2U_TOP, 2},
  {"TOP GP3 M2 6 Temp:"        , BIC_GPV3_NVME_TEMP_DEV6       , "C"    , FRU_2U_TOP, 2},
  {"TOP GP3 M2 7 Temp:"        , BIC_GPV3_NVME_TEMP_DEV7       , "C"    , FRU_2U_TOP, 2},
  {"TOP GP3 M2 8 Temp:"        , BIC_GPV3_NVME_TEMP_DEV8       , "C"    , FRU_2U_TOP, 2},
  {"TOP GP3 M2 9 Temp:"        , BIC_GPV3_NVME_TEMP_DEV9       , "C"    , FRU_2U_TOP, 2},
  {"TOP GP3 M2 10 Temp:"       , BIC_GPV3_NVME_TEMP_DEV10      , "C"    , FRU_2U_TOP, 2},
  {"TOP GP3 M2 11 Temp:"       , BIC_GPV3_NVME_TEMP_DEV11      , "C"    , FRU_2U_TOP, 2},
};

static sensor_desc_t cri_sensor_cwc_bot_gpv3[] =
{
// GPv3 sensors
  {"BOT GP3 Outlet Temp:"      , BIC_GPV3_OUTLET_TEMP          , "C"    , FRU_2U_BOT, 2},
  {"BOT GP3 PESW Temp:"        , BIC_GPV3_PCIE_SW_TEMP         , "C"    , FRU_2U_BOT, 2},
  {"BOT GP3 3V3S1 Temp:"       , BIC_GPV3_P3V3_STBY1_TEMP      , "C"    , FRU_2U_BOT, 2},
  {"BOT GP3 3V3S2 Temp:"       , BIC_GPV3_P3V3_STBY2_TEMP      , "C"    , FRU_2U_BOT, 2},
  {"BOT GP3 3V3S3 Temp:"       , BIC_GPV3_P3V3_STBY3_TEMP      , "C"    , FRU_2U_BOT, 2},
  {"BOT GP3 PESW VR P:"        , BIC_GPV3_VR_P0V84_POWER       , "W"    , FRU_2U_BOT, 2},
  {"BOT GP3 PESW VR T:"        , BIC_GPV3_VR_P0V84_TEMP        , "C"    , FRU_2U_BOT, 2},
  {"BOT GP3 P1V8 Pwr:"         , BIC_GPV3_VR_P1V8_POWER        , "W"    , FRU_2U_BOT, 2},
  {"BOT GP3 P1V8 Temp:"        , BIC_GPV3_VR_P1V8_TEMP         , "C"    , FRU_2U_BOT, 2},
  {"BOT GP3 E1S0 Temp:"        , BIC_GPV3_E1S_1_TEMP           , "C"    , FRU_2U_BOT, 2},
  {"BOT GP3 E1S1 Temp:"        , BIC_GPV3_E1S_2_TEMP           , "C"    , FRU_2U_BOT, 2},
  {"BOT GP3 M2 0 Temp:"        , BIC_GPV3_NVME_TEMP_DEV0       , "C"    , FRU_2U_BOT, 2},
  {"BOT GP3 M2 1 Temp:"        , BIC_GPV3_NVME_TEMP_DEV1       , "C"    , FRU_2U_BOT, 2},
  {"BOT GP3 M2 2 Temp:"        , BIC_GPV3_NVME_TEMP_DEV2       , "C"    , FRU_2U_BOT, 2},
  {"BOT GP3 M2 3 Temp:"        , BIC_GPV3_NVME_TEMP_DEV3       , "C"    , FRU_2U_BOT, 2},
  {"BOT GP3 M2 4 Temp:"        , BIC_GPV3_NVME_TEMP_DEV4       , "C"    , FRU_2U_BOT, 2},
  {"BOT GP3 M2 5 Temp:"        , BIC_GPV3_NVME_TEMP_DEV5       , "C"    , FRU_2U_BOT, 2},
  {"BOT GP3 M2 6 Temp:"        , BIC_GPV3_NVME_TEMP_DEV6       , "C"    , FRU_2U_BOT, 2},
  {"BOT GP3 M2 7 Temp:"        , BIC_GPV3_NVME_TEMP_DEV7       , "C"    , FRU_2U_BOT, 2},
  {"BOT GP3 M2 8 Temp:"        , BIC_GPV3_NVME_TEMP_DEV8       , "C"    , FRU_2U_BOT, 2},
  {"BOT GP3 M2 9 Temp:"        , BIC_GPV3_NVME_TEMP_DEV9       , "C"    , FRU_2U_BOT, 2},
  {"BOT GP3 M2 10 Temp:"       , BIC_GPV3_NVME_TEMP_DEV10      , "C"    , FRU_2U_BOT, 2},
  {"BOT GP3 M2 11 Temp:"       , BIC_GPV3_NVME_TEMP_DEV11      , "C"    , FRU_2U_BOT, 2},
};

size_t cri_sensor_sb_cnt = sizeof(cri_sensor_sb)/sizeof(sensor_desc_t);
size_t cri_sensor_1ou_edsff_cnt = sizeof(cri_sensor_1ou_edsff)/sizeof(sensor_desc_t);
size_t cri_sensor_gpv3_cnt = sizeof(cri_sensor_gpv3)/sizeof(sensor_desc_t);
size_t cri_sensor_cwc_cnt = sizeof(cri_sensor_cwc)/sizeof(sensor_desc_t);
size_t cri_sensor_cwc_top_gpv3_cnt = sizeof(cri_sensor_cwc_top_gpv3)/sizeof(sensor_desc_t);
size_t cri_sensor_cwc_bot_gpv3_cnt = sizeof(cri_sensor_cwc_bot_gpv3)/sizeof(sensor_desc_t);

bool plat_supported(void)
{
  return true;
}

int plat_get_post_phase(uint8_t fru, post_phase_desc_t **desc, size_t *desc_count)
{
  int8_t ret = 0;
  uint8_t bmc_location = 0;

  if (!desc || !desc_count) {
    return -1;
  }

  ret = fby3_common_get_bmc_location(&bmc_location);
  if (ret < 0) {
    syslog(LOG_WARNING, "%s() Cannot get the location of BMC\n", __func__);
    return ret;
  }
  if (bmc_location == NIC_BMC) {
    *desc = post_phase_desc_class2;
    *desc_count = sizeof(post_phase_desc_class2) / sizeof(post_phase_desc_class2[0]);
  } else {
    *desc = post_phase_desc;
    *desc_count = sizeof(post_phase_desc) / sizeof(post_phase_desc[0]);
  }
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
  uint8_t type_2ou = UNKNOWN_BOARD;
  uint8_t type = 0;
  uint8_t current_cnt = 0;
  uint8_t bmc_location = 0;
  int config_status = 0;
  int ret = 0;
  if (!desc || !desc_count) {
    return -1;
  }
  if(fru == FRU_ALL){ //uart select BMC position
    ret = fby3_common_get_bmc_location(&bmc_location);
    if ( ret < 0 ) {
      syslog(LOG_WARNING, "%s() Cannot get the location of BMC\n", __func__);
      goto error_exit;
    }
    if ( bmc_location == NIC_BMC ) {
        *desc = cri_sensor_bmc_class2;
        *desc_count = sizeof(cri_sensor_bmc_class2) / sizeof(cri_sensor_bmc_class2[0]);
    } else {
      *desc = cri_sensor_bmc;
      *desc_count = sizeof(cri_sensor_bmc) / sizeof(cri_sensor_bmc[0]);
    }
  } else {
    config_status = (pal_is_fw_update_ongoing(fru) == false) ? bic_is_m2_exp_prsnt(fru) : bic_is_m2_exp_prsnt_cache(fru);
    if (config_status < 0)
      config_status = 0;

    memset(dynamic_cri_sensor, 0, sizeof(dynamic_cri_sensor));
    memcpy(dynamic_cri_sensor, cri_sensor_sb, sizeof(cri_sensor_sb));
    current_cnt = cri_sensor_sb_cnt;

    if ( (config_status & PRESENT_1OU) == PRESENT_1OU ) {
      bic_get_1ou_type_cache(fru, &type);
      if (type == EDSFF_1U) {
        memcpy(&dynamic_cri_sensor[current_cnt], cri_sensor_1ou_edsff, sizeof(cri_sensor_1ou_edsff));
        current_cnt += cri_sensor_1ou_edsff_cnt;
      }
    }

    if ( (config_status & PRESENT_2OU) == PRESENT_2OU ) {
      ret = fby3_common_get_2ou_board_type(fru, &type_2ou);
      if (ret < 0) {
        syslog(LOG_WARNING, "%s() Failed to get slot%d 2ou board type\n", __func__, fru);
        goto error_exit;
      } else if ( type_2ou == GPV3_MCHP_BOARD || type_2ou == GPV3_BRCM_BOARD ) {
        ret = pal_is_sensor_num_exceed(current_cnt + cri_sensor_gpv3_cnt);
        if (ret < 0) {
          goto error_exit;
        }
        memcpy(&dynamic_cri_sensor[current_cnt], cri_sensor_gpv3, sizeof(cri_sensor_gpv3));
        current_cnt += cri_sensor_gpv3_cnt;
      } else if (type_2ou == CWC_MCHP_BOARD) {
        ret = pal_is_sensor_num_exceed(current_cnt + cri_sensor_cwc_cnt + cri_sensor_cwc_top_gpv3_cnt + cri_sensor_cwc_bot_gpv3_cnt);
        if (ret < 0) {
          goto error_exit;
        }
        memcpy(&dynamic_cri_sensor[current_cnt], cri_sensor_cwc, sizeof(cri_sensor_cwc));
        current_cnt += cri_sensor_cwc_cnt;
        memcpy(&dynamic_cri_sensor[current_cnt], cri_sensor_cwc_top_gpv3, sizeof(cri_sensor_cwc_top_gpv3));
        current_cnt += cri_sensor_cwc_top_gpv3_cnt;
        memcpy(&dynamic_cri_sensor[current_cnt], cri_sensor_cwc_bot_gpv3, sizeof(cri_sensor_cwc_bot_gpv3));
        current_cnt += cri_sensor_cwc_bot_gpv3_cnt;
      }
    }
    
    *desc = dynamic_cri_sensor;
    *desc_count = current_cnt;
  }

error_exit:
  return ret;
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
  int ret = 0;
  const uint8_t cpld_addr = 0x80;
  uint8_t tbuf[4] = {0x00, 0x20, 0x00, 0x28};
  uint8_t rbuf[4] = {0x00};
  uint8_t tlen = 4;
  uint8_t rlen = 4;
  uint8_t i2c_bus = 12;
  int i2cfd = 0;
  uint8_t bmc_location = 0;
  uint8_t type_2ou = UNKNOWN_BOARD;

  if (fw_text == NULL)
    return -1;

  // Clear string buffer
  fw_text[0] = '\0';

  //CPLD Version
  if (slot_id == FRU_ALL) { //uart select BMC position
    ret = fby3_common_get_bmc_location(&bmc_location);
    if (ret < 0) {
      syslog(LOG_WARNING, "Failed to get bmc locaton");
      return -1;
    }

    if(bmc_location == NIC_BMC) {
      i2c_bus = 9;
    }

    ret = i2c_cdev_slave_open(i2c_bus, cpld_addr >> 1, I2C_SLAVE_FORCE_CLAIM);
    if (ret < 0) {
      syslog(LOG_WARNING, "Failed to open bus %d",i2c_bus);
      return -1;
    }

    i2cfd = ret;
    ret = i2c_rdwr_msg_transfer(i2cfd, cpld_addr, tbuf, tlen, rbuf, rlen);
    if ( i2cfd > 0 ) 
      close(i2cfd);
    if (ret < 0) {
      syslog(LOG_WARNING, "%s() Failed to do i2c_rdwr_msg_transfer to slave@0x%02X on bus %d", __func__, cpld_addr, i2c_bus);
      return -1;
    }
    sprintf(entry,"CPLD_ver:\n%02X%02X%02X%02X\n", rbuf[3], rbuf[2], rbuf[1], rbuf[0]);
    strcat(fw_text, entry);
  } else {
    //Bridge-IC Version
    if (bic_get_fw_ver(slot_id, FW_BIC, ver)) {
      strcat(fw_text,"BIC_ver:\nNA\n");
    } else {
      sprintf(entry,"BIC_ver:\nv%x.%02x\n", ver[0], ver[1]);
      strcat(fw_text, entry);
    }

    //Print Bridge-IC Bootloader Version
    if (bic_get_fw_ver(slot_id, FW_BIC_BOOTLOADER, ver)) {
      strcat(fw_text,"BICbl_ver:\nNA\n");
    } else {
      sprintf(entry,"BICbl_ver:\nv%x.%02x\n", ver[0], ver[1]);
      strcat(fw_text, entry);
    }

    if (pal_get_fw_info(slot_id, FW_CPLD, ver, &rlen)) {
      strcat(fw_text,"CPLD_ver:\nNA\n");
    } else {
      sprintf(entry,"CPLD_ver:\n%02X%02X%02X%02X\n", ver[0], ver[1], ver[2], ver[3]);
      strcat(fw_text, entry);
    }

    ret = bic_is_m2_exp_prsnt(slot_id);
    if (ret < 0) {
      syslog(LOG_WARNING, "Failed to get 1ou & 2ou present status");
    } else if ( (ret & PRESENT_2OU) == PRESENT_2OU ) {
      if ( fby3_common_get_2ou_board_type(slot_id, &type_2ou) < 0) {
        syslog(LOG_WARNING, "Failed to get slot%d 2ou board type\n",slot_id);
      } else if ( type_2ou == GPV3_MCHP_BOARD || type_2ou == GPV3_BRCM_BOARD ) {

        // 2OU Bridge-IC Version
        if (bic_get_fw_ver(slot_id, FW_2OU_BIC, ver)) {
          strcat(fw_text,"2OU_BIC_ver:\nNA\n");
        } else {
          sprintf(entry,"2OU_BIC_ver:\nv%x.%02x\n", ver[0], ver[1]);
          strcat(fw_text, entry);
        }

        // 2OU Bridge-IC Bootloader Version
        if (bic_get_fw_ver(slot_id, FW_2OU_BIC_BOOTLOADER, ver)) {
          strcat(fw_text,"2OU_BICbl_ver:\nNA\n");
        } else {
          sprintf(entry,"2OU_BICbl_ver:\nv%x.%02x\n", ver[0], ver[1]);
          strcat(fw_text, entry);
        }

        // FW_2OU_CPLD
        if (bic_get_fw_ver(slot_id, FW_2OU_CPLD, ver)) {
          strcat(fw_text,"2OU_CPLD_ver:\nNA\n");
        } else {
          sprintf(entry,"2OU_CPLD_ver:\n%02X%02X%02X%02X\n", ver[0], ver[1], ver[2], ver[3]);
          strcat(fw_text, entry);
        }

        // GPv3 PCIe Switch version
        if (bic_get_fw_ver(slot_id, FW_2OU_PESW_CFG_VER, ver)) {
          strcat(fw_text,"2OU_PESW_CFG_ver:\nNA\n");
        } else {
          sprintf(entry,"2OU_PESW_CFG_ver:\n%02X%02X%02X%02X\n", ver[0], ver[1], ver[2], ver[3]);
          strcat(fw_text, entry);
        }
      } else if ( type_2ou == CWC_MCHP_BOARD ) {

        // CWC BIC
        if (bic_get_fw_ver(slot_id, FW_CWC_BIC, ver)) {
          strcat(fw_text,"2U_BIC_ver:\nNA\n");
        } else {
          sprintf(entry,"2U_BIC_ver:\nv%x.%02x\n", ver[0], ver[1]);
          strcat(fw_text, entry);
        }

        // CWC BICBL
        if (bic_get_fw_ver(slot_id, FW_CWC_BIC_BL, ver)) {
          strcat(fw_text,"2U_BICbl_ver:\nNA\n");
        } else {
          sprintf(entry,"2U_BICbl_ver:\nv%x.%02x\n", ver[0], ver[1]);
          strcat(fw_text, entry);
        }

        // CWC CPLD
        if (bic_get_fw_ver(slot_id, FW_CWC_CPLD, ver)) {
          strcat(fw_text,"2U_CPLD_ver:\nNA\n");
        } else {
          sprintf(entry,"2U_CPLD_ver:\n%02X%02X%02X%02X\n", ver[0], ver[1], ver[2], ver[3]);
          strcat(fw_text, entry);
        }

        // CWC PESW
        if (bic_get_fw_ver(slot_id, FW_2OU_PESW_CFG_VER, ver)) {
          strcat(fw_text,"2U_PESW_CFG_ver:\nNA\n");
        } else {
          sprintf(entry,"2U_PESW_CFG_ver:\n%02X%02X%02X%02X\n", ver[0], ver[1], ver[2], ver[3]);
          strcat(fw_text, entry);
        }

        // TOP GPv3 BIC, BICBL, CPLD PESW version
        // TOP BIC
        if (bic_get_fw_ver(slot_id, FW_GPV3_TOP_BIC, ver)) {
          strcat(fw_text,"TOP_BIC_ver:\nNA\n");
        } else {
          sprintf(entry,"TOP_BIC_ver:\nv%x.%02x\n", ver[0], ver[1]);
          strcat(fw_text, entry);
        }

        // TOP BICBL
        if (bic_get_fw_ver(slot_id, FW_GPV3_TOP_BIC_BL, ver)) {
          strcat(fw_text,"TOP_BICbl_ver:\nNA\n");
        } else {
          sprintf(entry,"TOP_BICbl_ver:\nv%x.%02x\n", ver[0], ver[1]);
          strcat(fw_text, entry);
        }

        // TOP CPLD
        if (bic_get_fw_ver(slot_id, FW_GPV3_TOP_CPLD, ver)) {
          strcat(fw_text,"TOP_CPLD_ver:\nNA\n");
        } else {
          sprintf(entry,"TOP_CPLD_ver:\n%02X%02X%02X%02X\n", ver[0], ver[1], ver[2], ver[3]);
          strcat(fw_text, entry);
        }

        // TOP PESW
        if (bic_get_fw_ver(slot_id, FW_2U_TOP_PESW_CFG_VER, ver)) {
          strcat(fw_text,"TOP_PESW_CFG_ver:\nNA\n");
        } else {
          sprintf(entry,"TOP_PESW_CFG_ver:\n%02X%02X%02X%02X\n", ver[0], ver[1], ver[2], ver[3]);
          strcat(fw_text, entry);
        }

        // BOT GPv3 BIC, BICBL, CPLD PESW version
        // BOT BIC
        if (bic_get_fw_ver(slot_id, FW_GPV3_BOT_BIC, ver)) {
          strcat(fw_text,"BOT_BIC_ver:\nNA\n");
        } else {
          sprintf(entry,"BOT_BIC_ver:\nv%x.%02x\n", ver[0], ver[1]);
          strcat(fw_text, entry);
        }

        // BOT BICBL
        if (bic_get_fw_ver(slot_id, FW_GPV3_BOT_BIC_BL, ver)) {
          strcat(fw_text,"BOT_BICbl_ver:\nNA\n");
        } else {
          sprintf(entry,"BOT_BICbl_ver:\nv%x.%02x\n", ver[0], ver[1]);
          strcat(fw_text, entry);
        }

        // BOT CPLD
        if (bic_get_fw_ver(slot_id, FW_GPV3_BOT_CPLD, ver)) {
          strcat(fw_text,"BOT_CPLD_ver:\nNA\n");
        } else {
          sprintf(entry,"BOT_CPLD_ver:\n%02X%02X%02X%02X\n", ver[0], ver[1], ver[2], ver[3]);
          strcat(fw_text, entry);
        }

        // BOT PESW
        if (bic_get_fw_ver(slot_id, FW_2U_BOT_PESW_CFG_VER, ver)) {
          strcat(fw_text,"BOT_PESW_CFG_ver:\nNA\n");
        } else {
          sprintf(entry,"BOT_PESW_CFG_ver:\n%02X%02X%02X%02X\n", ver[0], ver[1], ver[2], ver[3]);
          strcat(fw_text, entry);
        }
      }
    }
  }

  return 0;
}

int plat_get_extra_sysinfo(uint8_t fru, char *info)
{
  int ret = 0;
  char fru_name[16];
  uint8_t index = 0;
  uint8_t bmc_location = 0;

  ret = fby3_common_get_bmc_location(&bmc_location);
  if (ret < 0) {
    syslog(LOG_WARNING, "Failed to get bmc locaton");
    return -1;
  }

  if(bmc_location == NIC_BMC) {
    if(bic_get_mb_index(&index) !=0)
      return -1;
    if((fru == FRU_SLOT1) && (index == FRU_SLOT3))
      fru = FRU_SLOT3;
  }

  if(fru == FRU_ALL) {
    fru = FRU_BMC;
  }

  if (!pal_get_fru_name( fru, fru_name)) {
    if(fru == FRU_BMC && bmc_location == NIC_BMC) {
      sprintf(info, "FRU:%s%d", fru_name,index);
    }
    else {
      sprintf(info, "FRU:%s", fru_name);
    }
  }
  return 0;
}
