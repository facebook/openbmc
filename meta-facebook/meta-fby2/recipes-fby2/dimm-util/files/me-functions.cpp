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
#include <facebook/bic.h>
#include "dimm-util.h"
#include "dimm-util-plat.h"

// #define DEBUG_DIMM_UTIL

#ifdef DEBUG_DIMM_UTIL
  #define DBG_PRINT(...) printf(__VA_ARGS__)
#else
  #define DBG_PRINT(...)
#endif

static uint8_t dev_addr[MAX_DIMM_NUM_FBY2/2] = {
      DIMM0_SPD_ADDR, DIMM1_SPD_ADDR,
      DIMM2_SPD_ADDR, DIMM3_SPD_ADDR,
};

// dimm location constant strings, matching silk screen
char const *dimm_label_fby2[NUM_CPU_FBY2][MAX_DIMM_NUM_FBY2] =
{{
  "A0",
  "A1",
  "B0",
  "B1",
  "D0",
  "D1",
  "E0",
  "E1",
}};

char const *fru_name_fby2[NUM_FRU_FBY2] =
{
  "slot1",
  "slot2",
  "slot3",
  "slot4",
  "all",
};

int
plat_init()
{
  num_frus  = NUM_FRU_FBY2;
  num_dimms_per_cpu = MAX_DIMM_NUM_FBY2;
  num_cpus  = NUM_CPU_FBY2;
  total_dimms = num_dimms_per_cpu * num_cpus;
  fru_name   = fru_name_fby2;
  fru_id_min = FRU_ID_MIN_FBY2;
  fru_id_max = FRU_ID_MAX_FBY2;
  fru_id_all = FRU_ID_ALL_FBY2;
  return 0;
}

const char *
get_dimm_label(uint8_t cpu, uint8_t dimm)
{
  if ((cpu >= NUM_CPU_FBY2)  || (dimm >= MAX_DIMM_NUM_FBY2)) {
    return "N/A";
  } else {
    return dimm_label_fby2[cpu][dimm];
  }
}


// send ME command to select page 0 or 1 on SPD
int
util_set_EE_page(uint8_t slot_id, uint8_t cpu, uint8_t dimm, uint8_t page_num)
{
#define EEPROM_MAX_PAGE 2
#define EEPROM_P0_SEL 0x6C
#define EEPROM_P1_SEL 0x6E
  uint8_t tbuf[256] = {MANU_FACEBOOK_0, MANU_FACEBOOK_1, MANU_FACEBOOK_2}; // IANA ID
  uint8_t rbuf[256] = {0};
  uint8_t page_sel[EEPROM_MAX_PAGE] = {EEPROM_P0_SEL, EEPROM_P1_SEL};
  uint8_t tlen = 0;
  uint8_t rlen = 0;
  uint8_t addr_msb = 0;
  int ret = 0;

  if (page_num >= EEPROM_MAX_PAGE)
    return -1;

  // calculate DIMM msb_addr
  //   msb_addr is 0 for dimms 0-3,  1 for 4-7
  if (dimm >= (MAX_DIMM_NUM_FBY2/2))
    addr_msb = 1;

  tlen = 3;
  tbuf[tlen++] = BIC_INTF_ME;

  /* Intel's IANA */
  tbuf[tlen++] = NETFN_NM_REQ << 2;  //BIC requires NETFN<<2
  tbuf[tlen++] = CMD_NM_WRITE_MEM_SM_BUS;

  /* Intel's IANA */
  tbuf[tlen++] = MANU_INTEL_0;
  tbuf[tlen++] = MANU_INTEL_1;
  tbuf[tlen++] = MANU_INTEL_2;

  tbuf[tlen++] = cpu;
  tbuf[tlen++] = addr_msb;
  tbuf[tlen++] = page_sel[page_num];
  tbuf[tlen++] = 0;
  tbuf[tlen++] = 0;
  tbuf[tlen++] = 0;

  ret = bic_ipmb_wrapper(slot_id, NETFN_OEM_1S_REQ, CMD_OEM_1S_MSG_OUT, tbuf,
    tlen, rbuf, &rlen);
  if (ret) {
    printf("ME no response!\n");
    return -1;
  }

  return ret;
}

