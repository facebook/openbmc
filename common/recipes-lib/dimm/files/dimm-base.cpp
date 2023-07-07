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
#include "dimm.h"


#ifdef DEBUG_DIMM_BASE
  #define DBG_PRINT(...) printf(__VA_ARGS__)
#else
  #define DBG_PRINT(...)
#endif


// dimm type decode table
#define MIN_DDR5_TYPE 18  // note: dimm_type[18]
static char const *dimm_type[] =
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
  "LPDDR4X SDRAM",
  "DDR5 SDRAM",
  "LPDDR5 SDRAM",
  "DDR5 NVDIMM-P",
};

// dimm speed decode table
static char const *speed_type[] =
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

// global variables on various platform specific attributes
//  to be initialized by platform-specific plat_init() function
int num_frus = 0;
int num_cpus = 0;
int num_dimms_per_cpu = 0;
int total_dimms = 0;
char const **fru_name = NULL;     // pointer to array of string
int fru_id_all = 0;
int fru_id_min = 0;
int fru_id_max = 0;
bool read_block = false;
int max_retries = DEFAULT_RETRIES;
int retry_intvl = DEFAULT_RETRY_INTVL_MSEC;

static
const char * dimm_type_string(uint8_t id)
{
  if (id >= ARRAY_SIZE(dimm_type) || dimm_type[id] == NULL) {
    return "Unknown";
  } else {
    return dimm_type[id];
  }
}

static
const char * dimm_speed_string(uint8_t id)
{
  if (id >= ARRAY_SIZE(speed_type) || speed_type[id] == NULL) {
    return "Unknown";
  } else {
    return speed_type[id];
  }
}

// Check if ME is in operational state (e.g. status 0x55)
// input:  fru_id
int __attribute__((weak))
util_check_me_status(uint8_t /*fru_id*/)
{
  return -1;
}

// send ME command to select page 0 or 1 on SPD
int __attribute__((weak))
util_set_EE_page(uint8_t /*fru_id*/, uint8_t /*cpu*/, uint8_t /*dimm*/, uint8_t /*page_num*/)
{
  return -1;
}

// read 1 byte off SPD bus,
// input:  fru_id/cpu/dimm/offset
int __attribute__((weak))
util_read_spd_byte(uint8_t /*fru_id*/, uint8_t /*cpu*/, uint8_t /*dimm*/, uint8_t /*offset*/)
{
  return -1;
}

int __attribute__((weak))
util_read_spd(uint8_t /*fru_id*/, uint8_t /*cpu*/, uint8_t /*dimm*/, uint16_t /*offset*/, uint8_t /*len*/, uint8_t * /*rxbuf*/)
{
  return -1;
}

// allows each platform to populate cpu num, dimm num, num frus
int __attribute__((weak))
plat_init(void)
{
  int ret = -1;  // to avoid cppcheck report [knownConditionTrueFalse]
  return ret;
}

//  get DIMM label, e.g. DIMMA0,DIMMA1 etc
const char * __attribute__((weak))
get_dimm_label(uint8_t /*cpu*/, uint8_t /*dimm*/)
{
  return "N/A";
}

uint8_t __attribute__((weak))
get_dimm_cache_id(uint8_t cpu, uint8_t dimm)
{
  return (cpu * num_dimms_per_cpu + dimm);
}

bool __attribute__((weak))
is_pmic_supported(void)
{
  return false;
}

int __attribute__((weak))
util_read_pmic(uint8_t /*fru_id*/, uint8_t /*cpu*/, uint8_t /*dimm*/, uint8_t /*offset*/, uint8_t /*len*/, uint8_t * /*rxbuf*/)
{
  return -1;
}

int __attribute__((weak))
util_write_pmic(uint8_t /*fru_id*/, uint8_t /*cpu*/, uint8_t /*dimm*/, uint8_t /*offset*/, uint8_t /*len*/, uint8_t * /*txbuf*/)
{
  return -1;
}

