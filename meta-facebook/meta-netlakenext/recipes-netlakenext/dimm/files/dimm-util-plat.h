/*
 *
 * Copyright 2024-present Facebook. All Rights Reserved.
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

#ifndef __DIMM_UTIL_PLAT_H__
#define __DIMM_UTIL_PLAT_H__

#ifdef __cplusplus
extern "C" {
#endif

#define NUM_CPU_NETLAKE2      1
#define MAX_DIMM_NUM_NETLAKE2  2

#define NUM_FRU_NETLAKE2  1
#define FRU_ID_MIN_NETLAKE2 1
#define FRU_ID_MAX_NETLAKE2 1
#define FRU_ID_ALL_NETLAKE2 1

// for NL2 to record PMIC SEL
#define ERR_PATTERN_LEN 6
#define PMIC_RETRY_INTERVAL_USEC 100000
#define PMIC_ERR_INJ_REG 0x35
#define PMIC_WRITE_PROTECT_BIT 2

enum {
  TYPE_UNDEF = 0,
  TYPE_SWITCHOVER,
  TYPE_CRIT_TEMP,
  TYPE_HIGH_TEMP,
  TYPE_PG_1V8,
  TYPE_HIGH_CURR,
  TYPE_CAMP_INPUT,
  TYPE_CURR_LIMIT,
};

enum {
  VOLT_UNDEF = 0,
  VOLT_OV = 0,
  VOLT_UV,
};

enum {
  RAIL_UNDEF = 0,
  RAIL_SWA_OUT,
  RAIL_SWB_OUT = 3,
  RAIL_SWC_OUT,
  RAIL_VIN_BULK,
  RAIL_ALL,
};

enum {
  ERR_INJ_DISABLE = 0,
  ERR_INJ_ENABLE,
};

// PMIC error injection register R35
typedef struct {
  uint8_t err_type:3;
  uint8_t uv_ov_select:1;
  uint8_t rail:3;
  uint8_t enable:1;
} err_inject_reg;

// PMIC has 6 register to reflect the errors (R05, R06, R08, R09, R0A, R0B)
typedef struct {
  uint8_t pattern[ERR_PATTERN_LEN];
  bool camp;
  const char *err_str;
  err_inject_reg einj_reg;
} pmic_err_info;

extern const uint8_t pmic_err_pattern_idx[];
extern pmic_err_info pmic_err[];

int get_pmic_error_data_raw(uint8_t slot_id, uint8_t dimm, uint8_t *error_data);
int compare_pmic_raw_and_log(uint8_t dimm, const uint8_t *data);

#ifdef __cplusplus
}
#endif

#endif