// read 1 byte off SPD bus,
// input:  cpu/dimm/addr_msb/addr_lsb/offset
int
util_read_spd_byte(uint8_t slot_id, uint8_t cpu, uint8_t dimm, uint8_t offset)
{

// cmd goes from BMC -> BIC -> ME  -> BIC -> BMC
// total valid response length from BIC on  ME SPD bus read
//  includes  BIC IANA,  BIC Completion code,  ME IANA, ME completion code
//             NETFN, CMD,   and actual payload
#define MIN_RESP_LEN (INTEL_ID_LEN + 1 /* BIC CC*/ + FACEBOOK_ID_LEN + 1 /* BIC CC */ \
      + 1 /*NetFn */ + 1 /*cmd */ + 1 /*payload*/)

  int ret;
  uint8_t tbuf[256] = {MANU_FACEBOOK_0, MANU_FACEBOOK_1, MANU_FACEBOOK_2}; // IANA ID
  uint8_t rbuf[256] = {0};
  uint8_t tlen = 0;
  uint8_t rlen = 0;
  int addr_msb = 0;
  int addr_lsb = 0;

  // calculate DIMM msb_addr & lsb_addr
  //   msb_addr is 0 for dimms 0-3,  1 for 4-7
  if (dimm >= (MAX_DIMM_NUM_FBY2/2))
    addr_msb = 1;
  // assumes MAX_DIMM_NUM is multiple of 2
  addr_lsb = dev_addr[dimm % (MAX_DIMM_NUM_FBY2/2)];

  tlen = 3;
  tbuf[tlen++] = BIC_INTF_ME;

  /* Intel's IANA */
  tbuf[tlen++] = NETFN_NM_REQ << 2;  //BIC requires NETFN<<2
  tbuf[tlen++] = CMD_NM_READ_MEM_SM_BUS;

  /* Intel's IANA */
  tbuf[tlen++] = MANU_INTEL_0;
  tbuf[tlen++] = MANU_INTEL_1;
  tbuf[tlen++] = MANU_INTEL_2;

  tbuf[tlen++] = cpu;
  tbuf[tlen++] = addr_msb;
  tbuf[tlen++] = addr_lsb;
  tbuf[tlen++] = offset;
  tbuf[tlen++] = 0;

#ifdef DEBUG_DIMM_UTIL
  int i;
  DBG_PRINT("tlen = %d\n",tlen);
  for (i = 0; i < tlen; ++i)
    DBG_PRINT("0x%x ",tbuf[i]);
  DBG_PRINT("\n");
#endif

  ret = bic_ipmb_wrapper(slot_id, NETFN_OEM_1S_REQ, CMD_OEM_1S_MSG_OUT, tbuf,
    tlen, rbuf, &rlen);
  if (ret) {
    printf("ME no response!\n");
    return -1;
  }

#ifdef DEBUG_DIMM_UTIL
  DBG_PRINT("rlen = %d\n",rlen);
  for (i = 0; i < rlen; ++i)
    DBG_PRINT("0x%x ",rbuf[i]);
  DBG_PRINT("\n");
#endif

  if (rlen < MIN_RESP_LEN) {
    return -1;
  }

  // actual SPD payload will be last byte following all IANA header and
  //  completion codes
  return rbuf[MIN_RESP_LEN - 1];
}

int
util_check_me_status(uint8_t slot_id) {
#define MAX_CMD_RETRY 2
  int ret, retry = MAX_CMD_RETRY;
  uint8_t tbuf[256] = {0x00};
  uint8_t rbuf[256] = {0x00};
  uint8_t tlen = 0;
  uint8_t rlen = 0;

  tbuf[0] = NETFN_APP_REQ << 2;
  tbuf[1] = CMD_APP_GET_SELFTEST_RESULTS;
  tlen = 2;

  while (retry >= 0) {
    ret = bic_me_xmit(slot_id, tbuf, tlen, rbuf, &rlen);
    if (ret == 0)
      break;
    retry--;
  }
  if (ret) {
    DBG_PRINT("ME no response!\n");
    return -1;
  }

  //log[0] = 0;
  if (rbuf[0] != 0x00) {
    DBG_PRINT("Completion Code: %02X, ", rbuf[0]);
    return -1;
  }
  if (rlen < 3) {
    DBG_PRINT("return incomplete len=%d\n", rlen);
    return -1;
  }

  DBG_PRINT("%02x %02x\n", rbuf[1], rbuf[2]);
  if (rbuf[1] == 0x55) {
    return 0;
  } else {
    // bad ME status
    return -1;
  }
}
