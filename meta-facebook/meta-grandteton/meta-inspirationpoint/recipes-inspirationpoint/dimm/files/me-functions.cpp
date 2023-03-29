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
#include "dimm.h"
#include "dimm-util-plat.h"

#include <iostream>
#include <fcntl.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <openbmc/i3c_dev.h>

#include <map>
#include <vector>
#include <string.h>
#include <fmt/format.h>

constexpr auto DEV_I3C = "/dev/bus/i3c";

// dimm location constant strings, matching silk screen
static constexpr const char* dimm_label[NUM_CPU_FBGTI][MAX_DIMM_NUM_FBGTI] = {
  { "A0","A1","A2","A3","A4","A5","A6","A7","A8","A9","A10","A11"},
  { "B0","B1","B2","B3","B4","B5","B6","B7","B8","B9","B10","B11"},
};


static uint8_t dimm_cache_id[NUM_CPU_FBGTI][MAX_DIMM_NUM_FBGTI] = {
  {  0,  1,  2,  3,  4,  5,  6,  7,  8,  9, 10, 11},
  { 12, 13, 14, 15, 16, 17, 18, 19, 20, 21, 22, 23},
};

static const char *fru_name_fbgti[NUM_FRU_FBGTI] = {
  "mb",
};

enum class DIMMDeviceType {
  SPD,
  PMIC
};

const std::map<DIMMDeviceType, std::string> dimmTypeMap = {
  {DIMMDeviceType::SPD,  "e5118"},
  {DIMMDeviceType::PMIC, "e5010"}
};

int
plat_init(void) {
  num_frus = NUM_FRU_FBGTI;
  num_dimms_per_cpu = MAX_DIMM_NUM_FBGTI;
  num_cpus          = NUM_CPU_FBGTI;
  total_dimms       = num_dimms_per_cpu * num_cpus;
  fru_name   = fru_name_fbgti;
  fru_id_min = FRU_ID_MIN_FBGTI;
  fru_id_max = FRU_ID_MAX_FBGTI;
  fru_id_all = FRU_ID_ALL_FBGTI;
  read_block = true;
  max_retries = 15;
  retry_intvl = 50;

  return 0;
}

const char *
get_dimm_label(uint8_t cpu, uint8_t dimm) {
  if ((cpu >= NUM_CPU_FBGTI)  || (dimm >= MAX_DIMM_NUM_FBGTI)) {
    return "N/A";
  }

  return dimm_label[cpu][dimm];
}

uint8_t
get_dimm_cache_id(uint8_t cpu, uint8_t dimm) {
  if ((cpu >= NUM_CPU_FBGTI)  || (dimm >= MAX_DIMM_NUM_FBGTI)) {
    return 0xff;
  }

  return dimm_cache_id[cpu][dimm];
}

std::string
get_dimm_addr(DIMMDeviceType type, uint8_t cpu, uint8_t dimm) {
  if (dimm >= MAX_DIMM_NUM_FBGTI) {
    return "";
  }

  cpu  = dimm < MAX_DIMM_NUM_FBGTI/2 ? cpu * 2 : cpu * 2 + 1;
  dimm = dimm < MAX_DIMM_NUM_FBGTI/2 ? dimm : dimm - MAX_DIMM_NUM_FBGTI/2;
  return fmt::format("{}/{}-{}{}000", DEV_I3C, cpu, dimmTypeMap.at(type), dimm);
}

int
util_read_spd(uint8_t /*fru_id*/, uint8_t cpu, uint8_t dimm, uint16_t offset,
              uint8_t len, uint8_t *rxbuf) 
{
  DIMMDeviceType DimmType = DIMMDeviceType::SPD;
  std::string SpdAddr = get_dimm_addr(DimmType, cpu, dimm);

  std::vector<uint8_t> wdata(2);
  uint32_t shift_offset = ((offset & 0x780) << 1) | (0x80 | (offset & 0x7F));
  wdata[1] = (uint8_t)(shift_offset >> 8);
  wdata[0] = (uint8_t)(shift_offset & 0xFF);


  int fd = i3c_open(SpdAddr.c_str());

  if (fd < 0) {
    return -1;
  }

  int ret = i3c_rdwr_msg_xfer(fd, wdata.data(), 2, rxbuf, len);
  i3c_close(fd);
  return ret < 0 ? -1 : len;
}

int
util_check_me_status(uint8_t /*fru_id*/) {
  return 0;
}

bool
is_pmic_supported(void) {
  return true;
}

int
util_read_pmic(uint8_t /*slot_id*/, uint8_t cpu, uint8_t dimm, uint8_t offset, 
               uint8_t len, uint8_t *rxbuf) 
{
  DIMMDeviceType DimmType = DIMMDeviceType::PMIC;
  std::string PmicAddr = get_dimm_addr(DimmType, cpu, dimm);

  int fd = i3c_open(PmicAddr.c_str());

  if (fd < 0) {
    return -1;
  }

  int ret = i3c_rdwr_msg_xfer(fd, &offset, 1, rxbuf, len);
  i3c_close(fd);
  return ret < 0 ? -1 : len;
}

int
util_write_pmic(uint8_t /*slot_id*/, uint8_t cpu, uint8_t dimm, uint8_t offset, 
                uint8_t len, uint8_t *txbuf) 

{
  DIMMDeviceType DimmType = DIMMDeviceType::PMIC;
  std::string PmicAddr = get_dimm_addr(DimmType, cpu, dimm);

  std::vector<uint8_t> wdata;
  wdata.push_back(offset);
  wdata.insert(wdata.end(), &txbuf[0], &txbuf[len]);

  int fd = i3c_open(PmicAddr.c_str());

  if (fd < 0) {
    return -1;
  }

  int ret = i3c_rdwr_msg_xfer(fd, wdata.data(), len + 1, NULL, 0);
  i3c_close(fd);
  return ret < 0 ? -1 : len;
}
