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

// #define DEBUG_DIMM_UTIL

#ifdef DEBUG_DIMM_UTIL
  #define DBG_PRINT(fmt, args...) printf(fmt, ##args)
#else
  #define DBG_PRINT(fmt, args...)
#endif


// dimm location constant strings, matching silk screen
static char *dimm_location[] =
{
  "A0",
  "A1",
  "B0",
  "B1",
  "D0",
  "D1",
  "E0",
  "E1",
  "All"
};

// dimm type decode table
static char *dimm_type[] =
{
  "n/a",
  "Fast Page Mode",
  "EDO",
  "Pipelined Mode",
  "SDRAM",
  "ROM",
  "DDR SGRAM",
  "DDR SDRAM",
  "DDR2 SDRAM",
  "DDR2 SDRAM FB_DIMM",
  "DDR2 SDRAM FB_DIMM PROBE",
  "DDR3 SDRAM",
  "DDR4 SDRAM",
  "n/a",
  "DDR4E SDRAM",
  "LPDDR3 SDRAM",
  "LPDDR4 SDRAM",
};

// dimm speed decode table
static char *speed_type[] =
{
  "n/a",
  "n/a",
  "n/a",
  "n/a",
  "n/a",
  "3200", //5
  "2666",
  "2400",
  "2133",
  "1866",
  "1600",
};

/* i2c slave address for DIMMs on the SPD bus*/
static uint8_t dev_addr[4] = {DIMM0_SPD_ADDR, DIMM1_SPD_ADDR,
      DIMM2_SPD_ADDR, DIMM3_SPD_ADDR};


static
const char * dimm_type_string(uint8_t id)
{
  if ((id < 0) ||
      (id >= ARRAY_SIZE(dimm_type)) ||
      (dimm_type[id] == NULL)) {
    return "Unknown";
  } else {
    return dimm_type[id];
  }
}

static
const char * dimm_speed_string(uint8_t id)
{
  if ((id < 0) ||
      (id >= ARRAY_SIZE(speed_type)) ||
      (speed_type[id] == NULL)) {
    return "Unknown";
  } else {
    return speed_type[id];
  }
}

// send ME command to select page 0 or 1 on SPD
static int
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
  if (dimm >= (MAX_DIMM_NUM/2))
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
static int
util_read_spd_byte(uint8_t slot_id, uint8_t cpu, uint8_t addr_msb, uint8_t addr_lsb,
              uint8_t offset)
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

// read multiple bytes of SPD (offset, len),  with retry on failure
//
// input
//      slot_id, dimm, offset, len
//      early_exit_cnt - value
//            if more than early_exit_cnt  number of 0 are read,
//                 do not read whole length and exit
//            0 to disable this check
//
// output returned in buf
// also returns a special flag  "present"
//         1 - if buf contains non zero
//         0 - if all data in buf are 0
static int
util_read_spd_with_retry(uint8_t slot_id, uint8_t dimm, uint16_t offset, uint16_t len,
                  uint16_t early_exit_cnt, uint8_t *buf, uint8_t *present) {
  uint16_t j, fail_cnt = 0;
  uint8_t retry = 0;
  int value = 0;
  int addr_msb = 0;

  // calculate DIMM msb_addr
  //   msb_addr is 0 for dimms 0-3,  1 for 4-7
  if (dimm >= (MAX_DIMM_NUM/2))
    addr_msb = 1;

  *present = 0;
  for (j = 0; j < len; ++j) {
    retry = 0;
    while (retry < MAX_RETRY) {
      value = util_read_spd_byte(slot_id, 0, addr_msb, dev_addr[dimm & 0x03], offset + j);
      if (value >= 0)
        break;
      retry++;
    }
    if (value >= 0) {
      buf[j] = value;
      *present = 1;
    } else {
      // only consider early exit if it's non-0
      if (early_exit_cnt) {
        fail_cnt ++;
        if (fail_cnt == early_exit_cnt) {
          *present = 0;
          return -1;
        }
      }
    }
  }

  return 0;
}

