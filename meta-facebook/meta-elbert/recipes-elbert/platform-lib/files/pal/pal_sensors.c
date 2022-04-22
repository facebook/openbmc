/*
 * Copyright 2021-present Facebook. All Rights Reserved.
 *
 * This file contains code to support IPMI2.0 Specification available @
 * http://www.intel.com/content/www/us/en/servers/ipmi/ipmi-specifications.html
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

/*
 * This file contains functions and logics that depends on Wedge100 specific
 * hardware and kernel drivers. Here, some of the empty "default" functions
 * are overridden with simple functions that returns non-zero error code.
 * This is for preventing any potential escape of failures through the
 * default functions that will return 0 no matter what.
 */

#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <syslog.h>
#include <time.h>
#include <unistd.h>
#include <pthread.h>
#include <ctype.h>
#include <linux/limits.h>
#include <linux/version.h>
#include "pal.h"
#include "pal_sensors.h"
#include <math.h>
#include <openbmc/obmc-i2c.h>
#include <openbmc/misc-utils.h>
#include <openbmc/log.h>

#define RUN_SHELL_CMD(_cmd)                              \
  do {                                                   \
    int _ret = system(_cmd);                             \
    if (_ret != 0)                                       \
      OBMC_WARN("'%s' command returned %d", _cmd, _ret); \
  } while (0)

static uint8_t psu_bus[] = { 24, 25, 26, 27 };

static float cfm_lookup_8pim16q_0pim8ddm_2psu[] = {
  89.15, 112.14, 147.91, 176.69, 212.77, 250.76,
  285.81, 315.18, 347.17, 378.02, 381.12
};

static float cfm_lookup_5pim16q_3pim8ddm_2psu[] = {
  95.41, 122.05, 151.8, 186.63, 223.53, 259.95,
  297.9, 329.78, 362.11, 390.28, 394.78
};

static float cfm_lookup_2pim16q_6pim8ddm_4psu[] = {
  92.33, 117.19, 147.91, 176.69, 220.89, 259.96,
  301.83, 338.58, 373.32, 410.86, 412.29
};

static float cfm_lookup_0pim16q_8pim8ddm_4psu[] = {
  95.41, 122.05, 151.8, 179.97, 226.14, 262.21,
  303.77, 342.04, 378.02, 417.96, 417.96
};

typedef struct {
  char name[32];
} sensor_desc_t;

typedef struct {
  uint16_t flag;
  float ucr;
  float unc;
  float unr;
  float lcr;
  float lnc;
  float lnr;
  float pos_hyst;
  float neg_hyst;
  int curr_st;
  char name[32];
} _sensor_thresh_t;

typedef struct {
  char name[NAME_MAX];
} sensor_path_t;

static sensor_desc_t m_snr_desc[MAX_NUM_FRUS][MAX_SENSOR_NUM + 1] = {0};
static sensor_path_t snr_path[MAX_NUM_FRUS][MAX_SENSOR_NUM + 1] = {0};
static uint8_t sdr_fru_update_flag[MAX_NUM_FRUS] = {0};
static bool init_threshold_done[MAX_NUM_FRUS + 1] = {false};

// List of SCM sensors that need to be monitored
const uint8_t scm_sensor_list[] = {
  SCM_ECB_VIN,
  SCM_ECB_VOUT,
  SCM_ECB_CURR,
  SCM_ECB_POWER,
  SCM_ECB_TEMP,
  SCM_BOARD_TEMP,
  SCM_INLET_TEMP,
  SCM_BMC_TEMP,
};

// List of SMB sensors that need to be monitored
const uint8_t smb_sensor_list[] = {
  SMB_POS_0V75_CORE,
  SMB_POS_0V75_ANLG,
  SMB_POS_1V2,
  SMB_POS_1V2_ANLG_L,
  SMB_POS_1V2_ANLG_R,
  SMB_POS_1V8,
  SMB_POS_1V8_ANLG,
  SMB_POS_3V3,
  SMB_POS_3V3_DPM,
  SMB_POS_5V0,
  SMB_POS_12V_B,
  SMB_POS_12V_A,
  SMB_POS_1V2B_E,
  SMB_POS_2V5B_E,
  SMB_POS_3V3B_E,
  TH4_VRD1_VIN,
  TH4_VRD1_VOUT_LOOP0,
  TH4_VRD1_TEMP_LOOP0,
  TH4_VRD1_PIN,
  TH4_VRD1_POUT_LOOP0,
  TH4_VRD1_IIN,
  TH4_VRD1_IOUT_LOOP0,
  TH4_VRD2_VIN,
  TH4_VRD2_VOUT_LOOP0,
  TH4_VRD2_VOUT_LOOP1,
  TH4_VRD2_VOUT_LOOP2,
  TH4_VRD2_TEMP_LOOP0,
  TH4_VRD2_TEMP_LOOP1,
  TH4_VRD2_TEMP_LOOP2,
  TH4_VRD2_TEMP_INTERNAL,
  TH4_VRD2_PIN_LOOP0,
  TH4_VRD2_PIN_LOOP1,
  TH4_VRD2_PIN_LOOP2,
  TH4_VRD2_POUT_LOOP0,
  TH4_VRD2_POUT_LOOP1,
  TH4_VRD2_POUT_LOOP2,
  TH4_VRD2_IIN_LOOP0,
  TH4_VRD2_IIN_LOOP1,
  TH4_VRD2_IIN_LOOP2,
  TH4_VRD2_IOUT_LOOP0,
  TH4_VRD2_IOUT_LOOP1,
  TH4_VRD2_IOUT_LOOP2,
  SMB_T,
  TH4_DIE_TEMP_0,
  TH4_DIE_TEMP_1,
  SMB_R,
  SMB_U,
  SMB_L,
  BLACKHAWK_CORE_0_5_TEMP,
  BLACKHAWK_CORE_6_12_TEMP,
  BLACKHAWK_CORE_13_18_TEMP,
  BLACKHAWK_CORE_19_25_TEMP,
  BLACKHAWK_CORE_26_31_TEMP,
  BLACKHAWK_CORE_32_37_TEMP,
  BLACKHAWK_CORE_38_44_TEMP,
  BLACKHAWK_CORE_45_50_TEMP,
  BLACKHAWK_CORE_51_57_TEMP,
  BLACKHAWK_CORE_58_63_TEMP,
  CORE_0_TEMP,
  CORE_1_TEMP,
  CORE_2_TEMP,
  CORE_3_TEMP,
  PIM_QSFP200,
  PIM_QSFP400,
  PIM_F104,
};

// List of PIM16Q2 sensors that need to be monitored
const uint8_t pim16q2_sensor_list[MAX_PIM][PIM16Q2_SENSOR_COUNT] = {
  {
    PIM_POS_3V3_U_VOUT,
    PIM_POS_3V3_U_TEMP,
    PIM_POS_3V3_U_CURR,
    PIM_POS_3V3_L_VOUT,
    PIM_POS_3V3_L_TEMP,
    PIM_POS_3V3_L_CURR,
  }, {
    PIM_POS_3V3_U_VOUT,
    PIM_POS_3V3_U_TEMP,
    PIM_POS_3V3_U_CURR,
    PIM_POS_3V3_L_VOUT,
    PIM_POS_3V3_L_TEMP,
    PIM_POS_3V3_L_CURR,
  }, {
    PIM_POS_3V3_U_VOUT,
    PIM_POS_3V3_U_TEMP,
    PIM_POS_3V3_U_CURR,
    PIM_POS_3V3_L_VOUT,
    PIM_POS_3V3_L_TEMP,
    PIM_POS_3V3_L_CURR,
  }, {
    PIM_POS_3V3_U_VOUT,
    PIM_POS_3V3_U_TEMP,
    PIM_POS_3V3_U_CURR,
    PIM_POS_3V3_L_VOUT,
    PIM_POS_3V3_L_TEMP,
    PIM_POS_3V3_L_CURR,
  }, {
    PIM_POS_3V3_U_VOUT,
    PIM_POS_3V3_U_TEMP,
    PIM_POS_3V3_U_CURR,
    PIM_POS_3V3_L_VOUT,
    PIM_POS_3V3_L_TEMP,
    PIM_POS_3V3_L_CURR,
  }, {
    PIM_POS_3V3_U_VOUT,
    PIM_POS_3V3_U_TEMP,
    PIM_POS_3V3_U_CURR,
    PIM_POS_3V3_L_VOUT,
    PIM_POS_3V3_L_TEMP,
    PIM_POS_3V3_L_CURR,
  }, {
    PIM_POS_3V3_U_VOUT,
    PIM_POS_3V3_U_TEMP,
    PIM_POS_3V3_U_CURR,
    PIM_POS_3V3_L_VOUT,
    PIM_POS_3V3_L_TEMP,
    PIM_POS_3V3_L_CURR,
  }, {
    PIM_POS_3V3_U_VOUT,
    PIM_POS_3V3_U_TEMP,
    PIM_POS_3V3_U_CURR,
    PIM_POS_3V3_L_VOUT,
    PIM_POS_3V3_L_TEMP,
    PIM_POS_3V3_L_CURR,
  },
};

// List of PIM16Q sensors that need to be monitored
const uint8_t pim16q_sensor_list[MAX_PIM][PIM16Q_SENSOR_COUNT] = {
  {
    PIM_POS_3V3_U_VOUT,
    PIM_POS_3V3_U_TEMP,
    PIM_POS_3V3_U_CURR,
    PIM_POS_3V3_L_VOUT,
    PIM_POS_3V3_L_TEMP,
    PIM_POS_3V3_L_CURR,
    PIM_LM73_TEMP,
    PIM_POS_12V,
    PIM_POS_3V3_E,
    PIM_POS_1V2_E,
    PIM_POS_3V3_U,
    PIM_POS_3V3_L,
    PIM_POS_3V8_LEDS,
    PIM_DPM_TEMP,
  }, {
    PIM_POS_3V3_U_VOUT,
    PIM_POS_3V3_U_TEMP,
    PIM_POS_3V3_U_CURR,
    PIM_POS_3V3_L_VOUT,
    PIM_POS_3V3_L_TEMP,
    PIM_POS_3V3_L_CURR,
    PIM_LM73_TEMP,
    PIM_POS_12V,
    PIM_POS_3V3_E,
    PIM_POS_1V2_E,
    PIM_POS_3V3_U,
    PIM_POS_3V3_L,
    PIM_POS_3V8_LEDS,
    PIM_DPM_TEMP,
  }, {
    PIM_POS_3V3_U_VOUT,
    PIM_POS_3V3_U_TEMP,
    PIM_POS_3V3_U_CURR,
    PIM_POS_3V3_L_VOUT,
    PIM_POS_3V3_L_TEMP,
    PIM_POS_3V3_L_CURR,
    PIM_LM73_TEMP,
    PIM_POS_12V,
    PIM_POS_3V3_E,
    PIM_POS_1V2_E,
    PIM_POS_3V3_U,
    PIM_POS_3V3_L,
    PIM_POS_3V8_LEDS,
    PIM_DPM_TEMP,
  }, {
    PIM_POS_3V3_U_VOUT,
    PIM_POS_3V3_U_TEMP,
    PIM_POS_3V3_U_CURR,
    PIM_POS_3V3_L_VOUT,
    PIM_POS_3V3_L_TEMP,
    PIM_POS_3V3_L_CURR,
    PIM_LM73_TEMP,
    PIM_POS_12V,
    PIM_POS_3V3_E,
    PIM_POS_1V2_E,
    PIM_POS_3V3_U,
    PIM_POS_3V3_L,
    PIM_POS_3V8_LEDS,
    PIM_DPM_TEMP,
  }, {
    PIM_POS_3V3_U_VOUT,
    PIM_POS_3V3_U_TEMP,
    PIM_POS_3V3_U_CURR,
    PIM_POS_3V3_L_VOUT,
    PIM_POS_3V3_L_TEMP,
    PIM_POS_3V3_L_CURR,
    PIM_LM73_TEMP,
    PIM_POS_12V,
    PIM_POS_3V3_E,
    PIM_POS_1V2_E,
    PIM_POS_3V3_U,
    PIM_POS_3V3_L,
    PIM_POS_3V8_LEDS,
    PIM_DPM_TEMP,
  }, {
    PIM_POS_3V3_U_VOUT,
    PIM_POS_3V3_U_TEMP,
    PIM_POS_3V3_U_CURR,
    PIM_POS_3V3_L_VOUT,
    PIM_POS_3V3_L_TEMP,
    PIM_POS_3V3_L_CURR,
    PIM_LM73_TEMP,
    PIM_POS_12V,
    PIM_POS_3V3_E,
    PIM_POS_1V2_E,
    PIM_POS_3V3_U,
    PIM_POS_3V3_L,
    PIM_POS_3V8_LEDS,
    PIM_DPM_TEMP,
  }, {
    PIM_POS_3V3_U_VOUT,
    PIM_POS_3V3_U_TEMP,
    PIM_POS_3V3_U_CURR,
    PIM_POS_3V3_L_VOUT,
    PIM_POS_3V3_L_TEMP,
    PIM_POS_3V3_L_CURR,
    PIM_LM73_TEMP,
    PIM_POS_12V,
    PIM_POS_3V3_E,
    PIM_POS_1V2_E,
    PIM_POS_3V3_U,
    PIM_POS_3V3_L,
    PIM_POS_3V8_LEDS,
    PIM_DPM_TEMP,
  }, {
    PIM_POS_3V3_U_VOUT,
    PIM_POS_3V3_U_TEMP,
    PIM_POS_3V3_U_CURR,
    PIM_POS_3V3_L_VOUT,
    PIM_POS_3V3_L_TEMP,
    PIM_POS_3V3_L_CURR,
    PIM_LM73_TEMP,
    PIM_POS_12V,
    PIM_POS_3V3_E,
    PIM_POS_1V2_E,
    PIM_POS_3V3_U,
    PIM_POS_3V3_L,
    PIM_POS_3V8_LEDS,
    PIM_DPM_TEMP,
  },
};

// List of PIM8DDM sensors that need to be monitored
const uint8_t pim8ddm_sensor_list[MAX_PIM][PIM8DDM_SENSOR_COUNT] = {
  {
    PIM_POS_3V3_U_VOUT,
    PIM_POS_3V3_U_TEMP,
    PIM_POS_3V3_U_CURR,
    PIM_POS_3V3_L_VOUT,
    PIM_POS_3V3_L_TEMP,
    PIM_POS_3V3_L_CURR,
    PIM_LM73_TEMP,
    PIM_POS_12V,
    PIM_POS_3V3_E,
    PIM_POS_1V2_E,
    PIM_POS_3V3_U,
    PIM_POS_3V3_L,
    PIM_DPM_TEMP,
    PIM_POS_5V0,
    PIM_POS_0V9,
    PIM_POS_1V5,
    PIM_POS_1V8,
    PIM_ISL_VIN,
    PIM_ISL_VOUT_LOOP0,
    PIM_ISL_VOUT_LOOP1,
    PIM_ISL_VOUT_LOOP2,
    PIM_ISL_TEMP_LOOP0,
    PIM_ISL_TEMP_LOOP1,
    PIM_ISL_TEMP_LOOP2,
    PIM_ISL_TEMP_INTERNAL,
    PIM_ISL_PIN_LOOP0,
    PIM_ISL_PIN_LOOP1,
    PIM_ISL_PIN_LOOP2,
    PIM_ISL_POUT_LOOP0,
    PIM_ISL_POUT_LOOP1,
    PIM_ISL_POUT_LOOP2,
    PIM_ISL_IIN_LOOP0,
    PIM_ISL_IIN_LOOP1,
    PIM_ISL_IIN_LOOP2,
    PIM_ISL_IOUT_LOOP0,
    PIM_ISL_IOUT_LOOP1,
    PIM_ISL_IOUT_LOOP2,
    PIM_ECB_VIN,
    PIM_ECB_VOUT,
    PIM_ECB_TEMP,
    PIM_ECB_POWER,
    PIM_ECB_CURR,
  }, {
    PIM_POS_3V3_U_VOUT,
    PIM_POS_3V3_U_TEMP,
    PIM_POS_3V3_U_CURR,
    PIM_POS_3V3_L_VOUT,
    PIM_POS_3V3_L_TEMP,
    PIM_POS_3V3_L_CURR,
    PIM_LM73_TEMP,
    PIM_POS_12V,
    PIM_POS_3V3_E,
    PIM_POS_1V2_E,
    PIM_POS_3V3_U,
    PIM_POS_3V3_L,
    PIM_DPM_TEMP,
    PIM_POS_5V0,
    PIM_POS_0V9,
    PIM_POS_1V5,
    PIM_POS_1V8,
    PIM_ISL_VIN,
    PIM_ISL_VOUT_LOOP0,
    PIM_ISL_VOUT_LOOP1,
    PIM_ISL_VOUT_LOOP2,
    PIM_ISL_TEMP_LOOP0,
    PIM_ISL_TEMP_LOOP1,
    PIM_ISL_TEMP_LOOP2,
    PIM_ISL_TEMP_INTERNAL,
    PIM_ISL_PIN_LOOP0,
    PIM_ISL_PIN_LOOP1,
    PIM_ISL_PIN_LOOP2,
    PIM_ISL_POUT_LOOP0,
    PIM_ISL_POUT_LOOP1,
    PIM_ISL_POUT_LOOP2,
    PIM_ISL_IIN_LOOP0,
    PIM_ISL_IIN_LOOP1,
    PIM_ISL_IIN_LOOP2,
    PIM_ISL_IOUT_LOOP0,
    PIM_ISL_IOUT_LOOP1,
    PIM_ISL_IOUT_LOOP2,
    PIM_ECB_VIN,
    PIM_ECB_VOUT,
    PIM_ECB_TEMP,
    PIM_ECB_POWER,
    PIM_ECB_CURR,
  }, {
    PIM_POS_3V3_U_VOUT,
    PIM_POS_3V3_U_TEMP,
    PIM_POS_3V3_U_CURR,
    PIM_POS_3V3_L_VOUT,
    PIM_POS_3V3_L_TEMP,
    PIM_POS_3V3_L_CURR,
    PIM_LM73_TEMP,
    PIM_POS_12V,
    PIM_POS_3V3_E,
    PIM_POS_1V2_E,
    PIM_POS_3V3_U,
    PIM_POS_3V3_L,
    PIM_DPM_TEMP,
    PIM_POS_5V0,
    PIM_POS_0V9,
    PIM_POS_1V5,
    PIM_POS_1V8,
    PIM_ISL_VIN,
    PIM_ISL_VOUT_LOOP0,
    PIM_ISL_VOUT_LOOP1,
    PIM_ISL_VOUT_LOOP2,
    PIM_ISL_TEMP_LOOP0,
    PIM_ISL_TEMP_LOOP1,
    PIM_ISL_TEMP_LOOP2,
    PIM_ISL_TEMP_INTERNAL,
    PIM_ISL_PIN_LOOP0,
    PIM_ISL_PIN_LOOP1,
    PIM_ISL_PIN_LOOP2,
    PIM_ISL_POUT_LOOP0,
    PIM_ISL_POUT_LOOP1,
    PIM_ISL_POUT_LOOP2,
    PIM_ISL_IIN_LOOP0,
    PIM_ISL_IIN_LOOP1,
    PIM_ISL_IIN_LOOP2,
    PIM_ISL_IOUT_LOOP0,
    PIM_ISL_IOUT_LOOP1,
    PIM_ISL_IOUT_LOOP2,
    PIM_ECB_VIN,
    PIM_ECB_VOUT,
    PIM_ECB_TEMP,
    PIM_ECB_POWER,
    PIM_ECB_CURR,
  }, {
    PIM_POS_3V3_U_VOUT,
    PIM_POS_3V3_U_TEMP,
    PIM_POS_3V3_U_CURR,
    PIM_POS_3V3_L_VOUT,
    PIM_POS_3V3_L_TEMP,
    PIM_POS_3V3_L_CURR,
    PIM_LM73_TEMP,
    PIM_POS_12V,
    PIM_POS_3V3_E,
    PIM_POS_1V2_E,
    PIM_POS_3V3_U,
    PIM_POS_3V3_L,
    PIM_DPM_TEMP,
    PIM_POS_5V0,
    PIM_POS_0V9,
    PIM_POS_1V5,
    PIM_POS_1V8,
    PIM_ISL_VIN,
    PIM_ISL_VOUT_LOOP0,
    PIM_ISL_VOUT_LOOP1,
    PIM_ISL_VOUT_LOOP2,
    PIM_ISL_TEMP_LOOP0,
    PIM_ISL_TEMP_LOOP1,
    PIM_ISL_TEMP_LOOP2,
    PIM_ISL_TEMP_INTERNAL,
    PIM_ISL_PIN_LOOP0,
    PIM_ISL_PIN_LOOP1,
    PIM_ISL_PIN_LOOP2,
    PIM_ISL_POUT_LOOP0,
    PIM_ISL_POUT_LOOP1,
    PIM_ISL_POUT_LOOP2,
    PIM_ISL_IIN_LOOP0,
    PIM_ISL_IIN_LOOP1,
    PIM_ISL_IIN_LOOP2,
    PIM_ISL_IOUT_LOOP0,
    PIM_ISL_IOUT_LOOP1,
    PIM_ISL_IOUT_LOOP2,
    PIM_ECB_VIN,
    PIM_ECB_VOUT,
    PIM_ECB_TEMP,
    PIM_ECB_POWER,
    PIM_ECB_CURR,
  }, {
    PIM_POS_3V3_U_VOUT,
    PIM_POS_3V3_U_TEMP,
    PIM_POS_3V3_U_CURR,
    PIM_POS_3V3_L_VOUT,
    PIM_POS_3V3_L_TEMP,
    PIM_POS_3V3_L_CURR,
    PIM_LM73_TEMP,
    PIM_POS_12V,
    PIM_POS_3V3_E,
    PIM_POS_1V2_E,
    PIM_POS_3V3_U,
    PIM_POS_3V3_L,
    PIM_DPM_TEMP,
    PIM_POS_5V0,
    PIM_POS_0V9,
    PIM_POS_1V5,
    PIM_POS_1V8,
    PIM_ISL_VIN,
    PIM_ISL_VOUT_LOOP0,
    PIM_ISL_VOUT_LOOP1,
    PIM_ISL_VOUT_LOOP2,
    PIM_ISL_TEMP_LOOP0,
    PIM_ISL_TEMP_LOOP1,
    PIM_ISL_TEMP_LOOP2,
    PIM_ISL_TEMP_INTERNAL,
    PIM_ISL_PIN_LOOP0,
    PIM_ISL_PIN_LOOP1,
    PIM_ISL_PIN_LOOP2,
    PIM_ISL_POUT_LOOP0,
    PIM_ISL_POUT_LOOP1,
    PIM_ISL_POUT_LOOP2,
    PIM_ISL_IIN_LOOP0,
    PIM_ISL_IIN_LOOP1,
    PIM_ISL_IIN_LOOP2,
    PIM_ISL_IOUT_LOOP0,
    PIM_ISL_IOUT_LOOP1,
    PIM_ISL_IOUT_LOOP2,
    PIM_ECB_VIN,
    PIM_ECB_VOUT,
    PIM_ECB_TEMP,
    PIM_ECB_POWER,
    PIM_ECB_CURR,
  }, {
    PIM_POS_3V3_U_VOUT,
    PIM_POS_3V3_U_TEMP,
    PIM_POS_3V3_U_CURR,
    PIM_POS_3V3_L_VOUT,
    PIM_POS_3V3_L_TEMP,
    PIM_POS_3V3_L_CURR,
    PIM_LM73_TEMP,
    PIM_POS_12V,
    PIM_POS_3V3_E,
    PIM_POS_1V2_E,
    PIM_POS_3V3_U,
    PIM_POS_3V3_L,
    PIM_DPM_TEMP,
    PIM_POS_5V0,
    PIM_POS_0V9,
    PIM_POS_1V5,
    PIM_POS_1V8,
    PIM_ISL_VIN,
    PIM_ISL_VOUT_LOOP0,
    PIM_ISL_VOUT_LOOP1,
    PIM_ISL_VOUT_LOOP2,
    PIM_ISL_TEMP_LOOP0,
    PIM_ISL_TEMP_LOOP1,
    PIM_ISL_TEMP_LOOP2,
    PIM_ISL_TEMP_INTERNAL,
    PIM_ISL_PIN_LOOP0,
    PIM_ISL_PIN_LOOP1,
    PIM_ISL_PIN_LOOP2,
    PIM_ISL_POUT_LOOP0,
    PIM_ISL_POUT_LOOP1,
    PIM_ISL_POUT_LOOP2,
    PIM_ISL_IIN_LOOP0,
    PIM_ISL_IIN_LOOP1,
    PIM_ISL_IIN_LOOP2,
    PIM_ISL_IOUT_LOOP0,
    PIM_ISL_IOUT_LOOP1,
    PIM_ISL_IOUT_LOOP2,
    PIM_ECB_VIN,
    PIM_ECB_VOUT,
    PIM_ECB_TEMP,
    PIM_ECB_POWER,
    PIM_ECB_CURR,
  }, {
    PIM_POS_3V3_U_VOUT,
    PIM_POS_3V3_U_TEMP,
    PIM_POS_3V3_U_CURR,
    PIM_POS_3V3_L_VOUT,
    PIM_POS_3V3_L_TEMP,
    PIM_POS_3V3_L_CURR,
    PIM_LM73_TEMP,
    PIM_POS_12V,
    PIM_POS_3V3_E,
    PIM_POS_1V2_E,
    PIM_POS_3V3_U,
    PIM_POS_3V3_L,
    PIM_DPM_TEMP,
    PIM_POS_5V0,
    PIM_POS_0V9,
    PIM_POS_1V5,
    PIM_POS_1V8,
    PIM_ISL_VIN,
    PIM_ISL_VOUT_LOOP0,
    PIM_ISL_VOUT_LOOP1,
    PIM_ISL_VOUT_LOOP2,
    PIM_ISL_TEMP_LOOP0,
    PIM_ISL_TEMP_LOOP1,
    PIM_ISL_TEMP_LOOP2,
    PIM_ISL_TEMP_INTERNAL,
    PIM_ISL_PIN_LOOP0,
    PIM_ISL_PIN_LOOP1,
    PIM_ISL_PIN_LOOP2,
    PIM_ISL_POUT_LOOP0,
    PIM_ISL_POUT_LOOP1,
    PIM_ISL_POUT_LOOP2,
    PIM_ISL_IIN_LOOP0,
    PIM_ISL_IIN_LOOP1,
    PIM_ISL_IIN_LOOP2,
    PIM_ISL_IOUT_LOOP0,
    PIM_ISL_IOUT_LOOP1,
    PIM_ISL_IOUT_LOOP2,
    PIM_ECB_VIN,
    PIM_ECB_VOUT,
    PIM_ECB_TEMP,
    PIM_ECB_POWER,
    PIM_ECB_CURR,
  }, {
    PIM_POS_3V3_U_VOUT,
    PIM_POS_3V3_U_TEMP,
    PIM_POS_3V3_U_CURR,
    PIM_POS_3V3_L_VOUT,
    PIM_POS_3V3_L_TEMP,
    PIM_POS_3V3_L_CURR,
    PIM_LM73_TEMP,
    PIM_POS_12V,
    PIM_POS_3V3_E,
    PIM_POS_1V2_E,
    PIM_POS_3V3_U,
    PIM_POS_3V3_L,
    PIM_DPM_TEMP,
    PIM_POS_5V0,
    PIM_POS_0V9,
    PIM_POS_1V5,
    PIM_POS_1V8,
    PIM_ISL_VIN,
    PIM_ISL_VOUT_LOOP0,
    PIM_ISL_VOUT_LOOP1,
    PIM_ISL_VOUT_LOOP2,
    PIM_ISL_TEMP_LOOP0,
    PIM_ISL_TEMP_LOOP1,
    PIM_ISL_TEMP_LOOP2,
    PIM_ISL_TEMP_INTERNAL,
    PIM_ISL_PIN_LOOP0,
    PIM_ISL_PIN_LOOP1,
    PIM_ISL_PIN_LOOP2,
    PIM_ISL_POUT_LOOP0,
    PIM_ISL_POUT_LOOP1,
    PIM_ISL_POUT_LOOP2,
    PIM_ISL_IIN_LOOP0,
    PIM_ISL_IIN_LOOP1,
    PIM_ISL_IIN_LOOP2,
    PIM_ISL_IOUT_LOOP0,
    PIM_ISL_IOUT_LOOP1,
    PIM_ISL_IOUT_LOOP2,
    PIM_ECB_VIN,
    PIM_ECB_VOUT,
    PIM_ECB_TEMP,
    PIM_ECB_POWER,
    PIM_ECB_CURR,
  },
};

