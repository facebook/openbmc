/*
 * Copyright 2022-present Facebook. All Rights Reserved.
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
#include "dimm-util.h"
#include "dimm-util-plat.h"

#define MIN_RESP_LEN (1 /* ME CC */ + INTEL_ID_LEN)

#ifdef DEBUG_DIMM_UTIL
  #define DBG_PRINT(fmt, args...) printf(fmt, ##args)
#else
  #define DBG_PRINT(fmt, args...)
#endif

static uint8_t spd_addr[MAX_DIMM_NUM_FBY35/2] = {
  DIMM0_SPD_ADDR,
  DIMM1_SPD_ADDR,
  DIMM2_SPD_ADDR,
};

static uint8_t pmic_addr[MAX_DIMM_NUM_FBY35/2] = {
  DIMM0_PMIC_ADDR,
  DIMM1_PMIC_ADDR,
  DIMM2_PMIC_ADDR,
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
plat_init(void) {
  num_frus = NUM_FRU_FBY35;
  num_dimms_per_cpu = MAX_DIMM_NUM_FBY35;
  num_cpus          = NUM_CPU_FBY35;
  total_dimms       = num_dimms_per_cpu * num_cpus;
  fru_name   = fru_name_fby35;
  fru_id_min = FRU_ID_MIN_FBY35;
  fru_id_max = FRU_ID_MAX_FBY35;
  fru_id_all = FRU_ID_ALL_FBY35;
  read_block = true;
  max_retries = 8;

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

static int
nm_read_dimm_smbus(uint8_t slot_id, uint8_t bus_id, uint8_t addr, uint8_t offs_len,
                   uint32_t offset, uint8_t len, uint8_t *rxbuf) {
  uint8_t tbuf[64] = {0};
  uint8_t rbuf[64] = {0};
  uint8_t tlen = 14;
  uint8_t rlen = sizeof(rbuf);

  tbuf[0] = NETFN_NM_REQ << 2;
  tbuf[1] = CMD_NM_READ_MEM_SM_BUS;
  tbuf[2] = MANU_INTEL_0;
  tbuf[3] = MANU_INTEL_1;
  tbuf[4] = MANU_INTEL_2;
  tbuf[5] = 0;  // CPU number
  tbuf[6] = bus_id;
  tbuf[7] = addr;
  tbuf[8] = offs_len;
  tbuf[9] = (len > 0) ? len - 1 : 0;
  memcpy(&tbuf[10], &offset, sizeof(offset));

  if (bic_me_xmit(slot_id, tbuf, tlen, rbuf, &rlen) != 0) {
    DBG_PRINT("ME no response!\n");
    return -1;
  }

  if (rlen != (MIN_RESP_LEN + len)) {
    DBG_PRINT("return incomplete len=%d\n", rlen);
    if (rlen == MIN_RESP_LEN) {
      DBG_PRINT("Completion Code: %02X\n", rbuf[0]);
      if (rbuf[0] == 0xAD) {  // illegal address
        return 0;
      }
    }
    return -1;
  }
  if (rbuf[0] != CC_SUCCESS) {
    DBG_PRINT("Completion Code: %02X\n", rbuf[0]);
    return -1;
  }

  memcpy(rxbuf, &rbuf[MIN_RESP_LEN], len);
  return len;
}

static int
nm_write_dimm_smbus(uint8_t slot_id, uint8_t bus_id, uint8_t addr, uint8_t offs_len,
                    uint32_t offset, uint8_t len, uint8_t *txbuf) {
  uint8_t tbuf[64] = {0};
  uint8_t rbuf[64] = {0};
  uint8_t tlen = 14 + len;
  uint8_t rlen = sizeof(rbuf);

  tbuf[0] = NETFN_NM_REQ << 2;
  tbuf[1] = CMD_NM_WRITE_MEM_SM_BUS;
  tbuf[2] = MANU_INTEL_0;
  tbuf[3] = MANU_INTEL_1;
  tbuf[4] = MANU_INTEL_2;
  tbuf[5] = 0;  // CPU number
  tbuf[6] = bus_id;
  tbuf[7] = addr;
  tbuf[8] = offs_len;
  tbuf[9] = (len > 0) ? len - 1 : 0;
  memcpy(&tbuf[10], &offset, sizeof(offset));
  memcpy(&tbuf[14], txbuf, len);

  if (bic_me_xmit(slot_id, tbuf, tlen, rbuf, &rlen) != 0) {
    DBG_PRINT("ME no response!\n");
    return -1;
  }

  if (rlen != MIN_RESP_LEN) {
    DBG_PRINT("return incomplete len=%d\n", rlen);
    return -1;
  }
  if (rbuf[0] != CC_SUCCESS) {
    DBG_PRINT("Completion Code: %02X\n", rbuf[0]);
    if (rbuf[0] == 0xAD) {  // illegal address
      return 0;
    }
    return -1;
  }

  return len;
}

int
util_read_spd(uint8_t slot_id, uint8_t cpu, uint8_t dimm, uint16_t offset, uint8_t len, uint8_t *rxbuf) {
  uint8_t bus_id = 0;
  uint8_t addr = 0;
  uint32_t spd_offset = ((offset & 0x780) << 1) | (0x80 | (offset & 0x7F));

  // SPR CPU supports 2 SPD buses
  if (dimm >= (MAX_DIMM_NUM_FBY35/2)) {
    bus_id = 1;
  }
  addr = spd_addr[dimm % (MAX_DIMM_NUM_FBY35/2)];

  return nm_read_dimm_smbus(slot_id, bus_id, addr, 2, spd_offset, len, rxbuf);
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
is_pmic_supported(void) {
  return true;
}

int
util_read_pmic(uint8_t slot_id, uint8_t cpu, uint8_t dimm, uint8_t offset, uint8_t len, uint8_t *rxbuf) {
  uint8_t bus_id = 0;
  uint8_t addr = 0;
  uint32_t pmic_offset = offset;

  // SPR CPU supports 2 SPD buses
  if (dimm >= (MAX_DIMM_NUM_FBY35/2)) {
    bus_id = 1;
  }
  addr = pmic_addr[dimm % (MAX_DIMM_NUM_FBY35/2)];

  return nm_read_dimm_smbus(slot_id, bus_id, addr, 1, pmic_offset, len, rxbuf);
}

int
util_write_pmic(uint8_t slot_id, uint8_t cpu, uint8_t dimm, uint8_t offset, uint8_t len, uint8_t *txbuf) {
  uint8_t bus_id = 0;
  uint8_t addr = 0;
  uint32_t pmic_offset = offset;

  // SPR CPU supports 2 SPD buses
  if (dimm >= (MAX_DIMM_NUM_FBY35/2)) {
    bus_id = 1;
  }
  addr = pmic_addr[dimm % (MAX_DIMM_NUM_FBY35/2)];

  return nm_write_dimm_smbus(slot_id, bus_id, addr, 1, pmic_offset, len, txbuf);
}