// read multiple bytes of SPD (offset, len),  with retry on failure
//
// input
//      fru_id, dimm, offset, len
//      early_exit_cnt - value
//            if more than early_exit_cnt  number of 0 are read,
//                 do not read whole length and exit
//            0 to disable this check
//
// output returned in buf
// also returns a special flag  "present"
//         1 - if buf contains non zero
//         0 - if all data in buf are 0
int
util_read_spd_with_retry(uint8_t fru_id, uint8_t cpu, uint8_t dimm, uint16_t offset, uint16_t len,
                  uint16_t early_exit_cnt, uint8_t *buf, uint8_t *present) {
  uint16_t j, fail_cnt = 0;
  uint8_t retry = 0, rlen = 1;
  int value = 0;

  *present = DIMM_EMPTY;
  for (j = 0; j < len;) {
    retry = 0;
    for (retry = 0; retry <= max_retries; retry++) {
      if (read_block == true) {
        rlen = ((len - j) < MAX_DIMM_SMB_XFER_LEN) ? (len - j) : MAX_DIMM_SMB_XFER_LEN;
        value = util_read_spd(fru_id, cpu, dimm, offset + j, rlen, &buf[j]);
      } else {
        value = util_read_spd_byte(fru_id, cpu, dimm, offset + j);
      }
      if ((value >= 0) || (retry == max_retries)) {
        break;
      }
      usleep(retry_intvl * 1000);
    }
    if ((read_block == true) && (value <= 0)) {
      *present = DIMM_EMPTY;
      return -1;
    } else if (value >= 0) {
      if (read_block == false) {
        buf[j] = value;
      }
      *present = DIMM_PRESENT;
    } else {
      // only consider early exit if it's non-0
      if (early_exit_cnt) {
        fail_cnt ++;
        if (fail_cnt == early_exit_cnt) {
          *present = DIMM_EMPTY;
          return -1;
        }
      }
    }
    j += rlen;
  }

  return 0;
}


// convert system dimm number to  (cpu, dimm) pair
//     eg.   on a 2-socket system with 24 dimms (0-23)
//               dimms 0-11 would be on cpu 0
//               dimms 12-23 would be on cpu 1
//          so dimm 14 would be (cpu=1, dimm=3)
//
//    Special case if user specify max dimm (e.g. 24),
//       then loop through all CPUs and dimms
static void
set_dimm_loop(uint8_t dimm, uint8_t *startCPU, uint8_t *endCPU,
              uint8_t *startDimm, uint8_t *endDimm) {
  if (dimm == total_dimms) {
    // special case, loop through All CPUs and DIMMs
    *startCPU  = 0;
    *endCPU    = num_cpus;
    *startDimm = 0;
    *endDimm   = num_dimms_per_cpu;
  } else {
    // normal case, select a (cpu, dimm) pair
    *startCPU   = dimm / num_dimms_per_cpu;
    *endCPU     = *startCPU + 1;
    *startDimm  = dimm % num_dimms_per_cpu;
    *endDimm    = *startDimm + 1;
  }
  DBG_PRINT("%s, %d  %d %d %d %d\n", __FUNCTION__,
              dimm, *startCPU, *endCPU, *startDimm, *endDimm);
}

static int
check_dimm_type_ddr5(uint8_t fru_id, uint8_t cpu, uint8_t dimm, uint8_t *type) {
  uint8_t dev_type, dimm_present;

  util_set_EE_page(fru_id, cpu, dimm, 0);
  util_read_spd_with_retry(fru_id, cpu, dimm, OFFSET_TYPE, 1, 0, &dev_type, &dimm_present);
  if (!dimm_present) {
    return -1;
  }

  if (type) {
    *type = dev_type;
  }
  if (dev_type >= MIN_DDR5_TYPE) {
    return 0;
  }

  return 1;  // DDR4
}

static int
get_serial(uint8_t fru_id, uint8_t cpu, uint8_t dimm, char *sn_str) {
  uint8_t dimm_present, i, buf[LEN_SERIAL] = {0};
  uint16_t offset = OFFSET_SERIAL;
  int ret;

  ret = check_dimm_type_ddr5(fru_id, cpu, dimm, NULL);
  if (!ret) {
    offset = OFFSET_SPD5_DIMM_SN;
  } else {
    if (ret < 0) {
      snprintf(sn_str, LEN_SERIAL_STRING, "Unknown");
      return -1;
    }
  }

  util_set_EE_page(fru_id, cpu, dimm, 1);
  util_read_spd_with_retry(fru_id, cpu, dimm, offset, LEN_SERIAL, 0, buf, &dimm_present);
  if (!dimm_present) {
    snprintf(sn_str, LEN_SERIAL_STRING, "Unknown");
    return -1;
  }

  for (i = 0; i < LEN_SERIAL; i++) {
    snprintf(sn_str + (2*i), LEN_SERIAL_STRING - (2*i), "%02X", buf[i]);
  }

  return 0;
}