// List of PSU sensors that need to be monitored
const uint8_t psu1_sensor_list[] = {
  PSU1_VIN,
  PSU1_VOUT,
  PSU1_FAN,
  PSU1_TEMP1,
  PSU1_TEMP2,
  PSU1_TEMP3,
  PSU1_PIN,
  PSU1_POUT,
  PSU1_IIN,
  PSU1_IOUT,
};

const uint8_t psu2_sensor_list[] = {
  PSU2_VIN,
  PSU2_VOUT,
  PSU2_FAN,
  PSU2_TEMP1,
  PSU2_TEMP2,
  PSU2_TEMP3,
  PSU2_PIN,
  PSU2_POUT,
  PSU2_IIN,
  PSU2_IOUT,
};

const uint8_t psu3_sensor_list[] = {
  PSU3_VIN,
  PSU3_VOUT,
  PSU3_FAN,
  PSU3_TEMP1,
  PSU3_TEMP2,
  PSU3_TEMP3,
  PSU3_PIN,
  PSU3_POUT,
  PSU3_IIN,
  PSU3_IOUT,
};

const uint8_t psu4_sensor_list[] = {
  PSU4_VIN,
  PSU4_VOUT,
  PSU4_FAN,
  PSU4_TEMP1,
  PSU4_TEMP2,
  PSU4_TEMP3,
  PSU4_PIN,
  PSU4_POUT,
  PSU4_IIN,
  PSU4_IOUT,
};

// List of Fan Card sensors that need to be monitored
const uint8_t fan_sensor_list[] = {
  FAN1_RPM,
  FAN2_RPM,
  FAN3_RPM,
  FAN4_RPM,
  FAN5_RPM,
  FAN_CARD_BOARD_TEMP,
  FAN_CARD_OUTLET_TEMP,
  SYSTEM_AIRFLOW,
};

float scm_sensor_threshold[MAX_SENSOR_NUM + 1][MAX_SENSOR_THRESHOLD + 1] = {0};
float smb_sensor_threshold[MAX_SENSOR_NUM + 1][MAX_SENSOR_THRESHOLD + 1] = {0};
float pim_sensor_threshold[MAX_PIM][MAX_SENSOR_NUM + 1][MAX_SENSOR_THRESHOLD + 1] = {0};
float psu_sensor_threshold[MAX_SENSOR_NUM + 1][MAX_SENSOR_THRESHOLD + 1] = {0};
float fan_sensor_threshold[MAX_SENSOR_NUM + 1][MAX_SENSOR_THRESHOLD + 1] = {0};

size_t scm_sensor_cnt = sizeof(scm_sensor_list) / sizeof(uint8_t);
size_t smb_sensor_cnt = sizeof(smb_sensor_list) / sizeof(uint8_t);
size_t psu1_sensor_cnt = sizeof(psu1_sensor_list) / sizeof(uint8_t);
size_t psu2_sensor_cnt = sizeof(psu2_sensor_list) / sizeof(uint8_t);
size_t psu3_sensor_cnt = sizeof(psu3_sensor_list) / sizeof(uint8_t);
size_t psu4_sensor_cnt = sizeof(psu4_sensor_list) / sizeof(uint8_t);
size_t fan_sensor_cnt = sizeof(fan_sensor_list) / sizeof(uint8_t);

static void
pim_thresh_array_init(uint8_t fru) {
  int i = 0;
  int type;

  switch (fru) {
    case FRU_PIM2:
    case FRU_PIM3:
    case FRU_PIM4:
    case FRU_PIM5:
    case FRU_PIM6:
    case FRU_PIM7:
    case FRU_PIM8:
    case FRU_PIM9:
      i = fru - FRU_PIM2;
      type = pal_get_pim_type_from_file(fru);
      if (type == PIM_TYPE_16Q2) {
        pim_sensor_threshold[i][PIM_POS_3V3_U_VOUT][UCR_THRESH] = 3.63;
        pim_sensor_threshold[i][PIM_POS_3V3_U_VOUT][LCR_THRESH] = 2.98;
        pim_sensor_threshold[i][PIM_POS_3V3_U_VOUT][UNC_THRESH] = 0; // unset
        pim_sensor_threshold[i][PIM_POS_3V3_U_VOUT][LNC_THRESH] = 0; // unset
        pim_sensor_threshold[i][PIM_POS_3V3_U_TEMP][UCR_THRESH] = 150;
        pim_sensor_threshold[i][PIM_POS_3V3_U_TEMP][LCR_THRESH] = 0; // unset
        pim_sensor_threshold[i][PIM_POS_3V3_U_TEMP][UNC_THRESH] = 0; // unset
        pim_sensor_threshold[i][PIM_POS_3V3_U_TEMP][LNC_THRESH] = 0; // unset
        pim_sensor_threshold[i][PIM_POS_3V3_U_CURR][UCR_THRESH] = 32;
        pim_sensor_threshold[i][PIM_POS_3V3_U_CURR][LCR_THRESH] = 0;
        pim_sensor_threshold[i][PIM_POS_3V3_U_CURR][UNC_THRESH] = 0; // unset
        pim_sensor_threshold[i][PIM_POS_3V3_U_CURR][LNC_THRESH] = 0; // unset
        pim_sensor_threshold[i][PIM_POS_3V3_L_VOUT][UCR_THRESH] = 3.63;
        pim_sensor_threshold[i][PIM_POS_3V3_L_VOUT][LCR_THRESH] = 2.98;
        pim_sensor_threshold[i][PIM_POS_3V3_L_VOUT][UNC_THRESH] = 0; // unset
        pim_sensor_threshold[i][PIM_POS_3V3_L_VOUT][LNC_THRESH] = 0; // unset
        pim_sensor_threshold[i][PIM_POS_3V3_L_TEMP][UCR_THRESH] = 150;
        pim_sensor_threshold[i][PIM_POS_3V3_L_TEMP][LCR_THRESH] = 0; // unset
        pim_sensor_threshold[i][PIM_POS_3V3_L_TEMP][UNC_THRESH] = 0; // unset
        pim_sensor_threshold[i][PIM_POS_3V3_L_TEMP][LNC_THRESH] = 0; // unset
        pim_sensor_threshold[i][PIM_POS_3V3_L_CURR][UCR_THRESH] = 32;
        pim_sensor_threshold[i][PIM_POS_3V3_L_CURR][LCR_THRESH] = 0;
        pim_sensor_threshold[i][PIM_POS_3V3_L_CURR][UNC_THRESH] = 0; // unset
        pim_sensor_threshold[i][PIM_POS_3V3_L_CURR][LNC_THRESH] = 0; // unset
      } else if (type == PIM_TYPE_16Q) {
        pim_sensor_threshold[i][PIM_POS_3V3_U_VOUT][UCR_THRESH] = 3.63;
        pim_sensor_threshold[i][PIM_POS_3V3_U_VOUT][LCR_THRESH] = 2.98;
        pim_sensor_threshold[i][PIM_POS_3V3_U_VOUT][UNC_THRESH] = 0; // unset
        pim_sensor_threshold[i][PIM_POS_3V3_U_VOUT][LNC_THRESH] = 0; // unset
        pim_sensor_threshold[i][PIM_POS_3V3_U_TEMP][UCR_THRESH] = 150;
        pim_sensor_threshold[i][PIM_POS_3V3_U_TEMP][LCR_THRESH] = 0; // unset
        pim_sensor_threshold[i][PIM_POS_3V3_U_TEMP][UNC_THRESH] = 0; // unset
        pim_sensor_threshold[i][PIM_POS_3V3_U_TEMP][LNC_THRESH] = 0; // unset
        pim_sensor_threshold[i][PIM_POS_3V3_U_CURR][UCR_THRESH] = 32;
        pim_sensor_threshold[i][PIM_POS_3V3_U_CURR][LCR_THRESH] = 0; // unset
        pim_sensor_threshold[i][PIM_POS_3V3_U_CURR][UNC_THRESH] = 0; // unset
        pim_sensor_threshold[i][PIM_POS_3V3_U_CURR][LNC_THRESH] = 0; // unset
        pim_sensor_threshold[i][PIM_POS_3V3_L_VOUT][UCR_THRESH] = 3.63;
        pim_sensor_threshold[i][PIM_POS_3V3_L_VOUT][LCR_THRESH] = 2.98;
        pim_sensor_threshold[i][PIM_POS_3V3_L_VOUT][UNC_THRESH] = 0; // unset
        pim_sensor_threshold[i][PIM_POS_3V3_L_VOUT][LNC_THRESH] = 0; // unset
        pim_sensor_threshold[i][PIM_POS_3V3_L_TEMP][UCR_THRESH] = 150;
        pim_sensor_threshold[i][PIM_POS_3V3_L_TEMP][LCR_THRESH] = 0; // unset
        pim_sensor_threshold[i][PIM_POS_3V3_L_TEMP][UNC_THRESH] = 0; // unset
        pim_sensor_threshold[i][PIM_POS_3V3_L_TEMP][LNC_THRESH] = 0; // unset
        pim_sensor_threshold[i][PIM_POS_3V3_L_CURR][UCR_THRESH] = 32;
        pim_sensor_threshold[i][PIM_POS_3V3_L_CURR][LCR_THRESH] = 0; // unset
        pim_sensor_threshold[i][PIM_POS_3V3_L_CURR][UNC_THRESH] = 0; // unset
        pim_sensor_threshold[i][PIM_POS_3V3_L_CURR][LNC_THRESH] = 0; // unset
        pim_sensor_threshold[i][PIM_LM73_TEMP][UNC_THRESH] = 90;
        pim_sensor_threshold[i][PIM_LM73_TEMP][UCR_THRESH] = 100;
        pim_sensor_threshold[i][PIM_LM73_TEMP][LNC_THRESH] = 0; // unset
        pim_sensor_threshold[i][PIM_LM73_TEMP][LCR_THRESH] = 0; // unset
        pim_sensor_threshold[i][PIM_POS_12V][UCR_THRESH] = 13.8;
        pim_sensor_threshold[i][PIM_POS_12V][LCR_THRESH] = 10.2;
        pim_sensor_threshold[i][PIM_POS_12V][UNC_THRESH] = 0; // unset
        pim_sensor_threshold[i][PIM_POS_12V][LNC_THRESH] = 0; // unset
        pim_sensor_threshold[i][PIM_POS_3V3_E][UCR_THRESH] = 3.63;
        pim_sensor_threshold[i][PIM_POS_3V3_E][LCR_THRESH] = 2.97;
        pim_sensor_threshold[i][PIM_POS_3V3_E][UNC_THRESH] = 0; // unset
        pim_sensor_threshold[i][PIM_POS_3V3_E][LNC_THRESH] = 0; // unset
        pim_sensor_threshold[i][PIM_POS_1V2_E][UCR_THRESH] = 1.32;
        pim_sensor_threshold[i][PIM_POS_1V2_E][LCR_THRESH] = 1.08;
        pim_sensor_threshold[i][PIM_POS_1V2_E][UNC_THRESH] = 0; // unset
        pim_sensor_threshold[i][PIM_POS_1V2_E][LNC_THRESH] = 0; // unset
        pim_sensor_threshold[i][PIM_POS_3V3_U][UCR_THRESH] = 3.63;
        pim_sensor_threshold[i][PIM_POS_3V3_U][LCR_THRESH] = 2.97;
        pim_sensor_threshold[i][PIM_POS_3V3_U][UNC_THRESH] = 0; // unset
        pim_sensor_threshold[i][PIM_POS_3V3_U][LNC_THRESH] = 0; // unset
        pim_sensor_threshold[i][PIM_POS_3V3_L][UCR_THRESH] = 3.63;
        pim_sensor_threshold[i][PIM_POS_3V3_L][LCR_THRESH] = 2.97;
        pim_sensor_threshold[i][PIM_POS_3V3_L][UNC_THRESH] = 0; // unset
        pim_sensor_threshold[i][PIM_POS_3V3_L][LNC_THRESH] = 0; // unset
        pim_sensor_threshold[i][PIM_POS_3V8_LEDS][UCR_THRESH] = 4.18;
        pim_sensor_threshold[i][PIM_POS_3V8_LEDS][LCR_THRESH] = 3.42;
        pim_sensor_threshold[i][PIM_POS_3V8_LEDS][UNC_THRESH] = 0; // unset
        pim_sensor_threshold[i][PIM_POS_3V8_LEDS][LNC_THRESH] = 0; // unset
        pim_sensor_threshold[i][PIM_DPM_TEMP][UCR_THRESH] = 125;
        pim_sensor_threshold[i][PIM_DPM_TEMP][LCR_THRESH] = 0; // unset
        pim_sensor_threshold[i][PIM_DPM_TEMP][UNC_THRESH] = 0; // unset
        pim_sensor_threshold[i][PIM_DPM_TEMP][LNC_THRESH] = 0; // unset
      } else if (type == PIM_TYPE_8DDM) {
        pim_sensor_threshold[i][PIM_POS_3V3_U_VOUT][UCR_THRESH] = 3.63;
        pim_sensor_threshold[i][PIM_POS_3V3_U_VOUT][LCR_THRESH] = 2.98;
        pim_sensor_threshold[i][PIM_POS_3V3_U_VOUT][UNC_THRESH] = 0; // unset
        pim_sensor_threshold[i][PIM_POS_3V3_U_VOUT][LNC_THRESH] = 0; // unset
        pim_sensor_threshold[i][PIM_POS_3V3_U_TEMP][UCR_THRESH] = 150;
        pim_sensor_threshold[i][PIM_POS_3V3_U_TEMP][LCR_THRESH] = 0; // unset
        pim_sensor_threshold[i][PIM_POS_3V3_U_TEMP][UNC_THRESH] = 0; // unset
        pim_sensor_threshold[i][PIM_POS_3V3_U_TEMP][LNC_THRESH] = 0; // unset
        pim_sensor_threshold[i][PIM_POS_3V3_U_CURR][UCR_THRESH] = 32;
        pim_sensor_threshold[i][PIM_POS_3V3_U_CURR][LCR_THRESH] = 0; // unset
        pim_sensor_threshold[i][PIM_POS_3V3_U_CURR][UNC_THRESH] = 0; // unset
        pim_sensor_threshold[i][PIM_POS_3V3_U_CURR][LNC_THRESH] = 0; // unset
        pim_sensor_threshold[i][PIM_POS_3V3_L_VOUT][UCR_THRESH] = 3.63;
        pim_sensor_threshold[i][PIM_POS_3V3_L_VOUT][LCR_THRESH] = 2.98;
        pim_sensor_threshold[i][PIM_POS_3V3_L_VOUT][UNC_THRESH] = 0; // unset
        pim_sensor_threshold[i][PIM_POS_3V3_L_VOUT][LNC_THRESH] = 0; // unset
        pim_sensor_threshold[i][PIM_POS_3V3_L_TEMP][UCR_THRESH] = 150;
        pim_sensor_threshold[i][PIM_POS_3V3_L_TEMP][LCR_THRESH] = 0; // unset
        pim_sensor_threshold[i][PIM_POS_3V3_L_TEMP][UNC_THRESH] = 0; // unset
        pim_sensor_threshold[i][PIM_POS_3V3_L_TEMP][LNC_THRESH] = 0; // unset
        pim_sensor_threshold[i][PIM_POS_3V3_L_CURR][UCR_THRESH] = 32;
        pim_sensor_threshold[i][PIM_POS_3V3_L_CURR][LCR_THRESH] = 0; // unset
        pim_sensor_threshold[i][PIM_POS_3V3_L_CURR][UNC_THRESH] = 0; // unset
        pim_sensor_threshold[i][PIM_POS_3V3_L_CURR][LNC_THRESH] = 0; // unset
        pim_sensor_threshold[i][PIM_LM73_TEMP][UNC_THRESH] = 90;
        pim_sensor_threshold[i][PIM_LM73_TEMP][UCR_THRESH] = 100;
        pim_sensor_threshold[i][PIM_LM73_TEMP][LNC_THRESH] = 0; // unset
        pim_sensor_threshold[i][PIM_LM73_TEMP][LCR_THRESH] = 0; // unset
        pim_sensor_threshold[i][PIM_POS_12V][UCR_THRESH] = 13.8;
        pim_sensor_threshold[i][PIM_POS_12V][LCR_THRESH] = 9.6;
        pim_sensor_threshold[i][PIM_POS_12V][UNC_THRESH] = 0; // unset
        pim_sensor_threshold[i][PIM_POS_12V][LNC_THRESH] = 0; // unset
        pim_sensor_threshold[i][PIM_POS_3V3_E][UCR_THRESH] = 3.63;
        pim_sensor_threshold[i][PIM_POS_3V3_E][LCR_THRESH] = 2.97;
        pim_sensor_threshold[i][PIM_POS_3V3_E][UNC_THRESH] = 0; // unset
        pim_sensor_threshold[i][PIM_POS_3V3_E][LNC_THRESH] = 0; // unset
        pim_sensor_threshold[i][PIM_POS_1V2_E][UCR_THRESH] = 1.32;
        pim_sensor_threshold[i][PIM_POS_1V2_E][LCR_THRESH] = 1.08;
        pim_sensor_threshold[i][PIM_POS_1V2_E][UNC_THRESH] = 0; // unset
        pim_sensor_threshold[i][PIM_POS_1V2_E][LNC_THRESH] = 0; // unset
        pim_sensor_threshold[i][PIM_POS_3V3_U][UCR_THRESH] = 3.63;
        pim_sensor_threshold[i][PIM_POS_3V3_U][LCR_THRESH] = 2.97;
        pim_sensor_threshold[i][PIM_POS_3V3_U][UNC_THRESH] = 0; // unset
        pim_sensor_threshold[i][PIM_POS_3V3_U][LNC_THRESH] = 0; // unset
        pim_sensor_threshold[i][PIM_POS_3V3_L][UCR_THRESH] = 3.63;
        pim_sensor_threshold[i][PIM_POS_3V3_L][LCR_THRESH] = 2.97;
        pim_sensor_threshold[i][PIM_POS_3V3_L][UNC_THRESH] = 0; // unset
        pim_sensor_threshold[i][PIM_POS_3V3_L][LNC_THRESH] = 0; // unset
        pim_sensor_threshold[i][PIM_DPM_TEMP][UCR_THRESH] = 125;
        pim_sensor_threshold[i][PIM_DPM_TEMP][LCR_THRESH] = 0; // unset
        pim_sensor_threshold[i][PIM_DPM_TEMP][UNC_THRESH] = 0; // unset
        pim_sensor_threshold[i][PIM_DPM_TEMP][LNC_THRESH] = 0; // unset
        pim_sensor_threshold[i][PIM_POS_5V0][UCR_THRESH] = 5.75;
        pim_sensor_threshold[i][PIM_POS_5V0][LCR_THRESH] = 4.25;
        pim_sensor_threshold[i][PIM_POS_5V0][UNC_THRESH] = 0; // unset
        pim_sensor_threshold[i][PIM_POS_5V0][LNC_THRESH] = 0; // unset
        pim_sensor_threshold[i][PIM_POS_0V9][UCR_THRESH] = 0.99;
        pim_sensor_threshold[i][PIM_POS_0V9][LCR_THRESH] = 0.81;
        pim_sensor_threshold[i][PIM_POS_0V9][UNC_THRESH] = 0; // unset
        pim_sensor_threshold[i][PIM_POS_0V9][LNC_THRESH] = 0; // unset
        pim_sensor_threshold[i][PIM_POS_1V5][UCR_THRESH] = 1.7;
        pim_sensor_threshold[i][PIM_POS_1V5][LCR_THRESH] = 1.39;
        pim_sensor_threshold[i][PIM_POS_1V5][UNC_THRESH] = 0; // unset
        pim_sensor_threshold[i][PIM_POS_1V5][LNC_THRESH] = 0; // unset
        pim_sensor_threshold[i][PIM_POS_1V8][UCR_THRESH] = 2.03;
        pim_sensor_threshold[i][PIM_POS_1V8][LCR_THRESH] = 1.66;
        pim_sensor_threshold[i][PIM_POS_1V8][UNC_THRESH] = 0; // unset
        pim_sensor_threshold[i][PIM_POS_1V8][LNC_THRESH] = 0; // unset
        pim_sensor_threshold[i][PIM_ISL_VIN][UCR_THRESH] = 16;
        pim_sensor_threshold[i][PIM_ISL_VIN][LCR_THRESH] = 0; // unset
        pim_sensor_threshold[i][PIM_ISL_VIN][UNC_THRESH] = 0; // unset
        pim_sensor_threshold[i][PIM_ISL_VIN][LNC_THRESH] = 0; // unset
        pim_sensor_threshold[i][PIM_ISL_VOUT_LOOP0][UCR_THRESH] = 0.99;
        pim_sensor_threshold[i][PIM_ISL_VOUT_LOOP0][LCR_THRESH] = 0.81;
        pim_sensor_threshold[i][PIM_ISL_VOUT_LOOP0][UNC_THRESH] = 0; // unset
        pim_sensor_threshold[i][PIM_ISL_VOUT_LOOP0][LNC_THRESH] = 0; // unset
        pim_sensor_threshold[i][PIM_ISL_VOUT_LOOP1][UCR_THRESH] = 1.7;
        pim_sensor_threshold[i][PIM_ISL_VOUT_LOOP1][LCR_THRESH] = 1.39;
        pim_sensor_threshold[i][PIM_ISL_VOUT_LOOP1][UNC_THRESH] = 0; // unset
        pim_sensor_threshold[i][PIM_ISL_VOUT_LOOP1][LNC_THRESH] = 0; // unset
        pim_sensor_threshold[i][PIM_ISL_VOUT_LOOP2][UCR_THRESH] = 2.03;
        pim_sensor_threshold[i][PIM_ISL_VOUT_LOOP2][LCR_THRESH] = 1.66;
        pim_sensor_threshold[i][PIM_ISL_VOUT_LOOP2][UNC_THRESH] = 0; // unset
        pim_sensor_threshold[i][PIM_ISL_VOUT_LOOP2][LNC_THRESH] = 0; // unset
        pim_sensor_threshold[i][PIM_ISL_TEMP_LOOP0][UCR_THRESH] = 125;
        pim_sensor_threshold[i][PIM_ISL_TEMP_LOOP0][LCR_THRESH] = 0; // unset
        pim_sensor_threshold[i][PIM_ISL_TEMP_LOOP0][UNC_THRESH] = 0; // unset
        pim_sensor_threshold[i][PIM_ISL_TEMP_LOOP0][LNC_THRESH] = 0; // unset
        pim_sensor_threshold[i][PIM_ISL_TEMP_LOOP1][UCR_THRESH] = 125;
        pim_sensor_threshold[i][PIM_ISL_TEMP_LOOP1][LCR_THRESH] = 0; // unset
        pim_sensor_threshold[i][PIM_ISL_TEMP_LOOP1][UNC_THRESH] = 0; // unset
        pim_sensor_threshold[i][PIM_ISL_TEMP_LOOP1][LNC_THRESH] = 0; // unset
        pim_sensor_threshold[i][PIM_ISL_TEMP_LOOP2][UCR_THRESH] = 125;
        pim_sensor_threshold[i][PIM_ISL_TEMP_LOOP2][LCR_THRESH] = 0; // unset
        pim_sensor_threshold[i][PIM_ISL_TEMP_LOOP2][UNC_THRESH] = 0; // unset
        pim_sensor_threshold[i][PIM_ISL_TEMP_LOOP2][LNC_THRESH] = 0; // unset
        pim_sensor_threshold[i][PIM_ISL_TEMP_INTERNAL][UCR_THRESH] = 0; // unset
        pim_sensor_threshold[i][PIM_ISL_TEMP_INTERNAL][LCR_THRESH] = 0; // unset
        pim_sensor_threshold[i][PIM_ISL_TEMP_INTERNAL][UNC_THRESH] = 0; // unset
        pim_sensor_threshold[i][PIM_ISL_TEMP_INTERNAL][LNC_THRESH] = 0; // unset
        pim_sensor_threshold[i][PIM_ISL_PIN_LOOP0][UCR_THRESH] = 116;
        pim_sensor_threshold[i][PIM_ISL_PIN_LOOP0][LCR_THRESH] = 0; // unset
        pim_sensor_threshold[i][PIM_ISL_PIN_LOOP0][UNC_THRESH] = 0; // unset
        pim_sensor_threshold[i][PIM_ISL_PIN_LOOP0][LNC_THRESH] = 0; // unset
        pim_sensor_threshold[i][PIM_ISL_PIN_LOOP1][UCR_THRESH] = 51;
        pim_sensor_threshold[i][PIM_ISL_PIN_LOOP1][LCR_THRESH] = 0; // unset
        pim_sensor_threshold[i][PIM_ISL_PIN_LOOP1][UNC_THRESH] = 0; // unset
        pim_sensor_threshold[i][PIM_ISL_PIN_LOOP1][LNC_THRESH] = 0; // unset
        pim_sensor_threshold[i][PIM_ISL_PIN_LOOP2][UCR_THRESH] = 26;
        pim_sensor_threshold[i][PIM_ISL_PIN_LOOP2][LCR_THRESH] = 0; // unset
        pim_sensor_threshold[i][PIM_ISL_PIN_LOOP2][UNC_THRESH] = 0; // unset
        pim_sensor_threshold[i][PIM_ISL_PIN_LOOP2][LNC_THRESH] = 0; // unset
        pim_sensor_threshold[i][PIM_ISL_POUT_LOOP0][UCR_THRESH] = 105;
        pim_sensor_threshold[i][PIM_ISL_POUT_LOOP0][LCR_THRESH] = 0; // unset
        pim_sensor_threshold[i][PIM_ISL_POUT_LOOP0][UNC_THRESH] = 0; // unset
        pim_sensor_threshold[i][PIM_ISL_POUT_LOOP0][LNC_THRESH] = 0; // unset
        pim_sensor_threshold[i][PIM_ISL_POUT_LOOP1][UCR_THRESH] = 44;
        pim_sensor_threshold[i][PIM_ISL_POUT_LOOP1][LCR_THRESH] = 0; // unset
        pim_sensor_threshold[i][PIM_ISL_POUT_LOOP1][UNC_THRESH] = 0; // unset
        pim_sensor_threshold[i][PIM_ISL_POUT_LOOP1][LNC_THRESH] = 0; // unset
        pim_sensor_threshold[i][PIM_ISL_POUT_LOOP2][UCR_THRESH] = 22;
        pim_sensor_threshold[i][PIM_ISL_POUT_LOOP2][LCR_THRESH] = 0; // unset
        pim_sensor_threshold[i][PIM_ISL_POUT_LOOP2][UNC_THRESH] = 0; // unset
        pim_sensor_threshold[i][PIM_ISL_POUT_LOOP2][LNC_THRESH] = 0; // unset
        pim_sensor_threshold[i][PIM_ISL_IIN_LOOP0][UCR_THRESH] = 50;
        pim_sensor_threshold[i][PIM_ISL_IIN_LOOP0][LCR_THRESH] = 0; // unset
        pim_sensor_threshold[i][PIM_ISL_IIN_LOOP0][UNC_THRESH] = 0; // unset
        pim_sensor_threshold[i][PIM_ISL_IIN_LOOP0][LNC_THRESH] = 0; // unset
        pim_sensor_threshold[i][PIM_ISL_IIN_LOOP1][UCR_THRESH] = 50;
        pim_sensor_threshold[i][PIM_ISL_IIN_LOOP1][LCR_THRESH] = 0; // unset
        pim_sensor_threshold[i][PIM_ISL_IIN_LOOP1][UNC_THRESH] = 0; // unset
        pim_sensor_threshold[i][PIM_ISL_IIN_LOOP1][LNC_THRESH] = 0; // unset
        pim_sensor_threshold[i][PIM_ISL_IIN_LOOP2][UCR_THRESH] = 50;
        pim_sensor_threshold[i][PIM_ISL_IIN_LOOP2][LCR_THRESH] = 0; // unset
        pim_sensor_threshold[i][PIM_ISL_IIN_LOOP2][UNC_THRESH] = 0; // unset
        pim_sensor_threshold[i][PIM_ISL_IIN_LOOP2][LNC_THRESH] = 0; // unset
        pim_sensor_threshold[i][PIM_ISL_IOUT_LOOP0][UCR_THRESH] = 116;
        pim_sensor_threshold[i][PIM_ISL_IOUT_LOOP0][UNC_THRESH] = 0; // unset
        pim_sensor_threshold[i][PIM_ISL_IOUT_LOOP0][LCR_THRESH] = 0; // unset
        pim_sensor_threshold[i][PIM_ISL_IOUT_LOOP0][LNC_THRESH] = 0; // unset
        pim_sensor_threshold[i][PIM_ISL_IOUT_LOOP1][UCR_THRESH] = 28;
        pim_sensor_threshold[i][PIM_ISL_IOUT_LOOP1][UNC_THRESH] = 0; // unset
        pim_sensor_threshold[i][PIM_ISL_IOUT_LOOP1][LCR_THRESH] = 0; // unset
        pim_sensor_threshold[i][PIM_ISL_IOUT_LOOP1][LNC_THRESH] = 0; // unset
        pim_sensor_threshold[i][PIM_ISL_IOUT_LOOP2][UCR_THRESH] = 12;
        pim_sensor_threshold[i][PIM_ISL_IOUT_LOOP2][UNC_THRESH] = 0; // unset
        pim_sensor_threshold[i][PIM_ISL_IOUT_LOOP2][LCR_THRESH] = 0; // unset
        pim_sensor_threshold[i][PIM_ISL_IOUT_LOOP2][LNC_THRESH] = 0; // unset
        pim_sensor_threshold[i][PIM_ECB_VIN][UCR_THRESH] = 14.5;
        pim_sensor_threshold[i][PIM_ECB_VIN][LCR_THRESH] = 4.8;
        pim_sensor_threshold[i][PIM_ECB_VIN][UNC_THRESH] = 0; // unset
        pim_sensor_threshold[i][PIM_ECB_VIN][LNC_THRESH] = 0; // unset
        pim_sensor_threshold[i][PIM_ECB_VOUT][UCR_THRESH] = 13.8;
        pim_sensor_threshold[i][PIM_ECB_VOUT][LCR_THRESH] = 9.6;
        pim_sensor_threshold[i][PIM_ECB_VOUT][UNC_THRESH] = 0; // unset
        pim_sensor_threshold[i][PIM_ECB_VOUT][LNC_THRESH] = 0; // unset
        pim_sensor_threshold[i][PIM_ECB_TEMP][UCR_THRESH] = 125;
        pim_sensor_threshold[i][PIM_ECB_TEMP][LCR_THRESH] = 0; // unset
        pim_sensor_threshold[i][PIM_ECB_TEMP][UNC_THRESH] = 0; // unset
        pim_sensor_threshold[i][PIM_ECB_TEMP][LNC_THRESH] = 0; // unset
        pim_sensor_threshold[i][PIM_ECB_POWER][UCR_THRESH] = 540;
        pim_sensor_threshold[i][PIM_ECB_POWER][LCR_THRESH] = 0; // unset
        pim_sensor_threshold[i][PIM_ECB_POWER][UNC_THRESH] = 0; // unset
        pim_sensor_threshold[i][PIM_ECB_POWER][LNC_THRESH] = 0; // unset
        pim_sensor_threshold[i][PIM_ECB_CURR][UCR_THRESH] = 45;
        pim_sensor_threshold[i][PIM_ECB_CURR][LCR_THRESH] = 0; // unset
        pim_sensor_threshold[i][PIM_ECB_CURR][UNC_THRESH] = 0; // unset
        pim_sensor_threshold[i][PIM_ECB_CURR][LNC_THRESH] = 0; // unset
      }
      break;
  }
}

