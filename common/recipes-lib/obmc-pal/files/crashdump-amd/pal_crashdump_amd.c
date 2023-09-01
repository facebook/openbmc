/*
 *
 * Copyright 2015-present Facebook. All Rights Reserved.
 *
 * This file contains code to support IPMI2.0 Specification available @
 * http://www.intel.com/content/www/us/en/servers/ipmi/ipmi-specifications.html
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

#include <inttypes.h>
#include <stdio.h>
#include <syslog.h>
#include <sys/sysinfo.h>
#include <pthread.h>
#include <openbmc/kv.h>

#include "pal.h"
#include "pal_crashdump_amd.h"

static amdcrd_mca_recv_list_t g_recv_list[MAX_NODES] = {0};
static amdcrd_wdt_addr_bank_t g_wdt_addr[MAX_NODES] = {0};
static pthread_mutex_t g_amdcrd_state_lock[MAX_NODES] = {[0 ... (MAX_NODES-1)] = PTHREAD_MUTEX_INITIALIZER};
static uint8_t g_amdcrd_state[MAX_NODES] = {[0 ... (MAX_NODES-1)] = AMDCRD_CTRL_BMC_FREE};

typedef struct crashdump_data {
  uint8_t   sid;  // 0 based
  pthread_t tid;
} crashdump_data_t;
static crashdump_data_t g_crashdump_data[MAX_NODES];

static uint8_t amdcrd_mac_bank_handler (FILE* fp, const uint8_t idx, const amdcrd_bank_hdr_t* phdr, const amdcrd_mca_bank_t* pbank);
static uint8_t amdcrd_virtual_bank_handler (FILE* fp, const uint8_t idx, const amdcrd_bank_hdr_t* phdr, const amdcrd_virtual_bank_t* pbank);
static uint8_t amdcrd_virtual_bank_v2_handler (FILE* fp, const uint8_t idx, const amdcrd_bank_hdr_t* phdr, const amdcrd_virtual_bank_v2_t* pbank);
static uint8_t amdcrd_header_bank_handler (FILE* fp, const uint8_t idx, const amdcrd_bank_hdr_t* phdr, const amdcrd_header_bank_t* pbank);
static uint8_t amdcrd_cpu_wdt_bank_handler (FILE* fp, const uint8_t idx, const amdcrd_bank_hdr_t* phdr, const amdcrd_cpu_wdt_bank_t* pbank);
static uint8_t amdcrd_wdt_addr_bank_handler (FILE* fp, const uint8_t idx, const amdcrd_bank_hdr_t* phdr, const amdcrd_wdt_addr_bank_t* pbank);
static uint8_t amdcrd_wdt_data_bank_handler (FILE* fp, const uint8_t idx, const amdcrd_bank_hdr_t* phdr, const amdcrd_wdt_data_bank_t* pbank);
static uint8_t amdcrd_tcdx_bank_handler (FILE* fp, const uint8_t idx, const amdcrd_bank_hdr_t* phdr, const amdcrd_tcdx_bank_t* pbank);
static uint8_t amdcrd_cake_bank_handler (FILE* fp, const uint8_t idx, const amdcrd_bank_hdr_t* phdr, const amdcrd_cake_bank_t* pbank);
static uint8_t amdcrd_pie0_bank_handler (FILE* fp, const uint8_t idx, const amdcrd_bank_hdr_t* phdr, const amdcrd_pie0_bank_t* pbank);
static uint8_t amdcrd_iom_bank_handler (FILE* fp, const uint8_t idx, const amdcrd_bank_hdr_t* phdr, const amdcrd_iom_bank_t* pbank);
static uint8_t amdcrd_ccix_bank_handler (FILE* fp, const uint8_t idx, const amdcrd_bank_hdr_t* phdr, const amdcrd_ccix_bank_t* pbank);
static uint8_t amdcrd_cs_bank_handler (FILE* fp, const uint8_t idx, const amdcrd_bank_hdr_t* phdr, const amdcrd_cs_bank_t* pbank);
static uint8_t amdcrd_pcie_aer_bank_handler (FILE* fp, const uint8_t idx, const amdcrd_bank_hdr_t* phdr, const amdcrd_pcie_aer_bank_t* pbank);
static uint8_t amdcrd_wdt_reg_bank_handler(FILE* fp, const uint8_t idx, const amdcrd_bank_hdr_t* phdr, const amdcrd_wdt_reg_bank_t* pbank);
static uint8_t amdcrd_ctrl_pkt_handler (FILE* fp, const uint8_t idx, const amdcrd_bank_hdr_t* phdr, const amdcrd_ctrl_pkt_t* ppkt, uint8_t* res_data, uint8_t* res_len);
static void* generate_dump(void* arg);

enum {
  AMDCRD_SET_STATE_SUCCESS     = 0,
  AMDCRD_SET_STATE_FAIL        = -1,
  AMDCRD_SET_STATE_THREAD_ERR  = -2,
  AMDCRD_SET_STATE_UNKNOW_ERR  = -3,
};

static int amdcrd_set_state(const uint8_t idx, const uint8_t new_state) {
  // ------------------------------------------
  // state machine diagram
  // ------------------------------------------
  //
  //               |-----|
  //               |     V
  // FREE  ---->  WAIT_DATA  ---->  PACK -|
  //   ^                                  |
  //   |----------------------------------
  //

  int ret = AMDCRD_SET_STATE_SUCCESS;

  pthread_mutex_lock(&g_amdcrd_state_lock[idx]);

  switch (g_amdcrd_state[idx]) {
    case AMDCRD_CTRL_BMC_FREE:
      if (new_state == AMDCRD_CTRL_BMC_WAIT_DATA) {
        ret = AMDCRD_SET_STATE_SUCCESS;
      } else {
        ret = AMDCRD_SET_STATE_FAIL;
      }
      break;

    case AMDCRD_CTRL_BMC_WAIT_DATA:
      if (new_state == AMDCRD_CTRL_BMC_WAIT_DATA) {
        ret = AMDCRD_SET_STATE_SUCCESS;
      } else if (new_state == AMDCRD_CTRL_BMC_PACK) {
        // create a thread for packing
        g_crashdump_data[idx].sid = idx;
        if (pthread_create(&g_crashdump_data[idx].tid, NULL, generate_dump, (void*)&g_crashdump_data[idx])) {
          syslog(LOG_ERR, "%s(): create generate_dump threrad failed for slot%d", __func__, (idx+1));
          ret = AMDCRD_SET_STATE_THREAD_ERR;
        } else {
          ret = AMDCRD_SET_STATE_SUCCESS;
        }
      } else {
        ret = AMDCRD_SET_STATE_FAIL;
      }
      break;

    case AMDCRD_CTRL_BMC_PACK:
      if (new_state == AMDCRD_CTRL_BMC_FREE) {
        ret = AMDCRD_SET_STATE_SUCCESS;
      } else {
        ret = AMDCRD_SET_STATE_FAIL;
      }
      break;

    default:
      ret = AMDCRD_SET_STATE_UNKNOW_ERR;
      break;
  }

  if (ret == AMDCRD_SET_STATE_SUCCESS) {
    g_amdcrd_state[idx] = new_state;
  } else {
    syslog(LOG_ERR, "%s(): set state failed for slot%d, current_state: %u, new_state: %u, err: %d",
      __func__, (idx+1), g_amdcrd_state[idx], new_state, ret);
  }

  pthread_mutex_unlock(&g_amdcrd_state_lock[idx]);
  return ret;
}

static void* generate_dump(void* arg) {
  pthread_detach(pthread_self());
  crashdump_data_t* data = (crashdump_data_t*)arg;
  char cmd[128] = {0};

#ifdef CRASHDUMP_AMD_MB
  snprintf(cmd, 128, "%s mb", CRASHDUMP_AMD_BIN);
#else
  snprintf(cmd, 128, "%s slot%d", CRASHDUMP_AMD_BIN, (data->sid + 1));
#endif
  log_system(cmd);

  if (amdcrd_set_state(data->sid, AMDCRD_CTRL_BMC_FREE) != AMDCRD_SET_STATE_SUCCESS) {
    syslog(LOG_ERR, "%s(): Failed set state to free (slot%d)", __func__, (data->sid+1));
  }

  pthread_exit(NULL);
}

uint8_t crashdump_initial(uint8_t slot) {
  char fname[128];

  //check if crashdump is already running
  if (pal_is_crashdump_ongoing(slot)) {
    syslog(LOG_CRIT, "Another auto crashdump for slot%d is running.", slot);
    return CC_UNSPECIFIED_ERROR;
  }

  snprintf(fname, sizeof(fname), CRASHDUMP_PID_PATH, slot);
  FILE *fp;
  fp = fopen(fname,"w");
  if (!fp) {
    syslog(LOG_ERR, "%s(): file open failed, path: %s", __func__, fname);
    return CC_UNSPECIFIED_ERROR;
  }
  fclose(fp);

  //Set crashdump timestamp
  struct sysinfo info;
  char value[64];
  sysinfo(&info);
  snprintf(value, sizeof(value), "%ld", (info.uptime+1200));
  snprintf(fname, sizeof(fname), CRASHDUMP_TIMESTAMP_FILE, slot);
  kv_set(fname, value, 0, 0);

  return CC_SUCCESS;
}

uint8_t
pal_amdcrd_save_mca_to_file(uint8_t slot, uint8_t* req_data, uint8_t req_len, uint8_t* res_data, uint8_t* res_len) {

  uint8_t completion_code;
  FILE* fp;
  char file_path[MAX_CRASHDUMP_FILE_NAME_LENGTH] = "";
  amdcrd_hdr_t* phdr = (amdcrd_hdr_t*)req_data;
  uint8_t* data_ptr = req_data + sizeof(amdcrd_hdr_t);

  /* slot is 0 based, slot_id is 1 based */
  snprintf(file_path, MAX_CRASHDUMP_FILE_NAME_LENGTH, MCA_DECODED_LOG_PATH, slot + 1);
  fp = fopen(file_path, "a+");
  if (!fp)
    return CC_UNSPECIFIED_ERROR;

  syslog(LOG_INFO, "%s(): slot: %u, cmd_ver: %u, type: %u, fmt_ver: %u, data_len: %u",
    __func__, (slot+1), phdr->cmd_hdr.cmd_ver, phdr->bank_hdr.bank_type, phdr->bank_hdr.bank_fmt_ver, req_len);

  switch (phdr->bank_hdr.bank_type) {
    case TYPE_MCA_BANK:
      completion_code = amdcrd_mac_bank_handler(fp, slot, &phdr->bank_hdr, (amdcrd_mca_bank_t*)data_ptr);
      break;

    case TYPE_VIRTUAL_BANK:
      if (phdr->bank_hdr.bank_fmt_ver == 1) {
        completion_code = amdcrd_virtual_bank_handler(fp, slot, &phdr->bank_hdr, (amdcrd_virtual_bank_t*)data_ptr);
      } else if (phdr->bank_hdr.bank_fmt_ver == 2) {
        completion_code = amdcrd_virtual_bank_v2_handler(fp, slot, &phdr->bank_hdr, (amdcrd_virtual_bank_v2_t*)data_ptr);
      } else {
        completion_code = CC_INVALID_PARAM;
      }
      break;

    case TYPE_CPU_WDT_BANK:
      completion_code = amdcrd_cpu_wdt_bank_handler(fp, slot, &phdr->bank_hdr, (amdcrd_cpu_wdt_bank_t*)data_ptr);
      break;

    case TYPE_WDT_ADDR_BANK:
      completion_code = amdcrd_wdt_addr_bank_handler(fp, slot, &phdr->bank_hdr, (amdcrd_wdt_addr_bank_t*)data_ptr);
      break;

    case TYPE_WDT_DATA_BANK:
      completion_code = amdcrd_wdt_data_bank_handler(fp, slot, &phdr->bank_hdr, (amdcrd_wdt_data_bank_t*)data_ptr);
      break;

    case TYPE_TCDX_BANK:
      completion_code = amdcrd_tcdx_bank_handler(fp, slot, &phdr->bank_hdr, (amdcrd_tcdx_bank_t*)data_ptr);
      break;

    case TYPE_CAKE_BANK:
      completion_code = amdcrd_cake_bank_handler(fp, slot, &phdr->bank_hdr, (amdcrd_cake_bank_t*)data_ptr);
      break;

    case TYPE_PIE0_BANK:
      completion_code = amdcrd_pie0_bank_handler(fp, slot, &phdr->bank_hdr, (amdcrd_pie0_bank_t*)data_ptr);
      break;

    case TYPE_IOM_BANK:
      completion_code = amdcrd_iom_bank_handler(fp, slot, &phdr->bank_hdr, (amdcrd_iom_bank_t*)data_ptr);
      break;

    case TYPE_CCIX_BANK:
      completion_code = amdcrd_ccix_bank_handler(fp, slot, &phdr->bank_hdr, (amdcrd_ccix_bank_t*)data_ptr);
      break;

    case TYPE_CS_BANK:
      completion_code = amdcrd_cs_bank_handler(fp, slot, &phdr->bank_hdr, (amdcrd_cs_bank_t*)data_ptr);
      break;

    case TYPE_PCIE_AER_BANK:
      completion_code = amdcrd_pcie_aer_bank_handler(fp, slot, &phdr->bank_hdr, (amdcrd_pcie_aer_bank_t*)data_ptr);
      break;

    case TYPE_WDT_REG_BANK:
      completion_code = amdcrd_wdt_reg_bank_handler(fp, slot, &phdr->bank_hdr, (amdcrd_wdt_reg_bank_t*)data_ptr);
      break;

    case TYPE_CONTROL_PKT:
      completion_code = amdcrd_ctrl_pkt_handler(fp, slot, &phdr->bank_hdr, (amdcrd_ctrl_pkt_t*)data_ptr, res_data, res_len);
      break;

    case TYPE_HEADER_BANK:
      completion_code = amdcrd_header_bank_handler(fp, slot, &phdr->bank_hdr, (amdcrd_header_bank_t*)data_ptr);
      break;

    default:
      completion_code = CC_INVALID_PARAM;
      break;
  }
  fclose(fp);

  pthread_mutex_lock(&g_amdcrd_state_lock[slot]);
  syslog(LOG_INFO, "%s(): slot: %u, cc: 0x%02x, current state: 0x%02x", __func__, (slot+1), completion_code, g_amdcrd_state[slot]);
  pthread_mutex_unlock(&g_amdcrd_state_lock[slot]);
  return completion_code;
}