static void
set_dimm_loop(uint8_t dimm, uint8_t *startDimm, uint8_t *endDimm) {
  if (dimm == MAX_DIMM_NUM) {
    *startDimm = 0;
    *endDimm   = MAX_DIMM_NUM;
  } else {
    *startDimm = dimm;
    *endDimm   = dimm + 1;
  }
}

static int
util_get_serial(uint8_t slot_id, uint8_t dimm, bool json) {
  uint8_t i, j, startDimm, endDimm, dimm_present = 0;
  uint8_t dimm_serial[MAX_DIMM_NUM][LEN_SERIAL] = {0};
  json_t *config_arr = NULL;
  char   sn[LEN_SERIAL_STRING] = {0};

  if (json) {
    config_arr = json_array();
    if (!config_arr)
      goto err_exit;
  } else {
    printf("Slot%d\n", slot_id);
  }

  set_dimm_loop(dimm, &startDimm, &endDimm);
  for (i = startDimm; i < endDimm; ++i) {
    util_set_EE_page(slot_id, 0, i, 1);
    util_read_spd_with_retry(slot_id, i, OFFSET_SERIAL, LEN_SERIAL, 0,
      dimm_serial[i], &dimm_present);

    if (dimm_present)
        for (j = 0; j < LEN_SERIAL; ++j)
          snprintf(sn + (2 * j), LEN_SERIAL_STRING - (2 * j), "%02X", dimm_serial[i][j]);

    if (json) {
      json_t *sn_obj = json_object();
      if (!sn_obj)
        goto err_exit;
      if (dimm_present)
        json_object_set_new(sn_obj, "Serial Number", json_string(sn));
      json_t *dimm_obj = json_object();
      if (!dimm_obj)
        goto err_exit;
      json_object_set_new(dimm_obj, "Slot", json_integer(slot_id));
      json_object_set_new(dimm_obj, "DIMM", json_string(dimm_location[i]));
      json_object_set_new(dimm_obj, "config", sn_obj);
      json_array_append(config_arr, dimm_obj);
    } else {
      printf("DIMM %s Serial Number: ", dimm_location[i]);
      if (dimm_present)
        printf("%s\n", sn);
      else
        printf("NO DIMM\n");
    }
  }
  if (json) {
    json_dumpf(config_arr, stdout, JSON_INDENT(4));
    json_decref(config_arr);
    printf("\n");
  }
  return 0;

err_exit:
  if (json) {
    if (config_arr)
      json_decref(config_arr);
  }
  return -1;
}

static int
util_get_part(uint8_t slot_id, uint8_t dimm, bool json) {
  uint8_t i, j, startDimm, endDimm, dimm_present = 0;
  uint8_t dimm_part[MAX_DIMM_NUM][LEN_PART_NUMBER] = {0};
  json_t *config_arr = NULL;
  char   pn[LEN_PN_STRING] = {0};

  if (json) {
    config_arr = json_array();
    if (!config_arr)
      goto err_exit;
  } else {
    printf("Slot%d\n", slot_id);
  }

  set_dimm_loop(dimm, &startDimm, &endDimm);
  for (i = startDimm; i < endDimm; ++i) {
    util_set_EE_page(slot_id, 0, i, 1);
    util_read_spd_with_retry(slot_id, i, OFFSET_PART_NUMBER, LEN_PART_NUMBER,
      MAX_FAIL_CNT, dimm_part[i], &dimm_present);

    if (dimm_present)
        for (j = 0; j < LEN_PART_NUMBER; ++j)
          snprintf(pn + j, LEN_PART_NUMBER - j, "%c", dimm_part[i][j]);

    if (json) {
      json_t *part_obj = json_object();
      if (!part_obj)
        goto err_exit;
      if (dimm_present)
        json_object_set_new(part_obj, "Part Number", json_string(pn));
      json_t *dimm_obj = json_object();
      if (!dimm_obj)
        goto err_exit;
      json_object_set_new(dimm_obj, "Slot", json_integer(slot_id));
      json_object_set_new(dimm_obj, "DIMM", json_string(dimm_location[i]));
      json_object_set_new(dimm_obj, "config", part_obj);
      json_array_append(config_arr, dimm_obj);
    } else {
      printf("DIMM %s Part Number: ", dimm_location[i]);
      if (dimm_present)
        printf("%s\n", pn);
      else
        printf("NO DIMM\n");
    }
  }
  if (json) {
    json_dumpf(config_arr, stdout, JSON_INDENT(4));
    json_decref(config_arr);
    printf("\n");
  }
  return 0;

err_exit:
  if (json) {
    if (config_arr)
      json_decref(config_arr);
  }
  return -1;
}