int
pal_set_pim_thresh(uint8_t fru) {
  uint8_t snr_num;
  uint8_t snr_first_num;
  uint8_t snr_last_num;
  int i = fru - FRU_PIM2;

  pim_thresh_array_init(fru);
  syslog(LOG_WARNING,
            "pal_set_pim_thresh: PIM %d set threshold value", fru - FRU_PIM2 + 2);

  switch(fru) {
    case FRU_PIM2:
    case FRU_PIM3:
    case FRU_PIM4:
    case FRU_PIM5:
    case FRU_PIM6:
    case FRU_PIM7:
    case FRU_PIM8:
    case FRU_PIM9:
      snr_first_num = PIM_POS_3V3_U_VOUT;
      snr_last_num = PIM_ISL_IOUT_LOOP2;
      break;
    default:
      return -1;
  }

  for (snr_num = snr_first_num; snr_num <= snr_last_num; snr_num++) {
    if (pim_sensor_threshold[i][snr_num][UNR_THRESH] != 0)
      pal_sensor_thresh_modify(fru, snr_num, UNR_THRESH, pim_sensor_threshold[i][snr_num][UNR_THRESH]);
    if (pim_sensor_threshold[i][snr_num][UCR_THRESH] != 0)
      pal_sensor_thresh_modify(fru, snr_num, UCR_THRESH, pim_sensor_threshold[i][snr_num][UCR_THRESH]);
    if (pim_sensor_threshold[i][snr_num][UNC_THRESH] != 0)
      pal_sensor_thresh_modify(fru, snr_num, UNC_THRESH, pim_sensor_threshold[i][snr_num][UNC_THRESH]);
    if (pim_sensor_threshold[i][snr_num][LNC_THRESH] != 0)
      pal_sensor_thresh_modify(fru, snr_num, LNC_THRESH, pim_sensor_threshold[i][snr_num][LNC_THRESH]);
    if (pim_sensor_threshold[i][snr_num][LCR_THRESH] != 0)
      pal_sensor_thresh_modify(fru, snr_num, LCR_THRESH, pim_sensor_threshold[i][snr_num][LCR_THRESH]);
    if (pim_sensor_threshold[i][snr_num][LNR_THRESH] != 0)
      pal_sensor_thresh_modify(fru, snr_num, LNR_THRESH, pim_sensor_threshold[i][snr_num][LNR_THRESH]);
  }

  return 0;
}

int
pal_clear_thresh_value(uint8_t fru) {
  int ret;
  char fpath[64];
  char cmd[128];
  char fruname[16] = {0};

  ret = pal_get_fru_name(fru, fruname);
  if (ret < 0) {
    printf("%s: Fail to get fru%d name\n", __func__, fru);
    return ret;
  }

  snprintf(fpath, sizeof(fpath), THRESHOLD_BIN, fruname);
  snprintf(cmd, sizeof(cmd), "rm -rf %s", fpath);
  RUN_SHELL_CMD(cmd);

  snprintf(fpath, sizeof(fpath), THRESHOLD_RE_FLAG, fruname);
  snprintf(cmd, sizeof(cmd), "touch %s", fpath);
  RUN_SHELL_CMD(cmd);

  return 0;
}

int
pal_get_fru_sensor_list(uint8_t fru, uint8_t **sensor_list, int *cnt) {
  uint8_t pim_type, pim_id;

  switch(fru) {
    case FRU_SCM:
      *sensor_list = (uint8_t *) scm_sensor_list;
      *cnt = scm_sensor_cnt;
      break;
    case FRU_SMB:
      *sensor_list = (uint8_t *) smb_sensor_list;
      *cnt = smb_sensor_cnt;
      break;
    case FRU_PIM2:
    case FRU_PIM3:
    case FRU_PIM4:
    case FRU_PIM5:
    case FRU_PIM6:
    case FRU_PIM7:
    case FRU_PIM8:
    case FRU_PIM9:
      pim_id = fru - FRU_PIM2;
      pim_type = pal_get_pim_type_from_file(fru);
      if (pim_type == PIM_TYPE_16Q2) {
        *sensor_list = (uint8_t *) pim16q2_sensor_list[pim_id];
        *cnt = sizeof(pim16q2_sensor_list[pim_id]) / sizeof(uint8_t);
      } else if (pim_type == PIM_TYPE_16Q) {
        *sensor_list = (uint8_t *) pim16q_sensor_list[pim_id];
        *cnt = sizeof(pim16q_sensor_list[pim_id]) / sizeof(uint8_t);
      } else if (pim_type == PIM_TYPE_8DDM) {
        *sensor_list = (uint8_t *) pim8ddm_sensor_list[pim_id];
        *cnt = sizeof(pim8ddm_sensor_list[pim_id]) / sizeof(uint8_t);
      } else {
        *sensor_list = NULL;
        *cnt = 0;
      }
      break;
    case FRU_PSU1:
      *sensor_list = (uint8_t *) psu1_sensor_list;
      *cnt = psu1_sensor_cnt;
      break;
    case FRU_PSU2:
      *sensor_list = (uint8_t *) psu2_sensor_list;
      *cnt = psu2_sensor_cnt;
      break;
    case FRU_PSU3:
      *sensor_list = (uint8_t *) psu3_sensor_list;
      *cnt = psu3_sensor_cnt;
      break;
    case FRU_PSU4:
      *sensor_list = (uint8_t *) psu4_sensor_list;
      *cnt = psu4_sensor_cnt;
      break;
    case FRU_FAN:
      *sensor_list = (uint8_t *) fan_sensor_list;
      *cnt = fan_sensor_cnt;
      break;
    default:
      if (fru > MAX_NUM_FRUS)
        return -1;
      // Nothing to read yet.
      *sensor_list = NULL;
      *cnt = 0;
  }

  return 0;
}

static int
get_current_dir(const char *device, char *dir_name) {
  char cmd[LARGEST_DEVICE_NAME + 1];
  FILE *fp;
  int size;

  // Get current working directory
  snprintf(cmd, sizeof(cmd), "cd %s;pwd", device);

  fp = popen(cmd, "r");
  if (NULL == fp)
     return -1;

  if (fgets(dir_name, LARGEST_DEVICE_NAME, fp) == NULL) {
    pclose(fp);
    return -1;
  }

  if (pclose(fp) == -1)
     OBMC_ERROR(errno, "pclose(%s) failed", cmd);

  // Remove the newline character at the end
  size = strlen(dir_name);
  if (size > 0 && isspace(dir_name[size - 1]))
    dir_name[size - 1] = '\0';

  return 0;
}

static int
check_and_read_sensor_value(uint8_t fru, uint8_t snr_num, const char *device,
                            const char *attr, int *value) {
  char dir_name[LARGEST_DEVICE_NAME + 1];

  if (snr_path[fru][snr_num].name != NULL) {
    if (!device_read(snr_path[fru][snr_num].name, value))
      return 0;
  }

  if (strstr(device, "hwmon*") != NULL) {
    /* Get current working directory */
    if (get_current_dir(device, dir_name)) {
      return -1;
    }
    snprintf(snr_path[fru][snr_num].name, sizeof(snr_path[fru][snr_num].name),
             "%s/%s", dir_name, attr);
  } else {
    snprintf(snr_path[fru][snr_num].name, sizeof(snr_path[fru][snr_num].name),
             "%s/%s", device, attr);
  }

  if (device_read(snr_path[fru][snr_num].name, value)) {
    return -1;
  }

  return 0;
}

static int
read_attr(uint8_t fru, uint8_t snr_num, const char *device,
          const char *attr, float *value) {
  int tmp;

  if (check_and_read_sensor_value(fru, snr_num, device, attr, &tmp))
    return -1;

  // Don't scale fan readings.
  if (strstr(attr, "fan"))
    *value = ((float)tmp);
  else
    *value = ((float)tmp) / UNIT_DIV;

  return 0;
}

static int
read_file(const char *device, float *value) {
  FILE *fp;
  int rc;

  fp = fopen(device, "r");
  if (!fp) {
    int err = errno;
#ifdef DEBUG
    syslog(LOG_INFO, "failed to open device %s", device);
#endif
    return err;
  }

  rc = fscanf(fp, "%f", value);
  fclose(fp);
  if (rc != 1) {
#ifdef DEBUG
    syslog(LOG_INFO, "failed to read device %s", device);
#endif
    return ENOENT;
  } else {
    return 0;
  }
}

static int
scm_sensor_read(uint8_t fru, uint8_t sensor_num, float *value) {
  int ret = -1;
  int i = 0;
  bool scm_sensor = false;

  while (i < scm_sensor_cnt) {
    if (sensor_num == scm_sensor_list[i++]) {
      scm_sensor = true;
      break;
    }
  }
  if (scm_sensor) {
    switch(sensor_num) {
      case SCM_ECB_VIN:
        ret = read_attr(fru, sensor_num, SCM_ECB_DEVICE, VOLT(1), value);
        *value = *value / 32;
        break;
      case SCM_ECB_VOUT:
        ret = read_attr(fru, sensor_num, SCM_ECB_DEVICE, VOLT(2), value);
        *value = *value / 32;
        break;
      case SCM_ECB_CURR:
        ret = read_attr(fru, sensor_num, SCM_ECB_DEVICE, CURR(1), value);
        *value = *value / 16;
        break;
      case SCM_ECB_POWER:
        ret = read_attr(fru, sensor_num, SCM_ECB_DEVICE, POWER(1), value);
        *value = *value / 1000;
        break;
      case SCM_ECB_TEMP:
        ret = read_attr(fru, sensor_num, SCM_ECB_DEVICE, TEMP(1), value);
        *value = *value / 2;
        break;
      case SCM_BOARD_TEMP:
        ret = read_attr(fru, sensor_num, SCM_MAX6658_DEVICE, TEMP(1), value);
        break;
      case SCM_INLET_TEMP:
        ret = read_attr(fru, sensor_num, SCM_MAX6658_DEVICE, TEMP(2), value);
        break;
      case SCM_BMC_TEMP:
        ret = read_attr(fru, sensor_num, SCM_LM73_DEVICE, TEMP(1), value);
        break;
      default:
        ret = READING_NA;
        break;
    }
  } else {
    ret = READING_NA;
  }
  return ret;
}

