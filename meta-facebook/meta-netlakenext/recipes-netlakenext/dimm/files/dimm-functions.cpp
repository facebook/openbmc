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

#ifdef DEBUG_DIMM_UTIL
  #define DBG_PRINT(...) printf(__VA_ARGS__)
#else
  #define DBG_PRINT(...)
#endif

static uint8_t netlake2_spd_addr[MAX_DIMM_PER_CPU/2] = {
  DIMMA_SPD_ADDR,
  DIMMB_SPD_ADDR,
};

static uint8_t netlake2_pmic_addr[MAX_DIMM_PER_CPU/2] = {
  DIMMA_PMIC_ADDR,
  DIMMB_PMIC_ADDR,
};

// dimm location constant strings, matching silk screen
static const char *netlake2_dimm_label[NUM_CPU_NETLAKE2][MAX_DIMM_PER_CPU] = {
  { "A", "B", },
};

static uint8_t netlake2_dimm_cache_id[NUM_CPU_NETLAKE2][MAX_DIMM_PER_CPU] = {
  { 0, 1},
};

static const char *fru_name_netlake2[NUM_FRU_NETLAKE2] = {
  "server",
};

static uint8_t *spd_addr = netlake2_spd_addr;
static uint8_t *pmic_addr = netlake2_pmic_addr;
static const char *(*dimm_label)[MAX_DIMM_PER_CPU] = netlake2_dimm_label;
static uint8_t (*dimm_cache_id)[MAX_DIMM_PER_CPU] = netlake2_dimm_cache_id;

int
plat_init(void) {
  num_frus = NUM_FRU_NETLAKE2;
  fru_name = fru_name_netlake2;
  fru_id_min = FRU_ID_MIN_NETLAKE2;
  fru_id_max = FRU_ID_MAX_NETLAKE2;
  fru_id_all = FRU_ID_ALL_NETLAKE2;

  spd_addr = netlake2_spd_addr;
  pmic_addr = netlake2_pmic_addr;
  dimm_label = netlake2_dimm_label;
  dimm_cache_id = netlake2_dimm_cache_id;

  num_dimms_per_cpu = MAX_DIMM_NUM_NETLAKE2;

  num_cpus = NUM_CPU_NETLAKE2;
  total_dimms = num_dimms_per_cpu * num_cpus;

  read_block = true;
  max_retries = 8;
  return 0;
}

const char *
get_dimm_label(uint8_t cpu, uint8_t dimm) {
  if ((cpu >= NUM_CPU_NETLAKE2) || (dimm >= num_dimms_per_cpu)) {
    return "N/A";
  }

  return dimm_label[cpu][dimm];
}

uint8_t
get_dimm_cache_id(uint8_t cpu, uint8_t dimm) {
  if ((cpu >= NUM_CPU_NETLAKE2) || (dimm >= num_dimms_per_cpu)) {
    return 0xff;
  }

  return dimm_cache_id[cpu][dimm];
}

bool
is_dimm_present(uint8_t slot_id, uint8_t dimm) {
   return true;
}

static int
read_dimm_i2c(uint8_t slot_id, uint8_t bus, uint8_t addr, uint8_t offs_len,
                    uint32_t offset, uint8_t len, uint8_t *rxbuf) {
  int fd = 0, ret = -1;
  uint8_t tbuf[16] = {0};
  uint8_t rbuf[64] = {0};

  if (rxbuf == NULL) {
    return -1;
  }

  fd = i2c_cdev_slave_open(bus, addr >> 1, I2C_SLAVE_FORCE_CLAIM);
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
write_dimm_i2c(uint8_t slot_id, uint8_t bus, uint8_t addr, uint8_t offs_len,
                     uint32_t offset, uint8_t len, uint8_t *txbuf) {
  int fd = 0, ret = -1;
  uint8_t tbuf[64] = {0};
  uint8_t rbuf[16] = {0};
  uint8_t tlen = offs_len + len;

  if (txbuf == NULL) {
    return -1;
  }

  fd = i2c_cdev_slave_open(bus, addr >> 1, I2C_SLAVE_FORCE_CLAIM);
  if (fd < 0) {
    return ret;
  }

  memcpy(tbuf, &offset, offs_len);
  memcpy(&tbuf[offs_len], txbuf, len);

  ret = i2c_rdwr_msg_transfer(fd, addr, tbuf, tlen, rbuf, 0);
  i2c_cdev_slave_close(fd);

  if (ret) {
    DBG_PRINT("dimm no response!\n");
    return -1;
  }

  return len;
}

int
util_read_spd(uint8_t slot_id, uint8_t /*cpu*/, uint8_t dimm, uint16_t offset, uint8_t len, uint8_t *rxbuf) {
  uint8_t bus = DIMM_BUS;
  uint8_t addr = 0;
  uint32_t spd_offset = ((offset & 0x780) << 1) | (0x80 | (offset & 0x7F));

  if (rxbuf == NULL) {
    return -1;
  }

  addr = spd_addr[dimm % num_dimms_per_cpu];

  return read_dimm_i2c(slot_id, bus, addr, 2, spd_offset, len, rxbuf);
}

int
util_set_EE_page(uint8_t slot_id, uint8_t /*cpu*/, uint8_t dimm, uint8_t /*page_num*/) {
  uint8_t bus = DIMM_BUS;
  uint8_t addr = 0;
  uint8_t buf[8];

  addr = spd_addr[dimm % num_dimms_per_cpu];

  // set MR11[3] = 1b for 2-bytes addressing (offset) mode
  buf[0] = 0x08;
  return write_dimm_i2c(slot_id, bus, addr, 1, 0x0b, 1, buf);
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
  uint8_t bus = DIMM_BUS;
  uint8_t addr = 0;
  uint32_t pmic_offset = offset;

  if (rxbuf == NULL) {
    return -1;
  }

  addr = pmic_addr[dimm % num_dimms_per_cpu];

  return read_dimm_i2c(slot_id, bus, addr, 1, pmic_offset, len, rxbuf);
}

int
util_write_pmic(uint8_t slot_id, uint8_t /*cpu*/, uint8_t dimm, uint8_t offset, uint8_t len, uint8_t *txbuf) {
  uint8_t bus = DIMM_BUS;
  uint8_t addr = 0;
  uint32_t pmic_offset = offset;

  if (txbuf == NULL) {
    return -1;
  }

  addr = pmic_addr[dimm % num_dimms_per_cpu];

  return write_dimm_i2c(slot_id, bus, addr, 1, pmic_offset, len, txbuf);
}

int
util_set_SODIMM_page(uint8_t slot_id, uint8_t /*cpu*/, uint8_t dimm, uint8_t /*page_num*/) {
  uint8_t bus = DIMM_BUS;
  uint8_t addr = 0;
  uint8_t buf[8];
  uint32_t spd_offset = 0x000b;

  addr = spd_addr[dimm % num_dimms_per_cpu];

  // set MR11[3] = 0b MR11[2:0] = '000' for setting page 0 
  buf[0] = 0x00;
  return write_dimm_i2c(slot_id, bus, addr, 2, spd_offset, 1, buf);
}