static int
util_get_raw_dump(uint8_t slot_id, uint8_t dimm, bool json) {
  uint8_t i, startDimm, endDimm, dimm_present = 0;
  uint16_t j = 0;
  uint16_t offset = DEFAULT_DUMP_OFFSET;
  uint16_t len = DEFAULT_DUMP_LEN;
  uint8_t buf[DEFAULT_DUMP_LEN] = {0};

  printf("Raw Dump, slot%d dimm=%s, offset=0x%x, len=0x%x\n",
    slot_id, dimm_location[dimm], offset + 0x100, len);

  set_dimm_loop(dimm, &startDimm, &endDimm);
  for (i = startDimm; i < endDimm; ++i) {
    memset(buf, 0, DEFAULT_DUMP_LEN);
    util_read_spd_with_retry(slot_id, i, DEFAULT_DUMP_OFFSET, DEFAULT_DUMP_LEN,
      0, buf, &dimm_present);

    printf("DIMM %s \n", dimm_location[i]);
    printf("0x%x: ", offset + 0x100);
    for (j = 0; j < DEFAULT_DUMP_LEN; ++j) {
      printf("0x%02x ", buf[j]);
      if (((j & 0x0f) == 0x0f))
        printf("\n0x%x: ", offset + j + 0x101);
    }
    printf("\n");
  }
  return 0;
}

static int
util_get_cache(uint8_t slot_id, uint8_t dimm, bool json) {
#define MAX_KEY_SIZE 100
#define MAX_VALUE_SIZE 128
  uint8_t i, startDimm, endDimm = 0;
  size_t len;
  char key[MAX_KEY_SIZE] = {0};
  char value[MAX_VALUE_SIZE] = {0};

  printf("Slot%d\n", slot_id);

  set_dimm_loop(dimm, &startDimm, &endDimm);
  for (i = startDimm; i < endDimm; ++i) {
    printf("DIMM %s Serial Number (cached): ", dimm_location[i]);
    snprintf(key, MAX_KEY_SIZE, "sys_config/fru%d_dimm%d_serial_num", slot_id, i);
    if(kv_get(key, (char *)value, &len, KV_FPERSIST) < 0) {
      printf("NO DIMM");
    } else {
      for (int j = 0; ((j < LEN_SERIAL) && value[j]); ++j)
        printf("%X", value[j]);
    }
    printf("\n");

    printf("DIMM %s Part Number   (cached): ", dimm_location[i]);
    snprintf(key, MAX_KEY_SIZE, "sys_config/fru%d_dimm%d_part_name", slot_id, i);
    if(kv_get(key, (char *)value, &len, KV_FPERSIST) < 0) {
      printf("NO DIMM");
    } else {
      for (int j = 0; ((j < LEN_PART_NUMBER) && value[j]); ++j)
        printf("%c", value[j]);
    }
    printf("\n");
  }

  return 0;
}