static int
smb_sensor_read(uint8_t fru, uint8_t sensor_num, float *value) {
  int ret = -1;
  int i = 0;
  bool smb_sensor = false;

  while (i < smb_sensor_cnt) {
    if (sensor_num == smb_sensor_list[i++]) {
      smb_sensor = true;
      break;
    }
  }
  if (smb_sensor) {
    switch(sensor_num) {
      case SMB_POS_0V75_CORE:
        ret = read_attr(fru, sensor_num, SMB_UCD90160_DEVICE, VOLT(1), value);
        break;
      case SMB_POS_0V75_ANLG:
        ret = read_attr(fru, sensor_num, SMB_UCD90160_DEVICE, VOLT(2), value);
        break;
      case SMB_POS_1V2:
        ret = read_attr(fru, sensor_num, SMB_UCD90160_DEVICE, VOLT(3), value);
        break;
      case SMB_POS_1V2_ANLG_L:
        ret = read_attr(fru, sensor_num, SMB_UCD90160_DEVICE, VOLT(4), value);
        break;
      case SMB_POS_1V2_ANLG_R:
        ret = read_attr(fru, sensor_num, SMB_UCD90160_DEVICE, VOLT(5), value);
        break;
      case SMB_POS_1V8:
        ret = read_attr(fru, sensor_num, SMB_UCD90160_DEVICE, VOLT(6), value);
        break;
      case SMB_POS_1V8_ANLG:
        ret = read_attr(fru, sensor_num, SMB_UCD90160_DEVICE, VOLT(7), value);
        break;
      case SMB_POS_3V3:
        ret = read_attr(fru, sensor_num, SMB_UCD90160_DEVICE, VOLT(8), value);
        break;
      case SMB_POS_3V3_DPM:
        ret = read_attr(fru, sensor_num, SMB_UCD90160_DEVICE, VOLT(9), value);
        break;
      case SMB_POS_5V0:
        ret = read_attr(fru, sensor_num, SMB_UCD90160_DEVICE, VOLT(10), value);
        break;
      case SMB_POS_12V_B:
        ret = read_attr(fru, sensor_num, SMB_UCD90160_DEVICE, VOLT(11), value);
        break;
      case SMB_POS_12V_A:
        ret = read_attr(fru, sensor_num, SMB_UCD90160_DEVICE, VOLT(12), value);
        break;
      case SMB_POS_1V2B_E:
        ret = read_attr(fru, sensor_num, SMB_UCD90160_DEVICE, VOLT(13), value);
        break;
      case SMB_POS_2V5B_E:
        ret = read_attr(fru, sensor_num, SMB_UCD90160_DEVICE, VOLT(14), value);
        break;
      case SMB_POS_3V3B_E:
        ret = read_attr(fru, sensor_num, SMB_UCD90160_DEVICE, VOLT(15), value);
        break;
      case TH4_VRD1_VIN:
        ret = read_attr(fru, sensor_num, SMB_RAA228228_DEVICE, VOLT(1), value);
        break;
      case TH4_VRD1_VOUT_LOOP0:
        ret = read_attr(fru, sensor_num, SMB_RAA228228_DEVICE, VOLT(3), value);
        break;
      case TH4_VRD1_TEMP_LOOP0:
        ret = read_attr(fru, sensor_num, SMB_RAA228228_DEVICE, TEMP(1), value);
        break;
      case TH4_VRD1_PIN:
        ret = read_attr(fru, sensor_num, SMB_RAA228228_DEVICE, POWER(1), value);
        *value = *value / 1000;
        break;
      case TH4_VRD1_POUT_LOOP0:
        ret = read_attr(fru, sensor_num, SMB_RAA228228_DEVICE, POWER(3), value);
        *value = *value / 1000;
        break;
      case TH4_VRD1_IIN:
        ret = read_attr(fru, sensor_num, SMB_RAA228228_DEVICE, CURR(1), value);
        break;
      case TH4_VRD1_IOUT_LOOP0:
        ret = read_attr(fru, sensor_num, SMB_RAA228228_DEVICE, CURR(3), value);
        break;
      case TH4_VRD2_VIN:
        ret = read_attr(fru, sensor_num, SMB_ISL68226_DEVICE, VOLT(1), value);
        break;
      case TH4_VRD2_VOUT_LOOP0:
        ret = read_attr(fru, sensor_num, SMB_ISL68226_DEVICE, VOLT(3), value);
        break;
      case TH4_VRD2_VOUT_LOOP1:
        ret = read_attr(fru, sensor_num, SMB_ISL68226_DEVICE, VOLT(4), value);
        break;
      case TH4_VRD2_VOUT_LOOP2:
        ret = read_attr(fru, sensor_num, SMB_ISL68226_DEVICE, VOLT(5), value);
        break;
      case TH4_VRD2_TEMP_LOOP0:
        ret = read_attr(fru, sensor_num, SMB_ISL68226_DEVICE, TEMP(1), value);
        break;
      case TH4_VRD2_TEMP_LOOP1:
        ret = read_attr(fru, sensor_num, SMB_ISL68226_DEVICE, TEMP(2), value);
        break;
      case TH4_VRD2_TEMP_LOOP2:
        ret = read_attr(fru, sensor_num, SMB_ISL68226_DEVICE, TEMP(3), value);
        break;
      case TH4_VRD2_TEMP_INTERNAL:
        ret = read_attr(fru, sensor_num, SMB_ISL68226_DEVICE, TEMP(4), value);
        break;
      case TH4_VRD2_PIN_LOOP0:
        ret = read_attr(fru, sensor_num, SMB_ISL68226_DEVICE, POWER(1), value);
        *value = *value / 1000;
        break;
      case TH4_VRD2_PIN_LOOP1:
        ret = read_attr(fru, sensor_num, SMB_ISL68226_DEVICE, POWER(2), value);
        *value = *value / 1000;
        break;
      case TH4_VRD2_PIN_LOOP2:
        ret = read_attr(fru, sensor_num, SMB_ISL68226_DEVICE, POWER(3), value);
        *value = *value / 1000;
        break;
      case TH4_VRD2_POUT_LOOP0:
        ret = read_attr(fru, sensor_num, SMB_ISL68226_DEVICE, POWER(4), value);
        *value = *value / 1000;
        break;
      case TH4_VRD2_POUT_LOOP1:
        ret = read_attr(fru, sensor_num, SMB_ISL68226_DEVICE, POWER(5), value);
        *value = *value / 1000;
        break;
      case TH4_VRD2_POUT_LOOP2:
        ret = read_attr(fru, sensor_num, SMB_ISL68226_DEVICE, POWER(6), value);
        *value = *value / 1000;
        break;
      case TH4_VRD2_IIN_LOOP0:
        ret = read_attr(fru, sensor_num, SMB_ISL68226_DEVICE, CURR(1), value);
        break;
      case TH4_VRD2_IIN_LOOP1:
        ret = read_attr(fru, sensor_num, SMB_ISL68226_DEVICE, CURR(2), value);
        break;
      case TH4_VRD2_IIN_LOOP2:
        ret = read_attr(fru, sensor_num, SMB_ISL68226_DEVICE, CURR(3), value);
        break;
      case TH4_VRD2_IOUT_LOOP0:
        ret = read_attr(fru, sensor_num, SMB_ISL68226_DEVICE, CURR(4), value);
        break;
      case TH4_VRD2_IOUT_LOOP1:
        ret = read_attr(fru, sensor_num, SMB_ISL68226_DEVICE, CURR(5), value);
        break;
      case TH4_VRD2_IOUT_LOOP2:
        ret = read_attr(fru, sensor_num, SMB_ISL68226_DEVICE, CURR(6), value);
        break;
      case SMB_T:
        ret = read_attr(fru, sensor_num, SMB_MAX6581_DEVICE, TEMP(1), value);
        break;
      case TH4_DIE_TEMP_0:
        ret = read_attr(fru, sensor_num, SMB_MAX6581_DEVICE, TEMP(2), value);
        break;
      case TH4_DIE_TEMP_1:
        ret = read_attr(fru, sensor_num, SMB_MAX6581_DEVICE, TEMP(3), value);
        break;
      case SMB_R:
        ret = read_attr(fru, sensor_num, SMB_MAX6581_DEVICE, TEMP(5), value);
        break;
      case SMB_U:
        ret = read_attr(fru, sensor_num, SMB_MAX6581_DEVICE, TEMP(6), value);
        break;
      case SMB_L:
        ret = read_attr(fru, sensor_num, SMB_MAX6581_DEVICE, TEMP(7), value);
        break;
      case BLACKHAWK_CORE_0_5_TEMP:
        ret = read_attr(fru, sensor_num, SMB_NET_BRCM_DEVICE, TEMP(1), value);
        *value = *value / 100;
        break;
      case BLACKHAWK_CORE_6_12_TEMP:
        ret = read_attr(fru, sensor_num, SMB_NET_BRCM_DEVICE, TEMP(2), value);
        *value = *value / 100;
        break;
      case BLACKHAWK_CORE_13_18_TEMP:
        ret = read_attr(fru, sensor_num, SMB_NET_BRCM_DEVICE, TEMP(3), value);
        *value = *value / 100;
        break;
      case BLACKHAWK_CORE_19_25_TEMP:
        ret = read_attr(fru, sensor_num, SMB_NET_BRCM_DEVICE, TEMP(4), value);
        *value = *value / 100;
        break;
      case BLACKHAWK_CORE_26_31_TEMP:
        ret = read_attr(fru, sensor_num, SMB_NET_BRCM_DEVICE, TEMP(5), value);
        *value = *value / 100;
        break;
      case BLACKHAWK_CORE_32_37_TEMP:
        ret = read_attr(fru, sensor_num, SMB_NET_BRCM_DEVICE, TEMP(6), value);
        *value = *value / 100;
        break;
      case BLACKHAWK_CORE_38_44_TEMP:
        ret = read_attr(fru, sensor_num, SMB_NET_BRCM_DEVICE, TEMP(7), value);
        *value = *value / 100;
        break;
      case BLACKHAWK_CORE_45_50_TEMP:
        ret = read_attr(fru, sensor_num, SMB_NET_BRCM_DEVICE, TEMP(8), value);
        *value = *value / 100;
        break;
      case BLACKHAWK_CORE_51_57_TEMP:
        ret = read_attr(fru, sensor_num, SMB_NET_BRCM_DEVICE, TEMP(9), value);
        *value = *value / 100;
        break;
      case BLACKHAWK_CORE_58_63_TEMP:
        ret = read_attr(fru, sensor_num, SMB_NET_BRCM_DEVICE, TEMP(10), value);
        *value = *value / 100;
        break;
      case CORE_0_TEMP:
        ret = read_attr(fru, sensor_num, SMB_NET_BRCM_DEVICE, TEMP(11), value);
        *value = *value / 100;
        break;
      case CORE_1_TEMP:
        ret = read_attr(fru, sensor_num, SMB_NET_BRCM_DEVICE, TEMP(12), value);
        *value = *value / 100;
        break;
      case CORE_2_TEMP:
        ret = read_attr(fru, sensor_num, SMB_NET_BRCM_DEVICE, TEMP(13), value);
        *value = *value / 100;
        break;
      case CORE_3_TEMP:
        ret = read_attr(fru, sensor_num, SMB_NET_BRCM_DEVICE, TEMP(14), value);
        *value = *value / 100;
        break;
      case PIM_QSFP200:
        ret = read_file( "/tmp/.PIM_QSFP200", value );
        break;
      case PIM_QSFP400:
        ret = read_file( "/tmp/.PIM_QSFP400", value );
        break;
      case PIM_F104:
        ret = read_file( "/tmp/.PIM_F104", value );
        break;
      default:
        ret = READING_NA;
        break;
    }
  } else {
    ret = READING_NA;
  }
  return ret;
}

static int
pim_sensor_read(uint8_t fru, uint8_t sensor_num, float *value) {
  char full_name[LARGEST_DEVICE_NAME + 1];
  uint8_t i2cbus = get_pim_i2cbus(fru);
  int ret = -1;

#define dir_pim_sensor_hwmon(str_buffer,i2c_bus,addr) \
      sprintf(str_buffer, \
              I2C_SYSFS_DEVICES"/%d-00%02x/hwmon/hwmon*/", \
              i2c_bus, \
              addr)

  switch(sensor_num) {
    case PIM_POS_3V3_U_VOUT:
      dir_pim_sensor_hwmon(full_name, i2cbus, PIM_TPS546D24_UPPER_DEVICE_ADDR);
      ret = read_attr(fru, sensor_num, full_name, VOLT(2), value);
      break;
    case PIM_POS_3V3_U_TEMP:
      dir_pim_sensor_hwmon(full_name, i2cbus, PIM_TPS546D24_UPPER_DEVICE_ADDR);
      ret = read_attr(fru, sensor_num, full_name, TEMP(1), value);
      break;
    case PIM_POS_3V3_U_CURR:
      dir_pim_sensor_hwmon(full_name, i2cbus, PIM_TPS546D24_UPPER_DEVICE_ADDR);
      ret = read_attr(fru, sensor_num, full_name, CURR(2), value);
      *value = *value + 2;
      break;
    case PIM_POS_3V3_L_VOUT:
      dir_pim_sensor_hwmon(full_name, i2cbus, PIM_TPS546D24_LOWER_DEVICE_ADDR);
      ret = read_attr(fru, sensor_num, full_name, VOLT(2), value);
      break;
    case PIM_POS_3V3_L_TEMP:
      dir_pim_sensor_hwmon(full_name, i2cbus, PIM_TPS546D24_LOWER_DEVICE_ADDR);
      ret = read_attr(fru, sensor_num, full_name, TEMP(1), value);
      break;
    case PIM_POS_3V3_L_CURR:
      dir_pim_sensor_hwmon(full_name, i2cbus, PIM_TPS546D24_LOWER_DEVICE_ADDR);
      ret = read_attr(fru, sensor_num, full_name, CURR(2), value);
      *value = *value + 2;
      break;
    case PIM_LM73_TEMP:
      dir_pim_sensor_hwmon(full_name, i2cbus, PIM_LM73_DEVICE_ADDR);
      ret = read_attr(fru, sensor_num, full_name, TEMP(1), value);
      break;
    case PIM_POS_12V:
      dir_pim_sensor_hwmon(full_name, i2cbus, PIM_UCD9090_DEVICE_ADDR);
      ret = read_attr(fru, sensor_num, full_name, VOLT(1), value);
      break;
    case PIM_POS_3V3_E:
      dir_pim_sensor_hwmon(full_name, i2cbus, PIM_UCD9090_DEVICE_ADDR);
      ret = read_attr(fru, sensor_num, full_name, VOLT(2), value);
      break;
    case PIM_POS_1V2_E:
      dir_pim_sensor_hwmon(full_name, i2cbus, PIM_UCD9090_DEVICE_ADDR);
      ret = read_attr(fru, sensor_num, full_name, VOLT(3), value);
      break;
    case PIM_POS_3V3_U:
      dir_pim_sensor_hwmon(full_name, i2cbus, PIM_UCD9090_DEVICE_ADDR);
      ret = read_attr(fru, sensor_num, full_name, VOLT(4), value);
      break;
    case PIM_POS_3V3_L:
      dir_pim_sensor_hwmon(full_name, i2cbus, PIM_UCD9090_DEVICE_ADDR);
      ret = read_attr(fru, sensor_num, full_name, VOLT(5), value);
      break;
    case PIM_POS_3V8_LEDS:
      dir_pim_sensor_hwmon(full_name, i2cbus, PIM_UCD9090_DEVICE_ADDR);
      ret = read_attr(fru, sensor_num, full_name, VOLT(6), value);
      break;
    case PIM_DPM_TEMP:
      dir_pim_sensor_hwmon(full_name, i2cbus, PIM_UCD9090_DEVICE_ADDR);
      ret = read_attr(fru, sensor_num, full_name, TEMP(1), value);
      break;
    case PIM_POS_5V0:
      dir_pim_sensor_hwmon(full_name, i2cbus, PIM_UCD9090_DEVICE_ADDR);
      ret = read_attr(fru, sensor_num, full_name, VOLT(6), value);
      break;
    case PIM_POS_0V9:
      dir_pim_sensor_hwmon(full_name, i2cbus, PIM_UCD9090_DEVICE_ADDR);
      ret = read_attr(fru, sensor_num, full_name, VOLT(7), value);
      break;
    case PIM_POS_1V5:
      dir_pim_sensor_hwmon(full_name, i2cbus, PIM_UCD9090_DEVICE_ADDR);
      ret = read_attr(fru, sensor_num, full_name, VOLT(8), value);
      break;
    case PIM_POS_1V8:
      dir_pim_sensor_hwmon(full_name, i2cbus, PIM_UCD9090_DEVICE_ADDR);
      ret = read_attr(fru, sensor_num, full_name, VOLT(9), value);
      break;
    case PIM_ISL_VIN:
      dir_pim_sensor_hwmon(full_name, i2cbus, PIM_ISL68224_DEVICE_ADDR);
      ret = read_attr(fru, sensor_num, full_name, VOLT(1), value);
      break;
    case PIM_ISL_VOUT_LOOP0:
      dir_pim_sensor_hwmon(full_name, i2cbus, PIM_ISL68224_DEVICE_ADDR);
      ret = read_attr(fru, sensor_num, full_name, VOLT(3), value);
      break;
    case PIM_ISL_VOUT_LOOP1:
      dir_pim_sensor_hwmon(full_name, i2cbus, PIM_ISL68224_DEVICE_ADDR);
      ret = read_attr(fru, sensor_num, full_name, VOLT(4), value);
      break;
    case PIM_ISL_VOUT_LOOP2:
      dir_pim_sensor_hwmon(full_name, i2cbus, PIM_ISL68224_DEVICE_ADDR);
      ret = read_attr(fru, sensor_num, full_name, VOLT(5), value);
      break;
    case PIM_ISL_TEMP_LOOP0:
      dir_pim_sensor_hwmon(full_name, i2cbus, PIM_ISL68224_DEVICE_ADDR);
      ret = read_attr(fru, sensor_num, full_name, TEMP(1), value);
      break;
    case PIM_ISL_TEMP_LOOP1:
      dir_pim_sensor_hwmon(full_name, i2cbus, PIM_ISL68224_DEVICE_ADDR);
      ret = read_attr(fru, sensor_num, full_name, TEMP(2), value);
      break;
    case PIM_ISL_TEMP_LOOP2:
      dir_pim_sensor_hwmon(full_name, i2cbus, PIM_ISL68224_DEVICE_ADDR);
      ret = read_attr(fru, sensor_num, full_name, TEMP(3), value);
      break;
    case PIM_ISL_TEMP_INTERNAL:
      dir_pim_sensor_hwmon(full_name, i2cbus, PIM_ISL68224_DEVICE_ADDR);
      ret = read_attr(fru, sensor_num, full_name, TEMP(4), value);
      break;
    case PIM_ISL_PIN_LOOP0:
      dir_pim_sensor_hwmon(full_name, i2cbus, PIM_ISL68224_DEVICE_ADDR);
      ret = read_attr(fru, sensor_num, full_name, POWER(1), value);
      *value = *value / 1000;
      break;
    case PIM_ISL_PIN_LOOP1:
      dir_pim_sensor_hwmon(full_name, i2cbus, PIM_ISL68224_DEVICE_ADDR);
      ret = read_attr(fru, sensor_num, full_name, POWER(2), value);
      *value = *value / 1000;
      break;
    case PIM_ISL_PIN_LOOP2:
      dir_pim_sensor_hwmon(full_name, i2cbus, PIM_ISL68224_DEVICE_ADDR);
      ret = read_attr(fru, sensor_num, full_name, POWER(3), value);
      *value = *value / 1000;
      break;
    case PIM_ISL_POUT_LOOP0:
      dir_pim_sensor_hwmon(full_name, i2cbus, PIM_ISL68224_DEVICE_ADDR);
      ret = read_attr(fru, sensor_num, full_name, POWER(4), value);
      *value = *value / 1000;
      break;
    case PIM_ISL_POUT_LOOP1:
      dir_pim_sensor_hwmon(full_name, i2cbus, PIM_ISL68224_DEVICE_ADDR);
      ret = read_attr(fru, sensor_num, full_name, POWER(5), value);
      *value = *value / 1000;
      break;
    case PIM_ISL_POUT_LOOP2:
      dir_pim_sensor_hwmon(full_name, i2cbus, PIM_ISL68224_DEVICE_ADDR);
      ret = read_attr(fru, sensor_num, full_name, POWER(6), value);
      *value = *value / 1000;
      break;
    case PIM_ISL_IIN_LOOP0:
      dir_pim_sensor_hwmon(full_name, i2cbus, PIM_ISL68224_DEVICE_ADDR);
      ret = read_attr(fru, sensor_num, full_name, CURR(1), value);
      break;
    case PIM_ISL_IIN_LOOP1:
      dir_pim_sensor_hwmon(full_name, i2cbus, PIM_ISL68224_DEVICE_ADDR);
      ret = read_attr(fru, sensor_num, full_name, CURR(2), value);
      break;
    case PIM_ISL_IIN_LOOP2:
      dir_pim_sensor_hwmon(full_name, i2cbus, PIM_ISL68224_DEVICE_ADDR);
      ret = read_attr(fru, sensor_num, full_name, CURR(3), value);
      break;
    case PIM_ISL_IOUT_LOOP0:
      dir_pim_sensor_hwmon(full_name, i2cbus, PIM_ISL68224_DEVICE_ADDR);
      ret = read_attr(fru, sensor_num, full_name, CURR(4), value);
      break;
    case PIM_ISL_IOUT_LOOP1:
      dir_pim_sensor_hwmon(full_name, i2cbus, PIM_ISL68224_DEVICE_ADDR);
      ret = read_attr(fru, sensor_num, full_name, CURR(5), value);
      break;
    case PIM_ISL_IOUT_LOOP2:
      dir_pim_sensor_hwmon(full_name, i2cbus, PIM_ISL68224_DEVICE_ADDR);
      ret = read_attr(fru, sensor_num, full_name, CURR(6), value);
      break;
    case PIM_ECB_VIN:
      dir_pim_sensor_hwmon(full_name, i2cbus, PIM_ECB_DEVICE_ADDR);
      ret = read_attr(fru, sensor_num, full_name, VOLT(1), value);
      *value = *value / 32;
      break;
    case PIM_ECB_VOUT:
      dir_pim_sensor_hwmon(full_name, i2cbus, PIM_ECB_DEVICE_ADDR);
      ret = read_attr(fru, sensor_num, full_name, VOLT(2), value);
      *value = *value / 32;
      break;
    case PIM_ECB_TEMP:
      dir_pim_sensor_hwmon(full_name, i2cbus, PIM_ECB_DEVICE_ADDR);
      ret = read_attr(fru, sensor_num, full_name, TEMP(1), value);
      *value = *value / 2;
      break;
    case PIM_ECB_POWER:
      dir_pim_sensor_hwmon(full_name, i2cbus, PIM_ECB_DEVICE_ADDR);
      ret = read_attr(fru, sensor_num, full_name, POWER(1), value);
      *value = *value / 1000;
      break;
    case PIM_ECB_CURR:
      dir_pim_sensor_hwmon(full_name, i2cbus, PIM_ECB_DEVICE_ADDR);
      ret = read_attr(fru, sensor_num, full_name, CURR(1), value);
      *value = *value / 16;
      break;
    default:
      ret = READING_NA;
      break;
  }

  return ret;
}

