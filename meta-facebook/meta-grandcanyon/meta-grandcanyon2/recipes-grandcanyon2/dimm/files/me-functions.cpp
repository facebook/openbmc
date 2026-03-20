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
#include <cstdio>
#include <cstdint>
#include <cstring>
#include "dimm.h"
#include "dimm-util-plat.h"

extern "C" {
    #include <facebook/bic.h>
    #include <facebook/bic_xfer.h>
}
#define MIN_RESP_LEN (1 /* ME CC */ + INTEL_ID_LEN)
#define BIC_I3C_BASE 13

#ifdef DEBUG_DIMM_UTIL
  #define DBG_PRINT(...) printf(__VA_ARGS__)
#else
  #define DBG_PRINT(...)
#endif

static uint8_t gc2_spd_addr[MAX_DIMM_PER_CPU/2] = {
  DIMM0_SPD_ADDR,
  DIMM1_SPD_ADDR,
};

static uint8_t gc2_pmic_addr[MAX_DIMM_PER_CPU/2] = {
  DIMM0_PMIC_ADDR,
  DIMM1_PMIC_ADDR,
};

// dimm location constant strings, matching silk screen
static const char *gc2_dimm_label[NUM_CPU_GC2][MAX_DIMM_PER_CPU] = {
  { "A2", "A3", "A6", "A7", },
};

static uint8_t gc2_dimm_cache_id[NUM_CPU_GC2][MAX_DIMM_PER_CPU] = {
  { 4, 6, 12, 14,},
};


// dimm location constant strings, matching silk screen

static uint8_t *spd_addr = gc2_spd_addr;
static uint8_t *pmic_addr = gc2_pmic_addr;
static const char *(*dimm_label)[MAX_DIMM_PER_CPU] = gc2_dimm_label;
static uint8_t (*dimm_cache_id)[MAX_DIMM_PER_CPU] = gc2_dimm_cache_id;

static const char *fru_name_gc2[NUM_FRU_GC2] = {
  "server",
};

static bool direct_xfer = false;
static uint8_t bic_bus_base = BIC_I3C_BASE;

int
plat_init(void) {

  num_frus = NUM_FRU_GC2;
  fru_name = fru_name_gc2;
  fru_id_min = FRU_ID_MIN_GC2;
  fru_id_max = FRU_ID_MAX_GC2;
  fru_id_all = FRU_ID_ALL_GC2;

  num_dimms_per_cpu = MAX_DIMM_NUM_GC2;
  num_cpus = NUM_CPU_GC2;
  total_dimms = num_dimms_per_cpu * num_cpus;

  read_block = true;
  max_retries = 8;

  return 0;
}

const char *
get_dimm_label(uint8_t cpu, uint8_t dimm) {
  if ((cpu >= NUM_CPU_GC2) || (dimm >= num_dimms_per_cpu)) {
    return "N/A";
  }

  return dimm_label[cpu][dimm];
}

