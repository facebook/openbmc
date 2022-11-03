/*
 *
 * Copyright 2014-present Facebook. All Rights Reserved.
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
#include <stdlib.h>
#include <getopt.h>
#include <unistd.h>
#include <stdint.h>
#include <string.h>
#include <jansson.h>
#include <openbmc/kv.h>
#include <openbmc/ipmi.h>
#include <openbmc/ipmb.h>
#include <openbmc/pal.h>
#include "dimm-util.h"
#include "dimm-util-plat.h"

// #define DEBUG_DIMM_UTIL

#ifdef DEBUG_DIMM_UTIL
  #define DBG_PRINT(...) printf(__VA_ARGS__)
#else
  #define DBG_PRINT(...)
#endif


// i2c address for TP single side
static uint8_t dev_addr[MAX_DIMM_NUM_FBTP/2] = {
      DIMM0_SPD_ADDR,
      DIMM2_SPD_ADDR,
      DIMM4_SPD_ADDR,
};

// DIMM label for Single side Tioga Pass
char const *dimm_label_fbtp[NUM_CPU_FBTP][MAX_DIMM_NUM_FBTP] =
  {{"A0", "A1", "A2", "A3", "A4", "A5"},
   {"B0", "B1", "B2", "B3", "B4", "B5"}};

char const *fru_name_fbtp[NUM_FRU_FBTP] =
{
  "mb",
};

int
plat_init()
{
  unsigned char board_id;
  int ret = 0;

  // check if this is FBTP SS.
  ret = pal_get_platform_id(&board_id);
  if (ret != 0) {
    printf("Error: Platform detection failed\n");
    // failed ot determine platform type, return error
    return -1;
  }
  if (board_id & PLAT_ID_SKU_MASK) {
    printf("Error: FBTP DS not currently supported\n");
    // FBTP double side. This config is currently not supported
    return -1;
  }

  num_frus  = NUM_FRU_FBTP;
  num_dimms_per_cpu = MAX_DIMM_NUM_FBTP;
  num_cpus  = NUM_CPU_FBTP;
  total_dimms = num_dimms_per_cpu * num_cpus;
  fru_name   = fru_name_fbtp;
  fru_id_min = FRU_ID_MIN_FBTP;
  fru_id_max = FRU_ID_MAX_FBTP;
  fru_id_all = FRU_ID_ALL_FBTP;
  return 0;
}

const char *
get_dimm_label(uint8_t cpu, uint8_t dimm)
{
  if ((cpu >= NUM_CPU_FBTP)  || (dimm >= MAX_DIMM_NUM_FBTP)) {
    return "N/A";
  } else {
    return dimm_label_fbtp[cpu][dimm];
  }
}

// send ME command to select page 0 or 1 on SPD
int
util_set_EE_page(uint8_t /*fru_id*/, uint8_t cpu, uint8_t dimm, uint8_t page_num)
{
#define EEPROM_MAX_PAGE 2
#define EEPROM_P0_SEL 0x6C
#define EEPROM_P1_SEL 0x6E

  uint8_t tbuf[256] = {0};
  uint8_t rbuf[256] = {0};
  uint8_t page_sel[EEPROM_MAX_PAGE] = {EEPROM_P0_SEL, EEPROM_P1_SEL};
  uint8_t tlen = 0;
  uint8_t rlen = 0;
  uint8_t addr_msb = 0;
  ipmb_req_t *req;
  int ret = 0;

  if (page_num >= EEPROM_MAX_PAGE)
    return -1;

  // calculate DIMM msb_addr
  //   msb_addr is 0 for dimms 0-3,  1 for 4-7
  if (dimm >= (MAX_DIMM_NUM_FBTP/2))
    addr_msb = 1;

  req = (ipmb_req_t*)tbuf;

  req->res_slave_addr = ME_SLAVE_ADDR;
  req->netfn_lun = NETFN_NM_REQ << 2;
  req->hdr_cksum = req->res_slave_addr +
                   req->netfn_lun;
  req->hdr_cksum = ZERO_CKSUM_CONST - req->hdr_cksum;

  req->req_slave_addr = BMC_SLAVE_ADDR;
  req->seq_lun = 0x00;

  req->cmd = CMD_NM_WRITE_MEM_SM_BUS;

  tlen = 6;  // length so far with all fields above

  /* Intel's IANA */
  tbuf[tlen++] = MANU_INTEL_0;
  tbuf[tlen++] = MANU_INTEL_1;
  tbuf[tlen++] = MANU_INTEL_2;

  /* actual cmd payload for EEPROM page selelect */
  tbuf[tlen++] = cpu;
  tbuf[tlen++] = addr_msb;
  tbuf[tlen++] = page_sel[page_num];
  tbuf[tlen++] = 0;
  tbuf[tlen++] = 0;
  tbuf[tlen++] = 0;

  // Invoke IPMB library handler
  lib_ipmb_handle(ME_BUS_ADDR, tbuf, tlen+1, rbuf, &rlen);

  if (ret) {
    printf("ME no response!\n");
    return -1;
  }

  return ret;
}

