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
#include "dimm-util.h"

// #define DEBUG_DIMM_UTIL

#ifdef DEBUG_DIMM_UTIL
  #define DBG_PRINT(fmt, args...) printf(fmt, ##args)
#else
  #define DBG_PRINT(fmt, args...)
#endif

// dimm type decode table
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
int __attribute__((weak))
util_set_EE_page(uint8_t fru_id, uint8_t cpu, uint8_t dimm, uint8_t page_num)
{
  return -1;
}

// read 1 byte off SPD bus,
// input:  fru_id/cpu/dimm/offset
int __attribute__((weak))
util_read_spd_byte(uint8_t fru_id, uint8_t cpu, uint8_t dimm, uint8_t offset)
{
  return -1;
}

// allows each platform to populate cpu num, dimm num, num frus
int __attribute__((weak))
plat_init()
{
  return -1;
}

//  get DIMM label, e.g. DIMMA0,DIMMA1 etc
const char * __attribute__((weak))
get_dimm_label(uint8_t cpu, uint8_t dimm)
{
  return "N/A";
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
static int
util_read_spd_with_retry(uint8_t fru_id, uint8_t cpu, uint8_t dimm, uint16_t offset, uint16_t len,
                  uint16_t early_exit_cnt, uint8_t *buf, uint8_t *present) {
  uint16_t j, fail_cnt = 0;
  uint8_t retry = 0;
  int value = 0;

  *present = 0;
  for (j = 0; j < len; ++j) {
    retry = 0;
    while (retry < MAX_RETRY) {
      value = util_read_spd_byte(fru_id, cpu, dimm, offset + j);
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
util_get_serial(uint8_t fru_id, uint8_t dimm, bool json) {
  uint8_t i, j, cpu, startCPU, endCPU, startDimm, endDimm, dimm_present = 0;
  uint8_t dimm_serial[MAX_CPU_NUM][MAX_DIMM_PER_CPU][LEN_SERIAL] = {0};
  json_t *config_arr = NULL;
  char   sn[LEN_SERIAL_STRING] = {0};

  if (json) {
    config_arr = json_array();
    if (!config_arr)
      goto err_exit;
  }

  set_dimm_loop(dimm, &startCPU, &endCPU, &startDimm, &endDimm);
  for (cpu = startCPU; cpu < endCPU; cpu++) {
    for (i = startDimm; i < endDimm; ++i) {
      util_set_EE_page(fru_id, cpu, i, 1);
      util_read_spd_with_retry(fru_id, cpu, i, OFFSET_SERIAL, LEN_SERIAL, 0,
        dimm_serial[cpu][i], &dimm_present);

      if (dimm_present)
          for (j = 0; j < LEN_SERIAL; ++j)
            snprintf(sn + (2 * j), LEN_SERIAL_STRING - (2 * j), "%02X", dimm_serial[cpu][i][j]);

      if (json) {
        json_t *sn_obj = json_object();
        if (!sn_obj)
          goto err_exit;
        if (dimm_present)
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
        if (dimm_present)
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
util_get_part(uint8_t fru_id, uint8_t dimm, bool json) {
  uint8_t i, j, cpu, startCPU, endCPU, startDimm, endDimm, dimm_present = 0;
  uint8_t dimm_part[MAX_CPU_NUM][MAX_DIMM_PER_CPU][LEN_PART_NUMBER] = {0};
  json_t *config_arr = NULL;
  char   pn[LEN_PN_STRING] = {0};

  if (json) {
    config_arr = json_array();
    if (!config_arr)
      goto err_exit;
  }

  set_dimm_loop(dimm, &startCPU, &endCPU, &startDimm, &endDimm);
  for (cpu = startCPU; cpu < endCPU; cpu++) {
    for (i = startDimm; i < endDimm; ++i) {
      util_set_EE_page(fru_id, cpu, i, 1);
      util_read_spd_with_retry(fru_id, cpu, i, OFFSET_PART_NUMBER, LEN_PART_NUMBER,
        MAX_FAIL_CNT, dimm_part[cpu][i], &dimm_present);

      if (dimm_present)
          for (j = 0; j < LEN_PART_NUMBER; ++j)
            snprintf(pn + j, LEN_PN_STRING - j, "%c", dimm_part[cpu][i][j]);

      if (json) {
        json_t *part_obj = json_object();
        if (!part_obj)
          goto err_exit;
        if (dimm_present)
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
        if (dimm_present)
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

static int
util_get_raw_dump(uint8_t fru_id, uint8_t dimm, bool json) {
  uint8_t i, cpu, startCPU, endCPU, startDimm, endDimm, dimm_present = 0;
  uint16_t j = 0;
  uint16_t offset = DEFAULT_DUMP_OFFSET;
  uint16_t len = DEFAULT_DUMP_LEN;
  uint8_t buf[DEFAULT_DUMP_LEN] = {0};

  printf("Raw Dump, fru_id %d dimm=%s, offset=0x%x, len=0x%x\n",
    fru_id, get_dimm_label(cpu,i), offset + 0x100, len);

  set_dimm_loop(dimm, &startCPU, &endCPU, &startDimm, &endDimm);
  for (cpu = startCPU; cpu < endCPU; cpu++) {
    for (i = startDimm; i < endDimm; ++i) {
      memset(buf, 0, DEFAULT_DUMP_LEN);
      util_read_spd_with_retry(fru_id, cpu, i, DEFAULT_DUMP_OFFSET, DEFAULT_DUMP_LEN,
        0, buf, &dimm_present);

      printf("DIMM %s \n", get_dimm_label(cpu,i));
      printf("0x%x: ", offset + 0x100);
      for (j = 0; j < DEFAULT_DUMP_LEN; ++j) {
        printf("%02x ", buf[j]);
        if (((j & 0x0f) == 0x0f))
          printf("\n0x%x: ", offset + j + 0x101);
      }
    }
    printf("\n");
  }
  return 0;
}

static int
util_get_cache(uint8_t fru_id, uint8_t dimm, bool json) {
#define MAX_KEY_SIZE 100
#define MAX_VALUE_SIZE 128
  uint8_t i, startCPU, endCPU, startDimm, endDimm = 0;
  size_t len;
  char key[MAX_KEY_SIZE] = {0};
  char value[MAX_VALUE_SIZE] = {0};

  set_dimm_loop(dimm, &startCPU, &endCPU, &startDimm, &endDimm);
  for (i = startDimm; i < endDimm; ++i) {
    printf("DIMM %s Serial Number (cached): ", get_dimm_label(0,i));
    snprintf(key, MAX_KEY_SIZE, "sys_config/fru%d_dimm%d_serial_num", fru_id, i);
    if(kv_get(key, (char *)value, &len, KV_FPERSIST) < 0) {
      printf("NO DIMM");
    } else {
      for (int j = 0; ((j < LEN_SERIAL) && value[j]); ++j)
        printf("%X", value[j]);
    }
    printf("\n");

    printf("DIMM %s Part Number   (cached): ", get_dimm_label(0,i));
    snprintf(key, MAX_KEY_SIZE, "sys_config/fru%d_dimm%d_part_name", fru_id, i);
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

// calculate total capacity of DIMM module
// based on SPD info
//
//  input:
//      char *size_str - pre-allocated, 0-filled string for capacity
//      uint8_t size_str_len - total buffer size of the above string
//      uint8_t *buf - Page 0 of SPD buffer.
//                     this function only uses offset0 to 13
static int
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
  int ranks        = get_package_rank(ranks_raw);
  uint16_t capacity  = 0;

  // calculate capacity based on formula specified in JEDEC SPD spec
  //   Annex L: Serial Presence Detect (SPD) for DDR4 SDRAM Modules
  if (die_capacity > 0 && bus_width > 0 && sdram_width > 0 && ranks > 0)
    capacity = die_capacity / 8 * bus_width / sdram_width * ranks;

  DBG_PRINT("1 cap %d bus_w %d ram_w %d ranks %d\n", die_capacity_raw, bus_width_raw, sdram_width_raw, ranks_raw);
  DBG_PRINT("2 cap %d bus_w %d ram_w %d ranks %d\n", die_capacity, bus_width, sdram_width, ranks);
  DBG_PRINT("3 capacity %d\n", capacity );

  snprintf(size_str, size_str_len, "%d MB", capacity);
  return 0;
}

// print DIMM Config
// PN, SN + vendor, manufacture_date, size, speed, clock speed.
static int
util_get_config(uint8_t fru_id, uint8_t dimm, bool json) {
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
  uint8_t i, j, cpu, startCPU, endCPU, startDimm, endDimm, dimm_present = 0;
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

  set_dimm_loop(dimm, &startCPU, &endCPU, &startDimm, &endDimm);
  for (cpu = startCPU; cpu < endCPU; cpu++) {
    for (i = startDimm; i < endDimm; ++i) {
      util_set_EE_page(fru_id, cpu, i, 0);
      // read page 0 to get type, speed, capacity
      memset(buf, 0, BUF_SIZE);
      util_read_spd_with_retry(fru_id, cpu, i, P0_OFFSET, P0_LEN, 0, buf, &dimm_present);
      if (dimm_present) {
        dimm_type = buf[TYPE_OFFSET];
        mincycle  = buf[MIN_CYCLE_TIME_OFFSET];
        util_get_size(size, BUF_SIZE, buf);

        // read page 1 to get pn, sn, manufacturer, manufacturer week
        util_set_EE_page(fru_id, cpu, i, 1);
        memset(buf, 0, BUF_SIZE);
        util_read_spd_with_retry(fru_id, cpu, i, P1_OFFSET, P1_LEN, 0, buf, &dimm_present);
        if (dimm_present) {
            for (j = 0; j < LEN_PART_NUMBER; ++j) {
              snprintf(pn + j, LEN_PN_STRING - j, "%c", buf[PN_OFFSET + j]);
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
        if (!dimm_obj) {
          json_decref(config_obj);
          goto err_exit;
        }
        json_object_set_new(dimm_obj, "FRU", json_string(fru_name[fru_id - 1]));
        json_object_set_new(dimm_obj, "DIMM", json_string(get_dimm_label(cpu,i)));
        json_object_set_new(dimm_obj, "config", config_obj);
        json_array_append_new(config_arr, dimm_obj);
      } else {
        printf("DIMM %s:\n", get_dimm_label(cpu,i));
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

  *dimm = total_dimms;
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
      if ((*dimm < 0) || (*dimm > total_dimms)) {
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
parse_arg_and_exec(int argc, char **argv, uint8_t fru_id,
                  int (*pF)(uint8_t, uint8_t, bool)) {
  uint8_t dimm = total_dimms;
  bool    json = false;
  int ret = 0;

  if (argc < 3) {
    printf("Error: invalid number of arguments\n");
    return ERR_INVALID_SYNTAX;
  } else if (argc > 3) {
    // parse option fields,
    // skipping first 3 arguments, which will be "dimm-util fru cmd"
    if (parse_cmdline_args(argc - 2, &(argv[2]), &dimm, &json) != 0)
      return ERR_INVALID_SYNTAX;
  }

  if (fru_id == fru_id_all) {
    for (int i = fru_id_min; i <= fru_id_max; ++i) {
      ret = pF(i, dimm, json);
    }
  } else {
    ret = pF(fru_id, dimm, json);
  }

  return ret;
}

static void
print_usage_help(void) {
  int i;

  printf("Usage: dimm-util <");
  for (i = 0; i < num_frus; ++i) {
    printf("%s", fru_name[i]);
    if (i < num_frus - 1)
      printf("|");
  }
  printf("> <CMD> [OPT1] [OPT2] ...\n");
  printf("\nCMD list:\n");
  printf("   --serial   - get DIMM module serial number\n");
  printf("   --part     - get DIMM module part number\n");
  printf("   --raw      - dump raw SPD data\n");
  printf("   --cache    - read from SMBIOS cache\n");
  printf("   --config   - print DIMM configuration info\n");
  printf("\nOPT list:\n");
  printf("   --dimm N  - DIMM number (0..%d) [default %d = ALL]\n",
          total_dimms - 1, total_dimms);
  printf("   --json    - output in JSON format\n");
}

static int
parse_fru(char* fru_str, uint8_t *fru) {
  uint8_t i, found = 0;

  for (i = 0; i < num_frus; ++i) {
    if (!strcmp(fru_str, fru_name[i])) {
      // fru_name string is 0 based but fru id is 1 based
      *fru = i + 1;
      found = 1;
      break;
    }
  }

  if (!found)
    return -1;
  else
    return 0;
}

int
main(int argc, char **argv) {
  uint8_t fru;
  int ret = 0;

  // init platform-specific variables, e.g. max dimms, max cpu, dimm labels,
  ret = plat_init();
  DBG_PRINT("num_frus(%d), num_dimms_per_cpu(%d), num_cpus(%d), total_dimms(%d)\n",
          num_frus, num_dimms_per_cpu, num_cpus, total_dimms);
  if (ret != 0) {
    printf("init failed (%d)\n", ret);
    return ret;
  }

  if (argc < 3)
    goto err_exit;

  if (parse_fru(argv[1], &fru) == -1) {
    goto err_exit;
  }

  if (!strcmp(argv[2], "--serial")) {
    ret = parse_arg_and_exec(argc, argv, fru, &util_get_serial);
  } else if (!strcmp(argv[2], "--part")) {
    ret = parse_arg_and_exec(argc, argv, fru, &util_get_part);
  } else if (!strcmp(argv[2], "--raw")) {
    ret = parse_arg_and_exec(argc, argv, fru, &util_get_raw_dump);
  } else if (!strcmp(argv[2], "--cache")) {
    ret = parse_arg_and_exec(argc, argv, fru, &util_get_cache);
  } else if (!strcmp(argv[2], "--config")) {
    ret = parse_arg_and_exec(argc, argv, fru, &util_get_config);
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