static uint8_t
amdcrd_mac_bank_handler (FILE* fp, const uint8_t idx, const amdcrd_bank_hdr_t* phdr, const amdcrd_mca_bank_t* pbank) {
  uint8_t completion_code = CC_SUCCESS;

  if (amdcrd_set_state(idx, AMDCRD_CTRL_BMC_WAIT_DATA) != AMDCRD_SET_STATE_SUCCESS) {
    completion_code = CC_NOT_SUPP_IN_CURR_STATE;
    goto out;
  }

  fprintf(fp, " %s : 0x%02X, %s : 0x%02X \n",
      "Bank ID", phdr->bank_id, "Core ID", phdr->core_id);
  fprintf(fp, " %-15s : 0x%08X_%08X \n",
      "MCA_CTRL", pbank->mca_ctrl_hf, pbank->mca_ctrl_lf);
  fprintf(fp, " %-15s : 0x%08X_%08X \n",
      "MCA_STATUS", pbank->mca_status_hf, pbank->mca_status_lf);
  fprintf(fp, " %-15s : 0x%08X_%08X \n",
      "MCA_ADDR", pbank->mca_addr_hf, pbank->mca_addr_lf);
  fprintf(fp,  " %-15s : 0x%08X_%08X \n",
      "MCA_MISC0", pbank->mca_misc0_hf, pbank->mca_misc0_lf);
  fprintf(fp, " %-15s : 0x%08X_%08X \n",
      "MCA_CTRL_MASK", pbank->mca_ctrl_mask_hf, pbank->mca_ctrl_mask_lf);
  fprintf(fp, " %-15s : 0x%08X_%08X \n",
      "MCA_CONFIG", pbank->mca_config_hf, pbank->mca_config_lf);
  fprintf(fp, " %-15s : 0x%08X_%08X \n",
      "MCA_IPID", pbank->mca_ipid_hf, pbank->mca_ipid_lf);
  fprintf(fp, " %-15s : 0x%08X_%08X \n",
      "MCA_SYND", pbank->mca_synd_hf, pbank->mca_synd_lf);
  fprintf(fp, " %-15s : 0x%08X_%08X \n",
      "MCA_DESTAT", pbank->mca_destat_hf, pbank->mca_destat_lf);
  fprintf(
      fp, " %-15s : 0x%08X_%08X \n",
      "MCA_DEADDR", pbank->mca_deaddr_hf, pbank->mca_deaddr_lf);
  fprintf(
      fp, " %-15s : 0x%08X_%08X \n",
      "MCA_MISC1", pbank->mca_misc1_hf, pbank->mca_misc1_lf);
  fprintf(fp, "\n");

  if (g_recv_list[idx].count < MAX_VAILD_LIST_LENGTH) {
    g_recv_list[idx].list[g_recv_list[idx].count].bank_id = phdr->bank_id;
    g_recv_list[idx].list[g_recv_list[idx].count].core_id = phdr->core_id;
    g_recv_list[idx].count++;
  } else {
    syslog(LOG_INFO, "%s(): mca recv count exceed %u", __func__, MAX_VAILD_LIST_LENGTH);
  }

out:
  return completion_code;
}