static int
psu_init_acok_key(uint8_t fru) {
  uint8_t psu_num = fru - FRU_PSU1 + 1;
  char key[MAX_KEY_LEN + 1];

  snprintf(key, MAX_KEY_LEN, "psu%d_acok_state", psu_num);
  kv_set(key, "1", 0, KV_FCREATE);

  return 0;
}

static int
psu_acok_log(uint8_t fru, uint8_t curr_state) {
  uint8_t psu_num = fru - FRU_PSU1 + 1;
  int ret, old_state;
  char key[MAX_KEY_LEN + 1];
  char cvalue[MAX_VALUE_LEN] = {0};

  snprintf(key, MAX_KEY_LEN, "psu%d_acok_state", psu_num);
  ret = kv_get(key, cvalue, NULL, 0);
  if (ret < 0) {
    return ret;
  }

  old_state = atoi(cvalue);

  if (curr_state != old_state) {
    if (curr_state == PSU_ACOK_UP) {
      pal_update_ts_sled();
      syslog(LOG_CRIT, "DEASSERT: PSU%d AC OK state - AC power up - FRU: %d",
             psu_num, fru);
      kv_set(key, "1", 0, 0);
    } else {
      pal_update_ts_sled();
      syslog(LOG_CRIT, "ASSERT: PSU%d AC OK state - AC power down - FRU: %d",
             psu_num, fru);
      kv_set(key, "0", 0, 0);
    }
  }

  return 0;
}

static int
psu_acok_check(uint8_t fru) {
  uint8_t val = 1;
  int ret;

  ret = pal_is_psu_power_ok(fru, &val);
  if (ret) {
      // Failed to ready PSU status.
      return 0;
  }

  if (!val) {
    return READING_NA;
  }

  return val;
}

static int
psu_sensor_read(uint8_t fru, uint8_t sensor_num, float *value) {
  char full_name[LARGEST_DEVICE_NAME + 1];
  int ret = -1;
  uint8_t psuid = fru - FRU_PSU1;
  uint8_t i2cbus = psu_bus[psuid];

  // If power is bad, log but continue reading sensors.
  ret = psu_acok_check(fru);
  if (ret == READING_NA) {
    psu_acok_log(fru, PSU_ACOK_DOWN);
  } else {
    psu_acok_log(fru, PSU_ACOK_UP);
  }

  sprintf(full_name, "%s/%d-00%02x/", I2C_SYSFS_DEVICES, i2cbus,
          PSU_DEVICE_ADDR);

  switch(sensor_num) {
    case PSU1_VIN:
    case PSU2_VIN:
    case PSU3_VIN:
    case PSU4_VIN:
      ret = read_attr(fru, sensor_num, full_name, VOLT(0), value);
      break;
    case PSU1_VOUT:
    case PSU2_VOUT:
    case PSU3_VOUT:
    case PSU4_VOUT:
      ret = read_attr(fru, sensor_num, full_name, VOLT(1), value);
      break;
    case PSU1_FAN:
    case PSU2_FAN:
    case PSU3_FAN:
    case PSU4_FAN:
      ret = read_attr(fru, sensor_num, full_name, FAN(1), value);
      break;
    case PSU1_TEMP1:
    case PSU2_TEMP1:
    case PSU3_TEMP1:
    case PSU4_TEMP1:
      ret = read_attr(fru, sensor_num, full_name, TEMP(1), value);
      break;
    case PSU1_TEMP2:
    case PSU2_TEMP2:
    case PSU3_TEMP2:
    case PSU4_TEMP2:
      ret = read_attr(fru, sensor_num, full_name, TEMP(2), value);
      break;
    case PSU1_TEMP3:
    case PSU2_TEMP3:
    case PSU3_TEMP3:
    case PSU4_TEMP3:
      ret = read_attr(fru, sensor_num, full_name, TEMP(3), value);
      break;
    case PSU1_PIN:
    case PSU2_PIN:
    case PSU3_PIN:
    case PSU4_PIN:
      ret = read_attr(fru, sensor_num, full_name, POWER(1), value);
      break;
    case PSU1_POUT:
    case PSU2_POUT:
    case PSU3_POUT:
    case PSU4_POUT:
      ret = read_attr(fru, sensor_num, full_name, POWER(2), value);
      break;
    case PSU1_IIN:
    case PSU2_IIN:
    case PSU3_IIN:
    case PSU4_IIN:
      ret = read_attr(fru, sensor_num, full_name, CURR(1), value);
      break;
    case PSU1_IOUT:
    case PSU2_IOUT:
    case PSU3_IOUT:
    case PSU4_IOUT:
      ret = read_attr(fru, sensor_num, full_name, CURR(2), value);
      break;
    default:
      ret = READING_NA;
      break;
  }

  return ret;
}

static int
fan_init_sys_airflow_cfg_key(void) {
  char value[MAX_VALUE_LEN];

  snprintf(value, sizeof(value), "%d", CONFIG_UNKNOWN);
  kv_set(KV_SYS_AIRFLOW_CFG_KEY, value, 0, KV_FCREATE);
  return 0;
}

static int
fan_sys_airflow_cfg_log(int curr_state) {
  int ret, old_state;
  char old_value[MAX_VALUE_LEN] = {0};
  char curr_value[MAX_VALUE_LEN] = {0};

  ret = kv_get(KV_SYS_AIRFLOW_CFG_KEY, old_value, NULL, 0);
  if (ret < 0) {
    return ret;
  }

  old_state = atoi(old_value);

  if (curr_state != old_state) {
    switch(curr_state) {
      case CONFIG_8PIM16Q_0PIM8DDM_2PSU:
        syslog(LOG_CRIT,
               "SYSTEM_AIRFLOW: 8 PIM16Q 0 PIM8DDM 2 PSU "
               "fan configuration detected");
        break;
      case CONFIG_5PIM16Q_3PIM8DDM_2PSU:
        syslog(LOG_CRIT,
               "SYSTEM_AIRFLOW: 5 PIM16Q 3 PIM8DDM 2 PSU "
               "fan configuration detected");
        break;
      case CONFIG_2PIM16Q_6PIM8DDM_4PSU:
        syslog(LOG_CRIT,
               "SYSTEM_AIRFLOW: 2 PIM16Q 6 PIM8DDM 4 PSU "
               "fan configuration detected");
        break;
      case CONFIG_0PIM16Q_8PIM8DDM_4PSU:
        syslog(LOG_CRIT,
               "SYSTEM_AIRFLOW: 0 PIM16Q 8 PIM8DDM 4 PSU "
               "fan configuration detected");
        break;
      case CONFIG_UNKNOWN:
      default:
        syslog(LOG_CRIT,
               "SYSTEM_AIRFLOW: UNKNOWN fan configuration detected, "
               "assuming 0 PIM16Q 8 PIM8DDM 4 PSU fan configuration");
        break;
    }

    snprintf(curr_value, sizeof(curr_value), "%d", curr_state);
    kv_set(KV_SYS_AIRFLOW_CFG_KEY, curr_value, 0, 0);
  }

  return 0;
}

static int
fan_sys_airflow_cfg_check(void) {
  uint8_t fru, psu_prsnt, pim_type;
  uint8_t num_pim8ddm = 0, num_pim16q = 0, num_psu = 0;

  for (fru = FRU_PIM2; fru <= FRU_PIM9; fru++) {
    pim_type = pal_get_pim_type_from_file(fru);
    if (pim_type == PIM_TYPE_16Q || pim_type == PIM_TYPE_16Q2) {
      num_pim16q++;
    } else if (pim_type == PIM_TYPE_8DDM) {
      num_pim8ddm++;
    }
  }

  for (fru = FRU_PSU1; fru <= FRU_PSU4; fru++) {
    pal_is_fru_prsnt(fru, &psu_prsnt);
    if (psu_prsnt) {
      num_psu++;
    }
  }

  if (num_pim16q == 8 && num_pim8ddm == 0 && num_psu == 2) {
    return CONFIG_8PIM16Q_0PIM8DDM_2PSU;
  } else if (num_pim16q == 5 && num_pim8ddm == 3 && num_psu == 2) {
    return CONFIG_5PIM16Q_3PIM8DDM_2PSU;
  } else if (num_pim16q == 2 && num_pim8ddm == 6 && num_psu == 4) {
    return CONFIG_2PIM16Q_6PIM8DDM_4PSU;
  } else if (num_pim16q == 0 && num_pim8ddm == 8 && num_psu == 4) {
    return CONFIG_0PIM16Q_8PIM8DDM_4PSU;
  } else {
    return CONFIG_UNKNOWN;
  }
}

static int
fan_sys_airflow_read(float *value) {
  int cfg, pwm_value, pwm_pct, cfm_div, cfm_rem, fan;
  float pwm_total = 0, cfm_delta, *cfm_lookup;
  sensor_path_t pwm_path;

  cfg = fan_sys_airflow_cfg_check();
  fan_sys_airflow_cfg_log(cfg);

  switch(cfg) {
    case CONFIG_8PIM16Q_0PIM8DDM_2PSU:
      cfm_lookup = cfm_lookup_8pim16q_0pim8ddm_2psu;
      break;
    case CONFIG_5PIM16Q_3PIM8DDM_2PSU:
      cfm_lookup = cfm_lookup_5pim16q_3pim8ddm_2psu;
      break;
    case CONFIG_2PIM16Q_6PIM8DDM_4PSU:
      cfm_lookup = cfm_lookup_2pim16q_6pim8ddm_4psu;
      break;
    case CONFIG_0PIM16Q_8PIM8DDM_4PSU:
      cfm_lookup = cfm_lookup_0pim16q_8pim8ddm_4psu;
      break;
    case CONFIG_UNKNOWN:
    default:
      cfm_lookup = cfm_lookup_0pim16q_8pim8ddm_4psu;
      break;
  }

  for (fan = 1; fan <= MAX_FAN; fan++) {
    snprintf(pwm_path.name, sizeof(pwm_path.name), "%s/fan%d_pwm",
             FAN_CPLD_DEVICE, fan);
    if (device_read(pwm_path.name, &pwm_value)) {
      return -1;
    }
    pwm_total += pwm_value;
  }

  pwm_pct = (pwm_total * 100) / (PWM_MAX * MAX_FAN);
  cfm_div = pwm_pct / CFM_STEP;
  cfm_rem = pwm_pct % CFM_STEP;
  cfm_delta = (cfm_lookup[cfm_div + 1] - cfm_lookup[cfm_div]) / CFM_STEP * cfm_rem;
  *value = cfm_lookup[cfm_div] + cfm_delta;

  return 0;
}

static int
fan_sensor_read(uint8_t fru, uint8_t sensor_num, float *value) {
  int ret = -1;
  int i = 0;
  bool fan_sensor = false;

  while (i < fan_sensor_cnt) {
    if (sensor_num == fan_sensor_list[i++]) {
      fan_sensor = true;
      break;
    }
  }

  if (fan_sensor) {
    switch(sensor_num) {
      case FAN1_RPM:
        ret = read_attr(fru, sensor_num, FAN_CPLD_DEVICE, FAN(1), value);
        break;
      case FAN2_RPM:
        ret = read_attr(fru, sensor_num, FAN_CPLD_DEVICE, FAN(2), value);
        break;
      case FAN3_RPM:
        ret = read_attr(fru, sensor_num, FAN_CPLD_DEVICE, FAN(3), value);
        break;
      case FAN4_RPM:
        ret = read_attr(fru, sensor_num, FAN_CPLD_DEVICE, FAN(4), value);
        break;
      case FAN5_RPM:
        ret = read_attr(fru, sensor_num, FAN_CPLD_DEVICE, FAN(5), value);
        break;
      case FAN_CARD_BOARD_TEMP:
        ret = read_attr(fru, sensor_num, FAN_MAX6658_DEVICE, TEMP(1), value);
        break;
      case FAN_CARD_OUTLET_TEMP:
        ret = read_attr(fru, sensor_num, FAN_MAX6658_DEVICE, TEMP(2), value);
        break;
      case SYSTEM_AIRFLOW:
        ret = fan_sys_airflow_read(value);
        break;
      default:
        ret = READING_NA;
        break;
    }
  } else {
    ret = READING_NA;
  }

  return ret;
}

bool is_bios_update_ongoing(void) {
  bool ret = false;
  uint8_t retry = MAX_READ_RETRY;
  char buf[PS_BUF_SIZE];
  FILE* fp;

  while (retry) {
    fp = popen(PS_CMD, "r");
    if (fp)
      break;
    msleep(50);
    retry--;
  }

  if (fp) {
    while (fgets(buf, PS_BUF_SIZE, fp) != NULL) {
      if (strstr(buf, ELBERT_BIOS_UTIL)) {
        ret = true;
        break;
      }
    }
    pclose(fp);
  } else {
#ifdef DEBUG
    syslog(LOG_WARNING, "%s: failed to get ps process output", __func__);
#endif
  }

  return ret;
}

int
pal_sensor_read_raw(uint8_t fru, uint8_t sensor_num, void *value) {
  char key[MAX_KEY_LEN];
  char fru_name[32];
  int ret, delay = 500;
  uint8_t prsnt = 0;

  pal_get_fru_name(fru, fru_name);

  ret = pal_is_fru_prsnt(fru, &prsnt);
  if (ret) {
    return ret;
  }

  if (!prsnt) {
#ifdef DEBUG
  syslog(LOG_INFO, "pal_sensor_read_raw(): %s is not present\n", fru_name);
#endif
    return -1;
  }

  // Don't poll sensors when bios is upgrading as this has been found to
  // cause upgrades to fail.
  if (is_bios_update_ongoing()) {
#ifdef DEBUG
  syslog(LOG_INFO, "pal_sensor_read_raw(): bios upgrade in progress\n");
#endif
    return -1;
  }

  sprintf(key, "%s_sensor%d", fru_name, sensor_num);
  switch(fru) {
    case FRU_SCM:
      ret = scm_sensor_read(fru, sensor_num, value);
      break;
    case FRU_SMB:
      ret = smb_sensor_read(fru, sensor_num, value);
      break;
    case FRU_PIM2:
    case FRU_PIM3:
    case FRU_PIM4:
    case FRU_PIM5:
    case FRU_PIM6:
    case FRU_PIM7:
    case FRU_PIM8:
    case FRU_PIM9:
      ret = pim_sensor_read(fru, sensor_num, value);
      break;
    case FRU_PSU1:
    case FRU_PSU2:
    case FRU_PSU3:
    case FRU_PSU4:
      ret = psu_sensor_read(fru, sensor_num, value);
      break;
    case FRU_FAN:
      ret = fan_sensor_read(fru, sensor_num, value);
      break;
    default:
      return -1;
  }

  if (ret == READING_NA || ret == -1) {
    return READING_NA;
  }
  msleep(delay);

  return 0;
}

static int
get_scm_sensor_name(uint8_t sensor_num, char *name) {
  switch(sensor_num) {
    case SCM_ECB_VIN:
      sprintf(name, "SCM_ECB_VIN");
      break;
    case SCM_ECB_VOUT:
      sprintf(name, "SCM_ECB_VOUT");
      break;
    case SCM_ECB_CURR:
      sprintf(name, "SCM_ECB_CURR");
      break;
    case SCM_ECB_POWER:
      sprintf(name, "SCM_ECB_POWER");
      break;
    case SCM_ECB_TEMP:
      sprintf(name, "SCM_ECB_TEMP");
      break;
    case SCM_BOARD_TEMP:
      sprintf(name, "SCM_BOARD_TEMP");
      break;
    case SCM_INLET_TEMP:
      sprintf(name, "SCM_INLET_TEMP");
      break;
    case SCM_BMC_TEMP:
      sprintf(name, "SCM_BMC_TEMP");
      break;
    default:
      return -1;
  }

  return 0;
}

static int
get_smb_sensor_name(uint8_t sensor_num, char *name) {
  switch(sensor_num) {
    case SMB_POS_0V75_CORE:
      sprintf(name, "SMB_POS_0V75_CORE");
      break;
    case SMB_POS_0V75_ANLG:
      sprintf(name, "SMB_POS_0V75_ANLG");
      break;
    case SMB_POS_1V2:
      sprintf(name, "SMB_POS_1V2");
      break;
    case SMB_POS_1V2_ANLG_L:
      sprintf(name, "SMB_POS_1V2_ANLG_L");
      break;
    case SMB_POS_1V2_ANLG_R:
      sprintf(name, "SMB_POS_1V2_ANLG_R");
      break;
    case SMB_POS_1V8:
      sprintf(name, "SMB_POS_1V8");
      break;
    case SMB_POS_1V8_ANLG:
      sprintf(name, "SMB_POS_1V8_ANLG");
      break;
    case SMB_POS_3V3:
      sprintf(name, "SMB_POS_3V3");
      break;
    case SMB_POS_3V3_DPM:
      sprintf(name, "SMB_POS_3V3_DPM");
      break;
    case SMB_POS_5V0:
      sprintf(name, "SMB_POS_5V0");
      break;
    case SMB_POS_12V_B:
      sprintf(name, "SMB_POS_12V_B");
      break;
    case SMB_POS_12V_A:
      sprintf(name, "SMB_POS_12V_A");
      break;
    case SMB_POS_1V2B_E:
      sprintf(name, "SMB_POS_1V2B_E");
      break;
    case SMB_POS_2V5B_E:
      sprintf(name, "SMB_POS_2V5B_E");
      break;
    case SMB_POS_3V3B_E:
      sprintf(name, "SMB_POS_3V3B_E");
      break;
    case TH4_VRD1_VIN:
      sprintf(name, "TH4_VRD1_VIN");
      break;
    case TH4_VRD1_VOUT_LOOP0:
      sprintf(name, "TH4_VRD1_VOUT_LOOP0");
      break;
    case TH4_VRD1_TEMP_LOOP0:
      sprintf(name, "TH4_VRD1_TEMP_LOOP0");
      break;
    case TH4_VRD1_PIN:
      sprintf(name, "TH4_VRD1_PIN");
      break;
    case TH4_VRD1_POUT_LOOP0:
      sprintf(name, "TH4_VRD1_POUT_LOOP0");
      break;
    case TH4_VRD1_IIN:
      sprintf(name, "TH4_VRD1_IIN");
      break;
    case TH4_VRD1_IOUT_LOOP0:
      sprintf(name, "TH4_VRD1_IOUT_LOOP0");
      break;
    case TH4_VRD2_VIN:
      sprintf(name, "TH4_VRD2_VIN");
      break;
    case TH4_VRD2_VOUT_LOOP0:
      sprintf(name, "TH4_VRD2_VOUT_LOOP0");
      break;
    case TH4_VRD2_VOUT_LOOP1:
      sprintf(name, "TH4_VRD2_VOUT_LOOP1");
      break;
    case TH4_VRD2_VOUT_LOOP2:
      sprintf(name, "TH4_VRD2_VOUT_LOOP2");
      break;
    case TH4_VRD2_TEMP_LOOP0:
      sprintf(name, "TH4_VRD2_TEMP_LOOP0");
      break;
    case TH4_VRD2_TEMP_LOOP1:
      sprintf(name, "TH4_VRD2_TEMP_LOOP1");
      break;
    case TH4_VRD2_TEMP_LOOP2:
      sprintf(name, "TH4_VRD2_TEMP_LOOP2");
      break;
    case TH4_VRD2_TEMP_INTERNAL:
      sprintf(name, "TH4_VRD2_TEMP_INTERNAL");
      break;
    case TH4_VRD2_PIN_LOOP0:
      sprintf(name, "TH4_VRD2_PIN_LOOP0");
      break;
    case TH4_VRD2_PIN_LOOP1:
      sprintf(name, "TH4_VRD2_PIN_LOOP1");
      break;
    case TH4_VRD2_PIN_LOOP2:
      sprintf(name, "TH4_VRD2_PIN_LOOP2");
      break;
    case TH4_VRD2_POUT_LOOP0:
      sprintf(name, "TH4_VRD2_POUT_LOOP0");
      break;
    case TH4_VRD2_POUT_LOOP1:
      sprintf(name, "TH4_VRD2_POUT_LOOP1");
      break;
    case TH4_VRD2_POUT_LOOP2:
      sprintf(name, "TH4_VRD2_POUT_LOOP2");
      break;
    case TH4_VRD2_IIN_LOOP0:
      sprintf(name, "TH4_VRD2_IIN_LOOP0");
      break;
    case TH4_VRD2_IIN_LOOP1:
      sprintf(name, "TH4_VRD2_IIN_LOOP1");
      break;
    case TH4_VRD2_IIN_LOOP2:
      sprintf(name, "TH4_VRD2_IIN_LOOP2");
      break;
    case TH4_VRD2_IOUT_LOOP0:
      sprintf(name, "TH4_VRD2_IOUT_LOOP0");
      break;
    case TH4_VRD2_IOUT_LOOP1:
      sprintf(name, "TH4_VRD2_IOUT_LOOP1");
      break;
    case TH4_VRD2_IOUT_LOOP2:
      sprintf(name, "TH4_VRD2_IOUT_LOOP2");
      break;
    case SMB_T:
      sprintf(name, "SMB_T");
      break;
    case TH4_DIE_TEMP_0:
      sprintf(name, "TH4_DIE_TEMP_0");
      break;
    case TH4_DIE_TEMP_1:
      sprintf(name, "TH4_DIE_TEMP_1");
      break;
    case SMB_R:
      sprintf(name, "SMB_R");
      break;
    case SMB_U:
      sprintf(name, "SMB_U");
      break;
    case SMB_L:
      sprintf(name, "SMB_L");
      break;
    case BLACKHAWK_CORE_0_5_TEMP:
      sprintf(name, "BLACKHAWK_CORE_0-5_TEMP");
      break;
    case BLACKHAWK_CORE_6_12_TEMP:
      sprintf(name, "BLACKHAWK_CORE_6-12_TEMP");
      break;
    case BLACKHAWK_CORE_13_18_TEMP:
      sprintf(name, "BLACKHAWK_CORE_13-18_TEMP");
      break;
    case BLACKHAWK_CORE_19_25_TEMP:
      sprintf(name, "BLACKHAWK_CORE_19-25_TEMP");
      break;
    case BLACKHAWK_CORE_26_31_TEMP:
      sprintf(name, "BLACKHAWK_CORE_26-31_TEMP");
      break;
    case BLACKHAWK_CORE_32_37_TEMP:
      sprintf(name, "BLACKHAWK_CORE_32-37_TEMP");
      break;
    case BLACKHAWK_CORE_38_44_TEMP:
      sprintf(name, "BLACKHAWK_CORE_38-44_TEMP");
      break;
    case BLACKHAWK_CORE_45_50_TEMP:
      sprintf(name, "BLACKHAWK_CORE_45-50_TEMP");
      break;
    case BLACKHAWK_CORE_51_57_TEMP:
      sprintf(name, "BLACKHAWK_CORE_51-57_TEMP");
      break;
    case BLACKHAWK_CORE_58_63_TEMP:
      sprintf(name, "BLACKHAWK_CORE_58-63_TEMP");
      break;
    case CORE_0_TEMP:
      sprintf(name, "CORE_0_TEMP");
      break;
    case CORE_1_TEMP:
      sprintf(name, "CORE_1_TEMP");
      break;
    case CORE_2_TEMP:
      sprintf(name, "CORE_2_TEMP");
      break;
    case CORE_3_TEMP:
      sprintf(name, "CORE_3_TEMP");
      break;
    case PIM_QSFP200:
      sprintf(name, "PIM_QSFP200");
      break;
    case PIM_QSFP400:
      sprintf(name, "PIM_QSFP400");
      break;
    case PIM_F104:
      sprintf(name, "PIM_F104");
      break;
    default:
      return -1;
  }

  return 0;
}

