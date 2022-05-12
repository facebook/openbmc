/*
 *
 * Copyright 2022-present Facebook. All Rights Reserved.
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
#include <stdint.h>
#include <string.h>
#include <facebook/bic_xfer.h>
#include <facebook/bic_ipmi.h>
#include "dimm-util.h"
#include "dimm-util-plat.h"

#ifdef DEBUG_DIMM_UTIL
  #define DBG_PRINT(fmt, args...) printf(fmt, ##args)
#else
  #define DBG_PRINT(fmt, args...)
#endif

static uint8_t dev_addr[MAX_DIMM_NUM_FBY35/2] = {
  DIMM0_SPD_ADDR,
  DIMM1_SPD_ADDR,
  DIMM2_SPD_ADDR,
};

// dimm location constant strings, matching silk screen
static char const *dimm_label[NUM_CPU_FBY35][MAX_DIMM_NUM_FBY35] = {
  { "A0", "A2", "A3", "A4", "A6", "A7" },
};

static uint8_t dimm_cache_id[NUM_CPU_FBY35][MAX_DIMM_NUM_FBY35] = {
  { 0, 4, 6, 8, 12, 14 },
};

static char const *fru_name_fby35[NUM_FRU_FBY35] = {
  "slot1",
  "slot2",
  "slot3",
  "slot4",
  "all",
};

int
plat_init() {
  num_frus = NUM_FRU_FBY35;
  num_dimms_per_cpu = MAX_DIMM_NUM_FBY35;
  num_cpus          = NUM_CPU_FBY35;
  total_dimms       = num_dimms_per_cpu * num_cpus;
  fru_name   = fru_name_fby35;
  fru_id_min = FRU_ID_MIN_FBY35;
  fru_id_max = FRU_ID_MAX_FBY35;
  fru_id_all = FRU_ID_ALL_FBY35;
  read_block = true;

  return 0;
}

const char *
get_dimm_label(uint8_t cpu, uint8_t dimm) {
  if ((cpu >= NUM_CPU_FBY35)  || (dimm >= MAX_DIMM_NUM_FBY35)) {
    return "N/A";
  }

  return dimm_label[cpu][dimm];
}

uint8_t
get_dimm_cache_id(uint8_t cpu, uint8_t dimm) {
  if ((cpu >= NUM_CPU_FBY35)  || (dimm >= MAX_DIMM_NUM_FBY35)) {
    return 0xff;
  }

  return dimm_cache_id[cpu][dimm];
}

int
util_read_spd(uint8_t slot_id, uint8_t cpu, uint8_t dimm, uint16_t offset, uint8_t len, uint8_t *rxbuf) {
#define MIN_RESP_LEN (1 /* ME CC */ + INTEL_ID_LEN)

  int ret;
  uint8_t tbuf[64] = {0};
  uint8_t rbuf[64] = {0};
  uint8_t tlen = 14;
  uint8_t rlen = sizeof(rbuf);
  uint8_t bus_id = 0;
  uint8_t addr = 0;

  // SPR CPU supports 2 SPD buses
  if (dimm >= (MAX_DIMM_NUM_FBY35/2)) {
    bus_id = 1;
  }
  addr = dev_addr[dimm % (MAX_DIMM_NUM_FBY35/2)];

  tbuf[0] = NETFN_NM_REQ << 2;
  tbuf[1] = CMD_NM_READ_MEM_SM_BUS;
  tbuf[2] = MANU_INTEL_0;
  tbuf[3] = MANU_INTEL_1;
  tbuf[4] = MANU_INTEL_2;
  tbuf[5] = cpu;
  tbuf[6] = bus_id;
  tbuf[7] = addr;
  tbuf[8] = 2;  // offset length
  tbuf[9] = (len > 0) ? len - 1 : 0;
  tbuf[10] = 0x80 | (offset & 0x7F);
  tbuf[11] = (offset & 0x780) >> 7;
  tbuf[12] = 0;
  tbuf[13] = 0;

  ret = bic_me_xmit(slot_id, tbuf, tlen, rbuf, &rlen);
  if (ret) {
    DBG_PRINT("ME no response!\n");
    return -1;
  }

  if (rlen != (MIN_RESP_LEN + len)) {
    DBG_PRINT("return incomplete len=%d\n", rlen);
    return -1;
  }
  if (rbuf[0] != CC_SUCCESS) {
    DBG_PRINT("Completion Code: %02X\n", rbuf[0]);
    return -1;
  }

  memcpy(rxbuf, &rbuf[4], len);
  return len;
}

int
util_check_me_status(uint8_t slot_id) {
  int ret;
  uint8_t tbuf[64] = {0x00};
  uint8_t rbuf[64] = {0x00};
  uint8_t tlen = 2;
  uint8_t rlen = sizeof(rbuf);

  tbuf[0] = NETFN_APP_REQ << 2;
  tbuf[1] = CMD_APP_GET_SELFTEST_RESULTS;
  ret = bic_me_xmit(slot_id, tbuf, tlen, rbuf, &rlen);
  if (ret) {
    DBG_PRINT("ME no response!\n");
    return -1;
  }

  if (rlen < 3) {
    DBG_PRINT("return incomplete len=%d\n", rlen);
    return -1;
  }
  if (rbuf[0] != CC_SUCCESS) {
    DBG_PRINT("Completion Code: %02X\n", rbuf[0]);
    return -1;
  }

  if (rbuf[1] == 0x55) {
    return 0;
  }

  return -1;
}

bool 
is_pmic_supported()
{
  return true;
}

int
pmic_list_err(uint8_t fru_id, uint8_t cpu, uint8_t dimm, uint8_t* err_list, uint8_t* err_cnt)
{ 
  return me_pmic_err_list(fru_id, dimm, err_list, err_cnt);
}

int
pmic_inject_err(uint8_t fru_id, uint8_t cpu, uint8_t dimm, uint8_t option)
{
  return me_pmic_err_inj(fru_id, dimm, option);
}

