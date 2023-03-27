/*
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

#ifndef __PAL_CRASHDUMP_AMD_H__
#define __PAL_CRASHDUMP_AMD_H__

#include <stdint.h>

#define log_system(cmd)                                                     \
do {                                                                        \
  int sysret = system(cmd);                                                 \
  if (sysret)                                                               \
    syslog(LOG_WARNING, "%s: system command failed, cmd: \"%s\",  ret: %d", \
            __func__, cmd, sysret);                                         \
} while(0)

#define CRASHDUMP_AMD_BIN       "/usr/local/bin/crashdump_amd.sh --event"      
#define MAX_CRASHDUMP_FILE_NAME_LENGTH 128
#define MAX_VAILD_LIST_LENGTH 128
#define AMDCRD_CMD_HEADER_LENGTH 4
#define AMDCRD_BANK_HEADER_LENGTH 4
#define AMDCRD_HEADER_LENGTH (AMDCRD_CMD_HEADER_LENGTH+AMDCRD_BANK_HEADER_LENGTH)
#define MCA_DECODED_LOG_PATH "/tmp/crashdump_slot%d_mca"
#define CRASHDUMP_PID_PATH "/var/run/autodump%d.pid"
#define CRASHDUMP_TIMESTAMP_FILE "fru%d_crashdump"

#define CPU_WDT_CCM_NUM 8
#define WDT_NBIO_NUM 4

#define TCDX_NUM 12
#define CAKE_NUM 6
#define IOM_NUM 4
#define CCIX_NUM 4
#define CS_NUM 8
#define WDT_NBIO_DATA_NUM 3

enum AMDCRD_DATA_TYPE{
  TYPE_MCA_BANK       = 0x01,
  TYPE_VIRTUAL_BANK   = 0x02,
  TYPE_CPU_WDT_BANK   = 0x03,
  TYPE_WDT_ADDR_BANK  = 0x04,
  TYPE_WDT_DATA_BANK  = 0x05,
  TYPE_TCDX_BANK      = 0x06,
  TYPE_CAKE_BANK      = 0x07,
  TYPE_PIE0_BANK      = 0x08,
  TYPE_IOM_BANK       = 0x09,
  TYPE_CCIX_BANK      = 0x0A,
  TYPE_CS_BANK        = 0x0B,
  TYPE_PCIE_AER_BANK  = 0x0C,
  TYPE_WDT_REG_BANK   = 0x0D,

  TYPE_CONTROL_PKT    = 0x80,
  TYPE_HEADER_BANK    = 0x81,
};

enum AMDCRD_CTRL_CMD {
  AMDCRD_CTRL_GET_STATE      = 0x01,
  AMDCRD_CTRL_DUMP_FINIDHED  = 0x02,
};

enum AMDCRD_CTRL_STATE {
  AMDCRD_CTRL_BMC_FREE       = 0x01,
  AMDCRD_CTRL_BMC_WAIT_DATA  = 0x02,
  AMDCRD_CTRL_BMC_PACK       = 0x03,
};

typedef struct {
  uint8_t bank_id;
  uint8_t core_id;
} __attribute__((packed)) amdcrd_bank_core_id_t;

typedef struct {
  uint8_t count;
  amdcrd_bank_core_id_t list[MAX_VAILD_LIST_LENGTH];
} __attribute__((packed)) amdcrd_mca_recv_list_t;

//==============================================================================
// Crashdump Command Header
//==============================================================================
typedef struct {
  uint8_t cmd_ver;
  uint8_t reserved[3];
} __attribute__((packed)) amdcrd_cmd_hdr_t;

//==============================================================================
// Crashdump Bank Header
//==============================================================================
typedef struct {
  uint8_t bank_type;
  uint8_t bank_fmt_ver;
  union { /* bank data */
    struct { /* mca_bank (type 1) */
      uint8_t bank_id;
      uint8_t core_id;
    } __attribute__((packed));

    struct { /* other type */
      uint8_t reserved[2];
    } __attribute__((packed));
  };
} __attribute__((packed)) amdcrd_bank_hdr_t;

//==============================================================================
// Crashdump Header
//==============================================================================
typedef struct {
  amdcrd_cmd_hdr_t cmd_hdr;
  amdcrd_bank_hdr_t bank_hdr;
} __attribute__((packed)) amdcrd_hdr_t;