static int
get_pim_sensor_name(uint8_t sensor_num, uint8_t fru, char *name) {
  uint8_t pimid = fru - FRU_PIM2 + 2;

  switch(sensor_num) {
    case PIM_POS_3V3_U_VOUT:
      sprintf(name, "PIM%d_POS_3V3_U_VOUT", pimid);
      break;
    case PIM_POS_3V3_U_TEMP:
      sprintf(name, "PIM%d_POS_3V3_U_TEMP", pimid);
      break;
    case PIM_POS_3V3_U_CURR:
      sprintf(name, "PIM%d_POS_3V3_U_CURR", pimid);
      break;
    case PIM_POS_3V3_L_VOUT:
      sprintf(name, "PIM%d_POS_3V3_L_VOUT", pimid);
      break;
    case PIM_POS_3V3_L_TEMP:
      sprintf(name, "PIM%d_POS_3V3_L_TEMP", pimid);
      break;
    case PIM_POS_3V3_L_CURR:
      sprintf(name, "PIM%d_POS_3V3_L_CURR", pimid);
      break;
    case PIM_LM73_TEMP:
      sprintf(name, "PIM%d_LM73_TEMP", pimid);
      break;
    case PIM_POS_12V:
      sprintf(name, "PIM%d_POS_12V", pimid);
      break;
    case PIM_POS_3V3_E:
      sprintf(name, "PIM%d_POS_3V3_E", pimid);
      break;
    case PIM_POS_1V2_E:
      sprintf(name, "PIM%d_POS_1V2_E", pimid);
      break;
    case PIM_POS_3V3_U:
      sprintf(name, "PIM%d_POS_3V3_U", pimid);
      break;
    case PIM_POS_3V3_L:
      sprintf(name, "PIM%d_POS_3V3_L", pimid);
      break;
    case PIM_POS_3V8_LEDS:
      sprintf(name, "PIM%d_POS_3V8_LEDS", pimid);
      break;
    case PIM_DPM_TEMP:
      sprintf(name, "PIM%d_DPM_TEMP", pimid);
      break;
    case PIM_POS_5V0:
      sprintf(name, "PIM%d_POS_5V0", pimid);
      break;
    case PIM_POS_0V9:
      sprintf(name, "PIM%d_POS_0V9", pimid);
      break;
    case PIM_POS_1V5:
      sprintf(name, "PIM%d_POS_1V5", pimid);
      break;
    case PIM_POS_1V8:
      sprintf(name, "PIM%d_POS_1V8", pimid);
      break;
    case PIM_ISL_VIN:
      sprintf(name, "PIM%d_ISL_VIN", pimid);
      break;
    case PIM_ISL_VOUT_LOOP0:
      sprintf(name, "PIM%d_ISL_VOUT_LOOP0", pimid);
      break;
    case PIM_ISL_VOUT_LOOP1:
      sprintf(name, "PIM%d_ISL_VOUT_LOOP1", pimid);
      break;
    case PIM_ISL_VOUT_LOOP2:
      sprintf(name, "PIM%d_ISL_VOUT_LOOP2", pimid);
      break;
    case PIM_ISL_TEMP_LOOP0:
      sprintf(name, "PIM%d_ISL_TEMP_LOOP0", pimid);
      break;
    case PIM_ISL_TEMP_LOOP1:
      sprintf(name, "PIM%d_ISL_TEMP_LOOP1", pimid);
      break;
    case PIM_ISL_TEMP_LOOP2:
      sprintf(name, "PIM%d_ISL_TEMP_LOOP2", pimid);
      break;
    case PIM_ISL_TEMP_INTERNAL:
      sprintf(name, "PIM%d_ISL_TEMP_INTERNAL", pimid);
      break;
    case PIM_ISL_PIN_LOOP0:
      sprintf(name, "PIM%d_ISL_PIN_LOOP0", pimid);
      break;
    case PIM_ISL_PIN_LOOP1:
      sprintf(name, "PIM%d_ISL_PIN_LOOP1", pimid);
      break;
    case PIM_ISL_PIN_LOOP2:
      sprintf(name, "PIM%d_ISL_PIN_LOOP2", pimid);
      break;
    case PIM_ISL_POUT_LOOP0:
      sprintf(name, "PIM%d_ISL_POUT_LOOP0", pimid);
      break;
    case PIM_ISL_POUT_LOOP1:
      sprintf(name, "PIM%d_ISL_POUT_LOOP1", pimid);
      break;
    case PIM_ISL_POUT_LOOP2:
      sprintf(name, "PIM%d_ISL_POUT_LOOP2", pimid);
      break;
    case PIM_ISL_IIN_LOOP0:
      sprintf(name, "PIM%d_ISL_IIN_LOOP0", pimid);
      break;
    case PIM_ISL_IIN_LOOP1:
      sprintf(name, "PIM%d_ISL_IIN_LOOP1", pimid);
      break;
    case PIM_ISL_IIN_LOOP2:
      sprintf(name, "PIM%d_ISL_IIN_LOOP2", pimid);
      break;
    case PIM_ISL_IOUT_LOOP0:
      sprintf(name, "PIM%d_ISL_IOUT_LOOP0", pimid);
      break;
    case PIM_ISL_IOUT_LOOP1:
      sprintf(name, "PIM%d_ISL_IOUT_LOOP1", pimid);
      break;
    case PIM_ISL_IOUT_LOOP2:
      sprintf(name, "PIM%d_ISL_IOUT_LOOP2", pimid);
      break;
    case PIM_ECB_VIN:
      sprintf(name, "PIM%d_ECB_VIN", pimid);
      break;
    case PIM_ECB_VOUT:
      sprintf(name, "PIM%d_ECB_VOUT", pimid);
      break;
    case PIM_ECB_TEMP:
      sprintf(name, "PIM%d_ECB_TEMP", pimid);
      break;
    case PIM_ECB_POWER:
      sprintf(name, "PIM%d_ECB_POWER", pimid);
      break;
    case PIM_ECB_CURR:
      sprintf(name, "PIM%d_ECB_CURR", pimid);
      break;
    default:
      return -1;
  }

  return 0;
}

static int
get_psu_sensor_name(uint8_t sensor_num, uint8_t fru, char *name) {
  uint8_t psuid = fru - FRU_PSU1 + 1;

  switch (sensor_num) {
    case PSU1_VIN:
    case PSU2_VIN:
    case PSU3_VIN:
    case PSU4_VIN:
      sprintf(name, "PSU%d_VIN", psuid);
      break;
    case PSU1_VOUT:
    case PSU2_VOUT:
    case PSU3_VOUT:
    case PSU4_VOUT:
      sprintf(name, "PSU%d_VOUT", psuid);
      break;
    case PSU1_FAN:
    case PSU2_FAN:
    case PSU3_FAN:
    case PSU4_FAN:
      sprintf(name, "PSU%d_FAN", psuid);
      break;
    case PSU1_TEMP1:
    case PSU2_TEMP1:
    case PSU3_TEMP1:
    case PSU4_TEMP1:
      sprintf(name, "PSU%d_TEMP1", psuid);
      break;
    case PSU1_TEMP2:
    case PSU2_TEMP2:
    case PSU3_TEMP2:
    case PSU4_TEMP2:
      sprintf(name, "PSU%d_TEMP2", psuid);
      break;
    case PSU1_TEMP3:
    case PSU2_TEMP3:
    case PSU3_TEMP3:
    case PSU4_TEMP3:
      sprintf(name, "PSU%d_TEMP3", psuid);
      break;
    case PSU1_PIN:
    case PSU2_PIN:
    case PSU3_PIN:
    case PSU4_PIN:
      sprintf(name, "PSU%d_PIN", psuid);
      break;
    case PSU1_POUT:
    case PSU2_POUT:
    case PSU3_POUT:
    case PSU4_POUT:
      sprintf(name, "PSU%d_POUT", psuid);
      break;
    case PSU1_IIN:
    case PSU2_IIN:
    case PSU3_IIN:
    case PSU4_IIN:
      sprintf(name, "PSU%d_IIN", psuid);
      break;
    case PSU1_IOUT:
    case PSU2_IOUT:
    case PSU3_IOUT:
    case PSU4_IOUT:
      sprintf(name, "PSU%d_IOUT", psuid);
      break;
    default:
      return -1;
  }

  return 0;
}

static int
get_fan_sensor_name(uint8_t sensor_num, char *name) {
  switch(sensor_num) {
    case FAN1_RPM:
      sprintf(name, "FAN1");
      break;
    case FAN2_RPM:
      sprintf(name, "FAN2");
      break;
    case FAN3_RPM:
      sprintf(name, "FAN3");
      break;
    case FAN4_RPM:
      sprintf(name, "FAN4");
      break;
    case FAN5_RPM:
      sprintf(name, "FAN5");
      break;
    case FAN_CARD_BOARD_TEMP:
      sprintf(name, "FAN_CARD_BOARD_TEMP");
      break;
    case FAN_CARD_OUTLET_TEMP:
      sprintf(name, "FAN_CARD_OUTLET_TEMP");
      break;
    case SYSTEM_AIRFLOW:
      sprintf(name, "SYSTEM_AIRFLOW");
      break;
    default:
      return -1;
  }

  return 0;
}

int
pal_get_sensor_name(uint8_t fru, uint8_t sensor_num, char *name) {
  int ret = -1;

  switch(fru) {
    case FRU_SCM:
      ret = get_scm_sensor_name(sensor_num, name);
      break;
    case FRU_SMB:
      ret = get_smb_sensor_name(sensor_num, name);
      break;
    case FRU_PIM2:
    case FRU_PIM3:
    case FRU_PIM4:
    case FRU_PIM5:
    case FRU_PIM6:
    case FRU_PIM7:
    case FRU_PIM8:
    case FRU_PIM9:
      ret = get_pim_sensor_name(sensor_num, fru, name);
      break;
    case FRU_PSU1:
    case FRU_PSU2:
    case FRU_PSU3:
    case FRU_PSU4:
      ret = get_psu_sensor_name(sensor_num, fru, name);
      break;
    case FRU_FAN:
      ret = get_fan_sensor_name(sensor_num, name);
      break;
    default:
      return -1;
  }

  return ret;
}

static int
get_scm_sensor_units(uint8_t sensor_num, char *units) {
  switch(sensor_num) {
    case SCM_ECB_VIN:
    case SCM_ECB_VOUT:
      sprintf(units, VOLT_UNIT);
      break;
    case SCM_ECB_CURR:
      sprintf(units, CURR_UNIT);
      break;
    case SCM_ECB_POWER:
      sprintf(units, POWER_UNIT);
      break;
    case SCM_ECB_TEMP:
    case SCM_BOARD_TEMP:
    case SCM_INLET_TEMP:
    case SCM_BMC_TEMP:
      sprintf(units, TEMP_UNIT);
      break;
    default:
      return -1;
  }

  return 0;
}

static int
get_smb_sensor_units(uint8_t sensor_num, char *units) {
  switch(sensor_num) {
    case SMB_POS_0V75_CORE:
    case SMB_POS_0V75_ANLG:
    case SMB_POS_1V2:
    case SMB_POS_1V2_ANLG_L:
    case SMB_POS_1V2_ANLG_R:
    case SMB_POS_1V8:
    case SMB_POS_1V8_ANLG:
    case SMB_POS_3V3:
    case SMB_POS_3V3_DPM:
    case SMB_POS_5V0:
    case SMB_POS_12V_B:
    case SMB_POS_12V_A:
    case SMB_POS_1V2B_E:
    case SMB_POS_2V5B_E:
    case SMB_POS_3V3B_E:
    case TH4_VRD1_VIN:
    case TH4_VRD1_VOUT_LOOP0:
    case TH4_VRD2_VIN:
    case TH4_VRD2_VOUT_LOOP0:
    case TH4_VRD2_VOUT_LOOP1:
    case TH4_VRD2_VOUT_LOOP2:
      sprintf(units, VOLT_UNIT);
      break;
    case TH4_VRD1_IIN:
    case TH4_VRD1_IOUT_LOOP0:
    case TH4_VRD2_IIN_LOOP0:
    case TH4_VRD2_IIN_LOOP1:
    case TH4_VRD2_IIN_LOOP2:
    case TH4_VRD2_IOUT_LOOP0:
    case TH4_VRD2_IOUT_LOOP1:
    case TH4_VRD2_IOUT_LOOP2:
      sprintf(units, CURR_UNIT);
      break;
    case TH4_VRD1_PIN:
    case TH4_VRD1_POUT_LOOP0:
    case TH4_VRD2_PIN_LOOP0:
    case TH4_VRD2_PIN_LOOP1:
    case TH4_VRD2_PIN_LOOP2:
    case TH4_VRD2_POUT_LOOP0:
    case TH4_VRD2_POUT_LOOP1:
    case TH4_VRD2_POUT_LOOP2:
      sprintf(units, POWER_UNIT);
      break;
    case TH4_VRD1_TEMP_LOOP0:
    case TH4_VRD2_TEMP_LOOP0:
    case TH4_VRD2_TEMP_LOOP1:
    case TH4_VRD2_TEMP_LOOP2:
    case TH4_VRD2_TEMP_INTERNAL:
    case SMB_T:
    case TH4_DIE_TEMP_0:
    case TH4_DIE_TEMP_1:
    case SMB_R:
    case SMB_U:
    case SMB_L:
    case BLACKHAWK_CORE_0_5_TEMP:
    case BLACKHAWK_CORE_6_12_TEMP:
    case BLACKHAWK_CORE_13_18_TEMP:
    case BLACKHAWK_CORE_19_25_TEMP:
    case BLACKHAWK_CORE_26_31_TEMP:
    case BLACKHAWK_CORE_32_37_TEMP:
    case BLACKHAWK_CORE_38_44_TEMP:
    case BLACKHAWK_CORE_45_50_TEMP:
    case BLACKHAWK_CORE_51_57_TEMP:
    case BLACKHAWK_CORE_58_63_TEMP:
    case CORE_0_TEMP:
    case CORE_1_TEMP:
    case CORE_2_TEMP:
    case CORE_3_TEMP:
    case PIM_QSFP200:
    case PIM_QSFP400:
    case PIM_F104:
      sprintf(units, TEMP_UNIT);
      break;
    default:
      return -1;
  }

  return 0;
}

static int
get_pim_sensor_units(uint8_t sensor_num, char *units) {
  switch(sensor_num) {
    case PIM_POS_3V3_U_VOUT:
    case PIM_POS_3V3_L_VOUT:
    case PIM_POS_12V:
    case PIM_POS_3V3_E:
    case PIM_POS_1V2_E:
    case PIM_POS_3V3_U:
    case PIM_POS_3V3_L:
    case PIM_POS_3V8_LEDS:
    case PIM_POS_5V0:
    case PIM_POS_0V9:
    case PIM_POS_1V5:
    case PIM_POS_1V8:
    case PIM_ISL_VIN:
    case PIM_ISL_VOUT_LOOP0:
    case PIM_ISL_VOUT_LOOP1:
    case PIM_ISL_VOUT_LOOP2:
    case PIM_ECB_VIN:
    case PIM_ECB_VOUT:
      sprintf(units, VOLT_UNIT);
      break;
    case PIM_POS_3V3_U_TEMP:
    case PIM_POS_3V3_L_TEMP:
    case PIM_LM73_TEMP:
    case PIM_DPM_TEMP:
    case PIM_ISL_TEMP_LOOP0:
    case PIM_ISL_TEMP_LOOP1:
    case PIM_ISL_TEMP_LOOP2:
    case PIM_ISL_TEMP_INTERNAL:
    case PIM_ECB_TEMP:
      sprintf(units, TEMP_UNIT);
      break;
    case PIM_POS_3V3_U_CURR:
    case PIM_POS_3V3_L_CURR:
    case PIM_ISL_IIN_LOOP0:
    case PIM_ISL_IIN_LOOP1:
    case PIM_ISL_IIN_LOOP2:
    case PIM_ISL_IOUT_LOOP0:
    case PIM_ISL_IOUT_LOOP1:
    case PIM_ISL_IOUT_LOOP2:
    case PIM_ECB_CURR:
      sprintf(units, CURR_UNIT);
      break;
    case PIM_ISL_PIN_LOOP0:
    case PIM_ISL_PIN_LOOP1:
    case PIM_ISL_PIN_LOOP2:
    case PIM_ISL_POUT_LOOP0:
    case PIM_ISL_POUT_LOOP1:
    case PIM_ISL_POUT_LOOP2:
    case PIM_ECB_POWER:
      sprintf(units, POWER_UNIT);
      break;
    default:
      return -1;
  }

  return 0;
}

static int
get_psu_sensor_units(uint8_t sensor_num, char *units) {
  switch(sensor_num) {
    case PSU1_VIN:
    case PSU2_VIN:
    case PSU3_VIN:
    case PSU4_VIN:
    case PSU1_VOUT:
    case PSU2_VOUT:
    case PSU3_VOUT:
    case PSU4_VOUT:
      sprintf(units, VOLT_UNIT);
      break;
    case PSU1_FAN:
    case PSU2_FAN:
    case PSU3_FAN:
    case PSU4_FAN:
      sprintf(units, FAN_UNIT);
      break;
    case PSU1_TEMP1:
    case PSU2_TEMP1:
    case PSU3_TEMP1:
    case PSU4_TEMP1:
    case PSU1_TEMP2:
    case PSU2_TEMP2:
    case PSU3_TEMP2:
    case PSU4_TEMP2:
    case PSU1_TEMP3:
    case PSU2_TEMP3:
    case PSU3_TEMP3:
    case PSU4_TEMP3:
      sprintf(units, TEMP_UNIT);
      break;
    case PSU1_PIN:
    case PSU2_PIN:
    case PSU3_PIN:
    case PSU4_PIN:
    case PSU1_POUT:
    case PSU2_POUT:
    case PSU3_POUT:
    case PSU4_POUT:
      sprintf(units, POWER_UNIT);
      break;
    case PSU1_IIN:
    case PSU2_IIN:
    case PSU3_IIN:
    case PSU4_IIN:
    case PSU1_IOUT:
    case PSU2_IOUT:
    case PSU3_IOUT:
    case PSU4_IOUT:
      sprintf(units, CURR_UNIT);
      break;
    default:
      return -1;
  }

  return 0;
}

static int
get_fan_sensor_units(uint8_t sensor_num, char *units) {
  switch(sensor_num) {
    case FAN1_RPM:
    case FAN2_RPM:
    case FAN3_RPM:
    case FAN4_RPM:
    case FAN5_RPM:
      sprintf(units, FAN_UNIT);
      break;
    case FAN_CARD_BOARD_TEMP:
    case FAN_CARD_OUTLET_TEMP:
      sprintf(units, TEMP_UNIT);
      break;
    case SYSTEM_AIRFLOW:
      sprintf(units, CFM_UNIT);
      break;
    default:
      return -1;
  }

  return 0;
}

int
pal_get_sensor_units(uint8_t fru, uint8_t sensor_num, char *units) {
  int ret = -1;

  switch(fru) {
    case FRU_SCM:
      ret = get_scm_sensor_units(sensor_num, units);
      break;
    case FRU_SMB:
      ret = get_smb_sensor_units(sensor_num, units);
      break;
    case FRU_PIM2:
    case FRU_PIM3:
    case FRU_PIM4:
    case FRU_PIM5:
    case FRU_PIM6:
    case FRU_PIM7:
    case FRU_PIM8:
    case FRU_PIM9:
      ret = get_pim_sensor_units(sensor_num, units);
      break;
    case FRU_PSU1:
    case FRU_PSU2:
    case FRU_PSU3:
    case FRU_PSU4:
      ret = get_psu_sensor_units(sensor_num, units);
      break;
    case FRU_FAN:
      ret = get_fan_sensor_units(sensor_num, units);
      break;
    default:
      return -1;
  }
  return ret;
}