static uint8_t
amdcrd_virtual_bank_handler (FILE* fp, const uint8_t idx, const amdcrd_bank_hdr_t* phdr, const amdcrd_virtual_bank_t* pbank) {
  int ret;
  size_t i;
  uint8_t completion_code = CC_SUCCESS;

  if (amdcrd_set_state(idx, AMDCRD_CTRL_BMC_WAIT_DATA) != AMDCRD_SET_STATE_SUCCESS) {
    completion_code = CC_NOT_SUPP_IN_CURR_STATE;
    goto out;
  }

  fprintf(fp, " %s : \n", "Virtual Bank (ver 1)");
  fprintf(fp, " %-15s : 0x%08X \n",
      "S5_RESET_STATUS", pbank->bank_s5_reset_status);
  fprintf(fp, " %-15s : 0x%08X \n", "BREAKEVENT", pbank->bank_breakevent);
  fprintf(fp, " %-15s : ", "VAILD LIST");
  for (i = 0; i < pbank->valid_mca_count; i++) {
    fprintf(fp, "(0x%02x,0x%02x) ",
        pbank->valid_mca_list[i].bank_id, pbank->valid_mca_list[i].core_id);
  }
  fprintf(fp, "\n");
  fprintf(fp, " %-15s : ", "RECEIVE LIST");
  for (i = 0; i < g_recv_list[idx].count; i++) {
    fprintf(fp, "(0x%02x,0x%02x) ",
        g_recv_list[idx].list[i].bank_id, g_recv_list[idx].list[i].core_id);
  }
  fprintf(fp, "\n");
  memset(&g_recv_list[idx], 0, sizeof(amdcrd_mca_recv_list_t));

  ret = amdcrd_set_state(idx, AMDCRD_CTRL_BMC_PACK);
  if (ret == AMDCRD_SET_STATE_THREAD_ERR) {
    completion_code = CC_UNSPECIFIED_ERROR;
  } else if (ret == AMDCRD_SET_STATE_FAIL) {
    // basically, this code should never be run
    completion_code = CC_NOT_SUPP_IN_CURR_STATE;
  }

out:
  return completion_code;
}