//==============================================================================
// Type 0x01: MCA Bank
//==============================================================================
typedef struct {
  uint32_t mca_ctrl_lf;
  uint32_t mca_ctrl_hf;
  uint32_t mca_status_lf;
  uint32_t mca_status_hf;
  uint32_t mca_addr_lf;
  uint32_t mca_addr_hf;
  uint32_t mca_misc0_lf;
  uint32_t mca_misc0_hf;
  uint32_t mca_ctrl_mask_lf;
  uint32_t mca_ctrl_mask_hf;
  uint32_t mca_config_lf;
  uint32_t mca_config_hf;
  uint32_t mca_ipid_lf;
  uint32_t mca_ipid_hf;
  uint32_t mca_synd_lf;
  uint32_t mca_synd_hf;
  uint32_t mca_destat_lf;
  uint32_t mca_destat_hf;
  uint32_t mca_deaddr_lf;
  uint32_t mca_deaddr_hf;
  uint32_t mca_misc1_lf;
  uint32_t mca_misc1_hf;
} __attribute__((packed)) amdcrd_mca_bank_t;

//==============================================================================
// Type 0x02: Virtual/Global Bank
//==============================================================================
typedef struct {
  uint32_t bank_s5_reset_status;
  uint32_t bank_breakevent;
  uint16_t valid_mca_count;
  amdcrd_bank_core_id_t valid_mca_list[1];
} __attribute__((packed)) amdcrd_virtual_bank_t;

//==============================================================================
// Type 0x02 (ver 2): Virtual/Global Bank v2
//==============================================================================
typedef struct {
  uint32_t bank_s5_reset_status;
  uint32_t bank_breakevent;
  uint16_t valid_mca_count;

  // version 2 specific data
  uint16_t process_num;
  uint32_t apic_id;
  uint32_t eax;
  uint32_t ebx;
  uint32_t ecx;
  uint32_t edx;

  amdcrd_bank_core_id_t valid_mca_list[1];
} __attribute__((packed)) amdcrd_virtual_bank_v2_t;

//==============================================================================

//==============================================================================
// Type 0x03: CPU/Data Fabric Watchdog Timer bank
//==============================================================================
typedef struct {
  uint32_t hw_assert_sts_hi[CPU_WDT_CCM_NUM];
  uint32_t hw_assert_sts_low[CPU_WDT_CCM_NUM];
  uint32_t rspq_wdt_io_trans_log_hi[CPU_WDT_CCM_NUM];
  uint32_t rspq_wdt_io_trans_log_low[CPU_WDT_CCM_NUM];
  uint32_t hw_assert_msk_hi[CPU_WDT_CCM_NUM];
  uint32_t hw_assert_msk_low[CPU_WDT_CCM_NUM];
} __attribute__((packed)) amdcrd_cpu_wdt_bank_t;

//==============================================================================
// Type 0x04: SMU/PSP/PTDMA Watchdog Timers address bank
//------------------------------------------------------------------------------
// SHUB_MPX_LAST_XXREQ_LOG_ADDR [XX] [YY]
//------------------------------------------------------------------------------
// XX:  NBIO0 - NBIO3
// YY:  0-> ShubMp0WrTimeoutDetected
//      1-> ShubMp0RdTimeoutDetected
//      2-> ShubMp1WrTimeoutDetected
//      3-> ShubMp1RdTimeoutDetected
//==============================================================================
typedef struct {
  uint32_t addr[WDT_NBIO_NUM][4];
} __attribute__((packed)) amdcrd_wdt_addr_bank_t;

//==============================================================================
// Type 0x05: SMU/PSP/PTDMA Watchdog Timers data bank
//------------------------------------------------------------------------------
// SHUB_MPX_LAST_XXREQ_LOG_DATA [XX] [YY] [ZZ]
//------------------------------------------------------------------------------
// XX:  NBIO0 - NBIO3
// YY:  0-> ShubMp0WrTimeoutDetected
//      1-> ShubMp0RdTimeoutDetected
//      2-> ShubMp1WrTimeoutDetected
//      3-> ShubMp1RdTimeoutDetected
// ZZ:  Data0 - Data2
//==============================================================================
typedef struct {
  uint32_t data[WDT_NBIO_NUM][4][3];
} __attribute__((packed)) amdcrd_wdt_data_bank_t;

//==============================================================================
// Type 0x06: TCDX bank
//==============================================================================
typedef struct {
  uint32_t hw_assert_sts_hi[TCDX_NUM];
  uint32_t hw_assert_sts_low[TCDX_NUM];
  uint32_t hw_assert_msk_hi[TCDX_NUM];
  uint32_t hw_assert_msk_low[TCDX_NUM];
} __attribute__((packed)) amdcrd_tcdx_bank_t;

