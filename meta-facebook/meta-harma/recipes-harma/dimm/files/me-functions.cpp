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
#include <openbmc/obmc-i2c.h>
#include "dimm.h"
#include "dimm-util-plat.h"

#define MIN_RESP_LEN (1 /* ME CC */ + INTEL_ID_LEN)
#define BIC_I3C_BASE 13
#define F0M_DIMM_I2C_BASE 32

#ifdef DEBUG_DIMM_UTIL
  #define DBG_PRINT(...) printf(__VA_ARGS__)
#else
  #define DBG_PRINT(...)
#endif

static uint8_t f0m_spd_addr[MAX_DIMM_PER_CPU/2] = {
  DIMM0_SPD_ADDR,
  DIMM1_SPD_ADDR,
  DIMM2_SPD_ADDR,
  DIMM3_SPD_ADDR,
};

static uint8_t f0m_pmic_addr[MAX_DIMM_PER_CPU/2] = {
  DIMM0_PMIC_ADDR,
  DIMM1_PMIC_ADDR,
  DIMM2_PMIC_ADDR,
  DIMM3_PMIC_ADDR,
};

// dimm location constant strings, matching silk screen
static const char *f0m_dimm_label[NUM_CPU_F0M][MAX_DIMM_PER_CPU] = {
  { "A0", "A1", "A2", "A4", "A6", "A7", "A8", "A10", },
};

static uint8_t f0m_dimm_cache_id[NUM_CPU_F0M][MAX_DIMM_PER_CPU] = {
  { 0, 1, 2, 4, 6, 7, 8, 10},
};

static const char *fru_name_f0m[NUM_FRU_F0M] = {
  "mb",
};

static uint8_t *spd_addr = f0m_spd_addr;
static uint8_t *pmic_addr = f0m_pmic_addr;
static const char *(*dimm_label)[MAX_DIMM_PER_CPU] = f0m_dimm_label;
static uint8_t (*dimm_cache_id)[MAX_DIMM_PER_CPU] = f0m_dimm_cache_id;

static uint8_t i3c_to_i2c_bus_base = F0M_DIMM_I2C_BASE;

int
plat_init(void) {
  num_frus = NUM_FRU_F0M;
  fru_name = fru_name_f0m;
  fru_id_min = FRU_ID_MIN_F0M;
  fru_id_max = FRU_ID_MAX_F0M;
  fru_id_all = FRU_ID_ALL_F0M;

  spd_addr = f0m_spd_addr;
  pmic_addr = f0m_pmic_addr;
  dimm_label = f0m_dimm_label;
  dimm_cache_id = f0m_dimm_cache_id;

  num_dimms_per_cpu = MAX_DIMM_NUM_F0M;
  i3c_to_i2c_bus_base = F0M_DIMM_I2C_BASE;

  num_cpus = NUM_CPU_F0M;
  total_dimms = num_dimms_per_cpu * num_cpus;

  read_block = true;
  max_retries = 8;
  return 0;
}

const char *
get_dimm_label(uint8_t cpu, uint8_t dimm) {
  if ((cpu >= NUM_CPU_F0M) || (dimm >= num_dimms_per_cpu)) {
    return "N/A";
  }

  return dimm_label[cpu][dimm];
}

uint8_t
get_dimm_cache_id(uint8_t cpu, uint8_t dimm) {
  if ((cpu >= NUM_CPU_F0M) || (dimm >= num_dimms_per_cpu)) {
    return 0xff;
  }

  return dimm_cache_id[cpu][dimm];
}

bool
is_dimm_present(uint8_t slot_id, uint8_t dimm) {
   return true;
}