static uint8_t
amdcrd_virtual_bank_v2_handler (FILE* fp, const uint8_t idx, const amdcrd_bank_hdr_t* phdr, const amdcrd_virtual_bank_v2_t* pbank) {
  size_t i;
  uint8_t completion_code = CC_SUCCESS;

  if (amdcrd_set_state(idx, AMDCRD_CTRL_BMC_WAIT_DATA) != AMDCRD_SET_STATE_SUCCESS) {
    completion_code = CC_NOT_SUPP_IN_CURR_STATE;
    goto out;
  }

  fprintf(fp, " %s : \n", "Virtual Bank (ver 2)");
  fprintf(fp, " %-15s : 0x%08X \n",
      "S5_RESET_STATUS", pbank->bank_s5_reset_status);
  fprintf(fp, " %-15s : 0x%08X \n", "BREAKEVENT", pbank->bank_breakevent);

  fprintf(fp, " %-15s : 0x%08X \n", "PROCESSOR NUMER", pbank->process_num);
  fprintf(fp, " %-15s : 0x%08X \n", "APIC ID", pbank->apic_id);
  fprintf(fp, " %-15s : 0x%08X \n", "EAX", pbank->eax);
  fprintf(fp, " %-15s : 0x%08X \n", "EBX", pbank->ebx);
  fprintf(fp, " %-15s : 0x%08X \n", "ECX", pbank->ecx);
  fprintf(fp, " %-15s : 0x%08X \n", "EDX", pbank->edx);

  fprintf(fp, " %-15s : ", "VAILD LIST");
  for (i = 0; i < pbank->valid_mca_count; i++) {
    fprintf(fp, "(0x%02x,0x%02x) ",
        pbank->valid_mca_list[i].bank_id, pbank->valid_mca_list[i].core_id);
  }
  fprintf(fp, "\n");
  fprintf(fp, " %-15s : ", "RECEIVE LIST");
  for (i = 0; i < g_recv_list[idx].count; i++) {
    fprintf(fp, "(0x%02x,0x%02x) ",
        g_recv_list[idx].list[i].bank_id, g_recv_list[idx].list[i].core_id);
  }
  fprintf(fp, "\n");
  memset(&g_recv_list[idx], 0, sizeof(amdcrd_mca_recv_list_t));

out:
  return completion_code;
}