// print DIMM Config
// PN, SN + vendor, manufacture_date, size, speed, clock speed.
static int
util_get_config(uint8_t slot_id, uint8_t dimm, bool json) {
// page 0 constants
#define P0_OFFSET   0
#define P0_LEN      20
#define TYPE_OFFSET 2
#define MIN_CYCLE_TIME_OFFSET 18
// page 1 constants
#define P1_OFFSET 0x40
#define P1_LEN    0x30
#define MANUFACTURER_OFFSET 1
#define DATE_OFFSET 3
#define SERIAL_OFFSET 5
#define PN_OFFSET 9
#define BUF_SIZE 64
  uint8_t i, j, startDimm, endDimm, dimm_present = 0;
  uint8_t buf[BUF_SIZE] = {0};
  json_t *config_arr = NULL;
  char   pn[LEN_PN_STRING] = {0};
  char   sn[LEN_SERIAL_STRING] = {0};
  char   manu[BUF_SIZE] = {0};
  char   week[BUF_SIZE] = {0};
  char   size[BUF_SIZE] = {0};
  uint8_t dimm_type = 0;
  uint8_t mincycle = 0;

  if (json) {
    config_arr = json_array();
    if (!config_arr)
      goto err_exit;
  }

  set_dimm_loop(dimm, &startDimm, &endDimm);
  for (i = startDimm; i < endDimm; ++i) {
    util_set_EE_page(slot_id, 0, i, 0);
    // read page 0 to get type, speed, capacity
    memset(buf, 0, BUF_SIZE);
    util_read_spd_with_retry(slot_id, i, P0_OFFSET, P0_LEN, 0, buf, &dimm_present);
    if (dimm_present) {
      dimm_type = buf[TYPE_OFFSET];
      mincycle  = buf[MIN_CYCLE_TIME_OFFSET];

      // read page 1 to get pn, sn, manufacturer, manufacturer week
      util_set_EE_page(slot_id, 0, i, 1);
      memset(buf, 0, BUF_SIZE);
      util_read_spd_with_retry(slot_id, i, P1_OFFSET, P1_LEN, 0, buf, &dimm_present);
      if (dimm_present) {
          for (j = 0; j < LEN_PART_NUMBER; ++j) {
            snprintf(pn + j, LEN_PART_NUMBER - j, "%c", buf[PN_OFFSET + j]);
          }
          for (j = 0; j < LEN_SERIAL; ++j) {
            snprintf(sn + (2 * j), LEN_SERIAL_STRING - (2 * j), "%02X", buf[SERIAL_OFFSET + j]);
          }
          snprintf(manu, BUF_SIZE, "%s", manu_string(buf[MANUFACTURER_OFFSET]));
          snprintf(week, BUF_SIZE, "20%02x Week%02x",
                    buf[DATE_OFFSET], buf[DATE_OFFSET + 1]);
      }
    }


    if (json) {
      json_t *config_obj = json_object();
      if (!config_obj)
        goto err_exit;
      if (dimm_present) {
        json_object_set_new(config_obj, "Size", json_string(size));
        json_object_set_new(config_obj, "Type", json_string(dimm_type_string(dimm_type)));
        json_object_set_new(config_obj, "Speed", json_string(dimm_speed_string(mincycle)));
        json_object_set_new(config_obj, "Manufacturer", json_string(manu));
        json_object_set_new(config_obj, "Serial Number", json_string(sn));
        json_object_set_new(config_obj, "Part Number", json_string(pn));
        json_object_set_new(config_obj, "Manufacturing Date", json_string(week));
      }
      json_t *dimm_obj = json_object();
      if (!dimm_obj)
        goto err_exit;
      json_object_set_new(dimm_obj, "Slot", json_integer(slot_id));
      json_object_set_new(dimm_obj, "DIMM", json_string(dimm_location[i]));
      json_object_set_new(dimm_obj, "config", config_obj);
      json_array_append(config_arr, dimm_obj);
    } else {
      printf("CONFIG, slot%d\n", slot_id);
      printf("DIMM %s:\n", dimm_location[i]);
      if (dimm_present) {
        printf("\tSize: %s\n", size);
        printf("\tType: %s\n", dimm_type_string(dimm_type));
        printf("\tSpeed: %s\n", dimm_speed_string(mincycle));
        printf("\tManufacturer: %s\n", manu);
        printf("\tSerial Number: %s\n", sn);
        printf("\tPart Number: %s\n", pn);
        printf("\tManufacturing Date: %s\n", week);
      } else {
        printf("\tNO DIMM\n");
      }
    }
  }
  if (json) {
    json_dumpf(config_arr, stdout, JSON_INDENT(4));
    json_decref(config_arr);
    printf("\n");
  }
  return 0;

err_exit:
  if (json) {
    if (config_arr)
      json_decref(config_arr);
  }
  return -1;
}