uint8_t
get_dimm_cache_id(uint8_t cpu, uint8_t dimm) {
  if ((cpu >= NUM_CPU_GC2) || (dimm >= num_dimms_per_cpu)) {
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

  if (rxbuf == NULL) {
    return -1;
  }

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

  if (bic_me_xmit(tbuf, tlen, rbuf, &rlen) != 0) {
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

  if (txbuf == NULL) {
    return -1;
  }

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

  if (bic_me_xmit(tbuf, tlen, rbuf, &rlen) != 0) {
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

static int
bic_read_dimm_i3c(uint8_t slot_id, uint8_t bus_id, uint8_t dimm, uint8_t offs_len,
                   uint32_t offset, uint8_t len, uint8_t *rxbuf, uint8_t device_type) {
  uint8_t tbuf[16] = {0x00}; // IANA ID
  uint8_t rbuf[64] = {0x00};
  uint8_t rlen = len;
  uint8_t tlen = SIZE_IANA_ID;
  int ret = 0;

  // Fill the IANA ID
  memcpy(tbuf, (uint8_t *)&META_IANA_ID, SIZE_IANA_ID);
  tbuf[tlen++] = dimm;
  tbuf[tlen++] = device_type;
  tbuf[tlen++] = len;
  memcpy(&tbuf[tlen], &offset, offs_len);
  tlen += offs_len;

  ret = bic_ipmb_wrapper(NETFN_OEM_1S_REQ, BIC_CMD_OEM_READ_WRITE_DIMM, tbuf, tlen, rbuf, &rlen);

  memcpy(rxbuf, &rbuf[SIZE_IANA_ID], len);

  return ret;
}

static int
bic_write_dimm_i3c(uint8_t slot_id, uint8_t bus_id, uint8_t dimm, uint8_t offs_len,
                   uint32_t offset, uint8_t len, uint8_t *txbuf, uint8_t device_type) {
  uint8_t tbuf[64] = {0x00}; // IANA ID
  uint8_t rbuf[16] = {0x00};
  uint8_t rlen = sizeof(rbuf);
  uint8_t tlen = SIZE_IANA_ID;
  int ret = 0;

  // Fill the IANA ID
  memcpy(tbuf, (uint8_t *)&META_IANA_ID, SIZE_IANA_ID);
  tbuf[tlen++] = dimm;
  tbuf[tlen++] = device_type;
  tbuf[tlen++] = len;
  memcpy(&tbuf[tlen], &offset, offs_len);
  tlen += offs_len;
  memcpy(&tbuf[tlen], txbuf, len);
  tlen += len;

  ret = bic_ipmb_wrapper(NETFN_OEM_1S_REQ, BIC_CMD_OEM_READ_WRITE_DIMM, tbuf, tlen, rbuf, &rlen);

  return ret;
}

static int
bic_read_dimm_smbus(uint8_t slot_id, uint8_t bus_id, uint8_t addr, uint8_t offs_len,
                    uint32_t offset, uint8_t len, uint8_t *rxbuf) {
  uint8_t tbuf[16] = {0};
  uint8_t rbuf[64] = {0};

  if (rxbuf == NULL) {
    return -1;
  }

  bus_id = ((bus_id + bic_bus_base) << 1) + 1;
  memcpy(tbuf, &offset, offs_len);
  if (bic_master_write_read(slot_id, bus_id, addr, tbuf, offs_len, rbuf, len) != 0) {
    DBG_PRINT("BIC no response!\n");
    return -1;
  }

  memcpy(rxbuf, rbuf, len);
  return len;
}

static int
bic_write_dimm_smbus(uint8_t slot_id, uint8_t bus_id, uint8_t addr, uint8_t offs_len,
                     uint32_t offset, uint8_t len, uint8_t *txbuf) {
  uint8_t tbuf[64] = {0};
  uint8_t rbuf[16] = {0};
  uint8_t tlen = offs_len + len;

  if (txbuf == NULL) {
    return -1;
  }

  bus_id = ((bus_id + bic_bus_base) << 1) + 1;
  memcpy(tbuf, &offset, offs_len);
  memcpy(&tbuf[offs_len], txbuf, len);
  if (bic_master_write_read(slot_id, bus_id, addr, tbuf, tlen, rbuf, 0) != 0) {
    DBG_PRINT("BIC no response!\n");
    return -1;
  }

  return len;
}

int
util_read_spd(uint8_t slot_id, uint8_t /*cpu*/, uint8_t dimm, uint16_t offset, uint8_t len, uint8_t *rxbuf) {
  uint8_t bus_id = 0;
  uint8_t addr = 0;
  uint32_t spd_offset = ((offset & 0x780) << 1) | (0x80 | (offset & 0x7F));
  int ret = 0;

  if (rxbuf == NULL) {
    return -1;
  }

  // SPR CPU supports 2 SPD buses
  if (dimm >= (num_dimms_per_cpu/2)) {
    bus_id = 1;
  }
  addr = spd_addr[dimm % (num_dimms_per_cpu/2)];

  if (direct_xfer) {
    return bic_read_dimm_smbus(slot_id, bus_id, addr, 2, spd_offset, len, rxbuf);
  }

  ret = bic_read_dimm_i3c(slot_id, bus_id, dimm, 2, offset, len, rxbuf, DIMM_SPD_NVM);
  if (ret < 0) {
    return nm_read_dimm_smbus(slot_id, bus_id, addr, 2, spd_offset, len, rxbuf);
  }

  return len;
}

int
util_set_EE_page(uint8_t slot_id, uint8_t /*cpu*/, uint8_t dimm, uint8_t /*page_num*/) {
  uint8_t bus_id = 0;
  uint8_t addr = 0;
  uint8_t buf[8];

  if (direct_xfer) {
    // SPR CPU supports 2 SPD buses
    if (dimm >= (num_dimms_per_cpu/2)) {
      bus_id = 1;
    }
    addr = spd_addr[dimm % (num_dimms_per_cpu/2)];

    // set MR11[3] = 1b for 2-bytes addressing (offset) mode
    buf[0] = 0x08;
    return bic_write_dimm_smbus(slot_id, bus_id, addr, 1, 0x0b, 1, buf);
  }
  return 0;
}

int
util_check_me_status(uint8_t slot_id) {
  int ret;
  uint8_t tbuf[64] = {0x00};
  uint8_t rbuf[64] = {0x00};
  uint8_t tlen = 2;
  uint8_t rlen = sizeof(rbuf);

  if (direct_xfer) {
    return 0;
  }

  tbuf[0] = NETFN_APP_REQ << 2;
  tbuf[1] = CMD_APP_GET_SELFTEST_RESULTS;
  ret = bic_me_xmit(tbuf, tlen, rbuf, &rlen);
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
util_read_pmic(uint8_t slot_id, uint8_t /*cpu*/, uint8_t dimm, uint8_t offset, uint8_t len, uint8_t *rxbuf) {
  uint8_t bus_id = 0;
  uint8_t addr = 0;
  uint32_t pmic_offset = offset;
  int ret = 0;

  if (rxbuf == NULL) {
    return -1;
  }

  // SPR CPU supports 2 SPD buses
  if (dimm >= (num_dimms_per_cpu/2)) {
    bus_id = 1;
  }
  addr = pmic_addr[dimm % (num_dimms_per_cpu/2)];

  if (direct_xfer) {
    return bic_read_dimm_smbus(slot_id, bus_id, addr, 1, pmic_offset, len, rxbuf);
  }

  ret = bic_read_dimm_i3c(slot_id, bus_id, dimm, 1, pmic_offset, len, rxbuf, DIMM_PMIC);
  if (ret < 0) {
    return nm_read_dimm_smbus(slot_id, bus_id, addr, 1, pmic_offset, len, rxbuf);
  }

  return len;
}

int
util_write_pmic(uint8_t slot_id, uint8_t /*cpu*/, uint8_t dimm, uint8_t offset, uint8_t len, uint8_t *txbuf) {
  uint8_t bus_id = 0;
  uint8_t addr = 0;
  uint32_t pmic_offset = offset;
  int ret = 0;

  if (txbuf == NULL) {
    return -1;
  }

  // SPR CPU supports 2 SPD buses
  if (dimm >= (num_dimms_per_cpu/2)) {
    bus_id = 1;
  }
  addr = pmic_addr[dimm % (num_dimms_per_cpu/2)];

  if (direct_xfer) {
    return bic_write_dimm_smbus(slot_id, bus_id, addr, 1, pmic_offset, len, txbuf);
  }

  ret = bic_write_dimm_i3c(slot_id, bus_id, dimm, 1, pmic_offset, len, txbuf, DIMM_PMIC);
  if (ret < 0){
    return nm_write_dimm_smbus(slot_id, bus_id, addr, 1, pmic_offset, len, txbuf);
  }

  return len;
}