static uint8_t
amdcrd_header_bank_handler (FILE* fp, const uint8_t idx, const amdcrd_bank_hdr_t* phdr, const amdcrd_header_bank_t* pbank) {
  uint8_t completion_code = CC_SUCCESS;

  if (amdcrd_set_state(idx, AMDCRD_CTRL_BMC_WAIT_DATA) != AMDCRD_SET_STATE_SUCCESS) {
    completion_code = CC_NOT_SUPP_IN_CURR_STATE;
    goto out;
  }

  fprintf(fp, " %s : \n", "Crashdump Header");
  fprintf(fp, " %-15s : 0x%016"PRIx64" \n", "CPU PPIN", pbank->cpu_ppin);
  fprintf(fp, " %-15s : 0x%08X \n", "UCODE VERSION", pbank->ucode_ver);
  fprintf(fp, " %-15s : 0x%08X \n", "PMIO 80h", pbank->pmio);
  fprintf(fp, "    BIT0 - SMN Parity/SMN Timeouts PSP/SMU Parity and ECC/SMN On-Package Link Error : %u \n", (pbank->pmio & 0x1));
  fprintf(fp, "    BIT2 - PSP Parity and ECC : %d \n", ((pbank->pmio & 0x4)>>2));
  fprintf(fp, "    BIT3 - SMN Timeouts SMU : %d \n", ((pbank->pmio & 0x8)>>3));
  fprintf(fp, "    BIT4 - SMN Off-Package Link Packet Error : %d \n", ((pbank->pmio & 0x10)>>4));
  fprintf(fp, "\n");
  memset(&g_recv_list[idx], 0, sizeof(amdcrd_mca_recv_list_t));

out:
  return completion_code;
}

static uint8_t
amdcrd_cpu_wdt_bank_handler (FILE* fp, const uint8_t idx, const amdcrd_bank_hdr_t* phdr, const amdcrd_cpu_wdt_bank_t* pbank) {
  uint8_t i;
  uint8_t completion_code = CC_SUCCESS;

  if (amdcrd_set_state(idx, AMDCRD_CTRL_BMC_WAIT_DATA) != AMDCRD_SET_STATE_SUCCESS) {
    completion_code = CC_NOT_SUPP_IN_CURR_STATE;
    goto out;
  }

  for (i = 0; i < CPU_WDT_CCM_NUM; i++) {
    fprintf(fp, "  [CCM%u]\n", i);
    fprintf(fp, "    %-20s : 0x%08X \n", "HwAssertStsHi", pbank->hw_assert_sts_hi[i]);
    fprintf(fp, "    %-20s : 0x%08X \n", "HwAssertStsLow", pbank->hw_assert_sts_low[i]);
    fprintf(fp, "    %-20s : 0x%08X \n", "RSPQWDTIoTransLogHi", pbank->rspq_wdt_io_trans_log_hi[i]);
    fprintf(fp, "    %-20s : 0x%08X \n", "RSPQWDTIoTransLogLow", pbank->rspq_wdt_io_trans_log_low[i]);
    if (phdr->bank_fmt_ver == 2) {
      fprintf(fp, "    %-20s : 0x%08X \n", "HwAssertMskHi", pbank->hw_assert_msk_hi[i]);
      fprintf(fp, "    %-20s : 0x%08X \n", "HwAssertMskLow", pbank->hw_assert_sts_low[i]);
    }
  }
  fprintf(fp, "\n");

out:
  return completion_code;
}

static uint8_t
amdcrd_wdt_addr_bank_handler (FILE* fp, const uint8_t idx, const amdcrd_bank_hdr_t* phdr, const amdcrd_wdt_addr_bank_t* pbank) {
  uint8_t completion_code = CC_SUCCESS;

  if (amdcrd_set_state(idx, AMDCRD_CTRL_BMC_WAIT_DATA) != 0) {
    completion_code = CC_NOT_SUPP_IN_CURR_STATE;
    goto out;
  }

  memcpy(&g_wdt_addr[idx], pbank, sizeof(amdcrd_wdt_addr_bank_t));

out:
  return completion_code;
}