//==============================================================================
// Type 0x07: CAKE bank
//==============================================================================
typedef struct {
  uint32_t hw_assert_sts_hi[CAKE_NUM];
  uint32_t hw_assert_sts_low[CAKE_NUM];
  uint32_t hw_assert_msk_hi[CAKE_NUM];
  uint32_t hw_assert_msk_low[CAKE_NUM];
} __attribute__((packed)) amdcrd_cake_bank_t;

//==============================================================================
// Type 0x08: PIE0 bank
//==============================================================================
typedef struct {
  uint32_t hw_assert_sts_hi;
  uint32_t hw_assert_sts_low;
  uint32_t hw_assert_msk_hi;
  uint32_t hw_assert_msk_low;
} __attribute__((packed)) amdcrd_pie0_bank_t;

//==============================================================================
// Type 0x09: IOM bank
//==============================================================================
typedef struct {
  uint32_t hw_assert_sts_hi[IOM_NUM];
  uint32_t hw_assert_sts_low[IOM_NUM];
  uint32_t hw_assert_msk_hi[IOM_NUM];
  uint32_t hw_assert_msk_low[IOM_NUM];
} __attribute__((packed)) amdcrd_iom_bank_t;

//==============================================================================
// Type 0x0A: CCIX bank
//==============================================================================
typedef struct {
  uint32_t hw_assert_sts_hi[CCIX_NUM];
  uint32_t hw_assert_sts_low[CCIX_NUM];
  uint32_t hw_assert_msk_hi[CCIX_NUM];
  uint32_t hw_assert_msk_low[CCIX_NUM];
} __attribute__((packed)) amdcrd_ccix_bank_t;

//==============================================================================
// Type 0x0B: CS bank
//==============================================================================
typedef struct {
  uint32_t hw_assert_sts_hi[CS_NUM];
  uint32_t hw_assert_sts_low[CS_NUM];
  uint32_t hw_assert_msk_hi[CS_NUM];
  uint32_t hw_assert_msk_low[CS_NUM];
} __attribute__((packed)) amdcrd_cs_bank_t;

//==============================================================================
// Type 0x0C: PCIe AER bank
//==============================================================================
typedef struct {
  uint8_t bus;
  uint8_t dev;
  uint8_t fun;
  uint16_t cmd;
  uint16_t sts;
  uint16_t slot;
  uint8_t second_bus;
  uint16_t vendor_id;
  uint16_t dev_id;
  uint16_t class_code_lower; // Class Code 3 byte
  uint8_t class_code_upper;
  uint16_t second_sts;
  uint16_t ctrl;
  uint32_t uncorrectable_err_sts;
  uint32_t uncorrectable_err_msk;
  uint32_t uncorrectable_err_severity;
  uint32_t correctable_err_sts;
  uint32_t correctable_err_msk;
  uint32_t hdr_log_dw0;
  uint32_t hdr_log_dw1;
  uint32_t hdr_log_dw2;
  uint32_t hdr_log_dw3;
  uint32_t root_err_sts;
  uint16_t correctable_err_src_id;
  uint16_t err_src_id;
  uint32_t lane_err_sts;
} __attribute__((packed)) amdcrd_pcie_aer_bank_t;

//==============================================================================
// Type 0x0D: SMU/PSP/PTDMA Watchdog Timers regester bank
//==============================================================================
typedef struct {
  uint8_t nbio;
  char reg_name[32];
  uint32_t addr;
  uint8_t count;
  uint32_t reg_data[WDT_NBIO_DATA_NUM];
} __attribute__((packed)) amdcrd_wdt_reg_bank_t;

//==============================================================================
// Type 0x80: Control Packet
//==============================================================================
typedef struct {
  uint8_t cmd;
} __attribute__((packed)) amdcrd_ctrl_pkt_t;

//==============================================================================
// Type 0x81: Crashdump Header
//==============================================================================
typedef struct {
  uint64_t cpu_ppin;
  uint32_t ucode_ver;
  uint32_t pmio;
} __attribute__((packed)) amdcrd_header_bank_t;

uint8_t crashdump_initial(uint8_t slot);
uint8_t pal_amdcrd_save_mca_to_file(uint8_t slot, uint8_t* req_data, uint8_t req_len, uint8_t* res_data, uint8_t* res_len);

#endif /* __PAL_CRASHDUMP_AMD_H__ */