static void
sensor_thresh_array_init(uint8_t fru) {
  int i;

  if (init_threshold_done[fru])
    return;

  switch (fru) {
    case FRU_SCM:
      // ECB
      scm_sensor_threshold[SCM_ECB_VIN][UCR_THRESH] = 14.4;
      scm_sensor_threshold[SCM_ECB_VIN][LCR_THRESH] = 4.8;
      scm_sensor_threshold[SCM_ECB_VIN][UNC_THRESH] = 0; // unset
      scm_sensor_threshold[SCM_ECB_VIN][LNC_THRESH] = 0; // unset
      scm_sensor_threshold[SCM_ECB_VOUT][UCR_THRESH] = 14.4;
      scm_sensor_threshold[SCM_ECB_VOUT][LCR_THRESH] = 9.6;
      scm_sensor_threshold[SCM_ECB_VOUT][UNC_THRESH] = 0; // unset
      scm_sensor_threshold[SCM_ECB_VOUT][LNC_THRESH] = 0; // unset
      scm_sensor_threshold[SCM_ECB_CURR][UCR_THRESH] = 24;
      scm_sensor_threshold[SCM_ECB_CURR][LCR_THRESH] = 0; // unset
      scm_sensor_threshold[SCM_ECB_CURR][UNC_THRESH] = 0; // unset
      scm_sensor_threshold[SCM_ECB_CURR][LNC_THRESH] = 0; // unset
      scm_sensor_threshold[SCM_ECB_POWER][UCR_THRESH] = 288;
      scm_sensor_threshold[SCM_ECB_POWER][LCR_THRESH] = 0; // unset
      scm_sensor_threshold[SCM_ECB_POWER][UNC_THRESH] = 0; // unset
      scm_sensor_threshold[SCM_ECB_POWER][LNC_THRESH] = 0; // unset
      scm_sensor_threshold[SCM_ECB_TEMP][UCR_THRESH] = 125;
      scm_sensor_threshold[SCM_ECB_TEMP][LCR_THRESH] = 0; // unset
      scm_sensor_threshold[SCM_ECB_TEMP][UNC_THRESH] = 0; // unset
      scm_sensor_threshold[SCM_ECB_TEMP][LNC_THRESH] = 0; // unset
      // CPU board temp sensors
      scm_sensor_threshold[SCM_BOARD_TEMP][LNC_THRESH] = 0; // unset
      scm_sensor_threshold[SCM_BOARD_TEMP][LCR_THRESH] = 0; // unset
      scm_sensor_threshold[SCM_BOARD_TEMP][UNC_THRESH] = 75;
      scm_sensor_threshold[SCM_BOARD_TEMP][UCR_THRESH] = 85;
      // Front-panel temp sensors
      scm_sensor_threshold[SCM_INLET_TEMP][LNC_THRESH] = 0; // unset
      scm_sensor_threshold[SCM_INLET_TEMP][LCR_THRESH] = 0; // unset
      scm_sensor_threshold[SCM_INLET_TEMP][UNC_THRESH] = 70;
      scm_sensor_threshold[SCM_INLET_TEMP][UCR_THRESH] = 75;
      // BMC temp sensor
      scm_sensor_threshold[SCM_BMC_TEMP][LNC_THRESH] = 0; // unset
      scm_sensor_threshold[SCM_BMC_TEMP][LCR_THRESH] = 0; // unset
      scm_sensor_threshold[SCM_BMC_TEMP][UNC_THRESH] = 80;
      scm_sensor_threshold[SCM_BMC_TEMP][UCR_THRESH] = 90;
      break;
    case FRU_SMB:
      // ECB sensors
      smb_sensor_threshold[SMB_POS_0V75_CORE][UCR_THRESH] = 1.066;
      smb_sensor_threshold[SMB_POS_0V75_CORE][LCR_THRESH] = 0.64;
      smb_sensor_threshold[SMB_POS_0V75_CORE][UNC_THRESH] = 0; // unset
      smb_sensor_threshold[SMB_POS_0V75_CORE][LNC_THRESH] = 0; // unset
      smb_sensor_threshold[SMB_POS_0V75_ANLG][UCR_THRESH] = 0.885;
      smb_sensor_threshold[SMB_POS_0V75_ANLG][LCR_THRESH] = 0.655;
      smb_sensor_threshold[SMB_POS_0V75_ANLG][UNC_THRESH] = 0; // unset
      smb_sensor_threshold[SMB_POS_0V75_ANLG][LNC_THRESH] = 0; // unset
      smb_sensor_threshold[SMB_POS_1V2][UCR_THRESH] = 1.38;
      smb_sensor_threshold[SMB_POS_1V2][LCR_THRESH] = 1.02;
      smb_sensor_threshold[SMB_POS_1V2][UNC_THRESH] = 0; // unset
      smb_sensor_threshold[SMB_POS_1V2][LNC_THRESH] = 0; // unset
      smb_sensor_threshold[SMB_POS_1V2_ANLG_L][UCR_THRESH] = 1.38;
      smb_sensor_threshold[SMB_POS_1V2_ANLG_L][LCR_THRESH] = 1.02;
      smb_sensor_threshold[SMB_POS_1V2_ANLG_L][UNC_THRESH] = 0; // unset
      smb_sensor_threshold[SMB_POS_1V2_ANLG_L][LNC_THRESH] = 0; // unset
      smb_sensor_threshold[SMB_POS_1V2_ANLG_R][UCR_THRESH] = 1.38;
      smb_sensor_threshold[SMB_POS_1V2_ANLG_R][LCR_THRESH] = 1.02;
      smb_sensor_threshold[SMB_POS_1V2_ANLG_R][UNC_THRESH] = 0; // unset
      smb_sensor_threshold[SMB_POS_1V2_ANLG_R][LNC_THRESH] = 0; // unset
      smb_sensor_threshold[SMB_POS_1V8][UCR_THRESH] = 2.07;
      smb_sensor_threshold[SMB_POS_1V8][LCR_THRESH] = 1.53;
      smb_sensor_threshold[SMB_POS_1V8][UNC_THRESH] = 0; // unset
      smb_sensor_threshold[SMB_POS_1V8][LNC_THRESH] = 0; // unset
      smb_sensor_threshold[SMB_POS_1V8_ANLG][UCR_THRESH] = 2.07;
      smb_sensor_threshold[SMB_POS_1V8_ANLG][LCR_THRESH] = 1.53;
      smb_sensor_threshold[SMB_POS_1V8_ANLG][UNC_THRESH] = 0; // unset
      smb_sensor_threshold[SMB_POS_1V8_ANLG][LNC_THRESH] = 0; // unset
      smb_sensor_threshold[SMB_POS_3V3][UCR_THRESH] = 3.795;
      smb_sensor_threshold[SMB_POS_3V3][LCR_THRESH] = 2.805;
      smb_sensor_threshold[SMB_POS_3V3][UNC_THRESH] = 0; // unset
      smb_sensor_threshold[SMB_POS_3V3][LNC_THRESH] = 0; // unset
      smb_sensor_threshold[SMB_POS_3V3_DPM][UCR_THRESH] = 3.795;
      smb_sensor_threshold[SMB_POS_3V3_DPM][LCR_THRESH] = 2.805;
      smb_sensor_threshold[SMB_POS_3V3_DPM][UNC_THRESH] = 0; // unset
      smb_sensor_threshold[SMB_POS_3V3_DPM][LNC_THRESH] = 0; // unset
      smb_sensor_threshold[SMB_POS_5V0][UCR_THRESH] = 5.75;
      smb_sensor_threshold[SMB_POS_5V0][LCR_THRESH] = 4.25;
      smb_sensor_threshold[SMB_POS_5V0][UNC_THRESH] = 0; // unset
      smb_sensor_threshold[SMB_POS_5V0][LNC_THRESH] = 0; // unset
      smb_sensor_threshold[SMB_POS_12V_B][UCR_THRESH] = 13.8;
      smb_sensor_threshold[SMB_POS_12V_B][LCR_THRESH] = 9.5;
      smb_sensor_threshold[SMB_POS_12V_B][UNC_THRESH] = 0; // unset
      smb_sensor_threshold[SMB_POS_12V_B][LNC_THRESH] = 0; // unset
      smb_sensor_threshold[SMB_POS_12V_A][UCR_THRESH] = 13.8;
      smb_sensor_threshold[SMB_POS_12V_A][LCR_THRESH] = 9.5;
      smb_sensor_threshold[SMB_POS_12V_A][UNC_THRESH] = 0; // unset
      smb_sensor_threshold[SMB_POS_12V_A][LNC_THRESH] = 0; // unset
      smb_sensor_threshold[SMB_POS_1V2B_E][UCR_THRESH] = 1.38;
      smb_sensor_threshold[SMB_POS_1V2B_E][LCR_THRESH] = 1.02;
      smb_sensor_threshold[SMB_POS_1V2B_E][UNC_THRESH] = 0; // unset
      smb_sensor_threshold[SMB_POS_1V2B_E][LNC_THRESH] = 0; // unset
      smb_sensor_threshold[SMB_POS_2V5B_E][UCR_THRESH] = 2.875;
      smb_sensor_threshold[SMB_POS_2V5B_E][LCR_THRESH] = 2.125;
      smb_sensor_threshold[SMB_POS_2V5B_E][UNC_THRESH] = 0; // unset
      smb_sensor_threshold[SMB_POS_2V5B_E][LNC_THRESH] = 0; // unset
      smb_sensor_threshold[SMB_POS_3V3B_E][UCR_THRESH] = 3.795;
      smb_sensor_threshold[SMB_POS_3V3B_E][LCR_THRESH] = 2.805;
      smb_sensor_threshold[SMB_POS_3V3B_E][UNC_THRESH] = 0; // unset
      smb_sensor_threshold[SMB_POS_3V3B_E][LNC_THRESH] = 0; // unset
      // RAA228228 sensors
      smb_sensor_threshold[TH4_VRD1_VIN][UCR_THRESH] = 16;
      smb_sensor_threshold[TH4_VRD1_VIN][LCR_THRESH] = 0; // unset
      smb_sensor_threshold[TH4_VRD1_VIN][UNC_THRESH] = 0; // unset
      smb_sensor_threshold[TH4_VRD1_VIN][LNC_THRESH] = 0; // unset
      smb_sensor_threshold[TH4_VRD1_VOUT_LOOP0][UCR_THRESH] = 1.068;
      smb_sensor_threshold[TH4_VRD1_VOUT_LOOP0][LCR_THRESH] = 0.712;
      smb_sensor_threshold[TH4_VRD1_VOUT_LOOP0][UNC_THRESH] = 0; // unset
      smb_sensor_threshold[TH4_VRD1_VOUT_LOOP0][LNC_THRESH] = 0; // unset
      smb_sensor_threshold[TH4_VRD1_TEMP_LOOP0][UCR_THRESH] = 125;
      smb_sensor_threshold[TH4_VRD1_TEMP_LOOP0][LCR_THRESH] = -40;
      smb_sensor_threshold[TH4_VRD1_TEMP_LOOP0][UNC_THRESH] = 0; // unset
      smb_sensor_threshold[TH4_VRD1_TEMP_LOOP0][LNC_THRESH] = 0; // unset
      smb_sensor_threshold[TH4_VRD1_PIN][UCR_THRESH] = 600;
      smb_sensor_threshold[TH4_VRD1_PIN][LCR_THRESH] = 0; // unset
      smb_sensor_threshold[TH4_VRD1_PIN][UNC_THRESH] = 0; // unset
      smb_sensor_threshold[TH4_VRD1_PIN][LNC_THRESH] = 0; // unset
      smb_sensor_threshold[TH4_VRD1_POUT_LOOP0][UCR_THRESH] = 600;
      smb_sensor_threshold[TH4_VRD1_IIN][UCR_THRESH] = 50;
      smb_sensor_threshold[TH4_VRD1_IIN][LCR_THRESH] = 0; // unset
      smb_sensor_threshold[TH4_VRD1_IIN][UNC_THRESH] = 0; // unset
      smb_sensor_threshold[TH4_VRD1_IIN][LNC_THRESH] = 0; // unset
      smb_sensor_threshold[TH4_VRD1_IOUT_LOOP0][UCR_THRESH] = 660;
      smb_sensor_threshold[TH4_VRD1_IOUT_LOOP0][LCR_THRESH] = 0; // unset
      smb_sensor_threshold[TH4_VRD1_IOUT_LOOP0][UNC_THRESH] = 0; // unset
      smb_sensor_threshold[TH4_VRD1_IOUT_LOOP0][LNC_THRESH] = 0; // unset
      smb_sensor_threshold[TH4_VRD2_VIN][UCR_THRESH] = 16;
      smb_sensor_threshold[TH4_VRD2_VIN][LCR_THRESH] = 0; // unset
      smb_sensor_threshold[TH4_VRD2_VIN][UNC_THRESH] = 0; // unset
      smb_sensor_threshold[TH4_VRD2_VIN][LNC_THRESH] = 0; // unset
      smb_sensor_threshold[TH4_VRD2_VOUT_LOOP0][UCR_THRESH] = 0.847;
      smb_sensor_threshold[TH4_VRD2_VOUT_LOOP0][LCR_THRESH] = 0.693;
      smb_sensor_threshold[TH4_VRD2_VOUT_LOOP0][UNC_THRESH] = 0; // unset
      smb_sensor_threshold[TH4_VRD2_VOUT_LOOP0][LNC_THRESH] = 0; // unset
      smb_sensor_threshold[TH4_VRD2_VOUT_LOOP1][UCR_THRESH] = 1.32;
      smb_sensor_threshold[TH4_VRD2_VOUT_LOOP1][LCR_THRESH] = 1.08;
      smb_sensor_threshold[TH4_VRD2_VOUT_LOOP1][UNC_THRESH] = 0; // unset
      smb_sensor_threshold[TH4_VRD2_VOUT_LOOP1][LNC_THRESH] = 0; // unset
      smb_sensor_threshold[TH4_VRD2_VOUT_LOOP2][UCR_THRESH] = 1.32;
      smb_sensor_threshold[TH4_VRD2_VOUT_LOOP2][LCR_THRESH] = 1.08;
      smb_sensor_threshold[TH4_VRD2_VOUT_LOOP2][UNC_THRESH] = 0; // unset
      smb_sensor_threshold[TH4_VRD2_VOUT_LOOP2][LNC_THRESH] = 0; // unset
      smb_sensor_threshold[TH4_VRD2_TEMP_LOOP0][UCR_THRESH] = 125;
      smb_sensor_threshold[TH4_VRD2_TEMP_LOOP0][LCR_THRESH] = -40;
      smb_sensor_threshold[TH4_VRD2_TEMP_LOOP0][UNC_THRESH] = 0; // unset
      smb_sensor_threshold[TH4_VRD2_TEMP_LOOP0][LNC_THRESH] = 0; // unset
      smb_sensor_threshold[TH4_VRD2_TEMP_LOOP1][UCR_THRESH] = 125;
      smb_sensor_threshold[TH4_VRD2_TEMP_LOOP1][LCR_THRESH] = -40;
      smb_sensor_threshold[TH4_VRD2_TEMP_LOOP1][UNC_THRESH] = 0; // unset
      smb_sensor_threshold[TH4_VRD2_TEMP_LOOP1][LNC_THRESH] = 0; // unset
      smb_sensor_threshold[TH4_VRD2_TEMP_LOOP2][UCR_THRESH] = 125;
      smb_sensor_threshold[TH4_VRD2_TEMP_LOOP2][LCR_THRESH] = -40;
      smb_sensor_threshold[TH4_VRD2_TEMP_LOOP2][UNC_THRESH] = 0; // unset
      smb_sensor_threshold[TH4_VRD2_TEMP_LOOP2][LNC_THRESH] = 0; // unset
      smb_sensor_threshold[TH4_VRD2_TEMP_INTERNAL][UCR_THRESH] = 0; // unset
      smb_sensor_threshold[TH4_VRD2_TEMP_INTERNAL][LCR_THRESH] = 0; // unset
      smb_sensor_threshold[TH4_VRD2_TEMP_INTERNAL][UNC_THRESH] = 0; // unset
      smb_sensor_threshold[TH4_VRD2_TEMP_INTERNAL][LNC_THRESH] = 0; // unset
      smb_sensor_threshold[TH4_VRD2_PIN_LOOP0][UCR_THRESH] = 600;
      smb_sensor_threshold[TH4_VRD2_PIN_LOOP0][LCR_THRESH] = 0; // unset
      smb_sensor_threshold[TH4_VRD2_PIN_LOOP0][UNC_THRESH] = 0; // unset
      smb_sensor_threshold[TH4_VRD2_PIN_LOOP0][LNC_THRESH] = 0; // unset
      smb_sensor_threshold[TH4_VRD2_PIN_LOOP1][UCR_THRESH] = 0; // unset
      smb_sensor_threshold[TH4_VRD2_PIN_LOOP1][LCR_THRESH] = -600;
      smb_sensor_threshold[TH4_VRD2_PIN_LOOP1][UNC_THRESH] = 0; // unset
      smb_sensor_threshold[TH4_VRD2_PIN_LOOP1][LNC_THRESH] = 0; // unset
      smb_sensor_threshold[TH4_VRD2_PIN_LOOP2][UCR_THRESH] = 0; //unset
      smb_sensor_threshold[TH4_VRD2_PIN_LOOP2][LCR_THRESH] = -600;
      smb_sensor_threshold[TH4_VRD2_PIN_LOOP2][UNC_THRESH] = 0; // unset
      smb_sensor_threshold[TH4_VRD2_PIN_LOOP2][LNC_THRESH] = 0; // unset
      smb_sensor_threshold[TH4_VRD2_POUT_LOOP0][UCR_THRESH] = 135.5;
      smb_sensor_threshold[TH4_VRD2_POUT_LOOP0][LCR_THRESH] = 0; // unset
      smb_sensor_threshold[TH4_VRD2_POUT_LOOP0][UNC_THRESH] = 0; // unset
      smb_sensor_threshold[TH4_VRD2_POUT_LOOP0][LNC_THRESH] = 0; // unset
      smb_sensor_threshold[TH4_VRD2_POUT_LOOP1][UCR_THRESH] = 0; //unset
      smb_sensor_threshold[TH4_VRD2_POUT_LOOP1][LCR_THRESH] = -18;
      smb_sensor_threshold[TH4_VRD2_POUT_LOOP1][UNC_THRESH] = 0; // unset
      smb_sensor_threshold[TH4_VRD2_POUT_LOOP1][LNC_THRESH] = 0; // unset
      smb_sensor_threshold[TH4_VRD2_POUT_LOOP2][UCR_THRESH] = 0; //unset
      smb_sensor_threshold[TH4_VRD2_POUT_LOOP2][LCR_THRESH] = -18;
      smb_sensor_threshold[TH4_VRD2_POUT_LOOP2][UNC_THRESH] = 0; // unset
      smb_sensor_threshold[TH4_VRD2_POUT_LOOP2][LNC_THRESH] = 0; // unset
      smb_sensor_threshold[TH4_VRD2_IIN_LOOP0][UCR_THRESH] = 50;
      smb_sensor_threshold[TH4_VRD2_IIN_LOOP0][LCR_THRESH] = 0; // unset
      smb_sensor_threshold[TH4_VRD2_IIN_LOOP0][UNC_THRESH] = 0; // unset
      smb_sensor_threshold[TH4_VRD2_IIN_LOOP0][LNC_THRESH] = 0; // unset
      smb_sensor_threshold[TH4_VRD2_IIN_LOOP1][UCR_THRESH] = 0; // unset
      smb_sensor_threshold[TH4_VRD2_IIN_LOOP1][LCR_THRESH] = -50;
      smb_sensor_threshold[TH4_VRD2_IIN_LOOP1][UNC_THRESH] = 0; // unset
      smb_sensor_threshold[TH4_VRD2_IIN_LOOP1][LNC_THRESH] = 0; // unset
      smb_sensor_threshold[TH4_VRD2_IIN_LOOP2][UCR_THRESH] = 50;
      smb_sensor_threshold[TH4_VRD2_IIN_LOOP2][LCR_THRESH] = 0; // unset
      smb_sensor_threshold[TH4_VRD2_IIN_LOOP2][UNC_THRESH] = 0; // unset
      smb_sensor_threshold[TH4_VRD2_IIN_LOOP2][LNC_THRESH] = 0; // unset
      smb_sensor_threshold[TH4_VRD2_IOUT_LOOP0][UCR_THRESH] = 176;
      smb_sensor_threshold[TH4_VRD2_IOUT_LOOP0][LCR_THRESH] = 0; // unset
      smb_sensor_threshold[TH4_VRD2_IOUT_LOOP0][UNC_THRESH] = 0; // unset
      smb_sensor_threshold[TH4_VRD2_IOUT_LOOP0][LNC_THRESH] = 0; // unset
      smb_sensor_threshold[TH4_VRD2_IOUT_LOOP1][UCR_THRESH] = 15;
      smb_sensor_threshold[TH4_VRD2_IOUT_LOOP1][LCR_THRESH] = 0; // unset
      smb_sensor_threshold[TH4_VRD2_IOUT_LOOP1][UNC_THRESH] = 0; // unset
      smb_sensor_threshold[TH4_VRD2_IOUT_LOOP1][LNC_THRESH] = 0; // unset
      smb_sensor_threshold[TH4_VRD2_IOUT_LOOP2][UCR_THRESH] = 0; //unset
      smb_sensor_threshold[TH4_VRD2_IOUT_LOOP2][LCR_THRESH] = -15;
      smb_sensor_threshold[TH4_VRD2_IOUT_LOOP2][UNC_THRESH] = 0; // unset
      smb_sensor_threshold[TH4_VRD2_IOUT_LOOP2][LNC_THRESH] = 0; // unset
      // Topside board sensor
      smb_sensor_threshold[SMB_T][UNC_THRESH] = 90;
      smb_sensor_threshold[SMB_T][UCR_THRESH] = 100;
      smb_sensor_threshold[SMB_T][LNC_THRESH] = 0; // unset
      smb_sensor_threshold[SMB_T][LCR_THRESH] = 0; // unset
      // TH4 on-die temp sensors
      smb_sensor_threshold[TH4_DIE_TEMP_0][UNC_THRESH] = 105;
      smb_sensor_threshold[TH4_DIE_TEMP_0][UCR_THRESH] = 115;
      smb_sensor_threshold[TH4_DIE_TEMP_0][LNC_THRESH] = 0; // unset
      smb_sensor_threshold[TH4_DIE_TEMP_0][LCR_THRESH] = 0; // unset
      smb_sensor_threshold[TH4_DIE_TEMP_1][UNC_THRESH] = 105;
      smb_sensor_threshold[TH4_DIE_TEMP_1][UCR_THRESH] = 115;
      smb_sensor_threshold[TH4_DIE_TEMP_1][LNC_THRESH] = 0; // unset
      smb_sensor_threshold[TH4_DIE_TEMP_1][LCR_THRESH] = 0; // unset
      // Topside right board sensor
      smb_sensor_threshold[SMB_R][UNC_THRESH] = 75;
      smb_sensor_threshold[SMB_R][UCR_THRESH] = 90;
      smb_sensor_threshold[SMB_R][LNC_THRESH] = 0; // unset
      smb_sensor_threshold[SMB_R][LCR_THRESH] = 0; // unset
      // Bottomside board sensor
      smb_sensor_threshold[SMB_U][UNC_THRESH] = 90;
      smb_sensor_threshold[SMB_U][UCR_THRESH] = 100;
      smb_sensor_threshold[SMB_U][LNC_THRESH] = 0; // unset
      smb_sensor_threshold[SMB_U][LCR_THRESH] = 0; // unset
      // Topside left board sensor
      smb_sensor_threshold[SMB_L][UNC_THRESH] = 75;
      smb_sensor_threshold[SMB_L][UCR_THRESH] = 90;
      smb_sensor_threshold[SMB_L][LNC_THRESH] = 0; // unset
      smb_sensor_threshold[SMB_L][LCR_THRESH] = 0; // unset
      // TH4 internal temp sensors
      smb_sensor_threshold[BLACKHAWK_CORE_0_5_TEMP][UNC_THRESH] = 105;
      smb_sensor_threshold[BLACKHAWK_CORE_0_5_TEMP][UCR_THRESH] = 115;
      smb_sensor_threshold[BLACKHAWK_CORE_0_5_TEMP][LNC_THRESH] = 0; // unset
      smb_sensor_threshold[BLACKHAWK_CORE_0_5_TEMP][LCR_THRESH] = 0; // unset
      smb_sensor_threshold[BLACKHAWK_CORE_6_12_TEMP][UNC_THRESH] = 105;
      smb_sensor_threshold[BLACKHAWK_CORE_6_12_TEMP][UCR_THRESH] = 115;
      smb_sensor_threshold[BLACKHAWK_CORE_6_12_TEMP][LNC_THRESH] = 0; // unset
      smb_sensor_threshold[BLACKHAWK_CORE_6_12_TEMP][LCR_THRESH] = 0; // unset
      smb_sensor_threshold[BLACKHAWK_CORE_13_18_TEMP][UNC_THRESH] = 105;
      smb_sensor_threshold[BLACKHAWK_CORE_13_18_TEMP][UCR_THRESH] = 115;
      smb_sensor_threshold[BLACKHAWK_CORE_13_18_TEMP][LNC_THRESH] = 0; // unset
      smb_sensor_threshold[BLACKHAWK_CORE_13_18_TEMP][LCR_THRESH] = 0; // unset
      smb_sensor_threshold[BLACKHAWK_CORE_19_25_TEMP][UNC_THRESH] = 105;
      smb_sensor_threshold[BLACKHAWK_CORE_19_25_TEMP][UCR_THRESH] = 115;
      smb_sensor_threshold[BLACKHAWK_CORE_19_25_TEMP][LNC_THRESH] = 0; // unset
      smb_sensor_threshold[BLACKHAWK_CORE_19_25_TEMP][LCR_THRESH] = 0; // unset
      smb_sensor_threshold[BLACKHAWK_CORE_26_31_TEMP][UNC_THRESH] = 105;
      smb_sensor_threshold[BLACKHAWK_CORE_26_31_TEMP][UCR_THRESH] = 115;
      smb_sensor_threshold[BLACKHAWK_CORE_26_31_TEMP][LNC_THRESH] = 0; // unset
      smb_sensor_threshold[BLACKHAWK_CORE_26_31_TEMP][LCR_THRESH] = 0; // unset
      smb_sensor_threshold[BLACKHAWK_CORE_32_37_TEMP][UNC_THRESH] = 105;
      smb_sensor_threshold[BLACKHAWK_CORE_32_37_TEMP][UCR_THRESH] = 115;
      smb_sensor_threshold[BLACKHAWK_CORE_32_37_TEMP][LNC_THRESH] = 0; // unset
      smb_sensor_threshold[BLACKHAWK_CORE_32_37_TEMP][LCR_THRESH] = 0; // unset
      smb_sensor_threshold[BLACKHAWK_CORE_38_44_TEMP][UNC_THRESH] = 105;
      smb_sensor_threshold[BLACKHAWK_CORE_38_44_TEMP][UCR_THRESH] = 115;
      smb_sensor_threshold[BLACKHAWK_CORE_38_44_TEMP][LNC_THRESH] = 0; // unset
      smb_sensor_threshold[BLACKHAWK_CORE_38_44_TEMP][LCR_THRESH] = 0; // unset
      smb_sensor_threshold[BLACKHAWK_CORE_45_50_TEMP][UNC_THRESH] = 105;
      smb_sensor_threshold[BLACKHAWK_CORE_45_50_TEMP][UCR_THRESH] = 115;
      smb_sensor_threshold[BLACKHAWK_CORE_45_50_TEMP][LNC_THRESH] = 0; // unset
      smb_sensor_threshold[BLACKHAWK_CORE_45_50_TEMP][LCR_THRESH] = 0; // unset
      smb_sensor_threshold[BLACKHAWK_CORE_51_57_TEMP][UNC_THRESH] = 105;
      smb_sensor_threshold[BLACKHAWK_CORE_51_57_TEMP][UCR_THRESH] = 115;
      smb_sensor_threshold[BLACKHAWK_CORE_51_57_TEMP][LNC_THRESH] = 0; // unset
      smb_sensor_threshold[BLACKHAWK_CORE_51_57_TEMP][LCR_THRESH] = 0; // unset
      smb_sensor_threshold[BLACKHAWK_CORE_58_63_TEMP][UNC_THRESH] = 105;
      smb_sensor_threshold[BLACKHAWK_CORE_58_63_TEMP][UCR_THRESH] = 115;
      smb_sensor_threshold[BLACKHAWK_CORE_58_63_TEMP][LNC_THRESH] = 0; // unset
      smb_sensor_threshold[BLACKHAWK_CORE_58_63_TEMP][LCR_THRESH] = 0; // unset
      smb_sensor_threshold[CORE_0_TEMP][UNC_THRESH] = 105;
      smb_sensor_threshold[CORE_0_TEMP][UCR_THRESH] = 115;
      smb_sensor_threshold[CORE_0_TEMP][LNC_THRESH] = 0; // unset
      smb_sensor_threshold[CORE_0_TEMP][LCR_THRESH] = 0; // unset
      smb_sensor_threshold[CORE_1_TEMP][UNC_THRESH] = 105;
      smb_sensor_threshold[CORE_1_TEMP][UCR_THRESH] = 115;
      smb_sensor_threshold[CORE_1_TEMP][LNC_THRESH] = 0; // unset
      smb_sensor_threshold[CORE_1_TEMP][LCR_THRESH] = 0; // unset
      smb_sensor_threshold[CORE_2_TEMP][UNC_THRESH] = 105;
      smb_sensor_threshold[CORE_2_TEMP][UCR_THRESH] = 115;
      smb_sensor_threshold[CORE_2_TEMP][LNC_THRESH] = 0; // unset
      smb_sensor_threshold[CORE_2_TEMP][LCR_THRESH] = 0; // unset
      smb_sensor_threshold[CORE_3_TEMP][UNC_THRESH] = 105;
      smb_sensor_threshold[CORE_3_TEMP][UCR_THRESH] = 115;
      smb_sensor_threshold[CORE_3_TEMP][LNC_THRESH] = 0; // unset
      smb_sensor_threshold[CORE_3_TEMP][LCR_THRESH] = 0; // unset
      smb_sensor_threshold[PIM_QSFP200][UNC_THRESH] = 0; // unset
      smb_sensor_threshold[PIM_QSFP200][UCR_THRESH] = 65;
      smb_sensor_threshold[PIM_QSFP200][LNC_THRESH] = 0; // unset
      smb_sensor_threshold[PIM_QSFP200][LCR_THRESH] = 0; // unset
      smb_sensor_threshold[PIM_QSFP400][UNC_THRESH] = 0; // unset
      smb_sensor_threshold[PIM_QSFP400][UCR_THRESH] = 70;
      smb_sensor_threshold[PIM_QSFP400][LNC_THRESH] = 0; // unset
      smb_sensor_threshold[PIM_QSFP400][LCR_THRESH] = 0; // unset
      smb_sensor_threshold[PIM_F104][UNC_THRESH] = 110;
      smb_sensor_threshold[PIM_F104][UCR_THRESH] = 125;
      smb_sensor_threshold[PIM_F104][LNC_THRESH] = 0; // unset
      smb_sensor_threshold[PIM_F104][LCR_THRESH] = 0; // unset
      break;
    case FRU_PIM2:
    case FRU_PIM3:
    case FRU_PIM4:
    case FRU_PIM5:
    case FRU_PIM6:
    case FRU_PIM7:
    case FRU_PIM8:
    case FRU_PIM9:
      pim_thresh_array_init(fru);
      break;
    case FRU_PSU1:
    case FRU_PSU2:
    case FRU_PSU3:
    case FRU_PSU4:
      i = fru - FRU_PSU1;
      psu_sensor_threshold[PSU1_VIN + (i * PSU_SENSOR_COUNT)][UCR_THRESH] = 305;
      psu_sensor_threshold[PSU1_VIN + (i * PSU_SENSOR_COUNT)][LCR_THRESH] = 90;
      psu_sensor_threshold[PSU1_VIN + (i * PSU_SENSOR_COUNT)][UNC_THRESH] = 0; // unset
      psu_sensor_threshold[PSU1_VIN + (i * PSU_SENSOR_COUNT)][LNC_THRESH] = 0; // unset
      psu_sensor_threshold[PSU1_VOUT + (i * PSU_SENSOR_COUNT)][UCR_THRESH] = 13;
      psu_sensor_threshold[PSU1_VOUT + (i * PSU_SENSOR_COUNT)][LCR_THRESH] = 0; // unset
      psu_sensor_threshold[PSU1_VOUT + (i * PSU_SENSOR_COUNT)][UNC_THRESH] = 0; // unset
      psu_sensor_threshold[PSU1_VOUT + (i * PSU_SENSOR_COUNT)][LNC_THRESH] = 0; // unset
      psu_sensor_threshold[PSU1_FAN + (i * PSU_SENSOR_COUNT)][UCR_THRESH] = 0; // unset
      psu_sensor_threshold[PSU1_FAN + (i * PSU_SENSOR_COUNT)][LCR_THRESH] = 0; // unset
      psu_sensor_threshold[PSU1_FAN + (i * PSU_SENSOR_COUNT)][UNC_THRESH] = 0; // unset
      psu_sensor_threshold[PSU1_FAN + (i * PSU_SENSOR_COUNT)][LNC_THRESH] = 0; // unset
      psu_sensor_threshold[PSU1_TEMP1 + (i * PSU_SENSOR_COUNT)][UNC_THRESH] = 95;
      psu_sensor_threshold[PSU1_TEMP1 + (i * PSU_SENSOR_COUNT)][UCR_THRESH] = 100;
      psu_sensor_threshold[PSU1_TEMP1 + (i * PSU_SENSOR_COUNT)][LNC_THRESH] = 0; // unset
      psu_sensor_threshold[PSU1_TEMP1 + (i * PSU_SENSOR_COUNT)][LCR_THRESH] = 0; // unset
      psu_sensor_threshold[PSU1_TEMP2 + (i * PSU_SENSOR_COUNT)][UNC_THRESH] = 95;
      psu_sensor_threshold[PSU1_TEMP2 + (i * PSU_SENSOR_COUNT)][UCR_THRESH] = 100;
      psu_sensor_threshold[PSU1_TEMP2 + (i * PSU_SENSOR_COUNT)][LNC_THRESH] = 0; // unset
      psu_sensor_threshold[PSU1_TEMP2 + (i * PSU_SENSOR_COUNT)][LCR_THRESH] = 0; // unset
      psu_sensor_threshold[PSU1_TEMP3 + (i * PSU_SENSOR_COUNT)][UNC_THRESH] = 95;
      psu_sensor_threshold[PSU1_TEMP3 + (i * PSU_SENSOR_COUNT)][UCR_THRESH] = 100;
      psu_sensor_threshold[PSU1_TEMP3 + (i * PSU_SENSOR_COUNT)][LNC_THRESH] = 0; // unset
      psu_sensor_threshold[PSU1_TEMP3 + (i * PSU_SENSOR_COUNT)][LCR_THRESH] = 0; // unset
      psu_sensor_threshold[PSU1_PIN + (i * PSU_SENSOR_COUNT)][UCR_THRESH] = 0; // unset
      psu_sensor_threshold[PSU1_PIN + (i * PSU_SENSOR_COUNT)][LCR_THRESH] = 0; // unset
      psu_sensor_threshold[PSU1_PIN + (i * PSU_SENSOR_COUNT)][UNC_THRESH] = 0; // unset
      psu_sensor_threshold[PSU1_PIN + (i * PSU_SENSOR_COUNT)][LNC_THRESH] = 0; // unset
      psu_sensor_threshold[PSU1_POUT + (i * PSU_SENSOR_COUNT)][UCR_THRESH] = 2400;
      psu_sensor_threshold[PSU1_POUT + (i * PSU_SENSOR_COUNT)][LCR_THRESH] = 0; // unset
      psu_sensor_threshold[PSU1_POUT + (i * PSU_SENSOR_COUNT)][UNC_THRESH] = 0; // unset
      psu_sensor_threshold[PSU1_POUT + (i * PSU_SENSOR_COUNT)][LNC_THRESH] = 0; // unset
      psu_sensor_threshold[PSU1_IIN + (i * PSU_SENSOR_COUNT)][UCR_THRESH] = 14;
      psu_sensor_threshold[PSU1_IIN + (i * PSU_SENSOR_COUNT)][LCR_THRESH] = 0; // unset
      psu_sensor_threshold[PSU1_IIN + (i * PSU_SENSOR_COUNT)][UNC_THRESH] = 0; // unset
      psu_sensor_threshold[PSU1_IIN + (i * PSU_SENSOR_COUNT)][LNC_THRESH] = 0; // unset
      psu_sensor_threshold[PSU1_IOUT + (i * PSU_SENSOR_COUNT)][UCR_THRESH] = 200;
      psu_sensor_threshold[PSU1_IOUT + (i * PSU_SENSOR_COUNT)][LCR_THRESH] = 0; // unset
      psu_sensor_threshold[PSU1_IOUT + (i * PSU_SENSOR_COUNT)][UNC_THRESH] = 0; // unset
      psu_sensor_threshold[PSU1_IOUT + (i * PSU_SENSOR_COUNT)][LNC_THRESH] = 0; // unset
      break;
    case FRU_FAN:
      fan_sensor_threshold[FAN1_RPM][LCR_THRESH] = 3000;
      fan_sensor_threshold[FAN1_RPM][UCR_THRESH] = 16000;
      fan_sensor_threshold[FAN1_RPM][UNC_THRESH] = 0; // unset
      fan_sensor_threshold[FAN1_RPM][LNC_THRESH] = 0; // unset
      fan_sensor_threshold[FAN2_RPM][LCR_THRESH] = 3000;
      fan_sensor_threshold[FAN2_RPM][UCR_THRESH] = 16000;
      fan_sensor_threshold[FAN2_RPM][UNC_THRESH] = 0; // unset
      fan_sensor_threshold[FAN2_RPM][LNC_THRESH] = 0; // unset
      fan_sensor_threshold[FAN3_RPM][LCR_THRESH] = 3000;
      fan_sensor_threshold[FAN3_RPM][UCR_THRESH] = 16000;
      fan_sensor_threshold[FAN3_RPM][UNC_THRESH] = 0; // unset
      fan_sensor_threshold[FAN3_RPM][LNC_THRESH] = 0; // unset
      fan_sensor_threshold[FAN4_RPM][LCR_THRESH] = 3000;
      fan_sensor_threshold[FAN4_RPM][UCR_THRESH] = 16000;
      fan_sensor_threshold[FAN4_RPM][UNC_THRESH] = 0; // unset
      fan_sensor_threshold[FAN4_RPM][LNC_THRESH] = 0; // unset
      fan_sensor_threshold[FAN5_RPM][LCR_THRESH] = 3000;
      fan_sensor_threshold[FAN5_RPM][UCR_THRESH] = 16000;
      fan_sensor_threshold[FAN5_RPM][UNC_THRESH] = 0; // unset
      fan_sensor_threshold[FAN5_RPM][LNC_THRESH] = 0; // unset
      fan_sensor_threshold[FAN_CARD_BOARD_TEMP][UNC_THRESH] = 80;
      fan_sensor_threshold[FAN_CARD_BOARD_TEMP][UCR_THRESH] = 95;
      fan_sensor_threshold[FAN_CARD_BOARD_TEMP][LNC_THRESH] = 0; // unset
      fan_sensor_threshold[FAN_CARD_BOARD_TEMP][LCR_THRESH] = 0; // unset
      fan_sensor_threshold[FAN_CARD_OUTLET_TEMP][UNC_THRESH] = 75;
      fan_sensor_threshold[FAN_CARD_OUTLET_TEMP][UCR_THRESH] = 90;
      fan_sensor_threshold[FAN_CARD_OUTLET_TEMP][LNC_THRESH] = 0; // unset
      fan_sensor_threshold[FAN_CARD_OUTLET_TEMP][LCR_THRESH] = 0; // unset
      fan_sensor_threshold[SYSTEM_AIRFLOW][UNC_THRESH] = 0; // unset
      fan_sensor_threshold[SYSTEM_AIRFLOW][UCR_THRESH] = 0; // unset
      fan_sensor_threshold[SYSTEM_AIRFLOW][LNC_THRESH] = 0; // unset
      fan_sensor_threshold[SYSTEM_AIRFLOW][LCR_THRESH] = 0; // unset
      break;
  }
  init_threshold_done[fru] = true;
}