static uint8_t
amdcrd_wdt_data_bank_handler (FILE* fp, const uint8_t idx, const amdcrd_bank_hdr_t* phdr, const amdcrd_wdt_data_bank_t* pbank) {
  uint8_t i;
  uint8_t completion_code = CC_SUCCESS;

  if (amdcrd_set_state(idx, AMDCRD_CTRL_BMC_WAIT_DATA) != AMDCRD_SET_STATE_SUCCESS) {
    completion_code = CC_NOT_SUPP_IN_CURR_STATE;
    goto out;
  }

  fprintf(fp, " %s : \n", "SMU/PSP/PTDMA Watchdog Timers data bank");
  for (i = 0; i < WDT_NBIO_NUM; i++) {
    fprintf(fp, "  [NBIO%u]\n", i);
    fprintf(fp, "    %s\n", "ShubMp0WrTimeoutDetected");
    fprintf(fp, "      SHUB_MPX_LAST_XXREQ_LOG_ADDR : 0x%08X \n", g_wdt_addr[idx].addr[i][0]);
    fprintf(fp, "      SHUB_MPX_LAST_XXREQ_LOG_DATA0: 0x%08X \n", pbank->data[i][0][0]);
    fprintf(fp, "      SHUB_MPX_LAST_XXREQ_LOG_DATA1: 0x%08X \n", pbank->data[i][0][1]);
    fprintf(fp, "      SHUB_MPX_LAST_XXREQ_LOG_DATA2: 0x%08X \n", pbank->data[i][0][2]);
    fprintf(fp, "    %s\n", "ShubMp0RdTimeoutDetected");
    fprintf(fp, "      SHUB_MPX_LAST_XXREQ_LOG_ADDR : 0x%08X \n", g_wdt_addr[idx].addr[i][1]);
    fprintf(fp, "      SHUB_MPX_LAST_XXREQ_LOG_DATA0: 0x%08X \n", pbank->data[i][1][0]);
    fprintf(fp, "      SHUB_MPX_LAST_XXREQ_LOG_DATA1: 0x%08X \n", pbank->data[i][1][1]);
    fprintf(fp, "      SHUB_MPX_LAST_XXREQ_LOG_DATA2: 0x%08X \n", pbank->data[i][1][2]);
    fprintf(fp, "    %s\n", "ShubMp1WrTimeoutDetected");
    fprintf(fp, "      SHUB_MPX_LAST_XXREQ_LOG_ADDR : 0x%08X \n", g_wdt_addr[idx].addr[i][2]);
    fprintf(fp, "      SHUB_MPX_LAST_XXREQ_LOG_DATA0: 0x%08X \n", pbank->data[i][2][0]);
    fprintf(fp, "      SHUB_MPX_LAST_XXREQ_LOG_DATA1: 0x%08X \n", pbank->data[i][2][1]);
    fprintf(fp, "      SHUB_MPX_LAST_XXREQ_LOG_DATA2: 0x%08X \n", pbank->data[i][2][2]);
    fprintf(fp, "    %s\n", "ShubMp1RdTimeoutDetected");
    fprintf(fp, "      SHUB_MPX_LAST_XXREQ_LOG_ADDR : 0x%08X \n", g_wdt_addr[idx].addr[i][3]);
    fprintf(fp, "      SHUB_MPX_LAST_XXREQ_LOG_DATA0: 0x%08X \n", pbank->data[i][3][0]);
    fprintf(fp, "      SHUB_MPX_LAST_XXREQ_LOG_DATA1: 0x%08X \n", pbank->data[i][3][1]);
    fprintf(fp, "      SHUB_MPX_LAST_XXREQ_LOG_DATA2: 0x%08X \n", pbank->data[i][3][2]);
  }
  fprintf(fp, "\n");
  memset(&g_wdt_addr[idx], 0, sizeof(amdcrd_wdt_addr_bank_t));

out:
  return completion_code;
}

static uint8_t
amdcrd_wdt_reg_bank_handler(FILE* fp, const uint8_t idx, const amdcrd_bank_hdr_t* phdr, const amdcrd_wdt_reg_bank_t* pbank) {
  uint8_t i;
  uint8_t completion_code = CC_SUCCESS;

  if (amdcrd_set_state(idx, AMDCRD_CTRL_BMC_WAIT_DATA) != AMDCRD_SET_STATE_SUCCESS) {
    completion_code = CC_NOT_SUPP_IN_CURR_STATE;
    goto out;
  }

  fprintf(fp, "  [NBIO%u] %s\n", pbank->nbio,pbank->reg_name);
  fprintf(fp, "    Address:0x%08X \n", pbank->addr);
  fprintf(fp, "    Data count:%d \n", pbank->count);
  fprintf(fp, "    Data:\n");
  for (i=0;i<pbank->count;i++) {
    fprintf(fp, "      %d:0x%08X\n", i, pbank->reg_data[i]);
  }
  fprintf(fp, "\n");

  out:
    return completion_code;
  }

static uint8_t
amdcrd_tcdx_bank_handler (FILE* fp, const uint8_t idx, const amdcrd_bank_hdr_t* phdr, const amdcrd_tcdx_bank_t* pbank) {
  uint8_t i;
  uint8_t completion_code = CC_SUCCESS;

  if (amdcrd_set_state(idx, AMDCRD_CTRL_BMC_WAIT_DATA) != AMDCRD_SET_STATE_SUCCESS) {
    completion_code = CC_NOT_SUPP_IN_CURR_STATE;
    goto out;
  }

  for (i = 0; i < TCDX_NUM; i++) {
    fprintf(fp, "  [TCDX%u]\n", i);
    fprintf(fp, "    %-20s : 0x%08X \n", "HwAssertStsHi", pbank->hw_assert_sts_hi[i]);
    fprintf(fp, "    %-20s : 0x%08X \n", "HwAssertStsLow", pbank->hw_assert_sts_low[i]);
    fprintf(fp, "    %-20s : 0x%08X \n", "HwAssertMskHi", pbank->hw_assert_msk_hi[i]);
    fprintf(fp, "    %-20s : 0x%08X \n", "HwAssertMskLow", pbank->hw_assert_sts_low[i]);
  }
  fprintf(fp, "\n");

out:
  return completion_code;
}