int
util_get_serial(uint8_t fru_id, uint8_t dimm, bool json, uint8_t* options) {
  uint8_t i, cpu, startCPU, endCPU, startDimm, endDimm;
  int ret;
  json_t *config_arr = NULL;
  char   sn[LEN_SERIAL_STRING] = {0};

  if (options == NULL) {
    return ERR_INVALID_SYNTAX;
  }
  if (json) {
    config_arr = json_array();
    if (!config_arr)
      goto err_exit;
  } else {
    printf("FRU: %s\n", fru_name[fru_id - 1]);
  }

  set_dimm_loop(dimm, &startCPU, &endCPU, &startDimm, &endDimm);
  for (cpu = startCPU; cpu < endCPU; cpu++) {
    for (i = startDimm; i < endDimm; ++i) {
      ret = get_serial(fru_id, cpu, i, sn);
      if (json) {
        json_t *sn_obj = json_object();
        if (!sn_obj)
          goto err_exit;
        if (ret == 0)
          json_object_set_new(sn_obj, "Serial Number", json_string(sn));
        json_t *dimm_obj = json_object();
        if (!dimm_obj) {
          json_decref(sn_obj);
          goto err_exit;
        }
        json_object_set_new(dimm_obj, "FRU", json_string(fru_name[fru_id - 1]));
        json_object_set_new(dimm_obj, "DIMM", json_string(get_dimm_label(cpu,i)));
        json_object_set_new(dimm_obj, "config", sn_obj);
        json_array_append_new(config_arr, dimm_obj);
      } else {
        printf("DIMM %s Serial Number: ", get_dimm_label(cpu,i));
        if (ret == 0)
          printf("%s\n", sn);
        else
          printf("NO DIMM\n");
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

static int
get_part(uint8_t fru_id, uint8_t cpu, uint8_t dimm, char *pn_str) {
  uint8_t dimm_present, i, buf[LEN_SPD5_DIMM_PN] = {0};
  uint8_t len = LEN_PART_NUMBER;
  uint16_t offset = OFFSET_PART_NUMBER;
  int ret;

  ret = check_dimm_type_ddr5(fru_id, cpu, dimm, NULL);
  if (!ret) {
    offset = OFFSET_SPD5_DIMM_PN;
    len = LEN_SPD5_DIMM_PN;
  } else {
    if (ret < 0) {
      snprintf(pn_str, LEN_PN_STRING, "Unknown");
      return -1;
    }
  }

  util_set_EE_page(fru_id, cpu, dimm, 1);
  util_read_spd_with_retry(fru_id, cpu, dimm, offset, len, MAX_FAIL_CNT, buf, &dimm_present);
  if (!dimm_present) {
    snprintf(pn_str, LEN_PN_STRING, "Unknown");
    return -1;
  }

  for (i = 0; i < len; i++) {
    snprintf(pn_str + i, LEN_PN_STRING - i, "%c", buf[i]);
  }

  return 0;
}

int
util_get_part(uint8_t fru_id, uint8_t dimm, bool json, uint8_t* options) {
  uint8_t i, cpu, startCPU, endCPU, startDimm, endDimm;
  int ret;
  json_t *config_arr = NULL;
  char   pn[LEN_PN_STRING] = {0};

  if (options == NULL) {
    return ERR_INVALID_SYNTAX;
  }
  if (json) {
    config_arr = json_array();
    if (!config_arr)
      goto err_exit;
  } else {
    printf("FRU: %s\n", fru_name[fru_id - 1]);
  }

  set_dimm_loop(dimm, &startCPU, &endCPU, &startDimm, &endDimm);
  for (cpu = startCPU; cpu < endCPU; cpu++) {
    for (i = startDimm; i < endDimm; ++i) {
      ret = get_part(fru_id, cpu, i, pn);
      if (json) {
        json_t *part_obj = json_object();
        if (!part_obj)
          goto err_exit;
        if (ret == 0)
          json_object_set_new(part_obj, "Part Number", json_string(pn));
        json_t *dimm_obj = json_object();
        if (!dimm_obj) {
          json_decref(part_obj);
          goto err_exit;
        }
        json_object_set_new(dimm_obj, "FRU", json_string(fru_name[fru_id - 1]));
        json_object_set_new(dimm_obj, "DIMM", json_string(get_dimm_label(cpu,i)));
        json_object_set_new(dimm_obj, "config", part_obj);
        json_array_append_new(config_arr, dimm_obj);
      } else {
        printf("DIMM %s Part Number: ", get_dimm_label(cpu,i));
        if (ret == 0)
          printf("%s\n", pn);
        else
          printf("NO DIMM\n");
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

int
util_get_raw_dump(uint8_t fru_id, uint8_t dimm, bool /*json*/, uint8_t* options) {
  uint8_t i, page, cpu, startCPU, endCPU, startDimm, endDimm, dimm_present = 0;
  uint8_t page_num = 2;
  uint8_t buf[SPD5_DUMP_LEN] = {0};
  uint16_t j = 0;
  uint16_t offset = DEFAULT_DUMP_OFFSET, dump_len = DEFAULT_DUMP_LEN;
  int ret;

  if (options == NULL) {
    return ERR_INVALID_SYNTAX;
  }
  printf("FRU: %s\n", fru_name[fru_id - 1]);
  set_dimm_loop(dimm, &startCPU, &endCPU, &startDimm, &endDimm);
  for (cpu = startCPU; cpu < endCPU; cpu++) {
    for (i = startDimm; i < endDimm; ++i) {
      printf("DIMM %s:\n", get_dimm_label(cpu, i));
      ret = check_dimm_type_ddr5(fru_id, cpu, i, NULL);
      if (!ret) {
        page_num = 1;
        dump_len = SPD5_DUMP_LEN;
      } else {
        if (ret < 0) {
          printf("\tNO DIMM\n");
          continue;
        }
      }

      for (page = 0; page < page_num; page++) {
        memset(buf, 0, dump_len);
        util_set_EE_page(fru_id, cpu, i, page);
        util_read_spd_with_retry(fru_id, cpu, i, DEFAULT_DUMP_OFFSET, dump_len,
          0, buf, &dimm_present);
        printf("%03x: ", offset + (page * 0x100));
        for (j = 0; j < dump_len; ++j) {
          printf("%02x ", buf[j]);
          if ((j & 0x0f) == 0x0f) {
            if (j == (dump_len - 1))
              printf("\n");
            else
              printf("\n%03x: ", offset + j + (page * 0x100) + 1);
          }
        }
      }
    }
    printf("\n");
  }

  return 0;
}

int
util_get_cache(uint8_t fru_id, uint8_t dimm, bool /*json*/, uint8_t* options) {
#define MAX_KEY_SIZE 100
#define MAX_VALUE_SIZE 128
  uint8_t i, cpu, dimmNum, startCPU, endCPU, startDimm, endDimm = 0;
  size_t len;
  char key[MAX_KEY_SIZE] = {0};
  char value[MAX_VALUE_SIZE] = {0};

  if (options == NULL) {
    return ERR_INVALID_SYNTAX;
  }
  set_dimm_loop(dimm, &startCPU, &endCPU, &startDimm, &endDimm);
  for (cpu = startCPU; cpu < endCPU; cpu++) {
    for (i = startDimm; i < endDimm; ++i) {
      dimmNum = get_dimm_cache_id(cpu, i);
      printf("DIMM %s Serial Number (cached): ", get_dimm_label(cpu, i));
      snprintf(key, MAX_KEY_SIZE, "sys_config/fru%d_dimm%d_serial_num", fru_id, dimmNum);
      if (kv_get(key, (char *)value, &len, KV_FPERSIST) < 0) {
        printf("NO DIMM");
      } else {
        for (int j = 0; j < LEN_SERIAL; ++j)
          printf("%02X", value[j]);
      }
      printf("\n");

      printf("DIMM %s Part Number   (cached): ", get_dimm_label(cpu, i));
      snprintf(key, MAX_KEY_SIZE, "sys_config/fru%d_dimm%d_part_name", fru_id, dimmNum);
      if (kv_get(key, (char *)value, &len, KV_FPERSIST) < 0) {
        printf("NO DIMM");
      } else {
        for (int j = 0; ((j < LEN_SPD5_DIMM_PN) && value[j]); ++j)
          printf("%c", value[j]);
      }
      printf("\n");
    }
  }

  return 0;
}

// calculate total capacity of DIMM module
// based on SPD info
//
//  input:
//      char *size_str - pre-allocated, 0-filled string for capacity
//      uint8_t size_str_len - total buffer size of the above string
//      uint8_t *buf - Page 0 of SPD buffer.
//                     this function only uses offset0 to 13
static void
util_get_size(char *size_str, uint8_t size_str_len, uint8_t *buf) {

#define CAP_OFFSET 0x4
#define CAP_MASK   0xF
#define BUS_WIDTH_OFFSET 13
#define BUS_WIDTH_MASK 0x7
#define SDRAM_WIDTH_OFFSET 12
#define SDRAM_WIDTH_MASK 0x7
#define RANKS_OFFSET 12
#define RANKS_SHIFT  3
#define RANKS_MASK   0x7

  uint8_t die_capacity_raw = buf[CAP_OFFSET] & CAP_MASK;
  uint8_t bus_width_raw    = buf[BUS_WIDTH_OFFSET] & BUS_WIDTH_MASK;
  uint8_t sdram_width_raw  = buf[SDRAM_WIDTH_OFFSET] & SDRAM_WIDTH_MASK;
  uint8_t ranks_raw        = (buf[RANKS_OFFSET] >> RANKS_SHIFT) & RANKS_MASK;

  int die_capacity = get_die_capacity(die_capacity_raw);
  int bus_width    = get_bus_width_bits(bus_width_raw);
  int sdram_width  = get_device_width_bits(sdram_width_raw);
  int ranks        = PKG_RANK(ranks_raw);
  uint16_t capacity  = 0;

  // calculate capacity based on formula specified in JEDEC SPD spec
  //   Annex L: Serial Presence Detect (SPD) for DDR4 SDRAM Modules
  if (die_capacity > 0 && bus_width > 0 && sdram_width > 0 && ranks > 0)
    capacity = die_capacity / 8 * bus_width / sdram_width * ranks;

  DBG_PRINT("1 cap %d bus_w %d ram_w %d ranks %d\n", die_capacity_raw, bus_width_raw, sdram_width_raw, ranks_raw);
  DBG_PRINT("2 cap %d bus_w %d ram_w %d ranks %d\n", die_capacity, bus_width, sdram_width, ranks);
  DBG_PRINT("3 capacity %d\n", capacity);

  snprintf(size_str, size_str_len, "%d MB", capacity);
}

static void
get_config(uint8_t fru_id, uint8_t cpu, uint8_t dimm, uint8_t type, json_t *obj) {
// page 0 constants
#define P0_OFFSET   0
#define P0_LEN      20
#define MIN_CYCLE_TIME_OFFSET 18
// page 1 constants
#define P1_OFFSET 0x40
#define P1_LEN    0x30
#define MFG_OFFSET 0
#define DATE_OFFSET 3
#define SERIAL_OFFSET 5
#define PN_OFFSET 9
#define BUF_SIZE 64
  uint8_t i, dimm_present = 0;
  uint8_t buf[BUF_SIZE] = {0};
  uint8_t mincycle = 0;
  uint16_t mfg_id = 0;
  char pn[LEN_PN_STRING] = {0};
  char sn[LEN_SERIAL_STRING] = {0};
  char week[BUF_SIZE] = {0};
  char size[BUF_SIZE] = {0};

  // read page 0 to get type, speed, capacity
  memset(buf, 0, BUF_SIZE);
  util_read_spd_with_retry(fru_id, cpu, dimm, P0_OFFSET, P0_LEN, 0, buf, &dimm_present);
  if (dimm_present) {
    mincycle = buf[MIN_CYCLE_TIME_OFFSET];
    util_get_size(size, BUF_SIZE, buf);

    // read page 1 to get pn, sn, manufacturer, manufacturer week
    util_set_EE_page(fru_id, cpu, dimm, 1);
    memset(buf, 0, BUF_SIZE);
    util_read_spd_with_retry(fru_id, cpu, dimm, P1_OFFSET, P1_LEN, 0, buf, &dimm_present);
    if (dimm_present) {
      for (i = 0; i < LEN_PART_NUMBER; ++i) {
        snprintf(pn + i, LEN_PN_STRING - i, "%c", buf[PN_OFFSET + i]);
      }
      for (i = 0; i < LEN_SERIAL; ++i) {
        snprintf(sn + (2 * i), LEN_SERIAL_STRING - (2 * i), "%02X", buf[SERIAL_OFFSET + i]);
      }
      mfg_id = (buf[MFG_OFFSET + 1] << 8) | buf[MFG_OFFSET];
      snprintf(week, BUF_SIZE, "20%02x Week%02x", buf[DATE_OFFSET], buf[DATE_OFFSET + 1]);
    }
  }

  if (obj) {
    json_object_set_new(obj, "Size", json_string(size));
    json_object_set_new(obj, "Type", json_string(dimm_type_string(type)));
    json_object_set_new(obj, "Speed", json_string(dimm_speed_string(mincycle)));
    json_object_set_new(obj, "Manufacturer", json_string(mfg_string(mfg_id)));
    json_object_set_new(obj, "Serial Number", json_string(sn));
    json_object_set_new(obj, "Part Number", json_string(pn));
    json_object_set_new(obj, "Manufacturing Date", json_string(week));
  } else {
    printf("\tSize: %s\n", size);
    printf("\tType: %s\n", dimm_type_string(type));
    printf("\tSpeed: %s\n", dimm_speed_string(mincycle));
    printf("\tManufacturer: %s\n", mfg_string(mfg_id));
    printf("\tSerial Number: %s\n", sn);
    printf("\tPart Number: %s\n", pn);
    printf("\tManufacturing Date: %s\n", week);
  }
}

static void
get_config_spd5(uint8_t fru_id, uint8_t cpu, uint8_t dimm, uint8_t type, json_t *obj) {
  char size[LEN_SIZE_STRING] = {0};
  char speed[LEN_SPEED_STRING] = {0};
  char mfg[LEN_MFG_STRING] = {0};
  char mfg_date[LEN_MFG_STRING] = {0};
  char pn[LEN_PN_STRING] = {0};
  char sn[LEN_SERIAL_STRING] = {0};
  char reg_vendor[LEN_MFG_STRING] = {0};
  char pmic_vendor[LEN_MFG_STRING] = {0};

  get_spd5_dimm_size(fru_id, cpu, dimm, size);
  get_spd5_dimm_speed(fru_id, cpu, dimm, speed);
  get_spd5_dimm_vendor(fru_id, cpu, dimm, mfg);
  get_spd5_dimm_mfg_date(fru_id, cpu, dimm, mfg_date);
  get_part(fru_id, cpu, dimm, pn);
  get_serial(fru_id, cpu, dimm, sn);
  get_spd5_reg_vendor(fru_id, cpu, dimm, reg_vendor);
  get_spd5_pmic_vendor(fru_id, cpu, dimm, pmic_vendor);

  if (obj) {
    json_object_set_new(obj, "Type", json_string(dimm_type_string(type)));
    json_object_set_new(obj, "Size", json_string(size));
    json_object_set_new(obj, "Speed", json_string(speed));
    json_object_set_new(obj, "Manufacturer", json_string(mfg));
    json_object_set_new(obj, "Manufacturing Date", json_string(mfg_date));
    json_object_set_new(obj, "Part Number", json_string(pn));
    json_object_set_new(obj, "Serial Number", json_string(sn));
    json_object_set_new(obj, "Register Vendor", json_string(reg_vendor));
    json_object_set_new(obj, "PMIC Vendor", json_string(pmic_vendor));
  } else {
    printf("\tType: %s\n", dimm_type_string(type));
    printf("\tSize: %s\n", size);
    printf("\tSpeed: %s\n", speed);
    printf("\tManufacturer: %s\n", mfg);
    printf("\tManufacturing Date: %s\n", mfg_date);
    printf("\tPart Number: %s\n", pn);
    printf("\tSerial Number: %s\n", sn);
    printf("\tRegister Vendor: %s\n", reg_vendor);
    printf("\tPMIC Vendor: %s\n", pmic_vendor);
  }
}

// print DIMM Config
// PN, SN + vendor, manufacture_date, size, speed, clock speed.
int
util_get_config(uint8_t fru_id, uint8_t dimm, bool json, uint8_t* options) {
  uint8_t i, cpu, startCPU, endCPU, startDimm, endDimm, dev_type;
  int ret;
  json_t *config_arr = NULL;

  if (options == NULL) {
    return ERR_INVALID_SYNTAX;
  }
  if (json) {
    config_arr = json_array();
    if (!config_arr) {
      return -1;
    }
  } else {
    printf("FRU: %s\n", fru_name[fru_id - 1]);
  }

  set_dimm_loop(dimm, &startCPU, &endCPU, &startDimm, &endDimm);
  for (cpu = startCPU; cpu < endCPU; cpu++) {
    for (i = startDimm; i < endDimm; ++i) {
      json_t *config_obj = NULL;
      if (json) {
        config_obj = json_object();
        if (!config_obj) {
          json_decref(config_arr);
          return -1;
        }

        json_t *dimm_obj = json_object();
        if (!dimm_obj) {
          json_decref(config_obj);
          json_decref(config_arr);
          return -1;
        }
        json_object_set_new(dimm_obj, "FRU", json_string(fru_name[fru_id - 1]));
        json_object_set_new(dimm_obj, "DIMM", json_string(get_dimm_label(cpu,i)));
        json_object_set_new(dimm_obj, "config", config_obj);
        json_array_append_new(config_arr, dimm_obj);
      } else {
        printf("DIMM %s:\n", get_dimm_label(cpu, i));
      }

      ret = check_dimm_type_ddr5(fru_id, cpu, i, &dev_type);
      if (!ret) {
        get_config_spd5(fru_id, cpu, i, dev_type, config_obj);
      } else if (ret < 0) {
        if (!json) {
          printf("\tNO DIMM\n");
        }
      } else {
        get_config(fru_id, cpu, i, dev_type, config_obj);
      }
    }
  }
  if (json) {
    json_dumpf(config_arr, stdout, JSON_INDENT(4));
    json_decref(config_arr);
    printf("\n");
  }

  return 0;
}

int
util_pmic_err(uint8_t fru_id, uint8_t dimm, bool /*json*/, uint8_t* options) {
  int ret = 0, i = 0;
  uint8_t err_cnt = 0;
  uint8_t cpu = 0, startCPU = 0, endCPU = 0, startDimm = 0, endDimm = 0, dimm_num = 0;
  const char *err_list[MAX_PMIC_ERR_TYPE] = {0};

  if (options == NULL) {
    return ERR_INVALID_SYNTAX;
  }
  set_dimm_loop(dimm, &startCPU, &endCPU, &startDimm, &endDimm);
  if (options[0] == OPTION_LIST_ERR) {
    for (cpu = startCPU; cpu < endCPU; cpu++) {
      for (dimm_num = startDimm; dimm_num < endDimm; dimm_num++) {
        printf("DIMM %s PMIC Error: ", get_dimm_label(cpu, dimm_num));
        ret = pmic_list_err(fru_id, cpu, dimm_num, err_list, &err_cnt);
        if (ret < 0) {
          printf("No DIMM\n");
          continue;
        }
        if (err_cnt == 0) {
          printf("No Error\n");
          continue;
        }
        for (i = 0; i < err_cnt; i++) {
          printf("%s%s", err_list[i], (i < (err_cnt - 1)) ? ", " : "");
        }
        printf("\n");
      }
    }
  } else if (options[0] == OPTION_CLEAR_ERR) {
    for (cpu = startCPU; cpu < endCPU; cpu++) {
      for (dimm_num = startDimm; dimm_num < endDimm; dimm_num++) {
        printf("DIMM %s: ", get_dimm_label(cpu, dimm_num));
        ret = pmic_clear_err(fru_id, cpu, dimm_num);
        if (ret < 0) {
          printf("Fail to clear DIMM %s PMIC error!\n", get_dimm_label(cpu, dimm_num));
        } else {
          printf("Clear DIMM %s PMIC error done\n", get_dimm_label(cpu, dimm_num));
        }
      }
    }
  } else {
    if (dimm >= total_dimms) {
      printf("Please specify the DIMM to inject error (option --dimm)\n");
      return -1;
    }
    cpu = dimm / num_dimms_per_cpu;
    dimm = dimm % num_dimms_per_cpu;
    if (pmic_list_err(fru_id, cpu, dimm, err_list, &err_cnt) < 0) { // check PMIC is available
      printf("DIMM %s is not available now, please check ME and PMIC status\n", get_dimm_label(cpu, dimm));
    }
    ret = pmic_inject_err(fru_id, cpu, dimm, options[0]);
    if (ret == 0) {
      printf("Error inject successfully on DIMM %s\n", get_dimm_label(cpu, dimm));
    }
  }
  return ret;
}