int
pal_get_sensor_threshold(uint8_t fru, uint8_t sensor_num,
                         uint8_t thresh, void *value) {
  float *val = (float*) value;

  sensor_thresh_array_init(fru);

  switch(fru) {
  case FRU_SCM:
    *val = scm_sensor_threshold[sensor_num][thresh];
    break;
  case FRU_SMB:
    *val = smb_sensor_threshold[sensor_num][thresh];
    break;
  case FRU_PIM2:
  case FRU_PIM3:
  case FRU_PIM4:
  case FRU_PIM5:
  case FRU_PIM6:
  case FRU_PIM7:
  case FRU_PIM8:
  case FRU_PIM9:
    *val = pim_sensor_threshold[fru - FRU_PIM2][sensor_num][thresh];
    break;
  case FRU_PSU1:
  case FRU_PSU2:
  case FRU_PSU3:
  case FRU_PSU4:
    *val = psu_sensor_threshold[sensor_num][thresh];
    break;
  case FRU_FAN:
    *val = fan_sensor_threshold[sensor_num][thresh];
    break;
  default:
    return -1;
  }

  return 0;
}

static sensor_desc_t *
get_sensor_desc(uint8_t fru, uint8_t snr_num) {

  if (fru < 1 || fru > MAX_NUM_FRUS) {
    syslog(LOG_WARNING, "get_sensor_desc: Wrong FRU ID %d\n", fru);
    return NULL;
  }

  return &m_snr_desc[fru - 1][snr_num];
}

static void
scm_sensor_poll_interval(uint8_t sensor_num, uint32_t *value) {
  switch(sensor_num) {
    default:
      *value = 30;
      break;
  }
}

static void
smb_sensor_poll_interval(uint8_t sensor_num, uint32_t *value) {
  switch(sensor_num) {
    default:
      *value = 30;
      break;
  }
}

static void
pim_sensor_poll_interval(uint8_t sensor_num, uint32_t *value) {
  switch(sensor_num) {
    default:
      *value = 30;
      break;
  }
}

static void
psu_sensor_poll_interval(uint8_t sensor_num, uint32_t *value) {
  switch(sensor_num) {
    default:
      *value = 30;
      break;
  }
}

static void
fan_sensor_poll_interval(uint8_t sensor_num, uint32_t *value) {
  switch(sensor_num) {
    default:
      *value = 30;
      break;
  }
}

int
pal_get_sensor_poll_interval(uint8_t fru, uint8_t sensor_num, uint32_t *value) {
  switch(fru) {
    case FRU_SCM:
      scm_sensor_poll_interval(sensor_num, value);
      break;
    case FRU_SMB:
      smb_sensor_poll_interval(sensor_num, value);
      break;
    case FRU_PIM2:
    case FRU_PIM3:
    case FRU_PIM4:
    case FRU_PIM5:
    case FRU_PIM6:
    case FRU_PIM7:
    case FRU_PIM8:
    case FRU_PIM9:
      pim_sensor_poll_interval(sensor_num, value);
      break;
    case FRU_PSU1:
    case FRU_PSU2:
    case FRU_PSU3:
    case FRU_PSU4:
      psu_sensor_poll_interval(sensor_num, value);
      break;
    case FRU_FAN:
      fan_sensor_poll_interval(sensor_num, value);
      break;
    default:
      *value = 2;
      break;
  }

  return 0;
}

int
pal_init_sensor_check(uint8_t fru, uint8_t snr_num, void *snr) {
  _sensor_thresh_t *psnr = (_sensor_thresh_t *)snr;
  sensor_desc_t *snr_desc;

  snr_desc = get_sensor_desc(fru, snr_num);
  strncpy(snr_desc->name, psnr->name, sizeof(snr_desc->name));
  snr_desc->name[sizeof(snr_desc->name) - 1] = 0;

  pal_set_def_key_value();

  switch (fru) {
    case FRU_PSU1:
    case FRU_PSU2:
    case FRU_PSU3:
    case FRU_PSU4:
      psu_init_acok_key(fru);
      break;
    case FRU_FAN:
      fan_init_sys_airflow_cfg_key();
      break;
  }

  return 0;
}

int
pal_get_fru_health(uint8_t fru, uint8_t *value) {
  char cvalue[MAX_VALUE_LEN] = {0};
  char key[MAX_KEY_LEN] = {0};
  int ret;

  switch(fru) {
    case FRU_SCM:
      sprintf(key, "scm_sensor_health");
      break;
    case FRU_SMB:
      sprintf(key, "smb_sensor_health");
      break;
    case FRU_PIM2:
    case FRU_PIM3:
    case FRU_PIM4:
    case FRU_PIM5:
    case FRU_PIM6:
    case FRU_PIM7:
    case FRU_PIM8:
    case FRU_PIM9:
      sprintf(key, "pim%d_sensor_health", fru - FRU_PIM2 + 2);
      break;
    case FRU_PSU1:
    case FRU_PSU2:
    case FRU_PSU3:
    case FRU_PSU4:
      sprintf(key, "psu%d_sensor_health", fru - FRU_PSU1 + 1);
      break;
    case FRU_FAN:
      sprintf(key, "fan_sensor_health");
      break;
    default:
      return -1;
  }

  ret = pal_get_key_value(key, cvalue);
  if (ret) {
    return ret;
  }

  *value = atoi(cvalue);
  *value = *value & atoi(cvalue);

  return 0;
}

int
pal_set_sensor_health(uint8_t fru, uint8_t value) {
  char key[MAX_KEY_LEN] = {0};
  char cvalue[MAX_VALUE_LEN] = {0};

  switch(fru) {
    case FRU_SCM:
      sprintf(key, "scm_sensor_health");
      break;
    case FRU_SMB:
      sprintf(key, "smb_sensor_health");
      break;
    case FRU_PIM2:
    case FRU_PIM3:
    case FRU_PIM4:
    case FRU_PIM5:
    case FRU_PIM6:
    case FRU_PIM7:
    case FRU_PIM8:
    case FRU_PIM9:
      sprintf(key, "pim%d_sensor_health", fru - FRU_PIM2 + 2);
      break;
    case FRU_PSU1:
    case FRU_PSU2:
    case FRU_PSU3:
    case FRU_PSU4:
      sprintf(key, "psu%d_sensor_health", fru - FRU_PSU1 + 1);
      break;
    case FRU_FAN:
      sprintf(key, "fan_sensor_health");
      break;
    default:
      return -1;
  }

  sprintf(cvalue, (value > 0) ? "1": "0");

  return pal_set_key_value(key, cvalue);
}

int pal_set_sdr_update_flag(uint8_t slot, uint8_t update) {
  sdr_fru_update_flag[slot] = update;
  init_threshold_done[slot] = false;
  return 0;
}

int pal_get_sdr_update_flag(uint8_t slot) {
  return sdr_fru_update_flag[slot];
}

int
pal_get_sensor_util_timeout(uint8_t fru) {
  uint8_t pim_id = 0, pim_type = 0;
  size_t cnt = 0;

  switch(fru) {
    case FRU_SCM:
      cnt = scm_sensor_cnt;
      break;
    case FRU_SMB:
      cnt = smb_sensor_cnt;
      break;
    case FRU_PIM2:
    case FRU_PIM3:
    case FRU_PIM4:
    case FRU_PIM5:
    case FRU_PIM6:
    case FRU_PIM7:
    case FRU_PIM8:
    case FRU_PIM9:
      pim_id = fru - FRU_PIM2;
      pim_type = pal_get_pim_type_from_file(fru);
      if (pim_type == PIM_TYPE_16Q2) {
        cnt = sizeof(pim16q2_sensor_list[pim_id]) / sizeof(uint8_t);
      } else if (pim_type == PIM_TYPE_16Q) {
        cnt = sizeof(pim16q_sensor_list[pim_id]) / sizeof(uint8_t);
      } else if (pim_type == PIM_TYPE_8DDM) {
        cnt = sizeof(pim8ddm_sensor_list[pim_id]) / sizeof(uint8_t);
      }
      break;
    case FRU_PSU1:
    case FRU_PSU2:
    case FRU_PSU3:
    case FRU_PSU4:
      cnt = psu1_sensor_cnt;
      break;
    case FRU_FAN:
      cnt = fan_sensor_cnt;
    default:
      if (fru > MAX_NUM_FRUS)
        cnt = 5;
      break;
  }

  return (READ_UNIT_SENSOR_TIMEOUT * cnt);
}