static uint8_t
amdcrd_cake_bank_handler (FILE* fp, const uint8_t idx, const amdcrd_bank_hdr_t* phdr, const amdcrd_cake_bank_t* pbank) {
  uint8_t i;
  uint8_t completion_code = CC_SUCCESS;

  if (amdcrd_set_state(idx, AMDCRD_CTRL_BMC_WAIT_DATA) != AMDCRD_SET_STATE_SUCCESS) {
    completion_code = CC_NOT_SUPP_IN_CURR_STATE;
    goto out;
  }

  for (i = 0; i < CAKE_NUM; i++) {
    fprintf(fp, "  [CAKE%u]\n", i);
    fprintf(fp, "    %-20s : 0x%08X \n", "HwAssertStsHi", pbank->hw_assert_sts_hi[i]);
    fprintf(fp, "    %-20s : 0x%08X \n", "HwAssertStsLow", pbank->hw_assert_sts_low[i]);
    fprintf(fp, "    %-20s : 0x%08X \n", "HwAssertMskHi", pbank->hw_assert_msk_hi[i]);
    fprintf(fp, "    %-20s : 0x%08X \n", "HwAssertMskLow", pbank->hw_assert_sts_low[i]);
  }
  fprintf(fp, "\n");

out:
  return completion_code;
}

static uint8_t
amdcrd_pie0_bank_handler (FILE* fp, const uint8_t idx, const amdcrd_bank_hdr_t* phdr, const amdcrd_pie0_bank_t* pbank) {
  uint8_t completion_code = CC_SUCCESS;

  if (amdcrd_set_state(idx, AMDCRD_CTRL_BMC_WAIT_DATA) != AMDCRD_SET_STATE_SUCCESS) {
    completion_code = CC_NOT_SUPP_IN_CURR_STATE;
    goto out;
  }

  fprintf(fp, "  [PIE0]\n");
  fprintf(fp, "    %-20s : 0x%08X \n", "HwAssertStsHi", pbank->hw_assert_sts_hi);
  fprintf(fp, "    %-20s : 0x%08X \n", "HwAssertStsLow", pbank->hw_assert_sts_low);
  fprintf(fp, "    %-20s : 0x%08X \n", "HwAssertMskHi", pbank->hw_assert_msk_hi);
  fprintf(fp, "    %-20s : 0x%08X \n", "HwAssertMskLow", pbank->hw_assert_sts_low);
  fprintf(fp, "\n");

out:
  return completion_code;
}

static uint8_t
amdcrd_iom_bank_handler (FILE* fp, const uint8_t idx, const amdcrd_bank_hdr_t* phdr, const amdcrd_iom_bank_t* pbank) {
  uint8_t i;
  uint8_t completion_code = CC_SUCCESS;

  if (amdcrd_set_state(idx, AMDCRD_CTRL_BMC_WAIT_DATA) != AMDCRD_SET_STATE_SUCCESS) {
    completion_code = CC_NOT_SUPP_IN_CURR_STATE;
    goto out;
  }

  for (i = 0; i < IOM_NUM; i++) {
    fprintf(fp, "  [IOM%u]\n", i);
    fprintf(fp, "    %-20s : 0x%08X \n", "HwAssertStsHi", pbank->hw_assert_sts_hi[i]);
    fprintf(fp, "    %-20s : 0x%08X \n", "HwAssertStsLow", pbank->hw_assert_sts_low[i]);
    fprintf(fp, "    %-20s : 0x%08X \n", "HwAssertMskHi", pbank->hw_assert_msk_hi[i]);
    fprintf(fp, "    %-20s : 0x%08X \n", "HwAssertMskLow", pbank->hw_assert_sts_low[i]);
  }
  fprintf(fp, "\n");

out:
  return completion_code;
}

static uint8_t
amdcrd_ccix_bank_handler (FILE* fp, const uint8_t idx, const amdcrd_bank_hdr_t* phdr, const amdcrd_ccix_bank_t* pbank) {
  uint8_t i;
  uint8_t completion_code = CC_SUCCESS;

  if (amdcrd_set_state(idx, AMDCRD_CTRL_BMC_WAIT_DATA) != AMDCRD_SET_STATE_SUCCESS) {
    completion_code = CC_NOT_SUPP_IN_CURR_STATE;
    goto out;
  }

  for (i = 0; i < CCIX_NUM; i++) {
    fprintf(fp, "  [CCIX%u]\n", i);
    fprintf(fp, "    %-20s : 0x%08X \n", "HwAssertStsHi", pbank->hw_assert_sts_hi[i]);
    fprintf(fp, "    %-20s : 0x%08X \n", "HwAssertStsLow", pbank->hw_assert_sts_low[i]);
    fprintf(fp, "    %-20s : 0x%08X \n", "HwAssertMskHi", pbank->hw_assert_msk_hi[i]);
    fprintf(fp, "    %-20s : 0x%08X \n", "HwAssertMskLow", pbank->hw_assert_sts_low[i]);
  }
  fprintf(fp, "\n");

out:
  return completion_code;
}