static int
read_dimm_smbus(uint8_t slot_id, uint8_t bus_id, uint8_t addr, uint8_t offs_len,
                    uint32_t offset, uint8_t len, uint8_t *rxbuf) {
  int fd = 0, ret = -1;
  uint8_t tbuf[16] = {0};
  uint8_t rbuf[64] = {0};

  if (rxbuf == NULL) {
    return -1;
  }

  bus_id = bus_id + i3c_to_i2c_bus_base;
  fd = i2c_cdev_slave_open(bus_id, addr >> 1, I2C_SLAVE_FORCE_CLAIM);
  if (fd < 0) {
    return ret;
  }

  memcpy(tbuf, &offset, offs_len);

  ret = i2c_rdwr_msg_transfer(fd, addr, tbuf, offs_len, rbuf, len);
  i2c_cdev_slave_close(fd);

  if (ret) {
    DBG_PRINT("dimm no response!\n");
    return -1;
  }

  memcpy(rxbuf, rbuf, len);
  return len;
}

static int
write_dimm_smbus(uint8_t slot_id, uint8_t bus_id, uint8_t addr, uint8_t offs_len,
                     uint32_t offset, uint8_t len, uint8_t *txbuf) {
  int fd = 0, ret = -1;
  uint8_t tbuf[64] = {0};
  uint8_t rbuf[16] = {0};
  uint8_t tlen = offs_len + len;

  if (txbuf == NULL) {
    return -1;
  }

  bus_id = bus_id + i3c_to_i2c_bus_base;

  fd = i2c_cdev_slave_open(bus_id, addr >> 1, I2C_SLAVE_FORCE_CLAIM);
  if (fd < 0) {
    return ret;
  }

  memcpy(tbuf, &offset, offs_len);
  memcpy(&tbuf[offs_len], txbuf, len);

  ret = i2c_rdwr_msg_transfer(fd, addr, tbuf, tlen, rbuf, 0);
  i2c_cdev_slave_close(fd);

  if (ret) {
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

  if (rxbuf == NULL) {
    return -1;
  }

  // SPR CPU supports 2 SPD buses
  if (dimm >= (num_dimms_per_cpu/2)) {
    bus_id = 1;
  }
  addr = spd_addr[dimm % (num_dimms_per_cpu/2)];

  return read_dimm_smbus(slot_id, bus_id, addr, 2, spd_offset, len, rxbuf);
}

int
util_set_EE_page(uint8_t slot_id, uint8_t /*cpu*/, uint8_t dimm, uint8_t /*page_num*/) {
  uint8_t bus_id = 0;
  uint8_t addr = 0;
  uint8_t buf[8];


  // SPR CPU supports 2 SPD buses
  if (dimm >= (num_dimms_per_cpu/2)) {
    bus_id = 1;
  }
  addr = spd_addr[dimm % (num_dimms_per_cpu/2)];

  // set MR11[3] = 1b for 2-bytes addressing (offset) mode
  buf[0] = 0x08;
  return write_dimm_smbus(slot_id, bus_id, addr, 1, 0x0b, 1, buf);
}

int
util_check_me_status(uint8_t slot_id) {
  return 0;
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

  if (rxbuf == NULL) {
    return -1;
  }

  // SPR CPU supports 2 SPD buses
  if (dimm >= (num_dimms_per_cpu/2)) {
    bus_id = 1;
  }
  addr = pmic_addr[dimm % (num_dimms_per_cpu/2)];

  return read_dimm_smbus(slot_id, bus_id, addr, 1, pmic_offset, len, rxbuf);
}

int
util_write_pmic(uint8_t slot_id, uint8_t /*cpu*/, uint8_t dimm, uint8_t offset, uint8_t len, uint8_t *txbuf) {
  uint8_t bus_id = 0;
  uint8_t addr = 0;
  uint32_t pmic_offset = offset;

  if (txbuf == NULL) {
    return -1;
  }

  // SPR CPU supports 2 SPD buses
  if (dimm >= (num_dimms_per_cpu/2)) {
    bus_id = 1;
  }
  addr = pmic_addr[dimm % (num_dimms_per_cpu/2)];

  return write_dimm_smbus(slot_id, bus_id, addr, 1, pmic_offset, len, txbuf);
}