static int parse_cmdline_args(int argc, char **argv,
			      uint8_t *dimm, bool *json)
{
  int ret, opt_index = 0;
  static const char *optstring = "d:j";
  struct option long_opts[] = {
    {"dimm",	required_argument,		NULL,	'd'},
    {"json",	no_argument,		NULL,	'j'},
    {NULL,		0,			NULL,	0},
  };

  *dimm = MAX_DIMM_NUM;
  *json = false;
  while (1) {
    opt_index = 0;
    ret = getopt_long(argc, argv, optstring,
          long_opts, &opt_index);
    if (ret == -1)
      break;	/* end of arguments */

    switch (ret) {
    case 'd':
      *dimm = atoi(optarg);
      if ((*dimm < 0) || (*dimm > MAX_DIMM_NUM)) {
        printf("Error: invalid [--dimm]\n");
        return ERR_INVALID_SYNTAX;
      }
      break;
    case 'j':
      *json = true;
      break;
    default:
      return ERR_INVALID_SYNTAX;
    }
  }

  return 0;
}

static int
parse_arg_and_exec(int argc, char **argv, uint8_t slot_id,
                  int (*pF)(uint8_t, uint8_t, bool)) {
  uint8_t dimm = MAX_DIMM_NUM;
  bool    json = false;
  int ret = 0;

  if (argc < 3) {
    printf("Error: invalid number of arguments\n");
    return ERR_INVALID_SYNTAX;
  } else if (argc > 3) {
    // parse option fields,
    // skipping first 3 arguments, which will be "dimm-util slotX cmd"
  	if (parse_cmdline_args(argc - 2, &(argv[2]), &dimm, &json) != 0)
  		return ERR_INVALID_SYNTAX;
  }

  if (slot_id == SLOT_ALL) {
    for (int i = SLOT_MIN; i <= SLOT_MAX; ++i) {
      ret = pF(i, dimm, json);
    }
  } else {
    ret = pF(slot_id, dimm, json);
  }

  return ret;
}

static void
print_usage_help(void) {
  printf("Usage: dimm-util <slot1|slot2|slot3|slot4|all> <cmd> [opt1] [opt2] ...\n");
  printf("\ncmd list:\n");
  printf("   --serial   - get DIMM module serial number\n");
  printf("   --part     - get DIMM module part number\n");
  printf("   --raw      - dump raw SPD data\n");
  printf("   --cache    - read from SMBIOS cache\n");
  printf("   --config   - print DIMM configuration info\n");
  printf("\nopt list:\n");
  printf("  [--dimm N]  - DIMM number (0..%d) [default %d = ALL]\n",
          MAX_DIMM_NUM - 1 , MAX_DIMM_NUM);
  printf("  [--json]    - output in JSON format\n");
}

int
main(int argc, char **argv) {
  uint8_t slot_id;
  int ret = 0;

  if (argc < 3)
    goto err_exit;

  if (!strcmp(argv[1], "slot1")) {
    slot_id = 1;
  } else if (!strcmp(argv[1] , "slot2")) {
    slot_id = 2;
  } else if (!strcmp(argv[1] , "slot3")) {
    slot_id = 3;
  } else if (!strcmp(argv[1] , "slot4")) {
    slot_id = 4;
  } else if (!strcmp(argv[1] , "all")) {
    slot_id = SLOT_ALL;
  } else {
    goto err_exit;
  }

  if (!strcmp(argv[2], "--serial")) {
    ret = parse_arg_and_exec(argc, argv, slot_id, &util_get_serial);
  } else if (!strcmp(argv[2], "--part")) {
    ret = parse_arg_and_exec(argc, argv, slot_id, &util_get_part);
  } else if (!strcmp(argv[2], "--raw")) {
    ret = parse_arg_and_exec(argc, argv, slot_id, &util_get_raw_dump);
  } else if (!strcmp(argv[2], "--cache")) {
    ret = parse_arg_and_exec(argc, argv, slot_id, &util_get_cache);
  } else if (!strcmp(argv[2], "--config")) {
    ret = parse_arg_and_exec(argc, argv, slot_id, &util_get_config);
  } else {
    goto err_exit;
  }

  if (ret == ERR_INVALID_SYNTAX)
    goto err_exit;

  return ret;

err_exit:
  print_usage_help();
  return ERR_INVALID_SYNTAX;
}