static uint8_t
amdcrd_cs_bank_handler (FILE* fp, const uint8_t idx, const amdcrd_bank_hdr_t* phdr, const amdcrd_cs_bank_t* pbank) {
  uint8_t i;
  uint8_t completion_code = CC_SUCCESS;

  if (amdcrd_set_state(idx, AMDCRD_CTRL_BMC_WAIT_DATA) != AMDCRD_SET_STATE_SUCCESS) {
    completion_code = CC_NOT_SUPP_IN_CURR_STATE;
    goto out;
  }

  for (i = 0; i < CS_NUM; i++) {
    fprintf(fp, "  [CS%u]\n", i);
    fprintf(fp, "    %-20s : 0x%08X \n", "HwAssertStsHi", pbank->hw_assert_sts_hi[i]);
    fprintf(fp, "    %-20s : 0x%08X \n", "HwAssertStsLow", pbank->hw_assert_sts_low[i]);
    fprintf(fp, "    %-20s : 0x%08X \n", "HwAssertMskHi", pbank->hw_assert_msk_hi[i]);
    fprintf(fp, "    %-20s : 0x%08X \n", "HwAssertMskLow", pbank->hw_assert_sts_low[i]);
  }
  fprintf(fp, "\n");

out:
  return completion_code;
}

static uint8_t
amdcrd_pcie_aer_bank_handler (FILE* fp, const uint8_t idx, const amdcrd_bank_hdr_t* phdr, const amdcrd_pcie_aer_bank_t* pbank) {
  uint8_t completion_code = CC_SUCCESS;

  if (amdcrd_set_state(idx, AMDCRD_CTRL_BMC_WAIT_DATA) != AMDCRD_SET_STATE_SUCCESS) {
    completion_code = CC_NOT_SUPP_IN_CURR_STATE;
    goto out;
  }

  fprintf(fp, "  [Bus%u Dev%u Fun%u]\n", pbank->bus, pbank->dev,pbank->fun);
  fprintf(fp, "    Command                      : 0x%04X \n", pbank->cmd);
  fprintf(fp, "    Status                       : 0x%04X \n", pbank->sts);
  fprintf(fp, "    Slot                         : 0x%04X \n", pbank->slot);
  fprintf(fp, "    Secondary Bus                : 0x%02X \n", pbank->second_bus);
  fprintf(fp, "    Vendor ID                    : 0x%04X \n", pbank->vendor_id);
  fprintf(fp, "    Device ID                    : 0x%04X \n", pbank->dev_id);
  fprintf(fp, "    Class Code                   : 0x%02X%04X \n", pbank->class_code_upper,pbank->class_code_lower);
  fprintf(fp, "    Bridge: Secondary Status     : 0x%04X \n", pbank->second_sts);
  fprintf(fp, "    Bridge: Control              : 0x%04X \n", pbank->ctrl);
  fprintf(fp, "    Uncorrectable Error Status   : 0x%08X \n", pbank->uncorrectable_err_sts);
  fprintf(fp, "    Uncorrectable Error Mask     : 0x%08X \n", pbank->uncorrectable_err_msk);
  fprintf(fp, "    Uncorrectable Error Severity : 0x%08X \n", pbank->uncorrectable_err_severity);
  fprintf(fp, "    Correctable Error Status     : 0x%08X \n", pbank->correctable_err_sts);
  fprintf(fp, "    Correctable Error Mask       : 0x%08X \n", pbank->correctable_err_msk);
  fprintf(fp, "    Header Log DW0               : 0x%08X \n", pbank->hdr_log_dw0);
  fprintf(fp, "    Header Log DW1               : 0x%08X \n", pbank->hdr_log_dw1);
  fprintf(fp, "    Header Log DW2               : 0x%08X \n", pbank->hdr_log_dw2);
  fprintf(fp, "    Header Log DW3               : 0x%08X \n", pbank->hdr_log_dw3);
  fprintf(fp, "    Root Error Status            : 0x%08X \n", pbank->root_err_sts);
  fprintf(fp, "    Correctable Error Source ID  : 0x%04X \n", pbank->correctable_err_src_id);
  fprintf(fp, "    Error Source ID              : 0x%04X \n", pbank->err_src_id);
  fprintf(fp, "    Lane Error Status            : 0x%08X \n", pbank->lane_err_sts);
  fprintf(fp, "\n");

out:
  return completion_code;
}


static uint8_t
amdcrd_ctrl_pkt_handler (FILE* fp, const uint8_t idx, const amdcrd_bank_hdr_t* phdr, const amdcrd_ctrl_pkt_t* ppkt, uint8_t* res_data, uint8_t* res_len) {
  int ret;
  uint8_t completion_code = CC_SUCCESS;

  switch (ppkt->cmd) {
    case AMDCRD_CTRL_GET_STATE:
      pthread_mutex_lock(&g_amdcrd_state_lock[idx]);
      res_data[0] = g_amdcrd_state[idx];
      *res_len = 1;
      pthread_mutex_unlock(&g_amdcrd_state_lock[idx]);
      break;

    case AMDCRD_CTRL_DUMP_FINIDHED:
      ret = amdcrd_set_state(idx, AMDCRD_CTRL_BMC_PACK);
      if (ret == AMDCRD_SET_STATE_THREAD_ERR) {
        completion_code = CC_UNSPECIFIED_ERROR;
      } else if (ret == AMDCRD_SET_STATE_FAIL) {
        completion_code = CC_NOT_SUPP_IN_CURR_STATE;
      }
      break;

    default:
      return CC_INVALID_PARAM;
  }
  return completion_code;
}