// read 1 byte off SPD bus,
// input:  cpu/dimm/addr_msb/addr_lsb/offset
int
util_read_spd_byte(uint8_t /*fru_id*/, uint8_t cpu, uint8_t dimm, uint8_t offset)
{
// 7 bytes IPMB header + 3 bytes INTEL ID + 1 byte payload + 1 byte checksum
  constexpr size_t min_resp_len = (7 + INTEL_ID_LEN + 1 + 1 /* payload*/);

  uint8_t tbuf[256] = {0};
  uint8_t rbuf[256] = {0};
  uint8_t tlen = 0;
  uint8_t rlen = 0;
  int addr_msb = 0;
  int addr_lsb = 0;

  ipmb_req_t *req = (ipmb_req_t*)tbuf;;

  // calculate DIMM msb_addr
  //   msb_addr is 0 for dimms 0-3,  1 for 4-7
  if (dimm >= (MAX_DIMM_NUM_FBTP/2))
    addr_msb = 1;
  addr_lsb = dev_addr[dimm % (MAX_DIMM_NUM_FBTP/2)];

  req->res_slave_addr = ME_SLAVE_ADDR;
  req->netfn_lun = NETFN_NM_REQ << 2;
  req->hdr_cksum = req->res_slave_addr +
                   req->netfn_lun;
  req->hdr_cksum = ZERO_CKSUM_CONST - req->hdr_cksum;

  req->req_slave_addr = BMC_SLAVE_ADDR;
  req->seq_lun = 0x00;

  req->cmd = CMD_NM_READ_MEM_SM_BUS;

  tlen = 6;  // length so far with all fields above

  /* Intel's IANA */
  tbuf[tlen++] = MANU_INTEL_0;
  tbuf[tlen++] = MANU_INTEL_1;
  tbuf[tlen++] = MANU_INTEL_2;

  /* actual cmd payload for EEPROM page selelect */
  tbuf[tlen++] = cpu;
  tbuf[tlen++] = addr_msb;
  tbuf[tlen++] = addr_lsb;
  tbuf[tlen++] = offset;
  tbuf[tlen++] = 0;

  // Invoke IPMB library handler
  lib_ipmb_handle(ME_BUS_ADDR, tbuf, tlen+1, rbuf, &rlen);

#ifdef DEBUG_DIMM_UTIL
  int i = 0;
  DBG_PRINT("rlen = %d\n",rlen);
  for (i = 0; i < rlen; ++i)
    DBG_PRINT("0x%x ",rbuf[i]);
  DBG_PRINT("\n");
#endif

  if (rlen < min_resp_len) {
    return -1;
  }

  // actual SPD payload will be last byte following all IANA header and
  //  completion codes
  return rbuf[min_resp_len - 2];
}

int
util_check_me_status(uint8_t /*fru_id*/) {
// 7 bytes IPMB header + 2 byte payload + 1 byte checksum
  constexpr size_t min_resp_len = (7 + 2 + 1 /* payload*/);

  int me_status = 0;
  uint8_t tbuf[256] = {0};
  uint8_t rbuf[256] = {0};
  uint8_t tlen = 0;
  uint8_t rlen = 0;

  ipmb_req_t *req = (ipmb_req_t*)tbuf;;


  req->res_slave_addr = ME_SLAVE_ADDR;
  req->netfn_lun = NETFN_APP_REQ << 2;
  req->hdr_cksum = req->res_slave_addr +
                   req->netfn_lun;
  req->hdr_cksum = ZERO_CKSUM_CONST - req->hdr_cksum;

  req->req_slave_addr = IPMB_BMC_SLAVE_ADDR;
  req->seq_lun = 0x00;

  req->cmd = CMD_APP_GET_SELFTEST_RESULTS;

  tlen = 6;  // length so far with all fields above

  // Invoke IPMB library handler
  lib_ipmb_handle(ME_BUS_ADDR, tbuf, tlen+1, rbuf, &rlen);

#ifdef DEBUG_DIMM_UTIL
  int i = 0;
  DBG_PRINT("rlen = %d\n",rlen);
  for (i = 0; i < rlen; ++i)
    DBG_PRINT("0x%x ",rbuf[i]);
  DBG_PRINT("\n");
#endif

  if (rlen < min_resp_len) {
    return -1;
  }

  //  payload contains ME status (0x55, 0x00) and checksum
  //  ME status would be 3rd byte from last
  me_status = rbuf[min_resp_len - 3];
  DBG_PRINT("me_status 0x%x\n", me_status);

  if (me_status == 0x55) {
    return 0;
  } else {
    // bad ME status
    return -1;
  }
}
