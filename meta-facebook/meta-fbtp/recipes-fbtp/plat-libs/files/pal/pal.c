/*
 *
 * Copyright 2015-present Facebook. All Rights Reserved.
 *
 * This file contains code to support IPMI2.0 Specificaton available @
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
#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include <fcntl.h>
#include <errno.h>
#include <syslog.h>
#include <sys/mman.h>
#include <string.h>
#include <pthread.h>
#include <unistd.h>
#include "pal.h"
#include "vr.h"
#include "facebook/i2c.h"
#include "facebook/i2c-dev.h"

#define BIT(value, index) ((value >> index) & 1)

#define FBTP_PLATFORM_NAME "fbtp"
#define LAST_KEY "last_key"
#define FBTP_MAX_NUM_SLOTS 1
#define GPIO_VAL "/sys/class/gpio/gpio%d/value"
#define GPIO_DIR "/sys/class/gpio/gpio%d/direction"

#define GPIO_POWER 35
#define GPIO_POWER_GOOD 14
#define GPIO_POWER_LED 210
#define GPIO_POWER_RESET 33

#define GPIO_RST_BTN 144
#define GPIO_PWR_BTN 24

#define GPIO_HB_LED 165

#define GPIO_POSTCODE_0 48
#define GPIO_POSTCODE_1 49
#define GPIO_POSTCODE_2 50
#define GPIO_POSTCODE_3 51
#define GPIO_POSTCODE_4 124
#define GPIO_POSTCODE_5 125
#define GPIO_POSTCODE_6 126
#define GPIO_POSTCODE_7 127

#define GPIO_DBG_CARD_PRSNT 134

#define GPIO_BMC_READY_N  28

#define GPIO_BAT_SENSE_EN_N 46

#define GPIO_BOARD_SKU_ID0 120
#define GPIO_BOARD_SKU_ID1 121
#define GPIO_BOARD_SKU_ID2 122
#define GPIO_BOARD_SKU_ID3 123
#define GPIO_BOARD_SKU_ID4 124
#define GPIO_MB_SLOT_ID0 125
#define GPIO_MB_SLOT_ID1 126
#define GPIO_MB_SLOT_ID2 127
#define GPIO_BOARD_REV_ID0 25
#define GPIO_BOARD_REV_ID1 27
#define GPIO_BOARD_REV_ID2 29
#define GPIO_SLT_CFG0 142
#define GPIO_SLT_CFG1 143
#define GPIO_FM_CPU_CATERR_LVT3_N 49
#define GPIO_FM_CPU_MSMI_LVT3_N 107
#define GPIO_FM_CPU0_SKTOCC_LVT3_N 51
#define GPIO_FM_CPU1_SKTOCC_LVT3_N 208
#define GPIO_FM_BIOS_POST_CMPLT_N 215

#define PAGE_SIZE  0x1000
#define AST_SCU_BASE 0x1e6e2000
#define PIN_CTRL1_OFFSET 0x80
#define PIN_CTRL2_OFFSET 0x84
#define AST_WDT_BASE 0x1e785000
#define WDT_OFFSET 0x10

#define AST_LPC_BASE 0x1e789000
#define HICRA_OFFSET 0x9C
#define HICRA_MASK_UART1 0x70000
#define HICRA_MASK_UART2 0x380000
#define HICRA_MASK_UART3 0x1C00000
#define UART1_TO_UART3 0x5
#define UART2_TO_UART3 0x6
#define UART3_TO_UART1 0x5
#define UART3_TO_UART2 0x4
#define DEBUG_TO_UART1 0x0

#define UART1_TXD (1 << 22)
#define UART2_TXD (1 << 30)
#define UART3_TXD (1 << 22)
#define UART4_TXD (1 << 30)

#define DELAY_GRACEFUL_SHUTDOWN 1
#define DELAY_POWER_OFF 6
#define DELAY_POWER_CYCLE 10

#define CRASHDUMP_BIN       "/usr/local/bin/dump.sh"
#define CRASHDUMP_FILE      "/mnt/data/crashdump_"

#define LARGEST_DEVICE_NAME 120
#define PWM_DIR "/sys/devices/platform/ast_pwm_tacho.0"
#define PWM_UNIT_MAX 96

#define FAN0_TACH_INPUT 0
#define FAN1_TACH_INPUT 2

#define TACH_DIR "/sys/devices/platform/ast_pwm_tacho.0"
#define ADC_DIR "/sys/devices/platform/ast_adc.0"

#define MB_INLET_TEMP_DEVICE "/sys/devices/platform/ast-i2c.6/i2c-6/6-004e/hwmon/hwmon*"
#define MB_OUTLET_TEMP_DEVICE "/sys/devices/platform/ast-i2c.6/i2c-6/6-004f/hwmon/hwmon*"
#define MEZZ_TEMP_DEVICE "/sys/devices/platform/ast-i2c.8/i2c-8/8-001f/hwmon/hwmon*"
#define HSC_DEVICE "/sys/devices/platform/ast-i2c.7/i2c-7/7-0011/hwmon/hwmon*"

#define FAN_TACH_RPM "tacho%d_rpm"
#define ADC_VALUE "adc%d_value"
#define HSC_IN_VOLT "in1_input"
#define HSC_OUT_CURR "curr1_input"
#define HSC_TEMP "temp1_input"

#define UNIT_DIV 1000

#define MAX_SENSOR_NUM 0xFF
#define ALL_BYTES 0xFF
#define LAST_REC_ID 0xFFFF

#define VR_BUS_ID 0x5

#define VR_FW_PAGE 0x2f
#define VR_FW_PAGE_2 0x6F
#define VR_FW_PAGE_3 0x4F
#define VR_LOOP_PAGE_0 0x60
#define VR_LOOP_PAGE_1 0x61
#define VR_LOOP_PAGE_2 0x62

#define VR_FW_REG1 0xC
#define VR_FW_REG2 0xD
#define VR_FW_REG3 0x3D
#define VR_FW_REG4 0x3E
#define VR_FW_REG5 0x32

#define VR_TELEMETRY_VOLT 0x1A
#define VR_TELEMETRY_CURR 0x15
#define VR_TELEMETRY_POWER 0x2D
#define VR_TELEMETRY_TEMP 0x29

#define RISER_BUS_ID 0x1

#define GUID_SIZE 16
#define OFFSET_SYS_GUID 0x17F0
#define OFFSET_DEV_GUID 0x1800
#define FRU_EEPROM "/sys/devices/platform/ast-i2c.6/i2c-6/6-0054/eeprom"

#define READING_NA -2
#define READING_SKIP 1

#define NIC_MAX_TEMP 125
#define PLAT_ID_SKU_MASK 0x10 // BIT4: 0- Single Side, 1- Double Side

#define MAX_READ_RETRY 10

#define VR_UPDATE_IN_PROGRESS "/tmp/stop_monitor_vr"
#define MAX_VR_CHIPS 9
#define VR_TIMEOUT 200

static uint8_t gpio_rst_btn[] = { 0, 57, 56, 59, 58 };
const static uint8_t gpio_id_led[] = { 0, 41, 40, 43, 42 };
const static uint8_t gpio_prsnt[] = { 0, 61, 60, 63, 62 };
const char pal_fru_list[] = "all, mb, nic";
const char pal_server_list[] = "mb";

size_t pal_pwm_cnt = 2;
size_t pal_tach_cnt = 2;
const char pal_pwm_list[] = "0, 1";
const char pal_tach_list[] = "0, 1";

static uint8_t g_plat_id = 0x0;

char * key_list[] = {
"pwr_server_last_state",
"sysfw_ver_server",
"identify_sled",
"timestamp_sled",
"server_por_cfg",
"server_sensor_health",
"nic_sensor_health",
"server_sel_error",
"server_boot_order",
/* Add more Keys here */
LAST_KEY /* This is the last key of the list */
};

char * def_val_list[] = {
  "on", /* pwr_server_last_state */
  "0", /* sysfw_ver_server */
  "off", /* identify_sled */
  "0", /* timestamp_sled */
  "lps", /* server_por_cfg */
  "1", /* server_sensor_health */
  "1", /* nic_sensor_health */
  "1", /* server_sel_error */
  "0000000", /* server_boot_order */
  /* Add more def values for the correspoding keys*/
  LAST_KEY /* Same as last entry of the key_list */
};

// List of MB sensors to be monitored
const uint8_t mb_sensor_list[] = {
  MB_SENSOR_INLET_TEMP,
  MB_SENSOR_OUTLET_TEMP,
  MB_SENSOR_INLET_REMOTE_TEMP,
  MB_SENSOR_OUTLET_REMOTE_TEMP,
  MB_SENSOR_FAN0_TACH,
  MB_SENSOR_FAN1_TACH,
  MB_SENSOR_P3V3,
  MB_SENSOR_P5V,
  MB_SENSOR_P12V,
  MB_SENSOR_P1V05,
  MB_SENSOR_PVNN_PCH_STBY,
  MB_SENSOR_P3V3_STBY,
  MB_SENSOR_P5V_STBY,
  MB_SENSOR_P3V_BAT,
  MB_SENSOR_HSC_IN_VOLT,
  MB_SENSOR_HSC_OUT_CURR,
  MB_SENSOR_HSC_IN_POWER,
  MB_SENSOR_CPU0_TEMP,
  MB_SENSOR_CPU0_TJMAX,
  MB_SENSOR_CPU1_TEMP,
  MB_SENSOR_CPU1_TJMAX,
  MB_SENSOR_PCH_TEMP,
  MB_SENSOR_CPU0_DIMM_GRPA_TEMP,
  MB_SENSOR_CPU0_DIMM_GRPB_TEMP,
  MB_SENSOR_CPU1_DIMM_GRPC_TEMP,
  MB_SENSOR_CPU1_DIMM_GRPD_TEMP,
  MB_SENSOR_VR_CPU0_VCCIN_TEMP,
  MB_SENSOR_VR_CPU0_VCCIN_CURR,
  MB_SENSOR_VR_CPU0_VCCIN_VOLT,
  MB_SENSOR_VR_CPU0_VCCIN_POWER,
  MB_SENSOR_VR_CPU0_VSA_TEMP,
  MB_SENSOR_VR_CPU0_VSA_CURR,
  MB_SENSOR_VR_CPU0_VSA_VOLT,
  MB_SENSOR_VR_CPU0_VSA_POWER,
  MB_SENSOR_VR_CPU0_VCCIO_TEMP,
  MB_SENSOR_VR_CPU0_VCCIO_CURR,
  MB_SENSOR_VR_CPU0_VCCIO_VOLT,
  MB_SENSOR_VR_CPU0_VCCIO_POWER,
  MB_SENSOR_VR_CPU0_VDDQ_GRPA_TEMP,
  MB_SENSOR_VR_CPU0_VDDQ_GRPA_CURR,
  MB_SENSOR_VR_CPU0_VDDQ_GRPA_VOLT,
  MB_SENSOR_VR_CPU0_VDDQ_GRPA_POWER,
  MB_SENSOR_VR_CPU0_VDDQ_GRPB_TEMP,
  MB_SENSOR_VR_CPU0_VDDQ_GRPB_CURR,
  MB_SENSOR_VR_CPU0_VDDQ_GRPB_VOLT,
  MB_SENSOR_VR_CPU0_VDDQ_GRPB_POWER,
  MB_SENSOR_VR_CPU1_VCCIN_TEMP,
  MB_SENSOR_VR_CPU1_VCCIN_CURR,
  MB_SENSOR_VR_CPU1_VCCIN_VOLT,
  MB_SENSOR_VR_CPU1_VCCIN_POWER,
  MB_SENSOR_VR_CPU1_VSA_TEMP,
  MB_SENSOR_VR_CPU1_VSA_CURR,
  MB_SENSOR_VR_CPU1_VSA_VOLT,
  MB_SENSOR_VR_CPU1_VSA_POWER,
  MB_SENSOR_VR_CPU1_VCCIO_TEMP,
  MB_SENSOR_VR_CPU1_VCCIO_CURR,
  MB_SENSOR_VR_CPU1_VCCIO_VOLT,
  MB_SENSOR_VR_CPU1_VCCIO_POWER,
  MB_SENSOR_VR_CPU1_VDDQ_GRPC_TEMP,
  MB_SENSOR_VR_CPU1_VDDQ_GRPC_CURR,
  MB_SENSOR_VR_CPU1_VDDQ_GRPC_VOLT,
  MB_SENSOR_VR_CPU1_VDDQ_GRPC_POWER,
  MB_SENSOR_VR_CPU1_VDDQ_GRPD_TEMP,
  MB_SENSOR_VR_CPU1_VDDQ_GRPD_CURR,
  MB_SENSOR_VR_CPU1_VDDQ_GRPD_VOLT,
  MB_SENSOR_VR_CPU1_VDDQ_GRPD_POWER,
  MB_SENSOR_VR_PCH_PVNN_TEMP,
  MB_SENSOR_VR_PCH_PVNN_CURR,
  MB_SENSOR_VR_PCH_PVNN_VOLT,
  MB_SENSOR_VR_PCH_PVNN_POWER,
  MB_SENSOR_VR_PCH_P1V05_TEMP,
  MB_SENSOR_VR_PCH_P1V05_CURR,
  MB_SENSOR_VR_PCH_P1V05_VOLT,
  MB_SENSOR_VR_PCH_P1V05_POWER,
  MB_SENSOR_C2_AVA_FTEMP,
  MB_SENSOR_C2_AVA_RTEMP,
  MB_SENSOR_C2_1_NVME_CTEMP,
  MB_SENSOR_C2_2_NVME_CTEMP,
  MB_SENSOR_C2_3_NVME_CTEMP,
  MB_SENSOR_C2_4_NVME_CTEMP,
  MB_SENSOR_C3_AVA_FTEMP,
  MB_SENSOR_C3_AVA_RTEMP,
  MB_SENSOR_C3_1_NVME_CTEMP,
  MB_SENSOR_C3_2_NVME_CTEMP,
  MB_SENSOR_C3_3_NVME_CTEMP,
  MB_SENSOR_C3_4_NVME_CTEMP,
  MB_SENSOR_C4_AVA_FTEMP,
  MB_SENSOR_C4_AVA_RTEMP,
  MB_SENSOR_C4_1_NVME_CTEMP,
  MB_SENSOR_C4_2_NVME_CTEMP,
  MB_SENSOR_C4_3_NVME_CTEMP,
  MB_SENSOR_C4_4_NVME_CTEMP,
};

// List of NIC sensors to be monitored
const uint8_t nic_sensor_list[] = {
  MEZZ_SENSOR_TEMP,
};

float mb_sensor_threshold[MAX_SENSOR_NUM][MAX_SENSOR_THRESHOLD + 1] = {0};
float nic_sensor_threshold[MAX_SENSOR_NUM][MAX_SENSOR_THRESHOLD + 1] = {0};

size_t mb_sensor_cnt = sizeof(mb_sensor_list)/sizeof(uint8_t);
size_t nic_sensor_cnt = sizeof(nic_sensor_list)/sizeof(uint8_t);

uint8_t g_sys_guid[GUID_SIZE] = {0};
uint8_t g_dev_guid[GUID_SIZE] = {0};

static uint8_t g_board_rev_id = BOARD_REV_EVT;
static uint8_t g_vr_cpu0_vddq_abc;
static uint8_t g_vr_cpu0_vddq_def;
static uint8_t g_vr_cpu1_vddq_ghj;
static uint8_t g_vr_cpu1_vddq_klm;

static void
init_board_sensors(void) {
  pal_get_board_rev_id(&g_board_rev_id);

  if (g_board_rev_id == BOARD_REV_POWERON ||
      g_board_rev_id == BOARD_REV_EVT ) {
    g_vr_cpu0_vddq_abc = VR_CPU0_VDDQ_ABC_EVT;
    g_vr_cpu0_vddq_def = VR_CPU0_VDDQ_DEF_EVT;
    g_vr_cpu1_vddq_ghj = VR_CPU1_VDDQ_GHJ_EVT;
    g_vr_cpu1_vddq_klm = VR_CPU1_VDDQ_KLM_EVT;
  } else {
    g_vr_cpu0_vddq_abc = VR_CPU0_VDDQ_ABC;
    g_vr_cpu0_vddq_def = VR_CPU0_VDDQ_DEF;
    g_vr_cpu1_vddq_ghj = VR_CPU1_VDDQ_GHJ;
    g_vr_cpu1_vddq_klm = VR_CPU1_VDDQ_KLM;
  }
}

//Dynamic change CPU Temp threshold
static void
dyn_sensor_thresh_array_init() {
  static bool init_cpu0 = false;
  static bool init_cpu1 = false;
  static bool init_done = false;
  char key[MAX_KEY_LEN] = {0};
  char str[MAX_VALUE_LEN] = {0};

  // Return if both cpu thresholds are initialized
  if (init_done) {
    return;
  }

  // Checkd if cpu0 threshold needs to be initialized
  if (init_cpu0) {
    goto dyn_cpu1_init;
  }

  sprintf(key, "mb_sensor%d", MB_SENSOR_CPU0_TJMAX);
  if( edb_cache_get(key,str) >= 0 && (float) (strtof(str, NULL) - 4) > 0) {
    mb_sensor_threshold[MB_SENSOR_CPU0_TEMP][UCR_THRESH] = (float) (strtof(str, NULL) - 4);
    init_cpu0 = true;
  }else{
    mb_sensor_threshold[MB_SENSOR_CPU0_TEMP][UCR_THRESH] = 104;
  }

  // Check if cpu1 threshold needs to be initialized
dyn_cpu1_init:
  if (init_cpu1) {
    goto dyn_thresh_exit;
  }

  sprintf(key, "mb_sensor%d", MB_SENSOR_CPU1_TJMAX);
  if( edb_cache_get(key,str) >= 0 && (float) (strtof(str, NULL) - 4) > 0 ) {
    mb_sensor_threshold[MB_SENSOR_CPU1_TEMP][UCR_THRESH] = (float) (strtof(str, NULL) - 4);
    init_cpu1 = true;
  }else{
    mb_sensor_threshold[MB_SENSOR_CPU1_TEMP][UCR_THRESH] = 104;
  }

  // Mark init complete only if both thresholds are initialized
dyn_thresh_exit:
  if (init_cpu0 && init_cpu1) {
    init_done = true;
  }
}

static void
sensor_thresh_array_init() {
  static bool init_done = false;

  dyn_sensor_thresh_array_init();

  if (init_done)
    return;

  mb_sensor_threshold[MB_SENSOR_INLET_TEMP][UCR_THRESH] = 40;
  mb_sensor_threshold[MB_SENSOR_OUTLET_TEMP][UCR_THRESH] = 75;
  mb_sensor_threshold[MB_SENSOR_INLET_REMOTE_TEMP][UCR_THRESH] = 40;
  mb_sensor_threshold[MB_SENSOR_OUTLET_REMOTE_TEMP][UCR_THRESH] = 75;

  // Assign UCT based on the system is Single Side or Double Side
  if (!(pal_get_platform_id(&g_plat_id)) && !(g_plat_id & PLAT_ID_SKU_MASK)) {
    mb_sensor_threshold[MB_SENSOR_FAN0_TACH][UCR_THRESH] = 9000;
    mb_sensor_threshold[MB_SENSOR_FAN1_TACH][UCR_THRESH] = 9000;
  } else {
    mb_sensor_threshold[MB_SENSOR_FAN0_TACH][UCR_THRESH] = 13500;
    mb_sensor_threshold[MB_SENSOR_FAN1_TACH][UCR_THRESH] = 13500;
  }

  mb_sensor_threshold[MB_SENSOR_FAN0_TACH][LCR_THRESH] = 500;
  mb_sensor_threshold[MB_SENSOR_FAN1_TACH][LCR_THRESH] = 500;

  mb_sensor_threshold[MB_SENSOR_P3V3][UCR_THRESH] = 3.621;
  mb_sensor_threshold[MB_SENSOR_P3V3][LCR_THRESH] = 2.975;
  mb_sensor_threshold[MB_SENSOR_P5V][UCR_THRESH] = 5.486;
  mb_sensor_threshold[MB_SENSOR_P5V][LCR_THRESH] = 4.524;
  mb_sensor_threshold[MB_SENSOR_P12V][UCR_THRESH] = 13.23;
  mb_sensor_threshold[MB_SENSOR_P12V][LCR_THRESH] = 10.773;
  mb_sensor_threshold[MB_SENSOR_P1V05][UCR_THRESH] = 1.155;
  mb_sensor_threshold[MB_SENSOR_P1V05][LCR_THRESH] = 0.945;
  mb_sensor_threshold[MB_SENSOR_PVNN_PCH_STBY][UCR_THRESH] = 1.1;
  mb_sensor_threshold[MB_SENSOR_PVNN_PCH_STBY][LCR_THRESH] = 0.765;
  mb_sensor_threshold[MB_SENSOR_P3V3_STBY][UCR_THRESH] = 3.621;
  mb_sensor_threshold[MB_SENSOR_P3V3_STBY][LCR_THRESH] = 2.975;
  mb_sensor_threshold[MB_SENSOR_P5V_STBY][UCR_THRESH] = 5.486;
  mb_sensor_threshold[MB_SENSOR_P5V_STBY][LCR_THRESH] = 4.524;
  mb_sensor_threshold[MB_SENSOR_P3V_BAT][UCR_THRESH] = 3.738;
  mb_sensor_threshold[MB_SENSOR_P3V_BAT][LCR_THRESH] = 2.73;
  mb_sensor_threshold[MB_SENSOR_HSC_IN_VOLT][UCR_THRESH] = 13.2;
  mb_sensor_threshold[MB_SENSOR_HSC_IN_VOLT][LCR_THRESH] = 10.8;
  mb_sensor_threshold[MB_SENSOR_HSC_OUT_CURR][UCR_THRESH] = 47.705;
  mb_sensor_threshold[MB_SENSOR_HSC_IN_POWER][UCR_THRESH] = 790.40;
  mb_sensor_threshold[MB_SENSOR_PCH_TEMP][UCR_THRESH] = 84;
  mb_sensor_threshold[MB_SENSOR_CPU0_DIMM_GRPA_TEMP][UCR_THRESH] = 81;
  mb_sensor_threshold[MB_SENSOR_CPU0_DIMM_GRPB_TEMP][UCR_THRESH] = 81;
  mb_sensor_threshold[MB_SENSOR_CPU1_DIMM_GRPC_TEMP][UCR_THRESH] = 81;
  mb_sensor_threshold[MB_SENSOR_CPU1_DIMM_GRPD_TEMP][UCR_THRESH] = 81;

  mb_sensor_threshold[MB_SENSOR_VR_CPU0_VCCIN_TEMP][UCR_THRESH] = 100;
  mb_sensor_threshold[MB_SENSOR_VR_CPU0_VCCIN_CURR][LCR_THRESH] = -1.5;
  mb_sensor_threshold[MB_SENSOR_VR_CPU0_VCCIN_CURR][UCR_THRESH] = 228;
  mb_sensor_threshold[MB_SENSOR_VR_CPU0_VCCIN_POWER][UCR_THRESH] = 414;
  mb_sensor_threshold[MB_SENSOR_VR_CPU0_VCCIN_VOLT][LCR_THRESH] = 1.5;
  mb_sensor_threshold[MB_SENSOR_VR_CPU0_VCCIN_VOLT][UCR_THRESH] = 2.0;

  mb_sensor_threshold[MB_SENSOR_VR_CPU0_VSA_TEMP][UCR_THRESH] = 100;
  mb_sensor_threshold[MB_SENSOR_VR_CPU0_VSA_CURR][LCR_THRESH] = -1.5;
  mb_sensor_threshold[MB_SENSOR_VR_CPU0_VSA_CURR][UCR_THRESH] = 20;
  mb_sensor_threshold[MB_SENSOR_VR_CPU0_VSA_POWER][UCR_THRESH] = 18;
  mb_sensor_threshold[MB_SENSOR_VR_CPU0_VSA_VOLT][LCR_THRESH] = 0.7;
  mb_sensor_threshold[MB_SENSOR_VR_CPU0_VSA_VOLT][UCR_THRESH] = 1.2;


  mb_sensor_threshold[MB_SENSOR_VR_CPU0_VCCIO_TEMP][UCR_THRESH] = 100;
  mb_sensor_threshold[MB_SENSOR_VR_CPU0_VCCIO_CURR][LCR_THRESH] = -1.5;
  mb_sensor_threshold[MB_SENSOR_VR_CPU0_VCCIO_CURR][UCR_THRESH] = 21;
  mb_sensor_threshold[MB_SENSOR_VR_CPU0_VCCIO_POWER][UCR_THRESH] = 22;
  mb_sensor_threshold[MB_SENSOR_VR_CPU0_VCCIO_VOLT][LCR_THRESH] = 0.8;
  mb_sensor_threshold[MB_SENSOR_VR_CPU0_VCCIO_VOLT][UCR_THRESH] = 1.1;


  mb_sensor_threshold[MB_SENSOR_VR_CPU0_VDDQ_GRPA_TEMP][UCR_THRESH] = 90;
  mb_sensor_threshold[MB_SENSOR_VR_CPU0_VDDQ_GRPA_CURR][LCR_THRESH] = -1.5;
  mb_sensor_threshold[MB_SENSOR_VR_CPU0_VDDQ_GRPA_CURR][UCR_THRESH] = 72;
  mb_sensor_threshold[MB_SENSOR_VR_CPU0_VDDQ_GRPA_POWER][UCR_THRESH] = 88;
  mb_sensor_threshold[MB_SENSOR_VR_CPU0_VDDQ_GRPA_VOLT][LCR_THRESH] = 1.08;
  mb_sensor_threshold[MB_SENSOR_VR_CPU0_VDDQ_GRPA_VOLT][UCR_THRESH] = 1.32;

  mb_sensor_threshold[MB_SENSOR_VR_CPU0_VDDQ_GRPB_TEMP][UCR_THRESH] = 90;
  mb_sensor_threshold[MB_SENSOR_VR_CPU0_VDDQ_GRPB_CURR][LCR_THRESH] = -1.5;
  mb_sensor_threshold[MB_SENSOR_VR_CPU0_VDDQ_GRPB_CURR][UCR_THRESH] = 72;
  mb_sensor_threshold[MB_SENSOR_VR_CPU0_VDDQ_GRPB_POWER][UCR_THRESH] = 88;
  mb_sensor_threshold[MB_SENSOR_VR_CPU0_VDDQ_GRPB_VOLT][LCR_THRESH] = 1.08;
  mb_sensor_threshold[MB_SENSOR_VR_CPU0_VDDQ_GRPB_VOLT][UCR_THRESH] = 1.32;

  mb_sensor_threshold[MB_SENSOR_VR_CPU1_VCCIN_TEMP][UCR_THRESH] = 100;
  mb_sensor_threshold[MB_SENSOR_VR_CPU1_VCCIN_CURR][LCR_THRESH] = -1.5;
  mb_sensor_threshold[MB_SENSOR_VR_CPU1_VCCIN_CURR][UCR_THRESH] = 228;
  mb_sensor_threshold[MB_SENSOR_VR_CPU1_VCCIN_POWER][UCR_THRESH] = 414;
  mb_sensor_threshold[MB_SENSOR_VR_CPU1_VCCIN_VOLT][LCR_THRESH] = 1.5;
  mb_sensor_threshold[MB_SENSOR_VR_CPU1_VCCIN_VOLT][UCR_THRESH] = 2.0;

  mb_sensor_threshold[MB_SENSOR_VR_CPU1_VSA_TEMP][UCR_THRESH] = 100;
  mb_sensor_threshold[MB_SENSOR_VR_CPU1_VSA_CURR][LCR_THRESH] = -1.5;
  mb_sensor_threshold[MB_SENSOR_VR_CPU1_VSA_CURR][UCR_THRESH] = 20;
  mb_sensor_threshold[MB_SENSOR_VR_CPU1_VSA_POWER][UCR_THRESH] = 18;
  mb_sensor_threshold[MB_SENSOR_VR_CPU1_VSA_VOLT][LCR_THRESH] = 0.7;
  mb_sensor_threshold[MB_SENSOR_VR_CPU1_VSA_VOLT][UCR_THRESH] = 1.2;

  mb_sensor_threshold[MB_SENSOR_VR_CPU1_VCCIO_TEMP][UCR_THRESH] = 100;
  mb_sensor_threshold[MB_SENSOR_VR_CPU1_VCCIO_CURR][LCR_THRESH] = -1.5;
  mb_sensor_threshold[MB_SENSOR_VR_CPU1_VCCIO_CURR][UCR_THRESH] = 21;
  mb_sensor_threshold[MB_SENSOR_VR_CPU1_VCCIO_POWER][UCR_THRESH] = 22;
  mb_sensor_threshold[MB_SENSOR_VR_CPU1_VCCIO_VOLT][LCR_THRESH] = 0.8;
  mb_sensor_threshold[MB_SENSOR_VR_CPU1_VCCIO_VOLT][UCR_THRESH] = 1.1;

  mb_sensor_threshold[MB_SENSOR_VR_CPU1_VDDQ_GRPC_TEMP][UCR_THRESH] = 90;
  mb_sensor_threshold[MB_SENSOR_VR_CPU1_VDDQ_GRPC_CURR][LCR_THRESH] = -1.5;
  mb_sensor_threshold[MB_SENSOR_VR_CPU1_VDDQ_GRPC_CURR][UCR_THRESH] = 72;
  mb_sensor_threshold[MB_SENSOR_VR_CPU1_VDDQ_GRPC_POWER][UCR_THRESH] = 88;
  mb_sensor_threshold[MB_SENSOR_VR_CPU1_VDDQ_GRPC_VOLT][LCR_THRESH] = 1.08;
  mb_sensor_threshold[MB_SENSOR_VR_CPU1_VDDQ_GRPC_VOLT][UCR_THRESH] = 1.32;

  mb_sensor_threshold[MB_SENSOR_VR_CPU1_VDDQ_GRPD_TEMP][UCR_THRESH] = 90;
  mb_sensor_threshold[MB_SENSOR_VR_CPU1_VDDQ_GRPD_CURR][LCR_THRESH] = -1.5;
  mb_sensor_threshold[MB_SENSOR_VR_CPU1_VDDQ_GRPD_CURR][UCR_THRESH] = 72;
  mb_sensor_threshold[MB_SENSOR_VR_CPU1_VDDQ_GRPD_POWER][UCR_THRESH] = 88;
  mb_sensor_threshold[MB_SENSOR_VR_CPU1_VDDQ_GRPD_VOLT][LCR_THRESH] = 1.08;
  mb_sensor_threshold[MB_SENSOR_VR_CPU1_VDDQ_GRPD_VOLT][UCR_THRESH] = 1.32;

  mb_sensor_threshold[MB_SENSOR_VR_PCH_PVNN_TEMP][UCR_THRESH] = 80;
  mb_sensor_threshold[MB_SENSOR_VR_PCH_PVNN_CURR][LCR_THRESH] = -1.5;
  mb_sensor_threshold[MB_SENSOR_VR_PCH_PVNN_CURR][UCR_THRESH] = 26;
  mb_sensor_threshold[MB_SENSOR_VR_PCH_PVNN_POWER][UCR_THRESH] = 26;
  mb_sensor_threshold[MB_SENSOR_VR_PCH_PVNN_VOLT][LCR_THRESH] = 0.76;
  mb_sensor_threshold[MB_SENSOR_VR_PCH_PVNN_VOLT][UCR_THRESH] = 1.1;

  mb_sensor_threshold[MB_SENSOR_VR_PCH_P1V05_TEMP][UCR_THRESH] = 80;
  mb_sensor_threshold[MB_SENSOR_VR_PCH_P1V05_CURR][LCR_THRESH] = -1.5;
  mb_sensor_threshold[MB_SENSOR_VR_PCH_P1V05_CURR][UCR_THRESH] = 20;
  mb_sensor_threshold[MB_SENSOR_VR_PCH_P1V05_POWER][UCR_THRESH] = 22;
  mb_sensor_threshold[MB_SENSOR_VR_PCH_P1V05_VOLT][LCR_THRESH] = 0.94;
  mb_sensor_threshold[MB_SENSOR_VR_PCH_P1V05_VOLT][UCR_THRESH] = 1.15;

  mb_sensor_threshold[MB_SENSOR_C2_AVA_FTEMP][UCR_THRESH] = 95;
  mb_sensor_threshold[MB_SENSOR_C2_AVA_RTEMP][UCR_THRESH] = 95;
  mb_sensor_threshold[MB_SENSOR_C2_1_NVME_CTEMP][UCR_THRESH] = 95;
  mb_sensor_threshold[MB_SENSOR_C2_2_NVME_CTEMP][UCR_THRESH] = 95;
  mb_sensor_threshold[MB_SENSOR_C2_3_NVME_CTEMP][UCR_THRESH] = 95;
  mb_sensor_threshold[MB_SENSOR_C2_4_NVME_CTEMP][UCR_THRESH] = 95;
  mb_sensor_threshold[MB_SENSOR_C3_AVA_FTEMP][UCR_THRESH] = 95;
  mb_sensor_threshold[MB_SENSOR_C3_AVA_RTEMP][UCR_THRESH] = 95;
  mb_sensor_threshold[MB_SENSOR_C3_1_NVME_CTEMP][UCR_THRESH] = 95;
  mb_sensor_threshold[MB_SENSOR_C3_2_NVME_CTEMP][UCR_THRESH] = 95;
  mb_sensor_threshold[MB_SENSOR_C3_3_NVME_CTEMP][UCR_THRESH] = 95;
  mb_sensor_threshold[MB_SENSOR_C3_4_NVME_CTEMP][UCR_THRESH] = 95;
  mb_sensor_threshold[MB_SENSOR_C4_AVA_FTEMP][UCR_THRESH] = 95;
  mb_sensor_threshold[MB_SENSOR_C4_AVA_RTEMP][UCR_THRESH] = 95;
  mb_sensor_threshold[MB_SENSOR_C4_1_NVME_CTEMP][UCR_THRESH] = 95;
  mb_sensor_threshold[MB_SENSOR_C4_2_NVME_CTEMP][UCR_THRESH] = 95;
  mb_sensor_threshold[MB_SENSOR_C4_3_NVME_CTEMP][UCR_THRESH] = 95;
  mb_sensor_threshold[MB_SENSOR_C4_4_NVME_CTEMP][UCR_THRESH] = 95;

  nic_sensor_threshold[MEZZ_SENSOR_TEMP][UCR_THRESH] = 95;

  init_board_sensors();
  init_done = true;
}

// Helper Functions
static int
i2c_io(int fd, uint8_t addr, uint8_t *tbuf, uint8_t tcount, uint8_t *rbuf, uint8_t rcount) {
  struct i2c_rdwr_ioctl_data data;
  struct i2c_msg msg[2];
  int n_msg = 0;
  int rc;

  memset(&msg, 0, sizeof(msg));

  if (tcount) {
    msg[n_msg].addr = addr >> 1;
    msg[n_msg].flags = 0;
    msg[n_msg].len = tcount;
    msg[n_msg].buf = tbuf;
    n_msg++;
  }

  if (rcount) {
    msg[n_msg].addr = addr >> 1;
    msg[n_msg].flags = I2C_M_RD;
    msg[n_msg].len = rcount;
    msg[n_msg].buf = rbuf;
    n_msg++;
  }

  data.msgs = msg;
  data.nmsgs = n_msg;

  rc = ioctl(fd, I2C_RDWR, &data);
  if (rc < 0) {
    // syslog(LOG_ERR, "Failed to do raw io");
    return -1;
  }

  return 0;
}

static int
pal_control_mux(int fd, uint8_t addr, uint8_t channel) {
  uint8_t tcount = 1, rcount = 0;
  uint8_t tbuf[16] = {0};
  uint8_t rbuf[16] = {0};

  // PCA9544A
  if (channel < 4)
    tbuf[0] = 0x04 + channel;
  else
    tbuf[0] = 0x00; // close all channels

  return i2c_io(fd, addr, tbuf, tcount, rbuf, rcount);
}

static int
pal_control_switch(int fd, uint8_t addr, uint8_t channel) {
  uint8_t tcount = 1, rcount = 0;
  uint8_t tbuf[16] = {0};
  uint8_t rbuf[16] = {0};

  // PCA9846
  if (channel < 4)
    tbuf[0] = 0x01 << channel;
  else
    tbuf[0] = 0x00; // close all channels

  return i2c_io(fd, addr, tbuf, tcount, rbuf, rcount);
}

static int
read_device(const char *device, int *value) {
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

  rc = fscanf(fp, "%d", value);
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
read_device_float(const char *device, float *value) {
  FILE *fp;
  int rc;
  char tmp[10];

  fp = fopen(device, "r");
  if (!fp) {
    int err = errno;
#ifdef DEBUG
    syslog(LOG_INFO, "failed to open device %s", device);
#endif
    return err;
  }

  rc = fscanf(fp, "%s", tmp);
  fclose(fp);

  if (rc != 1) {
#ifdef DEBUG
    syslog(LOG_INFO, "failed to read device %s", device);
#endif
    return ENOENT;
  }

  *value = atof(tmp);

  return 0;
}

static int
read_device_hex(const char *device, int *value) {
  FILE *fp;
  int rc;

  fp = fopen(device, "r");
  if (!fp) {
#ifdef DEBUG
    syslog(LOG_INFO, "failed to open device %s", device);
#endif
    return errno;
  }

  rc = fscanf(fp, "%x", value);
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
write_device(const char *device, const char *value) {
  FILE *fp;
  int rc;

  fp = fopen(device, "w");
  if (!fp) {
    int err = errno;
#ifdef DEBUG
    syslog(LOG_INFO, "failed to open device for write %s", device);
#endif
    return err;
  }

  rc = fputs(value, fp);
  fclose(fp);

  if (rc < 0) {
#ifdef DEBUG
    syslog(LOG_INFO, "failed to write device %s", device);
#endif
    return ENOENT;
  } else {
    return 0;
  }
}

static int
read_temp_attr(const char *device, const char *attr, float *value) {
  char full_name[LARGEST_DEVICE_NAME + 1];
  char dir_name[LARGEST_DEVICE_NAME + 1];
  int tmp;
  FILE *fp;
  int size;

  // Get current working directory
  snprintf(
      full_name, LARGEST_DEVICE_NAME, "cd %s;pwd", device);

  fp = popen(full_name, "r");
  fgets(dir_name, LARGEST_DEVICE_NAME, fp);
  pclose(fp);

  // Remove the newline character at the end
  size = strlen(dir_name);
  dir_name[size-1] = '\0';

  snprintf(
      full_name, LARGEST_DEVICE_NAME, "%s/%s", dir_name, attr);

  if (read_device(full_name, &tmp)) {
    return -1;
  }

  *value = ((float)tmp)/UNIT_DIV;

  return 0;
}

static int
read_temp(const char *device, float *value) {
  return read_temp_attr(device, "temp1_input", value);
}

static int
read_nic_temp(const char *device, float *value) {
  char full_name[LARGEST_DEVICE_NAME + 1];
  char dir_name[LARGEST_DEVICE_NAME + 1];
  int tmp;
  FILE *fp;
  int size;

  // Get current working directory
  snprintf(
      full_name, LARGEST_DEVICE_NAME, "cd %s;pwd", device);

  fp = popen(full_name, "r");
  fgets(dir_name, LARGEST_DEVICE_NAME, fp);
  pclose(fp);

  // Remove the newline character at the end
  size = strlen(dir_name);
  dir_name[size-1] = '\0';

  snprintf(
      full_name, LARGEST_DEVICE_NAME, "%s/temp2_input", dir_name);

  if (read_device(full_name, &tmp)) {
    return -1;
  }

  *value = ((float)tmp)/UNIT_DIV;

  // Workaround: handle when NICs wrongly report higher temperatures
  if (*value > NIC_MAX_TEMP) {
    return -1;
  }

  return 0;
}

static int
read_fan_value(const int fan, const char *device, int *value) {
  char device_name[LARGEST_DEVICE_NAME];
  char full_name[LARGEST_DEVICE_NAME];

  snprintf(device_name, LARGEST_DEVICE_NAME, device, fan);
  snprintf(full_name, LARGEST_DEVICE_NAME, "%s/%s", TACH_DIR, device_name);
  return read_device(full_name, value);
}

static int
read_fan_value_f(const int fan, const char *device, float *value) {
  char device_name[LARGEST_DEVICE_NAME];
  char full_name[LARGEST_DEVICE_NAME];
  int ret;

  snprintf(device_name, LARGEST_DEVICE_NAME, device, fan);
  snprintf(full_name, LARGEST_DEVICE_NAME, "%s/%s", TACH_DIR, device_name);
  ret = read_device_float(full_name, value);
  if (*value < 500) {
    sleep(2);
    ret = read_device_float(full_name, value);
  }

  return ret;
}

static int
write_fan_value(const int fan, const char *device, const int value) {
  char full_name[LARGEST_DEVICE_NAME];
  char device_name[LARGEST_DEVICE_NAME];
  char output_value[LARGEST_DEVICE_NAME];

  snprintf(device_name, LARGEST_DEVICE_NAME, device, fan);
  snprintf(full_name, LARGEST_DEVICE_NAME, "%s/%s", PWM_DIR, device_name);
  snprintf(output_value, LARGEST_DEVICE_NAME, "%d", value);
  return write_device(full_name, output_value);
}

static int
read_adc_value(const int pin, const char *device, float *value) {
  char device_name[LARGEST_DEVICE_NAME];
  char full_name[LARGEST_DEVICE_NAME];

  snprintf(device_name, LARGEST_DEVICE_NAME, device, pin);
  snprintf(full_name, LARGEST_DEVICE_NAME, "%s/%s", ADC_DIR, device_name);
  return read_device_float(full_name, value);
}

static int
read_hsc_value(const char *device, float *value) {
  char full_name[LARGEST_DEVICE_NAME];
  char dir_name[LARGEST_DEVICE_NAME + 1];
  int tmp;
  FILE *fp;
  int size;

  // Get current working directory
  snprintf(
      full_name, LARGEST_DEVICE_NAME, "cd %s;pwd", HSC_DEVICE);

  fp = popen(full_name, "r");
  fgets(dir_name, LARGEST_DEVICE_NAME, fp);
  pclose(fp);

  // Remove the newline character at the end
  size = strlen(dir_name);
  dir_name[size-1] = '\0';

  snprintf(full_name, LARGEST_DEVICE_NAME, "%s/%s", dir_name, device);
  if(read_device(full_name, &tmp)) {
    return -1;
  }

  *value = ((float) tmp)/UNIT_DIV;

  return 0;
}

static int
read_hsc_current_value(float *value) {
  uint8_t bus_id = 0x4; //TODO: ME's address 0x2c in FBTP
  uint8_t tbuf[256] = {0x00};
  uint8_t rbuf[256] = {0x00};
  uint8_t tlen = 0;
  uint8_t rlen = 0;
  float hsc_b = 20475;
  float Rsence;
  ipmb_req_t *req;
  ipmb_res_t *res;
  char path[64] = {0};
  int val=0;
  int ret = 0;
  static int retry = 0;

  req = (ipmb_req_t*)tbuf;

  req->res_slave_addr = 0x2C; //ME's Slave Address
  req->netfn_lun = NETFN_NM_REQ<<2;
  req->hdr_cksum = req->res_slave_addr + req->netfn_lun;
  req->hdr_cksum = ZERO_CKSUM_CONST - req->hdr_cksum;

  req->req_slave_addr = 0x20;
  req->seq_lun = 0x00;

  req->cmd = CMD_NM_SEND_RAW_PMBUS;
  req->data[0] = 0x57;
  req->data[1] = 0x01;
  req->data[2] = 0x00;
  req->data[3] = 0x86;
  //HSC slave addr check for SS and DS
  sprintf(path, GPIO_VAL, GPIO_BOARD_SKU_ID4);
  read_device(path, &val);
  if (val){ //DS
    req->data[4] = 0x8A;
    Rsence = 0.265;
  }else{    //SS
    req->data[4] = 0x22;
    Rsence = 0.505;
  }
  req->data[5] = 0x00;
  req->data[6] = 0x00;
  req->data[7] = 0x01;
  req->data[8] = 0x02;
  req->data[9] = 0x8C;
  tlen = 16;

  // Invoke IPMB library handler
  lib_ipmb_handle(bus_id, tbuf, tlen+1, &rbuf, &rlen);

  if (rlen == 0) {
#ifdef DEBUG
    syslog(LOG_DEBUG, "read_hsc_current_value: Zero bytes received\n");
#endif
    ret = READING_NA;
  }
  if (rbuf[6] == 0)
  {
    *value = ((float) (rbuf[11] << 8 | rbuf[10])*10-hsc_b )/(800*Rsence);
    retry = 0;
  } else {
    ret = READING_NA;
  }

  if (ret == READING_NA) {
    retry++;
    if (retry <= 3 )
      ret = READING_SKIP;
  }

  return ret;
}

static int
read_sensor_reading_from_ME(uint8_t snr_num, float *value) {
  uint8_t bus_id = 0x4; //TODO: ME's address 0x2c in FBTP
  uint8_t tbuf[256] = {0x00};
  uint8_t rbuf[256] = {0x00};
  uint8_t tlen = 0;
  uint8_t rlen = 0;
  ipmb_req_t *req;
  ipmb_res_t *res;
  int ret = 0;
  enum {
    e_HSC_PIN,
    e_HSC_VIN,
    e_PCH_TEMP,
    e_MAX,
  };
  static uint8_t retry[e_MAX] = {0};

  req = (ipmb_req_t*)tbuf;
  req->res_slave_addr = 0x2C; //ME's Slave Address
  req->netfn_lun = NETFN_SENSOR_REQ<<2;
  req->hdr_cksum = req->res_slave_addr + req->netfn_lun;
  req->hdr_cksum = ZERO_CKSUM_CONST - req->hdr_cksum;

  req->req_slave_addr = 0x20;
  req->seq_lun = 0x00;
  req->cmd = CMD_SENSOR_GET_SENSOR_READING;
  req->data[0] = snr_num;
  tlen = 7;

  // Invoke IPMB library handler
  lib_ipmb_handle(bus_id, tbuf, tlen+1, &rbuf, &rlen);

  if (rlen == 0) {
  //ME no response
#ifdef DEBUG
    syslog(LOG_DEBUG, "read HSC %x from_ME: Zero bytes received\n", snr_num);
#endif
   ret = READING_NA;
  } else {
    if (rbuf[6] == 0)
    {
        if (rbuf[8] & 0x20) {
          //not available
          ret = READING_NA;
        }
    } else {
      ret = READING_NA;
    }
  }

  if(snr_num == MB_SENSOR_HSC_IN_POWER) {
    if (!ret) {
      *value = (((float) rbuf[7])*0x20 + 0 )/10 ;
      retry[e_HSC_PIN] = 0;
    } else {
      retry[e_HSC_PIN]++;
      if (retry[e_HSC_PIN] <= 3)
        ret = READING_SKIP;
    }
  } else if(snr_num == MB_SENSOR_HSC_IN_VOLT) {
    if (!ret) {
      *value = (((float) rbuf[7])*0x02 + (0x5e*10) )/100 ;
      retry[e_HSC_VIN] = 0;
    } else {
      retry[e_HSC_VIN]++;
      if (retry[e_HSC_VIN] <= 3)
        ret = READING_SKIP;
    }
  } else if(snr_num == MB_SENSOR_PCH_TEMP) {
    if (!ret) {
      *value = (float) rbuf[7];
      retry[e_PCH_TEMP] = 0;
    } else {
      retry[e_PCH_TEMP]++;
      if (retry[e_PCH_TEMP] <= 3)
        ret = READING_SKIP;
    }
  }
  return ret;
}

static int
read_cpu_temp(uint8_t snr_num, float *value) {
  int ret = 0;
  uint8_t bus_id = 0x4; //TODO: ME's address 0x2c in FBTP
  uint8_t tbuf[256] = {0x00};
  uint8_t rbuf1[256] = {0x00};
  static uint8_t tjmax[2] = {0x00};
  static uint8_t tjmax_flag[2] = {0};
  uint8_t tlen = 0;
  uint8_t rlen = 0;
  ipmb_req_t *req;
  ipmb_res_t *res;
  char key[MAX_KEY_LEN] = {0};
  char str[MAX_VALUE_LEN] = {0};
  int cpu_index;
  int16_t dts;
  static uint8_t retry[2] = {0x00}; // CPU0 and CPU1

  switch (snr_num) {
    case MB_SENSOR_CPU0_TEMP:
      cpu_index = 0;
      break;
    case MB_SENSOR_CPU1_TEMP:
      cpu_index = 1;
      break;
    default:
      return -1;
  }

  req = (ipmb_req_t*)tbuf;

  req->res_slave_addr = 0x2C; //ME's Slave Address

  req->netfn_lun = NETFN_NM_REQ<<2;
  req->hdr_cksum = req->res_slave_addr + req->netfn_lun;
  req->hdr_cksum = ZERO_CKSUM_CONST - req->hdr_cksum;

  req->req_slave_addr = 0x20;
  req->seq_lun = 0x00;

  if( tjmax_flag[cpu_index] == 0 ) { // First time to get CPU0/CPU1 Tjmax reading
    //Get CPU0/CPU1 Tjmax
    req->cmd = CMD_NM_SEND_RAW_PECI;
    req->data[0] = 0x57;
    req->data[1] = 0x01;
    req->data[2] = 0x00;
    req->data[3] = 0x30 + cpu_index;
    req->data[4] = 0x05;
    req->data[5] = 0x05;
    req->data[6] = 0xa1;
    req->data[7] = 0x00;
    req->data[8] = 0x10;
    req->data[9] = 0x00;
    req->data[10] = 0x00;
    tlen = 17;
    // Invoke IPMB library handler
    lib_ipmb_handle(bus_id, tbuf, tlen+1, &rbuf1, &rlen);
    if (rlen == 0) {
    //ME no response
#ifdef DEBUG
      syslog(LOG_DEBUG, "%s(%d): Zero bytes received\n", __func__, __LINE__);
#endif
    } else {
      if (rbuf1[6] == 0)
      {
        // If PECI command successes and got a reasonable value
        if ( (rbuf1[10] == 0x40) && rbuf1[13] > 50) {
          tjmax[cpu_index] = rbuf1[13];
          tjmax_flag[cpu_index] = 1;
        }
      }
    }
  }

  //Updated CPU Tjmax cache
  sprintf(key, "mb_sensor%d", (cpu_index?MB_SENSOR_CPU1_TJMAX:MB_SENSOR_CPU0_TJMAX));
  if (tjmax_flag[cpu_index] != 0) {
    sprintf(str, "%.2f",(float) tjmax[cpu_index]);
  } else {
    //ME no response or PECI command completion code error. Set "NA" in sensor cache.
    strcpy(str, "NA");
  }
  edb_cache_set(key, str);

  // Get CPU temp if BMC got TjMax
  ret = READING_NA;
  if (tjmax_flag[cpu_index] != 0) {
    rlen = 0;
    memset( rbuf1,0x00,sizeof(rbuf1) );
    //Get CPU Temp
    req->cmd = CMD_NM_SEND_RAW_PECI;
    req->data[0] = 0x57;
    req->data[1] = 0x01;
    req->data[2] = 0x00;
    req->data[3] = 0x30 + cpu_index;
    req->data[4] = 0x05;
    req->data[5] = 0x05;
    req->data[6] = 0xa1;
    req->data[7] = 0x00;
    req->data[8] = 0x02;
    req->data[9] = 0xff;
    req->data[10] = 0x00;
    tlen = 17;

    // Invoke IPMB library handler
    lib_ipmb_handle(bus_id, tbuf, tlen+1, &rbuf1, &rlen);

    if (rlen == 0) {
      //ME no response
#ifdef DEBUG
      syslog(LOG_DEBUG, "%s(%d): Zero bytes received\n", __func__, __LINE__);
#endif
    } else {
      if (rbuf1[6] == 0) { // ME Completion Code
        if ( (rbuf1[10] == 0x40) ) { // PECI Completion Code
          dts = (rbuf1[11] | rbuf1[12] << 8);
          // Intel Doc#554767 p.58: Reserved Values 0x8000~0x81ff
          if (dts <= -32257) {
            ret = READING_NA;
          } else {
            // 16-bit, 2s complement [15]Sign Bit;[14:6]Integer Value;[5:0]Fractional Value
            *value = (float) (tjmax[0] + (dts >> 6));
            ret = 0;
          }
        }
      }
    }
  }

  if (ret != 0) {
    retry[cpu_index]++;
    if (retry[cpu_index] <= 3) {
      ret = READING_SKIP;
      return ret;
    }
  } else
    retry[cpu_index] = 0;

  return ret;
}

static int
read_dimm_temp(uint8_t snr_num, float *value) {
  int ret = READING_NA;
  uint8_t bus_id = 0x4; //TODO: ME's address 0x2c in FBTP
  uint8_t tbuf[256] = {0x00};
  uint8_t rbuf1[256] = {0x00};
  uint8_t tlen = 0;
  uint8_t rlen = 0;
  ipmb_req_t *req;
  ipmb_res_t *res;
  char key[MAX_KEY_LEN] = {0};
  char str[MAX_VALUE_LEN] = {0};
  int dimm_index, i;
  int max = 0;
  static uint8_t retry[4] = {0x00};
  int val;
  char path[64] = {0};

  // show NA if BIOS has not completed POST.
  sprintf(path, GPIO_VAL, GPIO_FM_BIOS_POST_CMPLT_N);
  if (read_device(path, &val) || val) {
    return ret;
  }

  switch (snr_num) {
    case MB_SENSOR_CPU0_DIMM_GRPA_TEMP:
      dimm_index = 0;
      break;
    case MB_SENSOR_CPU0_DIMM_GRPB_TEMP:
      dimm_index = 1;
      break;
    case MB_SENSOR_CPU1_DIMM_GRPC_TEMP:
      dimm_index = 2;
      break;
    case MB_SENSOR_CPU1_DIMM_GRPD_TEMP:
      dimm_index = 3;
      break;
    default:
      return -1;
  }

  req = (ipmb_req_t*)tbuf;

  req->res_slave_addr = 0x2C; //ME's Slave Address

  req->netfn_lun = NETFN_NM_REQ<<2;
  req->hdr_cksum = req->res_slave_addr + req->netfn_lun;
  req->hdr_cksum = ZERO_CKSUM_CONST - req->hdr_cksum;

  req->req_slave_addr = 0x20;
  req->seq_lun = 0x00;

  for (i=0; i<3; i++) { // Get 3 channel for each DIMM group
    //Get DIMM Temp per channel
    req->cmd = CMD_NM_SEND_RAW_PECI;
    req->data[0] = 0x57;
    req->data[1] = 0x01;
    req->data[2] = 0x00;
    req->data[3] = 0x30 + (dimm_index / 2);
    req->data[4] = 0x05;
    req->data[5] = 0x05;
    req->data[6] = 0xa1;
    req->data[7] = 0x00;
    req->data[8] = 0x0e;
    req->data[9] = 0x00 + (dimm_index % 2 * 3) + i;
    req->data[10] = 0x00;
    tlen = 17;

    // Invoke IPMB library handler
    lib_ipmb_handle(bus_id, tbuf, tlen+1, &rbuf1, &rlen);
    if (rlen == 0) {
    //ME no response
#ifdef DEBUG
      syslog(LOG_DEBUG, "%s(%d): Zero bytes received\n", __func__, __LINE__);
#endif
    } else {
      if (rbuf1[6] == 0)
      {
        // If PECI command successes
        if ( (rbuf1[10] == 0x40)) {
          if (rbuf1[11] > max)
            max = rbuf1[11];
          if (rbuf1[12] > max)
            max = rbuf1[11];
        }
      }
    }
  }
  if (max != 0)
    ret = 0;

  if (ret != 0) {
    retry[dimm_index]++;
    if (retry[dimm_index] <= 3) {
      ret = READING_SKIP;
      return ret;
    }
  } else
    retry[dimm_index] = 0;

  if (ret == 0) {
    *value = (float)max;
  }

  return ret;
}

static int
read_vr_volt(uint8_t vr, uint8_t loop, float *value) {
  int fd;
  char fn[32];
  static int count = 0;
  int ret = -1;
  unsigned int retry = MAX_READ_RETRY;
  uint8_t tcount, rcount;
  uint8_t tbuf[16] = {0};
  uint8_t rbuf[16] = {0};

  // The following block for detecting vr_update is in progress or not
  if ( access(VR_UPDATE_IN_PROGRESS, F_OK) == 0 )
  {
    //Avoid sensord unmonitoring vr sensors due to unexpected condition happen during vr_update
    if ( count > VR_TIMEOUT )
    {
      remove(VR_UPDATE_IN_PROGRESS);
      count = 0;
    }

    syslog(LOG_WARNING, "[%d]Stop Monitor VR Volt due to VR update is in progress\n", count++);

    return READING_NA;
  }

  count = 0;

  snprintf(fn, sizeof(fn), "/dev/i2c-%d", VR_BUS_ID);
  while (retry) {
    fd = open(fn, O_RDWR);
    if (fd < 0) {
      syslog(LOG_WARNING, "pal_get_vr_volt: i2c_open failed for bus#%x\n", VR_BUS_ID);
      retry--;
      msleep(100);
    } else {
      break;
    }

    if(retry == 0)
      goto error_exit;
  }

  // Set the page to read Voltage
  tbuf[0] = 0x00;
  tbuf[1] = loop;

  tcount = 2;
  rcount = 0;

  retry = MAX_READ_RETRY;
  while (retry) {
    ret = i2c_io(fd, vr, tbuf, tcount, rbuf, rcount);
    if (ret) {
#ifdef DEBUG
      syslog(LOG_WARNING, "pal_get_vr_volt: i2c_io failed for bus#%x, dev#%x\n", VR_BUS_ID, vr);
#endif
      retry--;
      msleep(100);
    } else {
      break;
    }

    if(retry == 0)
      goto error_exit;
  }

  // Read 2 bytes from Voltage register
  tbuf[0] = VR_TELEMETRY_VOLT;

  tcount = 1;
  rcount = 2;

  retry = MAX_READ_RETRY;
  while(retry) {
    ret = i2c_io(fd, vr, tbuf, tcount, rbuf, rcount);
    if (ret) {
#ifdef DEBUG
      syslog(LOG_WARNING, "pal_get_vr_volt: i2c_io failed for bus#%x, dev#%x\n", VR_BUS_ID, vr);
#endif
      retry--;
      msleep(100);
    } else {
      break;
    }

    if (retry == 0)
      goto error_exit;
  }

  // Calculate Voltage
  *value = ((rbuf[1] & 0x0F) * 256 + rbuf[0] ) * 1.25;
  *value /= 1000;

error_exit:
  if (fd > 0) {
    close(fd);
  }

  return ret;
}

static int
read_vr_curr(uint8_t vr, uint8_t loop, float *value) {
  int fd;
  char fn[32];
  static int count = 0;
  int ret = -1;
  unsigned int retry = MAX_READ_RETRY;
  uint8_t tcount, rcount;
  uint8_t tbuf[16] = {0};
  uint8_t rbuf[16] = {0};

  // The following block for detecting vr_update is in progress or not
  if ( access(VR_UPDATE_IN_PROGRESS, F_OK) == 0 )
  {
    // Avoid sensord unmonitoring vr sensors due to unexpected condition happen during vr_update
    if ( count > VR_TIMEOUT )
    {
      remove(VR_UPDATE_IN_PROGRESS);
      count = 0;
    }

    syslog(LOG_WARNING, "[%d]Stop Monitor VR Curr due to VR update is in progress\n", count++);

    return READING_NA;
  }

  count = 0;

  snprintf(fn, sizeof(fn), "/dev/i2c-%d", VR_BUS_ID);
  while (retry) {
    fd = open(fn, O_RDWR);
    if (fd < 0) {
      syslog(LOG_WARNING, "pal_get_vr_curr: i2c_open failed for bus#%x\n", VR_BUS_ID);
      retry--;
      msleep(100);
    } else {
      break;
    }

    if(retry == 0)
      goto error_exit;
  }

  // Set the page to read Voltage
  tbuf[0] = 0x00;
  tbuf[1] = loop;

  tcount = 2;
  rcount = 0;

  retry = MAX_READ_RETRY;
  while(retry) {
    ret = i2c_io(fd, vr, tbuf, tcount, rbuf, rcount);
    if (ret) {
#ifdef DEBUG
      syslog(LOG_WARNING, "pal_get_vr_curr: i2c_io failed for bus#%x, dev#%x\n", VR_BUS_ID, vr);
#endif
      retry--;
      msleep(100);
    } else {
      break;
    }

    if (retry == 0)
      goto error_exit;
  }

  // Read 2 bytes from Voltage register
  tbuf[0] = VR_TELEMETRY_CURR;

  tcount = 1;
  rcount = 2;

  retry = MAX_READ_RETRY;
  while(retry) {
    ret = i2c_io(fd, vr, tbuf, tcount, rbuf, rcount);
    if (ret) {
#ifdef DEBUG
      syslog(LOG_WARNING, "pal_get_vr_curr: i2c_io failed for bus#%x, dev#%x\n", VR_BUS_ID, vr);
#endif
      retry--;
      msleep(100);
    } else {
      break;
    }

    if (retry == 0)
      goto error_exit;
  }

  // Calculate Current
  *value = ((rbuf[1] & 0x7F) * 256 + rbuf[0] ) * 62.5;
  *value /= 1000;

  // Handle illegal values observed
  if (*value > 1000) {
    ret = -1;
  }

error_exit:
  if (fd > 0) {
    close(fd);
  }

  return ret;
}

static int
read_vr_power(uint8_t vr, uint8_t loop, float *value) {
  int fd;
  char fn[32];
  static int count = 0;
  int ret = -1;
  unsigned int retry = MAX_READ_RETRY;
  uint8_t tcount, rcount;
  uint8_t tbuf[16] = {0};
  uint8_t rbuf[16] = {0};

  // The following block for detecting vr_update is in progress or not
  if ( access(VR_UPDATE_IN_PROGRESS, F_OK) == 0 )
  {
    //avoid sensord unmonitoring vr sensors due to unexpected condition happen during vr_update
    if ( count > VR_TIMEOUT )
    {
      remove(VR_UPDATE_IN_PROGRESS);
      count = 0;
    }

    syslog(LOG_WARNING, "[%d]Stop Monitor VR Power due to VR update is in progress\n", count++);

    return READING_NA;
  }

  count = 0;

  snprintf(fn, sizeof(fn), "/dev/i2c-%d", VR_BUS_ID);
  while (retry) {
    fd = open(fn, O_RDWR);
    if (fd < 0) {
      syslog(LOG_WARNING, "pal_get_vr_power: i2c_open failed for bus#%x\n", VR_BUS_ID);
      retry--;
      msleep(100);
    } else {
      break;
    }

    if(retry == 0)
      goto error_exit;
  }

  // Set the page to read Power
  tbuf[0] = 0x00;
  tbuf[1] = loop;

  tcount = 2;
  rcount = 0;

  retry = MAX_READ_RETRY;
  while(retry) {
    ret = i2c_io(fd, vr, tbuf, tcount, rbuf, rcount);
    if (ret) {
#ifdef DEBUG
      syslog(LOG_WARNING, "pal_get_vr_power: i2c_io failed for bus#%x, dev#%x\n", VR_BUS_ID, vr);
#endif
      retry--;
      msleep(100);
    } else {
      break;
    }

    if (retry == 0)
      goto error_exit;
  }

  // Read 2 bytes from Power register
  tbuf[0] = VR_TELEMETRY_POWER;

  tcount = 1;
  rcount = 2;

  retry = MAX_READ_RETRY;
  while(retry) {
    ret = i2c_io(fd, vr, tbuf, tcount, rbuf, rcount);
    if (ret) {
#ifdef DEBUG
      syslog(LOG_WARNING, "pal_get_vr_power: i2c_io failed for bus#%x, dev#%x\n", VR_BUS_ID, vr);
#endif
      retry--;
      msleep(100);
    } else {
      break;
    }

    if (retry == 0)
      goto error_exit;
  }

  // Calculate Power
  *value = ((rbuf[1] & 0x3F) * 256 + rbuf[0] ) * 0.04;

error_exit:
  if (fd > 0) {
    close(fd);
  }

  return ret;
}

static int
read_vr_temp(uint8_t vr, uint8_t loop, float *value) {
  int fd;
  char fn[32];
  static int count = 0;
  int ret = -1;
  unsigned int retry = MAX_READ_RETRY;
  uint8_t tcount, rcount;
  uint8_t tbuf[16] = {0};
  uint8_t rbuf[16] = {0};

  // The following block for detecting vr_update is in progress or not
  if ( access(VR_UPDATE_IN_PROGRESS, F_OK) == 0 )
  {
    // Avoid sensord unmonitoring vr sensors due to unexpected condition happen during vr_update
    if ( count > VR_TIMEOUT )
    {
      remove(VR_UPDATE_IN_PROGRESS);
      count = 0;
    }

    syslog(LOG_WARNING, "[%d]Stop Monitor VR Temp due to VR update is in progress\n", count++);

    return READING_NA;
  }

  count = 0;

  snprintf(fn, sizeof(fn), "/dev/i2c-%d", VR_BUS_ID);
  while (retry) {
    fd = open(fn, O_RDWR);
    if (fd < 0) {
      syslog(LOG_WARNING, "pal_get_vr_temp: i2c_open failed for bus#%x\n", VR_BUS_ID);
      retry--;
      msleep(100);
    } else {
      break;
    }

    if(retry == 0)
      goto error_exit;
  }

  // Set the page to read Temperature
  tbuf[0] = 0x00;
  tbuf[1] = loop;

  tcount = 2;
  rcount = 0;

  retry = MAX_READ_RETRY;
  while (retry) {
  ret = i2c_io(fd, vr, tbuf, tcount, rbuf, rcount);
  if (ret) {
#ifdef DEBUG
    syslog(LOG_WARNING, "pal_get_vr_temp: i2c_io failed for bus#%x, dev#%x\n", VR_BUS_ID, vr);
#endif
      retry--;
      msleep(100);
    } else {
      break;
    }

    if(retry == 0)
      goto error_exit;
  }

  // Read 2 bytes from Power register
  tbuf[0] = VR_TELEMETRY_TEMP;

  tcount = 1;
  rcount = 2;

  retry = MAX_READ_RETRY;
  while(retry) {
    ret = i2c_io(fd, vr, tbuf, tcount, rbuf, rcount);
    if (ret) {
#ifdef DEBUG
      syslog(LOG_WARNING, "pal_get_vr_temp: i2c_io failed for bus#%x, dev#%x\n", VR_BUS_ID, vr);
#endif
      retry--;
      msleep(100);
    } else {
      break;
    }

    if(retry == 0)
      goto error_exit;
  }

  // Calculate Power
  *value = ((rbuf[1] & 0x0F) * 256 + rbuf[0] ) * 0.125;

error_exit:
  if (fd > 0) {
    close(fd);
  }

  return ret;
}

static int
read_ava_temp(uint8_t sensor_num, float *value) {
  int fd;
  char fn[32];
  int ret = READING_NA;;
  static unsigned int retry = 0;
  uint8_t tcount, rcount, slot_cfg, addr, mux_chan, mux_addr = 0xe2;
  uint8_t tbuf[16] = {0};
  uint8_t rbuf[16] = {0};

  if (pal_get_slot_cfg_id(&slot_cfg) < 0)
    slot_cfg = SLOT_CFG_EMPTY;

  switch(sensor_num) {
    case MB_SENSOR_C2_AVA_FTEMP:
    case MB_SENSOR_C2_AVA_RTEMP:
      if(slot_cfg == SLOT_CFG_EMPTY)
        return READING_NA;
      mux_chan = 0;
      break;
    case MB_SENSOR_C3_AVA_FTEMP:
    case MB_SENSOR_C3_AVA_RTEMP:
      if(slot_cfg == SLOT_CFG_EMPTY)
        return READING_NA;
      mux_chan = 1;
      break;
    case MB_SENSOR_C4_AVA_FTEMP:
    case MB_SENSOR_C4_AVA_RTEMP:
      if(slot_cfg != SLOT_CFG_SS_3x8)
        return READING_NA;
      mux_chan = 2;
      break;
    default:
      return READING_NA;
  }

  switch(sensor_num) {
    case MB_SENSOR_C2_AVA_FTEMP:
    case MB_SENSOR_C3_AVA_FTEMP:
    case MB_SENSOR_C4_AVA_FTEMP:
      addr = 0x90;
      break;
    case MB_SENSOR_C2_AVA_RTEMP:
    case MB_SENSOR_C3_AVA_RTEMP:
    case MB_SENSOR_C4_AVA_RTEMP:
      addr = 0x92;
      break;
  }

  snprintf(fn, sizeof(fn), "/dev/i2c-%d", RISER_BUS_ID);
  fd = open(fn, O_RDWR);
  if (fd < 0) {
    ret = READING_NA;
    goto error_exit;
  }

  // control multiplexer to target channel.
  ret = pal_control_mux(fd, mux_addr, mux_chan);
  if (ret < 0) {
    ret = READING_NA;
    goto error_exit;
  }

  // Read 2 bytes from TMP75
  tbuf[0] = 0x00;
  tcount = 1;
  rcount = 2;

  ret = i2c_io(fd, addr, tbuf, tcount, rbuf, rcount);
  if (ret < 0) {
    ret = READING_NA;
    goto error_exit;
  }
  ret = 0;
  retry = 0;

  // rbuf:MSB, LSB; 12-bit value on Bit[15:4], unit: 0.0625
  *value = (float)(signed char)rbuf[0];

error_exit:
  if (fd > 0) {
    pal_control_mux(fd, mux_addr, 0xff); // close
    close(fd);
  }

  if (ret == READING_NA && ++retry <= 3)
    ret = READING_SKIP;

  return ret;
}

static int
read_nvme_temp(uint8_t sensor_num, float *value) {
  int fd;
  char fn[32];
  int ret = READING_NA;
  static unsigned int retry = 0;
  uint8_t tcount, rcount, slot_cfg, addr = 0xd4, mux_chan, mux_addr = 0xe2;
  uint8_t switch_chan, switch_addr=0xe6;
  uint8_t tbuf[16] = {0};
  uint8_t rbuf[16] = {0};

  if (pal_get_slot_cfg_id(&slot_cfg) < 0)
    slot_cfg = SLOT_CFG_EMPTY;

  switch(sensor_num) {
    case MB_SENSOR_C2_1_NVME_CTEMP:
    case MB_SENSOR_C2_2_NVME_CTEMP:
    case MB_SENSOR_C2_3_NVME_CTEMP:
    case MB_SENSOR_C2_4_NVME_CTEMP:
      if(slot_cfg == SLOT_CFG_EMPTY)
        return READING_NA;
      mux_chan = 0;
      break;
    case MB_SENSOR_C3_1_NVME_CTEMP:
    case MB_SENSOR_C3_2_NVME_CTEMP:
    case MB_SENSOR_C3_3_NVME_CTEMP:
    case MB_SENSOR_C3_4_NVME_CTEMP:
      if(slot_cfg == SLOT_CFG_EMPTY)
        return READING_NA;
      mux_chan = 1;
      break;
    case MB_SENSOR_C4_1_NVME_CTEMP:
    case MB_SENSOR_C4_2_NVME_CTEMP:
    case MB_SENSOR_C4_3_NVME_CTEMP:
    case MB_SENSOR_C4_4_NVME_CTEMP:
      if(slot_cfg != SLOT_CFG_SS_3x8)
        return READING_NA;
      mux_chan = 2;
      break;
    default:
      return READING_NA;
  }

  switch(sensor_num) {
    case MB_SENSOR_C2_1_NVME_CTEMP:
    case MB_SENSOR_C3_1_NVME_CTEMP:
    case MB_SENSOR_C4_1_NVME_CTEMP:
      switch_chan = 0;
      break;
    case MB_SENSOR_C2_2_NVME_CTEMP:
    case MB_SENSOR_C3_2_NVME_CTEMP:
    case MB_SENSOR_C4_2_NVME_CTEMP:
      switch_chan = 1;
      break;
    case MB_SENSOR_C2_3_NVME_CTEMP:
    case MB_SENSOR_C3_3_NVME_CTEMP:
    case MB_SENSOR_C4_3_NVME_CTEMP:
      switch_chan = 2;
      break;
    case MB_SENSOR_C2_4_NVME_CTEMP:
    case MB_SENSOR_C3_4_NVME_CTEMP:
    case MB_SENSOR_C4_4_NVME_CTEMP:
      switch_chan = 3;
      break;
  }

  snprintf(fn, sizeof(fn), "/dev/i2c-%d", RISER_BUS_ID);
  fd = open(fn, O_RDWR);
  if (fd < 0) {
    ret = READING_NA;
    goto error_exit;
  }

  // control I2C multiplexer to target channel.
  ret = pal_control_mux(fd, mux_addr, mux_chan);
  if (ret < 0) {
    ret = READING_NA;
    goto error_exit;
  }

  // control I2C switch to target channel.
  ret = pal_control_switch(fd, switch_addr, switch_chan);
  // Report temp of PCIe card on MB_SENSOR_CX_1_NVME_CTEMP senosrs,
  // no I2C Switch on PCIe Card
  if (ret < 0 && switch_chan != 0) {
    ret = READING_NA;
    goto error_exit;
  }

  // Read 8 bytes from NVMe
  tbuf[0] = 0x00;
  tcount = 1;
  rcount = 8;

  ret = i2c_io(fd, addr, tbuf, tcount, rbuf, rcount);
  if (ret < 0) {
    ret = READING_NA;
    goto error_exit;
  }
  ret = 0;
  retry = 0;

  // Cmd 0: length, SFLGS, SMART Warnings, CTemp, PDLU, Reserved, Reserved, PEC
  *value = (float)(signed char)rbuf[3];

error_exit:
  if (fd > 0) {
    pal_control_switch(fd, switch_addr, 0xff); // close
    pal_control_mux(fd, mux_addr, 0xff); // close
    close(fd);
  }

  if (ret == READING_NA && ++retry <= 3)
    ret = READING_SKIP;

  return ret;
}

static int
pal_key_check(char *key) {

  int ret;
  int i;

  i = 0;
  while(strcmp(key_list[i], LAST_KEY)) {

    // If Key is valid, return success
    if (!strcmp(key, key_list[i]))
      return 0;

    i++;
  }

#ifdef DEBUG
  syslog(LOG_WARNING, "pal_key_check: invalid key - %s", key);
#endif
  return -1;
}

int
pal_get_key_value(char *key, char *value) {

  // Check is key is defined and valid
  if (pal_key_check(key))
    return -1;

  return kv_get(key, value);
}
int
pal_set_key_value(char *key, char *value) {

  // Check is key is defined and valid
  if (pal_key_check(key))
    return -1;

  return kv_set(key, value);
}

// Set GPIO low or high
static void
gpio_set(uint8_t gpio, bool value ) {
  char vpath[64] = {0};
  char *val;

  sprintf(vpath, GPIO_VAL, gpio);

  if (value) {
    val = "1";
  } else {
    val = "0";
  }

  write_device(vpath, val);
}

// Power On the server in a given slot
static int
server_power_on(void) {
  char vpath[64] = {0};

  sprintf(vpath, GPIO_VAL, GPIO_POWER);

  if (write_device(vpath, "1")) {
    return -1;
  }

  if (write_device(vpath, "0")) {
    return -1;
  }

  sleep(1);

  if (write_device(vpath, "1")) {
    return -1;
  }

  sleep(2);

  system("/usr/bin/sv restart fscd >> /dev/null");

  return 0;
}

// Power Off the server in given slot
static int
server_power_off(bool gs_flag) {
  char vpath[64] = {0};

  sprintf(vpath, GPIO_VAL, GPIO_POWER);

  system("/usr/bin/sv stop fscd >> /dev/null");

  if (write_device(vpath, "1")) {
    return -1;
  }

  sleep(1);

  if (write_device(vpath, "0")) {
    return -1;
  }

  if (gs_flag) {
    sleep(DELAY_GRACEFUL_SHUTDOWN);
  } else {
    sleep(DELAY_POWER_OFF);
  }

  if (write_device(vpath, "1")) {
    return -1;
  }

  return 0;
}

// Power reset the server in given slot
static int
server_power_reset(void) {
  char vpath[64] = {0};

  sprintf(vpath, GPIO_VAL, GPIO_POWER_RESET);

  system("/usr/bin/sv stop fscd >> /dev/null");

  if (write_device(vpath, "1")) {
    return -1;
  }

  sleep(1);

  if (write_device(vpath, "0")) {
    return -1;
  }

  sleep(1);

  if (write_device(vpath, "1")) {
    return -1;
  }

  system("/etc/init.d/setup-fan.sh >> /dev/null");
  system("/usr/bin/sv start fscd >> /dev/null");

  return 0;
}

// Debug Card's UART and BMC/SoL port share UART port and need to enable only one
static int
control_sol_txd(uint8_t fru) {
  uint32_t lpc_fd;
  uint32_t ctrl;
  void *lpc_reg;
  void *lpc_hicr;

  lpc_fd = open("/dev/mem", O_RDWR | O_SYNC );
  if (lpc_fd < 0) {
#ifdef DEBUG
    syslog(LOG_WARNING, "control_sol_txd: open fails\n");
#endif
    return -1;
  }

  lpc_reg = mmap(NULL, PAGE_SIZE, PROT_READ|PROT_WRITE, MAP_SHARED, lpc_fd,
             AST_LPC_BASE);
  lpc_hicr = (char*)lpc_reg + HICRA_OFFSET;

  // Read HICRA register
  ctrl = *(volatile uint32_t*) lpc_hicr;
  // Clear bits for UART2 and UART3 routing
  ctrl &= (~HICRA_MASK_UART2);
  ctrl &= (~HICRA_MASK_UART3);

  // Route UART2 to UART3 for SoL purpose
  ctrl |= (UART2_TO_UART3 << 22);

  // Route DEBUG to UART1 for TXD control
  ctrl |= (UART3_TO_UART2 << 19);

  *(volatile uint32_t*) lpc_hicr = ctrl;

  munmap(lpc_reg, PAGE_SIZE);
  close(lpc_fd);

  return 0;
}

// Display the given POST code using GPIO port
static int
pal_post_display(uint8_t status) {
  char path[64] = {0};
  int ret;
  char *val;

#ifdef DEBUG
  syslog(LOG_WARNING, "pal_post_display: status is %d\n", status);
#endif

  sprintf(path, GPIO_VAL, GPIO_POSTCODE_0);

  if (BIT(status, 0)) {
    val = "1";
  } else {
    val = "0";
  }

  ret = write_device(path, val);
  if (ret) {
    goto post_exit;
  }

  sprintf(path, GPIO_VAL, GPIO_POSTCODE_1);
  if (BIT(status, 1)) {
    val = "1";
  } else {
    val = "0";
  }

  ret = write_device(path, val);
  if (ret) {
    goto post_exit;
  }

  sprintf(path, GPIO_VAL, GPIO_POSTCODE_2);
  if (BIT(status, 2)) {
    val = "1";
  } else {
    val = "0";
  }

  ret = write_device(path, val);
  if (ret) {
    goto post_exit;
  }

  sprintf(path, GPIO_VAL, GPIO_POSTCODE_3);
  if (BIT(status, 3)) {
    val = "1";
  } else {
    val = "0";
  }

  ret = write_device(path, val);
  if (ret) {
    goto post_exit;
  }

  sprintf(path, GPIO_VAL, GPIO_POSTCODE_4);
  if (BIT(status, 4)) {
    val = "1";
  } else {
    val = "0";
  }

  ret = write_device(path, val);
  if (ret) {
    goto post_exit;
  }

  sprintf(path, GPIO_VAL, GPIO_POSTCODE_5);
  if (BIT(status, 5)) {
    val = "1";
  } else {
    val = "0";
  }

  ret = write_device(path, val);
  if (ret) {
    goto post_exit;
  }

  sprintf(path, GPIO_VAL, GPIO_POSTCODE_6);
  if (BIT(status, 6)) {
    val = "1";
  } else {
    val = "0";
  }

  ret = write_device(path, val);
  if (ret) {
    goto post_exit;
  }

  sprintf(path, GPIO_VAL, GPIO_POSTCODE_7);
  if (BIT(status, 7)) {
    val = "1";
  } else {
    val = "0";
  }

  ret = write_device(path, val);
  if (ret) {
    goto post_exit;
  }

post_exit:
  if (ret) {
#ifdef DEBUG
    syslog(LOG_WARNING, "write_device failed for %s\n", path);
#endif
    return -1;
  } else {
    return 0;
  }
}

// Platform Abstraction Layer (PAL) Functions
int
pal_get_platform_name(char *name) {
  strcpy(name, FBTP_PLATFORM_NAME);

  return 0;
}

int
pal_get_num_slots(uint8_t *num) {
  *num = FBTP_MAX_NUM_SLOTS;

  return 0;
}

int
pal_is_fru_prsnt(uint8_t fru, uint8_t *status) {
  *status = 1;

  return 0;
}

int
pal_is_fru_ready(uint8_t fru, uint8_t *status) {
  *status = 1;

  return 0;
}
int
pal_get_CPU_CATERR(uint8_t fru, uint8_t *status) {
  int val;
  char path[64] = {0};

  sprintf(path, GPIO_VAL, GPIO_FM_CPU_CATERR_LVT3_N);

  if (read_device(path, &val)) {
    return -1;
  }

  if (val == 0x0) {
     *status = 0;
  } else {
     *status = 1;
  }
  return 0;
}

int
pal_get_CPU_MSMI(uint8_t fru, uint8_t *status) {
  int val;
  char path[64] = {0};

  sprintf(path, GPIO_VAL, GPIO_FM_CPU_MSMI_LVT3_N);

  if (read_device(path, &val)) {
    return -1;
  }

  if (val == 0x0) {
    *status = 0;
   } else {
    *status = 1;
   }

   return 0;
}

int
pal_is_debug_card_prsnt(uint8_t *status) {
  int val;
  char path[64] = {0};

  sprintf(path, GPIO_VAL, GPIO_DBG_CARD_PRSNT);

  if (read_device(path, &val)) {
    return -1;
  }

  if (val == 0x0) {
    *status = 1;
  } else {
    *status = 0;
  }

  return 0;
}

int
pal_get_server_power(uint8_t fru, uint8_t *status) {
  int val;
  char path[64] = {0};

  sprintf(path, GPIO_VAL, GPIO_POWER_GOOD);

  if (read_device(path, &val)) {
    return -1;
  }

  if (val == 0x0) {
    *status = 0;
  } else {
    *status = 1;
  }

  return 0;
}

static bool
is_server_off(void) {
  int ret;
  uint8_t status;
  ret = pal_get_server_power(FRU_MB, &status);
  if (ret) {
    return false;
  }

  if (status == SERVER_POWER_OFF) {
    return true;
  } else {
    return false;
  }
}


// Power Off, Power On, or Power Reset the server in given slot
int
pal_set_server_power(uint8_t fru, uint8_t cmd) {
  uint8_t status;
  bool gs_flag = false;

  if (pal_get_server_power(fru, &status) < 0) {
    return -1;
  }

  switch(cmd) {
    case SERVER_POWER_ON:
      if (status == SERVER_POWER_ON)
        return 1;
      else
        return server_power_on();
      break;

    case SERVER_POWER_OFF:
      if (status == SERVER_POWER_OFF)
        return 1;
      else
        return server_power_off(gs_flag);
      break;

    case SERVER_POWER_CYCLE:
      if (status == SERVER_POWER_ON) {
        if (server_power_off(gs_flag))
          return -1;

        sleep(DELAY_POWER_CYCLE);

        return server_power_on();

      } else if (status == SERVER_POWER_OFF) {

        return (server_power_on());
      }
      break;

    case SERVER_GRACEFUL_SHUTDOWN:
      if (status == SERVER_POWER_OFF)
        return 1;
      else
        gs_flag = true;
        return server_power_off(gs_flag);
      break;

   case SERVER_POWER_RESET:
      if (status == SERVER_POWER_ON)
        return server_power_reset();
      else if (status == SERVER_POWER_OFF)
        return 1;
      break;

    default:
      return -1;
  }

  return 0;
}

int
pal_sled_cycle(void) {
  uint8_t platform_id;
  // Remove the adm1275 module as the HSC device is busy
  system("rmmod adm1275");

  // Send command to HSC power cycle
  // Single Side
  system("i2cset -y 7 0x11 0xd9 c &> /dev/null");

  // Double Side
  system("i2cset -y 7 0x45 0xd9 c &> /dev/null");

  return 0;
}

// Read the Front Panel Hand Switch and return the position
int
pal_get_hand_sw(uint8_t *pos) {
  return 0;
}

// Return the Front panel Power Button
int
pal_get_pwr_btn(uint8_t *status) {
  char path[64] = {0};
  int val;

  sprintf(path, GPIO_VAL, GPIO_PWR_BTN);
  if (read_device(path, &val)) {
    return -1;
  }

  if (val) {
    *status = 0x0;
  } else {
    *status = 0x1;
  }

  return 0;
}

// Return the front panel's Reset Button status
int
pal_get_rst_btn(uint8_t *status) {
  char path[64] = {0};
  int val;

  sprintf(path, GPIO_VAL, GPIO_RST_BTN);
  if (read_device(path, &val)) {
    return -1;
  }

  if (val) {
    *status = 0x0;
  } else {
    *status = 0x1;
  }

  return 0;
}

// Update the Reset button input to the server at given slot
int
pal_set_rst_btn(uint8_t slot, uint8_t status) {
  char path[64] = {0};
  char *val;

  if (slot < 1 || slot > 4) {
    return -1;
  }

  if (status) {
    val = "1";
  } else {
    val = "0";
  }

  sprintf(path, GPIO_VAL, gpio_rst_btn[slot]);
  if (write_device(path, val)) {
    return -1;
  }

  return 0;
}

// Update the LED for the given slot with the status
int
pal_set_led(uint8_t fru, uint8_t status) {
  char path[64] = {0};
  char *val;

//TODO: Need to check power LED control from CPLD
  return 0;

  if (status) {
    val = "1";
  } else {
    val = "0";
  }

  sprintf(path, GPIO_VAL, GPIO_POWER_LED);
  if (write_device(path, val)) {
    return -1;
  }

  return 0;
}

// Update Heartbeet LED
int
pal_set_hb_led(uint8_t status) {
  char cmd[64] = {0};
  char *val;

  if (status) {
    val = "1";
  } else {
    val = "0";
  }

  sprintf(cmd, "devmem 0x1e6c0064 32 %s", val);

  system(cmd);

  return 0;
}

// Update the Identification LED for the given fru with the status
int
pal_set_id_led(uint8_t fru, uint8_t status) {
  char path[64] = {0};
  char *val;

  if (status) {
    val = "1";
  } else {
    val = "0";
  }

  sprintf(path, GPIO_VAL, GPIO_POWER_LED);

  if (write_device(path, val)) {
    return -1;
  }

  return 0;
}

static int
set_usb_mux(uint8_t state) {
  return 0;
}

// Update the USB Mux to the server at given slot
int
pal_switch_usb_mux(uint8_t slot) {
  return 0;
}

// Switch the UART mux to the given fru
int
pal_switch_uart_mux(uint8_t fru) {
  return control_sol_txd(fru);
}

// Enable POST buffer for the server in given slot
int
pal_post_enable(uint8_t slot) {
  int ret;
  int i;

  return 0;
}

// Disable POST buffer for the server in given slot
int
pal_post_disable(uint8_t slot) {
  int ret;
  int i;

  return 0;
}

// Get the last post code of the given slot
int
pal_post_get_last(uint8_t slot, uint8_t *status) {
  int ret;
  uint8_t len;
  int i;

  return 0;
}

// Handle the received post code, for now display it on debug card
int
pal_post_handle(uint8_t slot, uint8_t status) {
  uint8_t prsnt, pos;
  int ret;

  // Check for debug card presence
  ret = pal_is_debug_card_prsnt(&prsnt);
  if (ret) {
    return ret;
  }

  // No debug card  present, return
  if (!prsnt) {
    return 0;
  }

  // Get the hand switch position
  ret = pal_get_hand_sw(&pos);
  if (ret) {
    return ret;
  }

  // If the give server is not selected, return
  if (pos != slot) {
    return 0;
  }

  // Display the post code in the debug card
  ret = pal_post_display(status);
  if (ret) {
    return ret;
  }

  return 0;
}

int
pal_get_fru_list(char *list) {

  strcpy(list, pal_fru_list);
  return 0;
}

int
pal_get_fru_id(char *str, uint8_t *fru) {
  if (!strcmp(str, "all")) {
    *fru = FRU_ALL;
  } else if (!strcmp(str, "mb")) {
    *fru = FRU_MB;
  } else if (!strcmp(str, "nic")) {
    *fru = FRU_NIC;
  } else {
    syslog(LOG_WARNING, "pal_get_fru_id: Wrong fru#%s", str);
    return -1;
  }

  return 0;
}

int
pal_get_fru_name(uint8_t fru, char *name) {
  return 0;
}

int
pal_get_fru_sdr_path(uint8_t fru, char *path) {
  return -1;
}

int
pal_sensor_sdr_init(uint8_t fru, sensor_info_t *sinfo) {
  return -1;
}

int
pal_get_fru_sensor_list(uint8_t fru, uint8_t **sensor_list, int *cnt) {
  switch(fru) {
  case FRU_MB:
    *sensor_list = (uint8_t *) mb_sensor_list;
    *cnt = mb_sensor_cnt;
    break;
  case FRU_NIC:
    *sensor_list = (uint8_t *) nic_sensor_list;
    *cnt = nic_sensor_cnt;
    break;
  default:
    return -1;
  }

  return 0;
}

int
pal_fruid_write(uint8_t fru, char *path) {
  return 0;
}

int
pal_sensor_read(uint8_t fru, uint8_t sensor_num, void *value) {
  char key[MAX_KEY_LEN] = {0};
  char str[MAX_VALUE_LEN] = {0};
  int ret;

  switch(fru) {
  case FRU_MB:
    sprintf(key, "mb_sensor%d", sensor_num);
    break;
  case FRU_NIC:
    sprintf(key, "nic_sensor%d", sensor_num);
    break;
  default:
    return -1;
  }
  ret = edb_cache_get(key, str);
  if(ret < 0) {
#ifdef DEBUG
    syslog(LOG_WARNING, "pal_sensor_read: cache_get %s failed.", key);
#endif
    return ret;
  }

  if(strcmp(str, "NA") == 0)
    return -1;

  *((float*)value) = atof(str);

  return ret;
}

int
pal_sensor_read_raw(uint8_t fru, uint8_t sensor_num, void *value) {
  uint8_t status;
  char key[MAX_KEY_LEN] = {0};
  char str[MAX_VALUE_LEN] = {0};
  int ret;
  float volt, curr;
  static uint8_t poweron_10s_flag = 0;
  uint8_t retry = MAX_READ_RETRY;

  switch(fru) {
  case FRU_MB:
    sprintf(key, "mb_sensor%d", sensor_num);
    if (is_server_off()) {
      poweron_10s_flag = 0;
      // Power is OFF, so only some of the sensors can be read
      switch(sensor_num) {
      // Temp. Sensors
      case MB_SENSOR_INLET_TEMP:
        ret = read_temp(MB_INLET_TEMP_DEVICE, (float*) value);
        break;
      case MB_SENSOR_OUTLET_TEMP:
        ret = read_temp(MB_OUTLET_TEMP_DEVICE, (float*) value);
        break;
      case MB_SENSOR_INLET_REMOTE_TEMP:
        ret = read_temp_attr(MB_INLET_TEMP_DEVICE, "temp2_input", (float*) value);
        break;
      case MB_SENSOR_OUTLET_REMOTE_TEMP:
        ret = read_temp_attr(MB_OUTLET_TEMP_DEVICE, "temp2_input", (float*) value);
        break;
      case MB_SENSOR_P12V:
        ret = read_adc_value(ADC_PIN2, ADC_VALUE, (float*) value);
        break;
      case MB_SENSOR_P1V05:
        ret = read_adc_value(ADC_PIN3, ADC_VALUE, (float*) value);
        break;
      case MB_SENSOR_PVNN_PCH_STBY:
        ret = read_adc_value(ADC_PIN4, ADC_VALUE, (float*) value);
        break;
      case MB_SENSOR_P3V3_STBY:
        ret = read_adc_value(ADC_PIN5, ADC_VALUE, (float*) value);
        break;
      case MB_SENSOR_P5V_STBY:
        ret = read_adc_value(ADC_PIN6, ADC_VALUE, (float*) value);
        break;
      case MB_SENSOR_P3V_BAT:
        gpio_set(GPIO_BAT_SENSE_EN_N, 0);
        msleep(1);
        ret = read_adc_value(ADC_PIN7, ADC_VALUE, (float*) value);
        gpio_set(GPIO_BAT_SENSE_EN_N, 1);
        break;

      // Hot Swap Controller
      case MB_SENSOR_HSC_IN_VOLT:
        ret = read_sensor_reading_from_ME(MB_SENSOR_HSC_IN_VOLT, (float*) value);
        break;
      case MB_SENSOR_HSC_OUT_CURR:
        ret = read_hsc_current_value((float*) value);
        break;
      case MB_SENSOR_HSC_IN_POWER:
        ret = read_sensor_reading_from_ME(MB_SENSOR_HSC_IN_POWER, (float*) value);
        break;
      default:
        ret = READING_NA;
        break;
      }
    } else {
      if((poweron_10s_flag < 5) && ((sensor_num == MB_SENSOR_HSC_IN_VOLT) ||
         (sensor_num == MB_SENSOR_HSC_OUT_CURR) || (sensor_num == MB_SENSOR_HSC_IN_POWER) ||
         (sensor_num == MB_SENSOR_FAN0_TACH) || (sensor_num == MB_SENSOR_FAN1_TACH))) {
        if(sensor_num == MB_SENSOR_HSC_IN_POWER){
          poweron_10s_flag++;
        }
        ret = READING_NA;
        break;
      }
      switch(sensor_num) {
      // Temp. Sensors
      case MB_SENSOR_INLET_TEMP:
        ret = read_temp(MB_INLET_TEMP_DEVICE, (float*) value);
        break;
      case MB_SENSOR_OUTLET_TEMP:
        ret = read_temp(MB_OUTLET_TEMP_DEVICE, (float*) value);
        break;
      case MB_SENSOR_INLET_REMOTE_TEMP:
        ret = read_temp_attr(MB_INLET_TEMP_DEVICE, "temp2_input", (float*) value);
        break;
      case MB_SENSOR_OUTLET_REMOTE_TEMP:
        ret = read_temp_attr(MB_OUTLET_TEMP_DEVICE, "temp2_input", (float*) value);
        break;
      // Fan Sensors
      case MB_SENSOR_FAN0_TACH:
        ret = read_fan_value_f(FAN0, FAN_TACH_RPM, (float*) value);
        break;
      case MB_SENSOR_FAN1_TACH:
        ret = read_fan_value_f(FAN1, FAN_TACH_RPM, (float*) value);
        break;
      // Various Voltages
      case MB_SENSOR_P3V3:
        ret = read_adc_value(ADC_PIN0, ADC_VALUE, (float*) value);
        break;
      case MB_SENSOR_P5V:
        ret = read_adc_value(ADC_PIN1, ADC_VALUE, (float*) value);
        break;
      case MB_SENSOR_P12V:
        ret = read_adc_value(ADC_PIN2, ADC_VALUE, (float*) value);
        break;
      case MB_SENSOR_P1V05:
        ret = read_adc_value(ADC_PIN3, ADC_VALUE, (float*) value);
        break;
      case MB_SENSOR_PVNN_PCH_STBY:
        ret = read_adc_value(ADC_PIN4, ADC_VALUE, (float*) value);
        break;
      case MB_SENSOR_P3V3_STBY:
        ret = read_adc_value(ADC_PIN5, ADC_VALUE, (float*) value);
        break;
      case MB_SENSOR_P5V_STBY:
        ret = read_adc_value(ADC_PIN6, ADC_VALUE, (float*) value);
        break;
      case MB_SENSOR_P3V_BAT:
        gpio_set(GPIO_BAT_SENSE_EN_N, 0);
        msleep(1);
        ret = read_adc_value(ADC_PIN7, ADC_VALUE, (float*) value);
        gpio_set(GPIO_BAT_SENSE_EN_N, 1);
        break;

      // Hot Swap Controller
      case MB_SENSOR_HSC_IN_VOLT:
        ret = read_sensor_reading_from_ME(MB_SENSOR_HSC_IN_VOLT, (float*) value);
        break;
      case MB_SENSOR_HSC_OUT_CURR:
        ret = read_hsc_current_value((float*) value);
        break;
      case MB_SENSOR_HSC_IN_POWER:
        ret = read_sensor_reading_from_ME(MB_SENSOR_HSC_IN_POWER, (float*) value);
        break;
      //CPU, DIMM, PCH Temp
      case MB_SENSOR_CPU0_TEMP:
      case MB_SENSOR_CPU1_TEMP:
        ret = read_cpu_temp(sensor_num, (float*) value);
        break;
      case MB_SENSOR_CPU0_DIMM_GRPA_TEMP:
      case MB_SENSOR_CPU0_DIMM_GRPB_TEMP:
      case MB_SENSOR_CPU1_DIMM_GRPC_TEMP:
      case MB_SENSOR_CPU1_DIMM_GRPD_TEMP:
        ret = read_dimm_temp(sensor_num, (float*) value);
        break;
      case MB_SENSOR_PCH_TEMP:
        ret = read_sensor_reading_from_ME(MB_SENSOR_PCH_TEMP, (float*) value);
        break;
      //VR Sensors
      case MB_SENSOR_VR_CPU0_VCCIN_TEMP:
        ret = read_vr_temp(VR_CPU0_VCCIN, VR_LOOP_PAGE_0, (float*) value);
        break;
      case MB_SENSOR_VR_CPU0_VCCIN_CURR:
        ret = read_vr_curr(VR_CPU0_VCCIN, VR_LOOP_PAGE_0, (float*) value);
        break;
      case MB_SENSOR_VR_CPU0_VCCIN_VOLT:
        ret = read_vr_volt(VR_CPU0_VCCIN, VR_LOOP_PAGE_0, (float*) value);
        break;
      case MB_SENSOR_VR_CPU0_VCCIN_POWER:
        ret = read_vr_power(VR_CPU0_VCCIN, VR_LOOP_PAGE_0, (float*) value);
        break;
      case MB_SENSOR_VR_CPU0_VSA_TEMP:
        ret = read_vr_temp(VR_CPU0_VSA, VR_LOOP_PAGE_1, (float*) value);
        break;
      case MB_SENSOR_VR_CPU0_VSA_CURR:
        ret = read_vr_curr(VR_CPU0_VSA, VR_LOOP_PAGE_1, (float*) value);
        break;
      case MB_SENSOR_VR_CPU0_VSA_VOLT:
        ret = read_vr_volt(VR_CPU0_VSA, VR_LOOP_PAGE_1, (float*) value);
        break;
      case MB_SENSOR_VR_CPU0_VSA_POWER:
        ret = read_vr_power(VR_CPU0_VSA, VR_LOOP_PAGE_1, (float*) value);
        break;
      case MB_SENSOR_VR_CPU0_VCCIO_TEMP:
        ret = read_vr_temp(VR_CPU0_VCCIO, VR_LOOP_PAGE_0, (float*) value);
        break;
      case MB_SENSOR_VR_CPU0_VCCIO_CURR:
        ret = read_vr_curr(VR_CPU0_VCCIO, VR_LOOP_PAGE_0, (float*) value);
        break;
      case MB_SENSOR_VR_CPU0_VCCIO_VOLT:
        ret = read_vr_volt(VR_CPU0_VCCIO, VR_LOOP_PAGE_0, (float*) value);
        break;
      case MB_SENSOR_VR_CPU0_VCCIO_POWER:
        ret = read_vr_power(VR_CPU0_VCCIO, VR_LOOP_PAGE_0, (float*) value);
        break;
      case MB_SENSOR_VR_CPU0_VDDQ_GRPA_TEMP:
        ret = read_vr_temp(g_vr_cpu0_vddq_abc, VR_LOOP_PAGE_0, (float*) value);
        break;
      case MB_SENSOR_VR_CPU0_VDDQ_GRPA_CURR:
        ret = read_vr_curr(g_vr_cpu0_vddq_abc, VR_LOOP_PAGE_0, (float*) value);
        break;
      case MB_SENSOR_VR_CPU0_VDDQ_GRPA_VOLT:
        ret = read_vr_volt(g_vr_cpu0_vddq_abc, VR_LOOP_PAGE_0, (float*) value);
        break;
      case MB_SENSOR_VR_CPU0_VDDQ_GRPA_POWER:
        ret = read_vr_power(g_vr_cpu0_vddq_abc, VR_LOOP_PAGE_0, (float*) value);
        break;
      case MB_SENSOR_VR_CPU0_VDDQ_GRPB_TEMP:
        ret = read_vr_temp(g_vr_cpu0_vddq_def, VR_LOOP_PAGE_0, (float*) value);
        break;
      case MB_SENSOR_VR_CPU0_VDDQ_GRPB_CURR:
        ret = read_vr_curr(g_vr_cpu0_vddq_def, VR_LOOP_PAGE_0, (float*) value);
        break;
      case MB_SENSOR_VR_CPU0_VDDQ_GRPB_VOLT:
        ret = read_vr_volt(g_vr_cpu0_vddq_def, VR_LOOP_PAGE_0, (float*) value);
        break;
      case MB_SENSOR_VR_CPU0_VDDQ_GRPB_POWER:
        ret = read_vr_power(g_vr_cpu0_vddq_def, VR_LOOP_PAGE_0, (float*) value);
        break;
      case MB_SENSOR_VR_CPU1_VCCIN_TEMP:
        if (is_cpu1_socket_occupy())
          ret = read_vr_temp(VR_CPU1_VCCIN, VR_LOOP_PAGE_0, (float*) value);
        else
          ret = READING_NA;
        break;
      case MB_SENSOR_VR_CPU1_VCCIN_CURR:
        if (is_cpu1_socket_occupy())
          ret = read_vr_curr(VR_CPU1_VCCIN, VR_LOOP_PAGE_0, (float*) value);
        else
          ret = READING_NA;
        break;
      case MB_SENSOR_VR_CPU1_VCCIN_VOLT:
        if (is_cpu1_socket_occupy())
          ret = read_vr_volt(VR_CPU1_VCCIN, VR_LOOP_PAGE_0, (float*) value);
        else
          ret = READING_NA;
        break;
      case MB_SENSOR_VR_CPU1_VCCIN_POWER:
        if (is_cpu1_socket_occupy())
          ret = read_vr_power(VR_CPU1_VCCIN, VR_LOOP_PAGE_0, (float*) value);
        else
          ret = READING_NA;
        break;
      case MB_SENSOR_VR_CPU1_VSA_TEMP:
        if (is_cpu1_socket_occupy())
          ret = read_vr_temp(VR_CPU1_VSA, VR_LOOP_PAGE_1, (float*) value);
        else
          ret = READING_NA;
        break;
      case MB_SENSOR_VR_CPU1_VSA_CURR:
        if (is_cpu1_socket_occupy())
          ret = read_vr_curr(VR_CPU1_VSA, VR_LOOP_PAGE_1, (float*) value);
        else
          ret = READING_NA;
        break;
      case MB_SENSOR_VR_CPU1_VSA_VOLT:
        if (is_cpu1_socket_occupy())
          ret = read_vr_volt(VR_CPU1_VSA, VR_LOOP_PAGE_1, (float*) value);
        else
          ret = READING_NA;
        break;
      case MB_SENSOR_VR_CPU1_VSA_POWER:
        if (is_cpu1_socket_occupy())
          ret = read_vr_power(VR_CPU1_VSA, VR_LOOP_PAGE_1, (float*) value);
        else
          ret = READING_NA;
        break;
      case MB_SENSOR_VR_CPU1_VCCIO_TEMP:
        if (is_cpu1_socket_occupy())
          ret = read_vr_temp(VR_CPU1_VCCIO, VR_LOOP_PAGE_0, (float*) value);
        else
          ret = READING_NA;
        break;
      case MB_SENSOR_VR_CPU1_VCCIO_CURR:
        if (is_cpu1_socket_occupy())
          ret = read_vr_curr(VR_CPU1_VCCIO, VR_LOOP_PAGE_0, (float*) value);
        else
          ret = READING_NA;
        break;
      case MB_SENSOR_VR_CPU1_VCCIO_VOLT:
        if (is_cpu1_socket_occupy())
          ret = read_vr_volt(VR_CPU1_VCCIO, VR_LOOP_PAGE_0, (float*) value);
        else
          ret = READING_NA;
        break;
      case MB_SENSOR_VR_CPU1_VCCIO_POWER:
        if (is_cpu1_socket_occupy())
          ret = read_vr_power(VR_CPU1_VCCIO, VR_LOOP_PAGE_0, (float*) value);
        else
          ret = READING_NA;
        break;
      case MB_SENSOR_VR_CPU1_VDDQ_GRPC_TEMP:
        if (is_cpu1_socket_occupy())
          ret = read_vr_temp(g_vr_cpu1_vddq_ghj, VR_LOOP_PAGE_0, (float*) value);
        else
          ret = READING_NA;
        break;
      case MB_SENSOR_VR_CPU1_VDDQ_GRPC_CURR:
        if (is_cpu1_socket_occupy())
          ret = read_vr_curr(g_vr_cpu1_vddq_ghj, VR_LOOP_PAGE_0, (float*) value);
        else
          ret = READING_NA;
        break;
      case MB_SENSOR_VR_CPU1_VDDQ_GRPC_VOLT:
        if (is_cpu1_socket_occupy())
          ret = read_vr_volt(g_vr_cpu1_vddq_ghj, VR_LOOP_PAGE_0, (float*) value);
        else
          ret = READING_NA;
        break;
      case MB_SENSOR_VR_CPU1_VDDQ_GRPC_POWER:
        if (is_cpu1_socket_occupy())
          ret = read_vr_power(g_vr_cpu1_vddq_ghj, VR_LOOP_PAGE_0, (float*) value);
        else
          ret = READING_NA;
        break;
      case MB_SENSOR_VR_CPU1_VDDQ_GRPD_TEMP:
        if (is_cpu1_socket_occupy())
          ret = read_vr_temp(g_vr_cpu1_vddq_klm, VR_LOOP_PAGE_0, (float*) value);
        else
          ret = READING_NA;
        break;
      case MB_SENSOR_VR_CPU1_VDDQ_GRPD_CURR:
        if (is_cpu1_socket_occupy())
          ret = read_vr_curr(g_vr_cpu1_vddq_klm, VR_LOOP_PAGE_0, (float*) value);
        else
          ret = READING_NA;
        break;
      case MB_SENSOR_VR_CPU1_VDDQ_GRPD_VOLT:
        if (is_cpu1_socket_occupy())
          ret = read_vr_volt(g_vr_cpu1_vddq_klm, VR_LOOP_PAGE_0, (float*) value);
        else
          ret = READING_NA;
        break;
      case MB_SENSOR_VR_CPU1_VDDQ_GRPD_POWER:
        if (is_cpu1_socket_occupy())
          ret = read_vr_power(g_vr_cpu1_vddq_klm, VR_LOOP_PAGE_0, (float*) value);
        else
          ret = READING_NA;
        break;
      case MB_SENSOR_VR_PCH_PVNN_TEMP:
        ret = read_vr_temp(VR_PCH_PVNN, VR_LOOP_PAGE_0, (float*) value);
        break;
      case MB_SENSOR_VR_PCH_PVNN_CURR:
        ret = read_vr_curr(VR_PCH_PVNN, VR_LOOP_PAGE_0, (float*) value);
        break;
      case MB_SENSOR_VR_PCH_PVNN_VOLT:
        ret = read_vr_volt(VR_PCH_PVNN, VR_LOOP_PAGE_0, (float*) value);
        break;
      case MB_SENSOR_VR_PCH_PVNN_POWER:
        ret = read_vr_power(VR_PCH_PVNN, VR_LOOP_PAGE_0, (float*) value);
        break;
      case MB_SENSOR_VR_PCH_P1V05_TEMP:
        ret = read_vr_temp(VR_PCH_P1V05, VR_LOOP_PAGE_1, (float*) value);
        break;
      case MB_SENSOR_VR_PCH_P1V05_CURR:
        ret = read_vr_curr(VR_PCH_P1V05, VR_LOOP_PAGE_1, (float*) value);
        break;
      case MB_SENSOR_VR_PCH_P1V05_VOLT:
        ret = read_vr_volt(VR_PCH_P1V05, VR_LOOP_PAGE_1, (float*) value);
        break;
      case MB_SENSOR_VR_PCH_P1V05_POWER:
        ret = read_vr_power(VR_PCH_P1V05, VR_LOOP_PAGE_1, (float*) value);
        break;
      case MB_SENSOR_C2_AVA_FTEMP:
      case MB_SENSOR_C2_AVA_RTEMP:
      case MB_SENSOR_C3_AVA_FTEMP:
      case MB_SENSOR_C3_AVA_RTEMP:
      case MB_SENSOR_C4_AVA_FTEMP:
      case MB_SENSOR_C4_AVA_RTEMP:
        ret = read_ava_temp(sensor_num, (float*) value);
        break;
      case MB_SENSOR_C2_1_NVME_CTEMP:
      case MB_SENSOR_C2_2_NVME_CTEMP:
      case MB_SENSOR_C2_3_NVME_CTEMP:
      case MB_SENSOR_C2_4_NVME_CTEMP:
      case MB_SENSOR_C3_1_NVME_CTEMP:
      case MB_SENSOR_C3_2_NVME_CTEMP:
      case MB_SENSOR_C3_3_NVME_CTEMP:
      case MB_SENSOR_C3_4_NVME_CTEMP:
      case MB_SENSOR_C4_1_NVME_CTEMP:
      case MB_SENSOR_C4_2_NVME_CTEMP:
      case MB_SENSOR_C4_3_NVME_CTEMP:
      case MB_SENSOR_C4_4_NVME_CTEMP:
        ret = read_nvme_temp(sensor_num, (float*) value);
        break;

      default:
        return -1;
      }
    }
    break;
  case FRU_NIC:
    sprintf(key, "nic_sensor%d", sensor_num);
    switch(sensor_num) {
    case MEZZ_SENSOR_TEMP:
      ret = read_nic_temp(MEZZ_TEMP_DEVICE, (float*) value);
      break;
    default:
      return -1;
    }
    break;
  default:
    return -1;
  }

  if (ret) {
    if (ret == READING_NA || ret == -1) {
      strcpy(str, "NA");
    } else {
      return ret;
    }
  } else {
    sprintf(str, "%.2f",*((float*)value));
  }

  if(edb_cache_set(key, str) < 0) {
#ifdef DEBUG
     syslog(LOG_WARNING, "pal_sensor_read_raw: cache_set key = %s, str = %s failed.", key, str);
#endif
    return -1;
  } else {
    return ret;
  }

  return 0;
}

int
pal_sensor_threshold_flag(uint8_t fru, uint8_t snr_num, uint16_t *flag) {

  return 0;
}

int
pal_get_sensor_threshold(uint8_t fru, uint8_t sensor_num, uint8_t thresh, void *value) {
  float *val = (float*) value;
  sensor_thresh_array_init();

  switch(fru) {
  case FRU_MB:
    *val = mb_sensor_threshold[sensor_num][thresh];
    break;
  case FRU_NIC:
    *val = nic_sensor_threshold[sensor_num][thresh];
    break;
  default:
    return -1;
  }
  return 0;
}

int
pal_get_sensor_name(uint8_t fru, uint8_t sensor_num, char *name) {
  switch(fru) {
  case FRU_MB:
    switch(sensor_num) {
    case MB_SENSOR_INLET_TEMP:
      sprintf(name, "MB_INLET_TEMP");
      break;
    case MB_SENSOR_OUTLET_TEMP:
      sprintf(name, "MB_OUTLET_TEMP");
      break;
    case MB_SENSOR_INLET_REMOTE_TEMP:
      sprintf(name, "MB_INLET_REMOTE_TEMP");
      break;
    case MB_SENSOR_OUTLET_REMOTE_TEMP:
      sprintf(name, "MB_OUTLET_REMOTE_TEMP");
      break;
    case MB_SENSOR_FAN0_TACH:
      sprintf(name, "MB_FAN0_TACH");
      break;
    case MB_SENSOR_FAN1_TACH:
      sprintf(name, "MB_FAN1_TACH");
      break;
    case MB_SENSOR_P3V3:
      sprintf(name, "MB_P3V3");
      break;
    case MB_SENSOR_P5V:
      sprintf(name, "MB_P5V");
      break;
    case MB_SENSOR_P12V:
      sprintf(name, "MB_P12V");
      break;
    case MB_SENSOR_P1V05:
      sprintf(name, "MB_P1V05");
      break;
    case MB_SENSOR_PVNN_PCH_STBY:
      sprintf(name, "MB_PVNN_PCH_STBY");
      break;
    case MB_SENSOR_P3V3_STBY:
      sprintf(name, "MB_P3V3_STBY");
      break;
    case MB_SENSOR_P5V_STBY:
      sprintf(name, "MB_P5V_STBY");
      break;
    case MB_SENSOR_P3V_BAT:
      sprintf(name, "MB_P3V_BAT");
      break;
    case MB_SENSOR_HSC_IN_VOLT:
      sprintf(name, "MB_HSC_IN_VOLT");
      break;
    case MB_SENSOR_HSC_OUT_CURR:
      sprintf(name, "MB_HSC_OUT_CURR");
      break;
    case MB_SENSOR_HSC_IN_POWER:
      sprintf(name, "MB_HSC_IN_POWER");
      break;
    case MB_SENSOR_CPU0_TEMP:
      sprintf(name, "MB_CPU0_TEMP");
      break;
    case MB_SENSOR_CPU0_TJMAX:
      sprintf(name, "MB_CPU0_TJMAX");
      break;
    case MB_SENSOR_CPU1_TEMP:
      sprintf(name, "MB_CPU1_TEMP");
      break;
    case MB_SENSOR_CPU1_TJMAX:
      sprintf(name, "MB_CPU1_TJMAX");
      break;
    case MB_SENSOR_PCH_TEMP:
      sprintf(name, "MB_PCH_TEMP");
      break;
    case MB_SENSOR_CPU0_DIMM_GRPA_TEMP:
      sprintf(name, "MB_CPU0_DIMM_GRPA_TEMP");
      break;
    case MB_SENSOR_CPU0_DIMM_GRPB_TEMP:
      sprintf(name, "MB_CPU0_DIMM_GRPB_TEMP");
      break;
    case MB_SENSOR_CPU1_DIMM_GRPC_TEMP:
      sprintf(name, "MB_CPU1_DIMM_GRPC_TEMP");
      break;
    case MB_SENSOR_CPU1_DIMM_GRPD_TEMP:
      sprintf(name, "MB_CPU1_DIMM_GRPD_TEMP");
      break;
    case MB_SENSOR_VR_CPU0_VCCIN_TEMP:
      sprintf(name, "MB_VR_CPU0_VCCIN_TEMP");
      break;
    case MB_SENSOR_VR_CPU0_VCCIN_CURR:
      sprintf(name, "MB_VR_CPU0_VCCIN_CURR");
      break;
    case MB_SENSOR_VR_CPU0_VCCIN_VOLT:
      sprintf(name, "MB_VR_CPU0_VCCIN_VOLT");
      break;
    case MB_SENSOR_VR_CPU0_VCCIN_POWER:
      sprintf(name, "MB_VR_CPU0_VCCIN_POWER");
      break;
    case MB_SENSOR_VR_CPU0_VSA_TEMP:
      sprintf(name, "MB_VR_CPU0_VSA_TEMP");
      break;
    case MB_SENSOR_VR_CPU0_VSA_CURR:
      sprintf(name, "MB_VR_CPU0_VSA_CURR");
      break;
    case MB_SENSOR_VR_CPU0_VSA_VOLT:
      sprintf(name, "MB_VR_CPU0_VSA_VOLT");
      break;
    case MB_SENSOR_VR_CPU0_VSA_POWER:
      sprintf(name, "MB_VR_CPU0_VSA_POWER");
      break;
    case MB_SENSOR_VR_CPU0_VCCIO_TEMP:
      sprintf(name, "MB_VR_CPU0_VCCIO_TEMP");
      break;
    case MB_SENSOR_VR_CPU0_VCCIO_CURR:
      sprintf(name, "MB_VR_CPU0_VCCIO_CURR");
      break;
    case MB_SENSOR_VR_CPU0_VCCIO_VOLT:
      sprintf(name, "MB_VR_CPU0_VCCIO_VOLT");
      break;
    case MB_SENSOR_VR_CPU0_VCCIO_POWER:
      sprintf(name, "MB_VR_CPU0_VCCIO_POWER");
      break;
    case MB_SENSOR_VR_CPU0_VDDQ_GRPA_TEMP:
      sprintf(name, "MB_VR_CPU0_VDDQ_GRPA_TEMP");
      break;
    case MB_SENSOR_VR_CPU0_VDDQ_GRPA_CURR:
      sprintf(name, "MB_VR_CPU0_VDDQ_GRPA_CURR");
      break;
    case MB_SENSOR_VR_CPU0_VDDQ_GRPA_VOLT:
      sprintf(name, "MB_VR_CPU0_VDDQ_GRPA_VOLT");
      break;
    case MB_SENSOR_VR_CPU0_VDDQ_GRPA_POWER:
      sprintf(name, "MB_VR_CPU0_VDDQ_GRPA_POWER");
      break;
    case MB_SENSOR_VR_CPU0_VDDQ_GRPB_TEMP:
      sprintf(name, "MB_VR_CPU0_VDDQ_GRPB_TEMP");
      break;
    case MB_SENSOR_VR_CPU0_VDDQ_GRPB_CURR:
      sprintf(name, "MB_VR_CPU0_VDDQ_GRPB_CURR");
      break;
    case MB_SENSOR_VR_CPU0_VDDQ_GRPB_VOLT:
      sprintf(name, "MB_VR_CPU0_VDDQ_GRPB_VOLT");
      break;
    case MB_SENSOR_VR_CPU0_VDDQ_GRPB_POWER:
      sprintf(name, "MB_VR_CPU0_VDDQ_GRPB_POWER");
      break;
    case MB_SENSOR_VR_CPU1_VCCIN_TEMP:
      sprintf(name, "MB_VR_CPU1_VCCIN_TEMP");
      break;
    case MB_SENSOR_VR_CPU1_VCCIN_CURR:
      sprintf(name, "MB_VR_CPU1_VCCIN_CURR");
      break;
    case MB_SENSOR_VR_CPU1_VCCIN_VOLT:
      sprintf(name, "MB_VR_CPU1_VCCIN_VOLT");
      break;
    case MB_SENSOR_VR_CPU1_VCCIN_POWER:
      sprintf(name, "MB_VR_CPU1_VCCIN_POWER");
      break;
    case MB_SENSOR_VR_CPU1_VSA_TEMP:
      sprintf(name, "MB_VR_CPU1_VSA_TEMP");
      break;
    case MB_SENSOR_VR_CPU1_VSA_CURR:
      sprintf(name, "MB_VR_CPU1_VSA_CURR");
      break;
    case MB_SENSOR_VR_CPU1_VSA_VOLT:
      sprintf(name, "MB_VR_CPU1_VSA_VOLT");
      break;
    case MB_SENSOR_VR_CPU1_VSA_POWER:
      sprintf(name, "MB_VR_CPU1_VSA_POWER");
      break;
    case MB_SENSOR_VR_CPU1_VCCIO_TEMP:
      sprintf(name, "MB_VR_CPU1_VCCIO_TEMP");
      break;
    case MB_SENSOR_VR_CPU1_VCCIO_CURR:
      sprintf(name, "MB_VR_CPU1_VCCIO_CURR");
      break;
    case MB_SENSOR_VR_CPU1_VCCIO_VOLT:
      sprintf(name, "MB_VR_CPU1_VCCIO_VOLT");
      break;
    case MB_SENSOR_VR_CPU1_VCCIO_POWER:
      sprintf(name, "MB_VR_CPU1_VCCIO_POWER");
      break;
    case MB_SENSOR_VR_CPU1_VDDQ_GRPC_TEMP:
      sprintf(name, "MB_VR_CPU1_VDDQ_GRPC_TEMP");
      break;
    case MB_SENSOR_VR_CPU1_VDDQ_GRPC_CURR:
      sprintf(name, "MB_VR_CPU1_VDDQ_GRPC_CURR");
      break;
    case MB_SENSOR_VR_CPU1_VDDQ_GRPC_VOLT:
      sprintf(name, "MB_VR_CPU1_VDDQ_GRPC_VOLT");
      break;
    case MB_SENSOR_VR_CPU1_VDDQ_GRPC_POWER:
      sprintf(name, "MB_VR_CPU1_VDDQ_GRPC_POWER");
      break;
    case MB_SENSOR_VR_CPU1_VDDQ_GRPD_TEMP:
      sprintf(name, "MB_VR_CPU1_VDDQ_GRPD_TEMP");
      break;
    case MB_SENSOR_VR_CPU1_VDDQ_GRPD_CURR:
      sprintf(name, "MB_VR_CPU1_VDDQ_GRPD_CURR");
      break;
    case MB_SENSOR_VR_CPU1_VDDQ_GRPD_VOLT:
      sprintf(name, "MB_VR_CPU1_VDDQ_GRPD_VOLT");
      break;
    case MB_SENSOR_VR_CPU1_VDDQ_GRPD_POWER:
      sprintf(name, "MB_VR_CPU1_VDDQ_GRPD_POWER");
      break;
    case MB_SENSOR_VR_PCH_PVNN_TEMP:
      sprintf(name, "MB_VR_PCH_PVNN_TEMP");
      break;
    case MB_SENSOR_VR_PCH_PVNN_CURR:
      sprintf(name, "MB_VR_PCH_PVNN_CURR");
      break;
    case MB_SENSOR_VR_PCH_PVNN_VOLT:
      sprintf(name, "MB_VR_PCH_PVNN_VOLT");
      break;
    case MB_SENSOR_VR_PCH_PVNN_POWER:
      sprintf(name, "MB_VR_PCH_PVNN_POWER");
      break;
    case MB_SENSOR_VR_PCH_P1V05_TEMP:
      sprintf(name, "MB_VR_PCH_P1V05_TEMP");
      break;
    case MB_SENSOR_VR_PCH_P1V05_CURR:
      sprintf(name, "MB_VR_PCH_P1V05_CURR");
      break;
    case MB_SENSOR_VR_PCH_P1V05_VOLT:
      sprintf(name, "MB_VR_PCH_P1V05_VOLT");
      break;
    case MB_SENSOR_VR_PCH_P1V05_POWER:
      sprintf(name, "MB_VR_PCH_P1V05_POWER");
      break;
    case MB_SENSOR_C2_AVA_FTEMP:
      sprintf(name, "MB_C2_AVA_FTEMP");
      break;
    case MB_SENSOR_C2_AVA_RTEMP:
      sprintf(name, "MB_C2_AVA_RTEMP");
      break;
    case MB_SENSOR_C2_1_NVME_CTEMP:
      sprintf(name, "MB_C2_1_NVME_CTEMP");
      break;
    case MB_SENSOR_C2_2_NVME_CTEMP:
      sprintf(name, "MB_C2_2_NVME_CTEMP");
      break;
    case MB_SENSOR_C2_3_NVME_CTEMP:
      sprintf(name, "MB_C2_3_NVME_CTEMP");
      break;
    case MB_SENSOR_C2_4_NVME_CTEMP:
      sprintf(name, "MB_C2_4_NVME_CTEMP");
      break;
    case MB_SENSOR_C3_AVA_FTEMP:
      sprintf(name, "MB_C3_AVA_FTEMP");
      break;
    case MB_SENSOR_C3_AVA_RTEMP:
      sprintf(name, "MB_C3_AVA_RTEMP");
      break;
    case MB_SENSOR_C3_1_NVME_CTEMP:
      sprintf(name, "MB_C3_1_NVME_CTEMP");
      break;
    case MB_SENSOR_C3_2_NVME_CTEMP:
      sprintf(name, "MB_C3_2_NVME_CTEMP");
      break;
    case MB_SENSOR_C3_3_NVME_CTEMP:
      sprintf(name, "MB_C3_3_NVME_CTEMP");
      break;
    case MB_SENSOR_C3_4_NVME_CTEMP:
      sprintf(name, "MB_C3_4_NVME_CTEMP");
      break;
    case MB_SENSOR_C4_AVA_FTEMP:
      sprintf(name, "MB_C4_AVA_FTEMP");
      break;
    case MB_SENSOR_C4_AVA_RTEMP:
      sprintf(name, "MB_C4_AVA_RTEMP");
      break;
    case MB_SENSOR_C4_1_NVME_CTEMP:
      sprintf(name, "MB_C4_1_NVME_CTEMP");
      break;
    case MB_SENSOR_C4_2_NVME_CTEMP:
      sprintf(name, "MB_C4_2_NVME_CTEMP");
      break;
    case MB_SENSOR_C4_3_NVME_CTEMP:
      sprintf(name, "MB_C4_3_NVME_CTEMP");
      break;
    case MB_SENSOR_C4_4_NVME_CTEMP:
      sprintf(name, "MB_C4_4_NVME_CTEMP");
      break;

    default:
      return -1;
    }
    break;
  case FRU_NIC:
    switch(sensor_num) {
    case MEZZ_SENSOR_TEMP:
      sprintf(name, "MEZZ_SENSOR_TEMP");
      break;
    default:
      return -1;
    }
    break;
  default:
    return -1;
  }
  return 0;
}

int
pal_get_sensor_units(uint8_t fru, uint8_t sensor_num, char *units) {
  switch(fru) {
  case FRU_MB:
    switch(sensor_num) {
    case MB_SENSOR_INLET_TEMP:
    case MB_SENSOR_OUTLET_TEMP:
    case MB_SENSOR_INLET_REMOTE_TEMP:
    case MB_SENSOR_OUTLET_REMOTE_TEMP:
    case MB_SENSOR_CPU0_TEMP:
    case MB_SENSOR_CPU0_TJMAX:
    case MB_SENSOR_CPU1_TEMP:
    case MB_SENSOR_CPU1_TJMAX:
    case MB_SENSOR_PCH_TEMP:
    case MB_SENSOR_CPU0_DIMM_GRPA_TEMP:
    case MB_SENSOR_CPU0_DIMM_GRPB_TEMP:
    case MB_SENSOR_CPU1_DIMM_GRPC_TEMP:
    case MB_SENSOR_CPU1_DIMM_GRPD_TEMP:
    case MB_SENSOR_VR_CPU0_VCCIN_TEMP:
    case MB_SENSOR_VR_CPU0_VSA_TEMP:
    case MB_SENSOR_VR_CPU0_VCCIO_TEMP:
    case MB_SENSOR_VR_CPU0_VDDQ_GRPA_TEMP:
    case MB_SENSOR_VR_CPU0_VDDQ_GRPB_TEMP:
    case MB_SENSOR_VR_CPU1_VCCIN_TEMP:
    case MB_SENSOR_VR_CPU1_VSA_TEMP:
    case MB_SENSOR_VR_CPU1_VCCIO_TEMP:
    case MB_SENSOR_VR_CPU1_VDDQ_GRPC_TEMP:
    case MB_SENSOR_VR_CPU1_VDDQ_GRPD_TEMP:
    case MB_SENSOR_VR_PCH_PVNN_TEMP:
    case MB_SENSOR_VR_PCH_P1V05_TEMP:
    case MB_SENSOR_C2_AVA_FTEMP:
    case MB_SENSOR_C2_AVA_RTEMP:
    case MB_SENSOR_C2_1_NVME_CTEMP:
    case MB_SENSOR_C2_2_NVME_CTEMP:
    case MB_SENSOR_C2_3_NVME_CTEMP:
    case MB_SENSOR_C2_4_NVME_CTEMP:
    case MB_SENSOR_C3_AVA_FTEMP:
    case MB_SENSOR_C3_AVA_RTEMP:
    case MB_SENSOR_C3_1_NVME_CTEMP:
    case MB_SENSOR_C3_2_NVME_CTEMP:
    case MB_SENSOR_C3_3_NVME_CTEMP:
    case MB_SENSOR_C3_4_NVME_CTEMP:
    case MB_SENSOR_C4_AVA_FTEMP:
    case MB_SENSOR_C4_AVA_RTEMP:
    case MB_SENSOR_C4_1_NVME_CTEMP:
    case MB_SENSOR_C4_2_NVME_CTEMP:
    case MB_SENSOR_C4_3_NVME_CTEMP:
    case MB_SENSOR_C4_4_NVME_CTEMP:
      sprintf(units, "C");
      break;
    case MB_SENSOR_FAN0_TACH:
    case MB_SENSOR_FAN1_TACH:
      sprintf(units, "RPM");
      break;
    case MB_SENSOR_P3V3:
    case MB_SENSOR_P5V:
    case MB_SENSOR_P12V:
    case MB_SENSOR_P1V05:
    case MB_SENSOR_PVNN_PCH_STBY:
    case MB_SENSOR_P3V3_STBY:
    case MB_SENSOR_P5V_STBY:
    case MB_SENSOR_P3V_BAT:
    case MB_SENSOR_HSC_IN_VOLT:
    case MB_SENSOR_VR_CPU0_VCCIN_VOLT:
    case MB_SENSOR_VR_CPU0_VSA_VOLT:
    case MB_SENSOR_VR_CPU0_VCCIO_VOLT:
    case MB_SENSOR_VR_CPU0_VDDQ_GRPA_VOLT:
    case MB_SENSOR_VR_CPU0_VDDQ_GRPB_VOLT:
    case MB_SENSOR_VR_CPU1_VCCIN_VOLT:
    case MB_SENSOR_VR_CPU1_VSA_VOLT:
    case MB_SENSOR_VR_CPU1_VCCIO_VOLT:
    case MB_SENSOR_VR_CPU1_VDDQ_GRPC_VOLT:
    case MB_SENSOR_VR_CPU1_VDDQ_GRPD_VOLT:
    case MB_SENSOR_VR_PCH_PVNN_VOLT:
    case MB_SENSOR_VR_PCH_P1V05_VOLT:
      sprintf(units, "Volts");
      break;
    case MB_SENSOR_HSC_OUT_CURR:
    case MB_SENSOR_VR_CPU0_VCCIN_CURR:
    case MB_SENSOR_VR_CPU0_VSA_CURR:
    case MB_SENSOR_VR_CPU0_VCCIO_CURR:
    case MB_SENSOR_VR_CPU0_VDDQ_GRPA_CURR:
    case MB_SENSOR_VR_CPU0_VDDQ_GRPB_CURR:
    case MB_SENSOR_VR_CPU1_VCCIN_CURR:
    case MB_SENSOR_VR_CPU1_VSA_CURR:
    case MB_SENSOR_VR_CPU1_VCCIO_CURR:
    case MB_SENSOR_VR_CPU1_VDDQ_GRPC_CURR:
    case MB_SENSOR_VR_CPU1_VDDQ_GRPD_CURR:
    case MB_SENSOR_VR_PCH_PVNN_CURR:
    case MB_SENSOR_VR_PCH_P1V05_CURR:
      sprintf(units, "Amps");
      break;
    case MB_SENSOR_HSC_IN_POWER:
    case MB_SENSOR_VR_CPU0_VCCIN_POWER:
    case MB_SENSOR_VR_CPU0_VSA_POWER:
    case MB_SENSOR_VR_CPU0_VCCIO_POWER:
    case MB_SENSOR_VR_CPU0_VDDQ_GRPA_POWER:
    case MB_SENSOR_VR_CPU0_VDDQ_GRPB_POWER:
    case MB_SENSOR_VR_CPU1_VCCIN_POWER:
    case MB_SENSOR_VR_CPU1_VSA_POWER:
    case MB_SENSOR_VR_CPU1_VCCIO_POWER:
    case MB_SENSOR_VR_CPU1_VDDQ_GRPC_POWER:
    case MB_SENSOR_VR_CPU1_VDDQ_GRPD_POWER:
    case MB_SENSOR_VR_PCH_PVNN_POWER:
    case MB_SENSOR_VR_PCH_P1V05_POWER:
      sprintf(units, "Watts");
      break;
    default:
      return -1;
    }
    break;
  case FRU_NIC:
    switch(sensor_num) {
    case MEZZ_SENSOR_TEMP:
      sprintf(units, "C");
      break;
    default:
      return -1;
    }
    break;
  default:
    return -1;
  }
  return 0;
}

int
pal_get_fruid_path(uint8_t fru, char *path) {
  char fname[16] = {0};

  switch(fru) {
  case FRU_MB:
    sprintf(fname, "mb");
    break;
  case FRU_NIC:
    sprintf(fname, "nic");
    break;
  default:
    return -1;
  }

  sprintf(path, "/tmp/fruid_%s.bin", fname);

  return 0;
}

int
pal_get_fruid_eeprom_path(uint8_t fru, char *path) {
  switch(fru) {
  case FRU_MB:
    sprintf(path, "/sys/devices/platform/ast-i2c.6/i2c-6/6-0054/eeprom");
    break;
  default:
    return -1;
  }

  return 0;
}

int
pal_get_fruid_name(uint8_t fru, char *name) {
  switch(fru) {
  case FRU_MB:
    sprintf(name, "Mother Board");
    break;
  case FRU_NIC:
    sprintf(name, "NIC Mezzanine");
    break;
  default:
    return -1;
  }
  return 0;
}

int
pal_set_def_key_value() {

  int ret;
  int i;
  int fru;
  char key[MAX_KEY_LEN] = {0};
  char kpath[MAX_KEY_PATH_LEN] = {0};

  i = 0;
  while(strcmp(key_list[i], LAST_KEY)) {

    memset(key, 0, MAX_KEY_LEN);
    memset(kpath, 0, MAX_KEY_PATH_LEN);

    sprintf(kpath, KV_STORE, key_list[i]);

    if (access(kpath, F_OK) == -1) {

      if ((ret = kv_set(key_list[i], def_val_list[i])) < 0) {
#ifdef DEBUG
          syslog(LOG_WARNING, "pal_set_def_key_value: kv_set failed. %d", ret);
#endif
      }
    }

    i++;
  }

  /* Actions to be taken on Power On Reset */
  if (pal_is_bmc_por()) {
    /* Clear all the SEL errors */
    memset(key, 0, MAX_KEY_LEN);

    /* Write the value "1" which means FRU_STATUS_GOOD */
    ret = pal_set_key_value(key, "1");

    /* Clear all the sensor health files*/
    memset(key, 0, MAX_KEY_LEN);

    /* Write the value "1" which means FRU_STATUS_GOOD */
    ret = pal_set_key_value(key, "1");
  }

  return 0;
}

int
pal_get_fru_devtty(uint8_t fru, char *devtty) {
  sprintf(devtty, "/dev/ttyS1");
  return 0;
}

void
pal_dump_key_value(void) {
  int i;
  int ret;

  char value[MAX_VALUE_LEN] = {0x0};

  while (strcmp(key_list[i], LAST_KEY)) {
    printf("%s:", key_list[i]);
    if (ret = kv_get(key_list[i], value) < 0) {
      printf("\n");
    } else {
      printf("%s\n",  value);
    }
    i++;
    memset(value, 0, MAX_VALUE_LEN);
  }
}

int
pal_set_last_pwr_state(uint8_t fru, char *state) {

  int ret;
  char key[MAX_KEY_LEN] = {0};

  sprintf(key, "%s", "pwr_server_last_state");

  ret = pal_set_key_value(key, state);
  if (ret < 0) {
#ifdef DEBUG
    syslog(LOG_WARNING, "pal_set_last_pwr_state: pal_set_key_value failed for "
        "fru %u", fru);
#endif
  }

  return ret;
}

int
pal_get_last_pwr_state(uint8_t fru, char *state) {
  int ret;
  char key[MAX_KEY_LEN] = {0};

  sprintf(key, "%s", "pwr_server_last_state");

  ret = pal_get_key_value(key, state);
  if (ret < 0) {
#ifdef DEBUG
    syslog(LOG_WARNING, "pal_get_last_pwr_state: pal_get_key_value failed for "
        "fru %u", fru);
#endif
  }

  return ret;
}

// GUID for System and Device
static int
pal_get_guid(uint16_t offset, char *guid) {
  int fd = 0;
  uint64_t tmp[GUID_SIZE];
  ssize_t bytes_rd;

  errno = 0;

  // Check if file is present
  if (access(FRU_EEPROM, F_OK) == -1) {
      syslog(LOG_ERR, "pal_get_guid: unable to access the %s file: %s",
          FRU_EEPROM, strerror(errno));
      return errno;
  }

  // Open the file
  fd = open(FRU_EEPROM, O_RDONLY);
  if (fd == -1) {
    syslog(LOG_ERR, "pal_get_guid: unable to open the %s file: %s",
        FRU_EEPROM, strerror(errno));
    return errno;
  }

  // seek to the offset
  lseek(fd, offset, SEEK_SET);

  // Read bytes from location
  bytes_rd = read(fd, guid, GUID_SIZE);
  if (bytes_rd != GUID_SIZE) {
    syslog(LOG_ERR, "pal_get_guid: read to %s file failed: %s",
        FRU_EEPROM, strerror(errno));
    goto err_exit;
  }

err_exit:
  close(fd);
  return errno;
}

static int
pal_set_guid(uint16_t offset, char *guid) {
  int fd = 0;
  uint64_t tmp[GUID_SIZE];
  ssize_t bytes_wr;
  int i = 0;

  errno = 0;

  // Check for file presence
  if (access(FRU_EEPROM, F_OK) == -1) {
      syslog(LOG_ERR, "pal_set_guid: unable to access the %s file: %s",
          FRU_EEPROM, strerror(errno));
      return errno;
  }

  // Open file
  fd = open(FRU_EEPROM, O_WRONLY);
  if (fd == -1) {
    syslog(LOG_ERR, "pal_set_guid: unable to open the %s file: %s",
        FRU_EEPROM, strerror(errno));
    return errno;
  }

  // Seek the offset
  lseek(fd, offset, SEEK_SET);

  // Write GUID data
  bytes_wr = write(fd, guid, GUID_SIZE);
  if (bytes_wr != GUID_SIZE) {
    syslog(LOG_ERR, "pal_set_guid: write to %s file failed: %s",
        FRU_EEPROM, strerror(errno));
    goto err_exit;
  }

err_exit:
  close(fd);
  return errno;
}

// GUID based on RFC4122 format @ https://tools.ietf.org/html/rfc4122
static void
pal_populate_guid(uint8_t *guid, uint8_t *str) {
  unsigned int secs;
  unsigned int usecs;
  struct timeval tv;
  uint8_t count;
  uint8_t lsb, msb;
  int i, r;

  // Populate time
  gettimeofday(&tv, NULL);

  secs = tv.tv_sec;
  usecs = tv.tv_usec;
  guid[0] = usecs & 0xFF;
  guid[1] = (usecs >> 8) & 0xFF;
  guid[2] = (usecs >> 16) & 0xFF;
  guid[3] = (usecs >> 24) & 0xFF;
  guid[4] = secs & 0xFF;
  guid[5] = (secs >> 8) & 0xFF;
  guid[6] = (secs >> 16) & 0xFF;
  guid[7] = (secs >> 24) & 0x0F;

  // Populate version
  guid[7] |= 0x10;

  // Populate clock seq with randmom number
  //getrandom(&guid[8], 2, 0);
  srand(time(NULL));
  //memcpy(&guid[8], rand(), 2);
  r = rand();
  guid[8] = r & 0xFF;
  guid[9] = (r>>8) & 0xFF;

  // Use string to populate 6 bytes unique
  // e.g. LSP62100035 => 'S' 'P' 0x62 0x10 0x00 0x35
  count = 0;
  for (i = strlen(str)-1; i >= 0; i--) {
    if (count == 6) {
      break;
    }

    // If alphabet use the character as is
    if (isalpha(str[i])) {
      guid[15-count] = str[i];
      count++;
      continue;
    }

    // If it is 0-9, use two numbers as BCD
    lsb = str[i] - '0';
    if (i > 0) {
      i--;
      if (isalpha(str[i])) {
        i++;
        msb = 0;
      } else {
        msb = str[i] - '0';
      }
    } else {
      msb = 0;
    }
    guid[15-count] = (msb << 4) | lsb;
    count++;
  }

  // zero the remaining bytes, if any
  if (count != 6) {
    memset(&guid[10], 0, 6-count);
  }

}

int
pal_set_sys_guid(uint8_t fru, char *str) {
  pal_populate_guid(g_sys_guid, str);

  return pal_set_guid(OFFSET_SYS_GUID, g_sys_guid);
}

int
pal_set_dev_guid(uint8_t fru, char *str) {
  pal_populate_guid(g_dev_guid, str);

  return pal_set_guid(OFFSET_DEV_GUID, g_dev_guid);
}

int
pal_get_sys_guid(uint8_t fru, char *guid) {
  pal_get_guid(OFFSET_SYS_GUID, g_sys_guid);
  memcpy(guid, g_sys_guid, GUID_SIZE);

  return 0;
}

int
pal_get_dev_guid(uint8_t fru, char *guid) {

  pal_get_guid(OFFSET_DEV_GUID, g_dev_guid);

  memcpy(guid, g_dev_guid, GUID_SIZE);

  return 0;
}

int
pal_set_sysfw_ver(uint8_t fru, uint8_t *ver) {
  int i;
  char key[MAX_KEY_LEN] = {0};
  char str[MAX_VALUE_LEN] = {0};
  char tstr[10] = {0};

  sprintf(key, "sysfw_ver_server");

  for (i = 0; i < SIZE_SYSFW_VER; i++) {
    sprintf(tstr, "%02x", ver[i]);
    strcat(str, tstr);
  }

  return pal_set_key_value(key, str);
}

int
pal_get_sysfw_ver(uint8_t fru, uint8_t *ver) {
  int i;
  int j = 0;
  int ret;
  int msb, lsb;
  char key[MAX_KEY_LEN] = {0};
  char str[MAX_VALUE_LEN] = {0};
  char tstr[4] = {0};

  sprintf(key, "sysfw_ver_server");

  ret = pal_get_key_value(key, str);
  if (ret) {
    return ret;
  }

  for (i = 0; i < 2*SIZE_SYSFW_VER; i += 2) {
    sprintf(tstr, "%c\n", str[i]);
    msb = strtol(tstr, NULL, 16);

    sprintf(tstr, "%c\n", str[i+1]);
    lsb = strtol(tstr, NULL, 16);
    ver[j++] = (msb << 4) | lsb;
  }

  return 0;
}

int
pal_set_boot_order(uint8_t fru, uint8_t *boot) {
  int i;
  char key[MAX_KEY_LEN] = {0};
  char str[MAX_VALUE_LEN] = {0};
  char tstr[10] = {0};

  sprintf(key, "server_boot_order");

  for (i = 0; i < SIZE_BOOT_ORDER; i++) {
    snprintf(tstr, 3, "%02x", boot[i]);
    strncat(str, tstr, 3);
  }

  return pal_set_key_value(key, str);
}

int
pal_get_boot_order(uint8_t fru, uint8_t *boot) {
  int i;
  int j = 0;
  int ret;
  int msb, lsb;
  char key[MAX_KEY_LEN] = {0};
  char str[MAX_VALUE_LEN] = {0};
  char tstr[4] = {0};

  sprintf(key, "server_boot_order");

  ret = pal_get_key_value(key, str);
  if (ret) {
    return ret;
  }

  for (i = 0; i < 2*SIZE_BOOT_ORDER; i += 2) {
    sprintf(tstr, "%c\n", str[i]);
    msb = strtol(tstr, NULL, 16);

    sprintf(tstr, "%c\n", str[i+1]);
    lsb = strtol(tstr, NULL, 16);
    boot[j++] = (msb << 4) | lsb;
  }

  return 0;
}

static int
pal_fetch_vr_ver(uint8_t vr, uint8_t *ver) {
  int fd;
  char fn[32];
  int ret = -1;
  unsigned int retry = MAX_READ_RETRY;
  uint8_t tcount, rcount;
  uint8_t tbuf[16] = {0};
  uint8_t rbuf[16] = {0};
  char key[MAX_KEY_LEN] = {0}, value[MAX_VALUE_LEN] = {0};

  snprintf(fn, sizeof(fn), "/dev/i2c-%d", VR_BUS_ID);
  while (retry) {
    fd = open(fn, O_RDWR);
    if (fd < 0) {
      syslog(LOG_WARNING, "pal_get_vr_ver: i2c_open failed for bus#%x\n", VR_BUS_ID);
      retry--;
      msleep(100);
    } else {
      break;
    }

    if(retry == 0)
      goto error_exit;
  }

  // Set the page to read FW Version
  tbuf[0] = 0x00;
  tbuf[1] = VR_FW_PAGE;

  tcount = 2;
  rcount = 0;

  retry = MAX_READ_RETRY;
  while(retry) {
  ret = i2c_io(fd, vr, tbuf, tcount, rbuf, rcount);
  if (ret) {
#ifdef DEBUG
    syslog(LOG_WARNING, "pal_get_vr_ver: i2c_io failed for bus#%x, dev#%x\n", VR_BUS_ID, vr);
#endif
      retry--;
      msleep(100);
    } else {
      break;
    }

    if(retry == 0)
      goto error_exit;
  }

  // Read 2 bytes from first register
  tbuf[0] = VR_FW_REG1;

  tcount = 1;
  rcount = 2;

  retry = MAX_READ_RETRY;
  while(retry) {
  ret = i2c_io(fd, vr, tbuf, tcount, rbuf, rcount);
  if (ret) {
#ifdef DEBUG
    syslog(LOG_WARNING, "pal_get_vr_ver: i2c_io failed for bus#%x, dev#%x\n", VR_BUS_ID, vr);
#endif
      retry--;
      msleep(100);
    } else {
      break;
    }

    if(retry == 0)
      goto error_exit;
  }

  ver[0] = rbuf[1];
  ver[1] = rbuf[0];

  tbuf[0] = VR_FW_REG2;

  tcount = 1;
  rcount = 2;

  retry = MAX_READ_RETRY;
  while(retry) {
  ret = i2c_io(fd, vr, tbuf, tcount, rbuf, rcount);
  if (ret) {
#ifdef DEBUG
    syslog(LOG_WARNING, "pal_get_vr_ver: i2c_io failed for bus#%x, dev#%x\n", VR_BUS_ID, vr);
#endif
      retry--;
      msleep(100);
    } else {
      break;
    }

    if(retry == 0)
      goto error_exit;
  }

  ver[2] = rbuf[1];
  ver[3] = rbuf[0];

  sprintf(key, "vr_%02Xh_ver", vr);
  sprintf(value, "%08X", *(unsigned int*)ver);
  edb_cache_set(key, value);

error_exit:
  if (fd > 0) {
    close(fd);
  }

  return ret;
}

static int
pal_fetch_vr_checksum(uint8_t vr, uint8_t *checksum) {
  int fd;
  char fn[32];
  int ret = -1;
  unsigned int retry = MAX_READ_RETRY;
  uint8_t tcount, rcount;
  uint8_t tbuf[16] = {0};
  uint8_t rbuf[16] = {0};
  char key[MAX_KEY_LEN] = {0}, value[MAX_VALUE_LEN] = {0};

  snprintf(fn, sizeof(fn), "/dev/i2c-%d", VR_BUS_ID);
  while (retry) {
    fd = open(fn, O_RDWR);
    if (fd < 0) {
      syslog(LOG_WARNING, "pal_get_vr_checksum: i2c_open failed for bus#%x\n", VR_BUS_ID);
      retry--;
      msleep(100);
    } else {
      break;
    }

    if(retry == 0)
      goto error_exit;
  }

  // Set the page to read FW checksum
  tbuf[0] = 0x00;
  tbuf[1] = VR_FW_PAGE_2;

  tcount = 2;
  rcount = 0;

  retry = MAX_READ_RETRY;
  while(retry) {
  ret = i2c_io(fd, vr, tbuf, tcount, rbuf, rcount);
  if (ret) {
#ifdef DEBUG
    syslog(LOG_WARNING, "pal_get_vr_checksum: i2c_io failed for bus#%x, dev#%x\n", VR_BUS_ID, vr);
#endif
      retry--;
      msleep(100);
    } else {
      break;
    }

    if(retry == 0)
      goto error_exit;
  }

  // Read 2 bytes from first register
  tbuf[0] = VR_FW_REG4;

  tcount = 1;
  rcount = 2;

  retry = MAX_READ_RETRY;
  while(retry) {
  ret = i2c_io(fd, vr, tbuf, tcount, rbuf, rcount);
  if (ret) {
#ifdef DEBUG
    syslog(LOG_WARNING, "pal_get_vr_checksum: i2c_io failed for bus#%x, dev#%x\n", VR_BUS_ID, vr);
#endif
      retry--;
      msleep(100);
    } else {
      break;
    }

    if(retry == 0)
      goto error_exit;
  }

  checksum[0] = rbuf[1];
  checksum[1] = rbuf[0];

  tbuf[0] = VR_FW_REG3;

  tcount = 1;
  rcount = 2;

  retry = MAX_READ_RETRY;
  while(retry) {
  ret = i2c_io(fd, vr, tbuf, tcount, rbuf, rcount);
  if (ret) {
#ifdef DEBUG
    syslog(LOG_WARNING, "pal_get_vr_checksum: i2c_io failed for bus#%x, dev#%x\n", VR_BUS_ID, vr);
#endif
      retry--;
      msleep(100);
    } else {
      break;
    }

    if(retry == 0)
      goto error_exit;
  }

  checksum[2] = rbuf[1];
  checksum[3] = rbuf[0];

  sprintf(key, "vr_%02Xh_checksum", vr);
  sprintf(value, "%08X", *(unsigned int*)checksum);
  edb_cache_set(key, value);

error_exit:
  if (fd > 0) {
    close(fd);
  }

  return ret;
}

static int
pal_fetch_vr_deviceId(uint8_t vr, uint8_t *deviceId) {
  int fd;
  char fn[32];
  int ret = -1;
  unsigned int retry = MAX_READ_RETRY;
  uint8_t tcount, rcount;
  uint8_t tbuf[16] = {0};
  uint8_t rbuf[16] = {0};
  char key[MAX_KEY_LEN] = {0}, value[MAX_VALUE_LEN] = {0};

  snprintf(fn, sizeof(fn), "/dev/i2c-%d", VR_BUS_ID);
  while (retry) {
    fd = open(fn, O_RDWR);
    if (fd < 0) {
      syslog(LOG_WARNING, "pal_get_vr_deviceId: i2c_open failed for bus#%x\n", VR_BUS_ID);
      retry--;
      msleep(100);
    } else {
      break;
    }

    if(retry == 0)
      goto error_exit;
  }

  // Set the page to read FW deviceId
  tbuf[0] = 0x00;
  tbuf[1] = VR_FW_PAGE_3;

  tcount = 2;
  rcount = 0;

  retry = MAX_READ_RETRY;
  while(retry) {
  ret = i2c_io(fd, vr, tbuf, tcount, rbuf, rcount);
  if (ret) {
#ifdef DEBUG
    syslog(LOG_WARNING, "pal_get_vr_deviceId: i2c_io failed for bus#%x, dev#%x\n", VR_BUS_ID, vr);
#endif
      retry--;
      msleep(100);
    } else {
      break;
    }

    if(retry == 0)
      goto error_exit;
  }

  // Read 2 bytes from first register
  tbuf[0] = VR_FW_REG5;

  tcount = 1;
  rcount = 2;

  retry = MAX_READ_RETRY;
  while(retry) {
  ret = i2c_io(fd, vr, tbuf, tcount, rbuf, rcount);
  if (ret) {
#ifdef DEBUG
    syslog(LOG_WARNING, "pal_get_vr_deviceId: i2c_io failed for bus#%x, dev#%x\n", VR_BUS_ID, vr);
#endif
      retry--;
      msleep(100);
    } else {
      break;
    }

    if(retry == 0)
      goto error_exit;
  }

  deviceId[0] = rbuf[1];
  deviceId[1] = rbuf[0];

  sprintf(key, "vr_%02Xh_deviceId", vr);
  sprintf(value, "%04X", *(unsigned short*)deviceId);
  edb_cache_set(key, value);

error_exit:
  if (fd > 0) {
    close(fd);
  }

  return ret;
}

int
pal_get_vr_ver(uint8_t vr, uint8_t *ver) {
  char key[MAX_KEY_LEN] = {0}, value[MAX_VALUE_LEN] = {0};
  sprintf(key, "vr_%02Xh_ver", vr);
  if (edb_cache_get(key, value) < 0)
    return pal_fetch_vr_ver(vr, ver);
  *(unsigned int*)ver = (unsigned int)strtoul(value, NULL, 16);
  return 0;
}

int
pal_get_vr_checksum(uint8_t vr, uint8_t *checksum) {
  char key[MAX_KEY_LEN] = {0}, value[MAX_VALUE_LEN] = {0};
  sprintf(key, "vr_%02Xh_checksum", vr);
  if (edb_cache_get(key, value) < 0)
    return pal_fetch_vr_checksum(vr, checksum);
  *(unsigned int*)checksum = (unsigned int)strtoul(value, NULL, 16);
  return 0;
}

int
pal_get_vr_deviceId(uint8_t vr, uint8_t *deviceId) {
  char key[MAX_KEY_LEN] = {0}, value[MAX_VALUE_LEN] = {0};
  sprintf(key, "vr_%02Xh_deviceId", vr);
  if (edb_cache_get(key, value) < 0)
    return pal_fetch_vr_deviceId(vr, deviceId);
  *(unsigned short*)deviceId = (unsigned short)strtoul(value, NULL, 16);
  return 0;
}

int
pal_is_bmc_por(void) {
  uint32_t scu_fd;
  uint32_t wdt;
  void *scu_reg;
  void *scu_wdt;

  scu_fd = open("/dev/mem", O_RDWR | O_SYNC );
  if (scu_fd < 0) {
    return 0;
  }

  scu_reg = mmap(NULL, PAGE_SIZE, PROT_READ|PROT_WRITE, MAP_SHARED, scu_fd,
             AST_WDT_BASE);
  scu_wdt = (char*)scu_reg + WDT_OFFSET;

  wdt = *(volatile uint32_t*) scu_wdt;

  munmap(scu_reg, PAGE_SIZE);
  close(scu_fd);

  if (wdt & 0xff00) {
    return 0;
  } else {
    return 1;
  }
}

int
pal_get_fru_discrete_list(uint8_t fru, uint8_t **sensor_list, int *cnt) {

    return 0;
}

static void
_print_sensor_discrete_log(uint8_t fru, uint8_t snr_num, char *snr_name,
    uint8_t val, char *event) {
  if (val) {
    syslog(LOG_CRIT, "ASSERT: %s discrete - raised - FRU: %d, num: 0x%X,"
        " snr: %-16s val: %d", event, fru, snr_num, snr_name, val);
  } else {
    syslog(LOG_CRIT, "DEASSERT: %s discrete - settled - FRU: %d, num: 0x%X,"
        " snr: %-16s val: %d", event, fru, snr_num, snr_name, val);
  }
  pal_update_ts_sled();
}

int
pal_sensor_discrete_check(uint8_t fru, uint8_t snr_num, char *snr_name,
    uint8_t o_val, uint8_t n_val) {

  char name[32];
  bool valid = false;
  uint8_t diff = o_val ^ n_val;
  return 0;
}

static int
pal_store_crashdump(uint8_t fru) {

  return 0;
}

int
pal_sel_handler(uint8_t fru, uint8_t snr_num) {

  char key[MAX_KEY_LEN] = {0};
  char cvalue[MAX_VALUE_LEN] = {0};

  /* Write the value "0" which means FRU_STATUS_BAD */
  return pal_set_key_value(key, "0");
}

int
pal_get_event_sensor_name(uint8_t fru, uint8_t snr_num, char *name) {

  switch(snr_num) {
    case SYSTEM_EVENT:
      sprintf(name, "SYSTEM_EVENT");
      break;
    case THERM_THRESH_EVT:
      sprintf(name, "THERM_THRESH_EVT");
      break;
    case CRITICAL_IRQ:
      sprintf(name, "CRITICAL_IRQ");
      break;
    case POST_ERROR:
      sprintf(name, "POST_ERROR");
      break;
    case MACHINE_CHK_ERR:
      sprintf(name, "MACHINE_CHK_ERR");
      break;
    case PCIE_ERR:
      sprintf(name, "PCIE_ERR");
      break;
    case IIO_ERR:
      sprintf(name, "IIO_ERR");
      break;
    case MEMORY_ECC_ERR:
      sprintf(name, "MEMORY_ECC_ERR");
      break;
    case MEMORY_ERR_LOG_DIS:
      sprintf(name, "MEMORY_ERR_LOG_DIS");
      break;
    case PWR_ERR:
      sprintf(name, "PWR_ERR");
      break;
    case CATERR:
      sprintf(name, "CATERR");
      break;
    case CPU_DIMM_HOT:
      sprintf(name, "CPU_DIMM_HOT");
      break;
    case SOFTWARE_NMI:
      sprintf(name, "SOFTWARE_NMI");
      break;
    case CPU0_THERM_STATUS:
      sprintf(name, "CPU0_THERM_STATUS");
      break;
    case CPU1_THERM_STATUS:
      sprintf(name, "CPU1_THERM_STATUS");
      break;
    case SPS_FW_HEALTH:
      sprintf(name, "SPS_FW_HEALTH");
      break;
    case NM_EXCEPTION:
      sprintf(name, "NM_EXCEPTION");
      break;
    case PWR_THRESH_EVT:
      sprintf(name, "PWR_THRESH_EVT");
      break;
    default:
      sprintf(name, "Unknown");
      break;
  }

  return 0;
}

int
pal_parse_sel(uint8_t fru, uint8_t snr_num, uint8_t *event_data,
    char *error_log) {

  char *ed = &event_data[3];
  char temp_log[128] = {0};
  uint8_t temp;

  switch(snr_num) {
    case SYSTEM_EVENT:
      sprintf(error_log, "");
      if (ed[0] == 0xE5) {
        strcat(error_log, "Cause of Time change - ");

        if (ed[2] == 0x00)
          strcat(error_log, "NTP");
        else if (ed[2] == 0x01)
          strcat(error_log, "Host RTL");
        else if (ed[2] == 0x02)
          strcat(error_log, "Set SEL time cmd ");
        else if (ed[2] == 0x03)
          strcat(error_log, "Set SEL time UTC offset cmd");
        else
          strcat(error_log, "Unknown");

        if (ed[1] == 0x00)
          strcat(error_log, " - First Time");
        else if(ed[1] == 0x80)
          strcat(error_log, " - Second Time");

      }
      break;

    case THERM_THRESH_EVT:
      sprintf(error_log, "");
      if (ed[0] == 0x1)
        strcat(error_log, "Limit Exceeded");
      else
        strcat(error_log, "Unknown");
      break;

    case CRITICAL_IRQ:
      sprintf(error_log, "");
      if (ed[0] == 0x0)
        strcat(error_log, "NMI / Diagnostic Interrupt");
      else if (ed[0] == 0x03)
        strcat(error_log, "Software NMI");
      else
        strcat(error_log, "Unknown");
      break;

    case POST_ERROR:
      sprintf(error_log, "");
      if ((ed[0] & 0x0F) == 0x0)
        strcat(error_log, "System Firmware Error");
      else
        strcat(error_log, "Unknown");
      if (((ed[0] >> 6) & 0x03) == 0x3) {
        // TODO: Need to implement IPMI spec based Post Code
        strcat(error_log, ", IPMI Post Code");
       } else if (((ed[0] >> 6) & 0x03) == 0x2) {
         sprintf(temp_log, ", OEM Post Code 0x%X 0x%X", ed[2], ed[1]);
         strcat(error_log, temp_log);
       }
      break;

    case MACHINE_CHK_ERR:
      sprintf(error_log, "");
      if ((ed[0] & 0x0F) == 0x0B) {
        strcat(error_log, "Uncorrectable");
      } else if ((ed[0] & 0x0F) == 0x0C) {
        strcat(error_log, "Correctable");
      } else {
        strcat(error_log, "Unknown");
      }

      sprintf(temp_log, ", Machine Check bank Number %d ", ed[1]);
      strcat(error_log, temp_log);
      sprintf(temp_log, ", CPU %d, Core %d ", ed[2] >> 5, ed[2] & 0x1F);
      strcat(error_log, temp_log);

      break;

    case PCIE_ERR:
      sprintf(error_log, "");
      if ((ed[0] & 0xF) == 0x4)
        strcat(error_log, "PCI PERR");
      else if ((ed[0] & 0xF) == 0x5)
        strcat(error_log, "PCI SERR");
      else if ((ed[0] & 0xF) == 0x7)
        strcat(error_log, "Correctable");
      else if ((ed[0] & 0xF) == 0x8)
        strcat(error_log, "Uncorrectable");
      else if ((ed[0] & 0xF) == 0xA)
        strcat(error_log, "Bus Fatal");
      else
        strcat(error_log, "Unknown");

      sprintf(temp_log, " (Bus %02X / Dev %02X / Fun %02X)", ed[2], ed[1] >> 3, ed[1] & 0x7);
      strcat(error_log, temp_log);
      break;

    case IIO_ERR:
      sprintf(error_log, "");
      if ((ed[0] & 0xF) == 0) {

        sprintf(temp_log, "CPU %d, Error ID 0x%X", (ed[2] & 0xE0) >> 5,
            ed[1]);
        strcat(error_log, temp_log);

        temp = ed[2] & 0x7;
        if (temp == 0x0)
          strcat(error_log, " - IRP0");
        else if (temp == 0x1)
          strcat(error_log, " - IRP1");
        else if (temp == 0x2)
          strcat(error_log, " - IIO-Core");
        else if (temp == 0x3)
          strcat(error_log, " - VT-d");
        else if (temp == 0x4)
          strcat(error_log, " - Intel Quick Data");
        else if (temp == 0x5)
          strcat(error_log, " - Misc");
        else
          strcat(error_log, " - Reserved");
      } else
        strcat(error_log, "Unknown");
      break;

    case MEMORY_ECC_ERR:
    case MEMORY_ERR_LOG_DIS:
      sprintf(error_log, "");
      if (snr_num == MEMORY_ERR_LOG_DIS) {
        if ((ed[0] & 0x0F) == 0x0)
          strcat(error_log, "Correctable Memory Error Logging Disabled");
        else
          strcat(error_log, "Unknown");
      } else {
        if ((ed[0] & 0x0F) == 0x0)
          strcat(error_log, "Correctable");
        else if ((ed[0] & 0x0F) == 0x1)
          strcat(error_log, "Uncorrectable");
        else if ((ed[0] & 0x0F) == 0x5)
          strcat(error_log, "Correctable ECC error Logging Limit Reached");
        else
          strcat(error_log, "Unknown");
      }

      sprintf(temp_log, " (DIMM %02X)", ed[2]);
      strcat(error_log, temp_log);

      sprintf(temp_log, " Logical Rank %d", ed[1] & 0x03);
      strcat(error_log, temp_log);

      switch(ed[1] & 0x0C) {
        case 0x00:
          //Ignore when " All info available"
          break;
        case 0x01:
          strcat(error_log, " DIMM info not valid");
          break;
        case 0x02:
          strcat(error_log, " CHN info not valid");
          break;
        case 0x03:
          strcat(error_log, " CPU info not valid");
          break;
        default:
          strcat(error_log, " Unknown");
      }

      break;

    case PWR_ERR:
      sprintf(error_log, "");
      if (ed[0] == 0x2)
        strcat(error_log, "PCH_PWROK failure");
      else
        strcat(error_log, "Unknown");
      break;

    case CATERR:
      sprintf(error_log, "");
      if (ed[0] == 0x0)
        strcat(error_log, "IERR/CATERR");
      else if (ed[0] == 0xB)
        strcat(error_log, "MCERR/CATERR");
      else
        strcat(error_log, "Unknown");
      break;

    case MSMI:
      sprintf(error_log, "");
      if (ed[0] == 0x0)
        strcat(error_log, "IERR/MSMI");
      else if (ed[0] == 0xB)
        strcat(error_log, "MCERR/MSMI");
      else
        strcat(error_log, "Unknown");
      break;

    case CPU_DIMM_HOT:
      sprintf(error_log, "");
      if ((ed[0] << 16 | ed[1] << 8 | ed[2]) == 0x01FFFF)
        strcat(error_log, "SOC MEMHOT");
      else
        strcat(error_log, "Unknown");
      break;

    case SOFTWARE_NMI:
      sprintf(error_log, "");
      if ((ed[0] << 16 | ed[1] << 8 | ed[2]) == 0x03FFFF)
        strcat(error_log, "Software NMI");
      else
        strcat(error_log, "Unknown");
      break;

    case SPS_FW_HEALTH:
      sprintf(error_log, "");
      if (event_data[0] == 0xDC && ed[1] == 0x06) {
        strcat(error_log, "FW UPDATE");
        return 1;
      } else
         strcat(error_log, "Unknown");
      break;

    case NM_EXCEPTION:
      sprintf(error_log, "");
      if ((ed[0] << 16 | ed[1] << 8 | ed[2]) == 0xA80001) {
        strcat(error_log, "Policy Correction Time Exceeded");
        return 1;
      } else
         strcat(error_log, "Unknown");
      break;

    case CPU0_THERM_STATUS:
    case CPU1_THERM_STATUS:
      sprintf(error_log, "");
      if (ed[0] == 0x00)
        strcat(error_log, "CPU Critical Temperature");
      else if (ed[0] == 0x01)
        strcat(error_log, "PROCHOT# Assertions");
      else if (ed[0] == 0x02)
        strcat(error_log, "TCC Activation");
      else
        strcat(error_log, "Unknown");
      break;

    default:
      sprintf(error_log, "Unknown");
      break;
  }

  return 0;
}

// Helper function for msleep
void
msleep(int msec) {
  struct timespec req;

  req.tv_sec = 0;
  req.tv_nsec = msec * 1000 * 1000;

  while(nanosleep(&req, &req) == -1 && errno == EINTR) {
    continue;
  }
}

int
pal_set_sensor_health(uint8_t fru, uint8_t value) {

  char key[MAX_KEY_LEN] = {0};
  char cvalue[MAX_VALUE_LEN] = {0};

  sprintf(cvalue, (value > 0) ? "1": "0");

  return pal_set_key_value(key, cvalue);
}

int
pal_get_fru_health(uint8_t fru, uint8_t *value) {

  char cvalue[MAX_VALUE_LEN] = {0};
  char key[MAX_KEY_LEN] = {0};
  int ret;

  ret = pal_get_key_value(key, cvalue);
  if (ret) {
    return ret;
  }

  *value = atoi(cvalue);

  memset(key, 0, MAX_KEY_LEN);
  memset(cvalue, 0, MAX_VALUE_LEN);

  ret = pal_get_key_value(key, cvalue);
  if (ret) {
    return ret;
  }

  *value = *value & atoi(cvalue);
  return 0;
}

void
pal_inform_bic_mode(uint8_t fru, uint8_t mode) {
}

int
pal_get_fan_name(uint8_t num, char *name) {

  switch(num) {

    case FAN_0:
      sprintf(name, "Fan 0");
      break;

    case FAN_1:
      sprintf(name, "Fan 1");
      break;

    default:
      return -1;
  }

  return 0;
}


int
pal_set_fan_speed(uint8_t fan, uint8_t pwm) {
  int unit;
  int ret;

  if (fan >= pal_pwm_cnt) {
    syslog(LOG_INFO, "pal_set_fan_speed: fan number is invalid - %d", fan);
    return -1;
  }

  // Convert the percentage to our 1/96th unit.
  unit = pwm * PWM_UNIT_MAX / 100;

  // For 0%, turn off the PWM entirely
  if (unit == 0) {
    write_fan_value(fan, "pwm%d_en", 0);
    if (ret < 0) {
      syslog(LOG_INFO, "set_fan_speed: write_fan_value failed");
      return -1;
    }
    return 0;

  // For 100%, set falling and rising to the same value
  } else if (unit == PWM_UNIT_MAX) {
    unit = 0;
  }

  ret = write_fan_value(fan, "pwm%d_type", 0);
  if (ret < 0) {
    syslog(LOG_INFO, "set_fan_speed: write_fan_value failed");
    return -1;
  }

  ret = write_fan_value(fan, "pwm%d_rising", 0);
  if (ret < 0) {
    syslog(LOG_INFO, "set_fan_speed: write_fan_value failed");
    return -1;
  }

  ret = write_fan_value(fan, "pwm%d_falling", unit);
  if (ret < 0) {
    syslog(LOG_INFO, "set_fan_speed: write_fan_value failed");
    return -1;
  }

  ret = write_fan_value(fan, "pwm%d_en", 1);
  if (ret < 0) {
    syslog(LOG_INFO, "set_fan_speed: write_fan_value failed");
    return -1;
  }

  return 0;
}

int
pal_get_fan_speed(uint8_t fan, int *rpm) {
  if (fan == 0) {
    return read_fan_value(FAN0_TACH_INPUT, "tacho%d_rpm", rpm);
  } else if (fan == 1) {
    return read_fan_value(FAN1_TACH_INPUT, "tacho%d_rpm", rpm);
  } else {
    syslog(LOG_INFO, "get_fan_speed: invalid fan#:%d", fan);
    return -1;
  }
}

void
pal_update_ts_sled()
{
  char key[MAX_KEY_LEN] = {0};
  char tstr[MAX_VALUE_LEN] = {0};
  struct timespec ts;

  clock_gettime(CLOCK_REALTIME, &ts);
  sprintf(tstr, "%d", ts.tv_sec);

  sprintf(key, "timestamp_sled");

  pal_set_key_value(key, tstr);
}

int
pal_handle_dcmi(uint8_t fru, uint8_t *request, uint8_t req_len, uint8_t *response, uint8_t *rlen) {
  return 0;
}

int
pal_get_platform_id(uint8_t *id) {
  int val;
  char path[64] = {0};

  sprintf(path, GPIO_VAL, GPIO_BOARD_SKU_ID0);
  if (read_device(path, &val)) {
    return -1;
  }
  *id = val&0x01;

  sprintf(path, GPIO_VAL, GPIO_BOARD_SKU_ID1);
  if (read_device(path, &val)) {
    return -1;
  }
  *id = *id | (val<<1);

  sprintf(path, GPIO_VAL, GPIO_BOARD_SKU_ID2);
  if (read_device(path, &val)) {
    return -1;
  }
  *id = *id | (val<<2);

  sprintf(path, GPIO_VAL, GPIO_BOARD_SKU_ID3);
  if (read_device(path, &val)) {
    return -1;
  }
  *id = *id | (val<<3);

  sprintf(path, GPIO_VAL, GPIO_BOARD_SKU_ID4);
  if (read_device(path, &val)) {
    return -1;
  }
  *id = *id | (val<<4);

  return 0;
}

int
pal_get_board_rev_id(uint8_t *id) {
  int val;
  char path[64] = {0};

  sprintf(path, GPIO_VAL, GPIO_BOARD_REV_ID0);
  if (read_device(path, &val)) {
    return -1;
  }
  *id = val&0x01;

  sprintf(path, GPIO_VAL, GPIO_BOARD_REV_ID1);
  if (read_device(path, &val)) {
    return -1;
  }
  *id = *id | (val<<1);

  sprintf(path, GPIO_VAL, GPIO_BOARD_REV_ID2);
  if (read_device(path, &val)) {
    return -1;
  }
  *id = *id | (val<<2);

  return 0;
}

int
pal_get_mb_slot_id(uint8_t *id) {
  int val;
  char path[64] = {0};

  sprintf(path, GPIO_VAL, GPIO_MB_SLOT_ID0);
  if (read_device(path, &val)) {
    return -1;
  }
  *id = val&0x01;

  sprintf(path, GPIO_VAL, GPIO_MB_SLOT_ID1);
  if (read_device(path, &val)) {
    return -1;
  }
  *id = *id | (val<<1);

  sprintf(path, GPIO_VAL, GPIO_MB_SLOT_ID2);
  if (read_device(path, &val)) {
    return -1;
  }
  *id = *id | (val<<2);

  return 0;
}


int
pal_get_slot_cfg_id(uint8_t *id) {
  int val;
  char path[64] = {0};

  sprintf(path, GPIO_VAL, GPIO_SLT_CFG0);
  if (read_device(path, &val)) {
    return -1;
  }
  *id = val&0x01;

  sprintf(path, GPIO_VAL, GPIO_SLT_CFG1);
  if (read_device(path, &val)) {
    return -1;
  }
  *id = *id | (val<<1);

   return 0;
}

void
pal_log_clear(char *fru) {
  if (!strcmp(fru, "mb")) {
    pal_set_key_value("server_sensor_health", "1");
    pal_set_key_value("server_sel_error", "1");
  } else if (!strcmp(fru, "nic")) {
    pal_set_key_value("nic_sensor_health", "1");
  } else if (!strcmp(fru, "all")) {
    pal_set_key_value("server_sensor_health", "1");
    pal_set_key_value("server_sel_error", "1");
    pal_set_key_value("nic_sensor_health", "1");
  }
}

//For OEM command "CMD_OEM_GET_PLAT_INFO" 0x7e
int
pal_get_plat_sku_id(void) {

  return 0;
}

int
pal_get_poss_pcie_config(uint8_t *pcie_config){

  return 0;
}

int
pal_get_pwm_value(uint8_t fan_num, uint8_t *value) {
  char path[LARGEST_DEVICE_NAME] = {0};
  char device_name[LARGEST_DEVICE_NAME] = {0};
  int val = 0;
  int pwm_enable = 0;

  if (fan_num < 0 || fan_num >= pal_pwm_cnt) {
    syslog(LOG_INFO, "pal_get_pwm_value: fan number is invalid - %d", fan_num);
    return -1;
  }

  // Need check pwmX_en to determine the PWM is 0 or 100.
  snprintf(device_name, LARGEST_DEVICE_NAME, "pwm%d_en", fan_num);
  snprintf(path, LARGEST_DEVICE_NAME, "%s/%s", PWM_DIR, device_name);
  if (read_device(path, &pwm_enable)) {
    syslog(LOG_INFO, "pal_get_pwm_value: read %s failed", path);
    return -1;
  }

  if(pwm_enable) {
    snprintf(device_name, LARGEST_DEVICE_NAME, "pwm%d_falling", fan_num);
    snprintf(path, LARGEST_DEVICE_NAME, "%s/%s", PWM_DIR, device_name);
    if (read_device_hex(path, &val)) {
      syslog(LOG_INFO, "pal_get_pwm_value: read %s failed", path);
      return -1;
    }

    if(val == 0)
      *value = 100;
    else
      *value = (100 * val + (PWM_UNIT_MAX-1)) / PWM_UNIT_MAX;
  } else {
    *value = 0;
  }

  return 0;
}

int
pal_fan_dead_handle(int fan_num) {

  // TODO: Add action in case of fan dead
  return 0;
}

int
pal_fan_recovered_handle(int fan_num) {

  // TODO: Add action in case of fan recovered
  return 0;
}

static bool
is_cpu0_socket_occupy(void) {
  char path[64] = {0};
  int val;

  sprintf(path, GPIO_VAL, GPIO_FM_CPU0_SKTOCC_LVT3_N);
  if (read_device(path, &val)) {
    return false;
  }

  if (val) {
    return false;
  } else {
    return true;
  }

}

static bool
is_cpu1_socket_occupy(void) {
  char path[64] = {0};
  int val;

  sprintf(path, GPIO_VAL, GPIO_FM_CPU1_SKTOCC_LVT3_N);
  if (read_device(path, &val)) {
    return false;
  }

  if (val) {
    return false;
  } else {
    return true;
  }

}

static int
pal_i2c_io_master_write(char *BusName, uint8_t addr, uint8_t *buf, uint8_t count) {
  int fd;
  int Retry = MAX_READ_RETRY;
  int RetVal;

  //open device
  while ( Retry )
  {
    fd = open(BusName, O_RDWR);
    if ( fd < 0 ) {
      syslog(LOG_WARNING, "[%s] i2c_open failed for bus#%x\n", __func__, VR_BUS_ID);
      Retry--;
      msleep(100);
#ifdef VR_DEBUG
      printf("[%s] i2c_open failed for bus#%x\n", __func__, VR_BUS_ID);
#endif
    } else {
      break;
    }

    if ( 0 == Retry ) {
      syslog(LOG_WARNING, "[%s] i2c_open failed for bus#%x\n", __func__, VR_BUS_ID);
#ifdef VR_DEBUG
      printf("[%s] i2c_open failed for bus#%x\n", __func__, VR_BUS_ID);
#endif
      goto error_exit;
    }
  }

  Retry = MAX_READ_RETRY;

  //Retry transmission
  while ( Retry )
  {
    RetVal = i2c_io(fd, addr, buf, count, NULL, 0);
    if ( RetVal != 0 )
    {
      syslog(LOG_WARNING, "[%s] Wrtie Bus#%x Slave address: %x failed.\n", __func__, VR_BUS_ID, addr);
      msleep(500);
      Retry--;
#ifdef VR_DEBUG
      printf("[%s] Wrtie Bus#%x Slave address: %x failed.\n", __func__, VR_BUS_ID, addr);
#endif
    } else {
      break;
    }
  }

  if ( RetVal != 0  )
  {
    syslog(LOG_WARNING, "[%s] Wrtie Bus#%x Slave address: %x failed.\n", __func__, VR_BUS_ID, addr);
#ifdef VR_DEBUG
    printf("[%s] Wrtie Bus#%x Slave address: %x failed.\n", __func__, VR_BUS_ID, addr);
#endif
    goto error_exit;
  }

error_exit:

  close(fd);

  return RetVal;
}

static int
pal_i2c_io_rw(char *BusName, uint8_t addr, uint8_t *txbuf, uint8_t txcount, uint8_t *rxbuf,
              uint8_t rxcount)
{
  int fd;
  int Retry = MAX_READ_RETRY;
  int RetVal;

  //open device
  while ( Retry )
  {
    fd = open(BusName, O_RDWR);
    if (fd < 0)
    {
      syslog(LOG_WARNING, "[%s] i2c_open failed for bus#%x\n", __func__, VR_BUS_ID);
      Retry--;
      msleep(100);
#ifdef VR_DEBUG
      printf("[%s] i2c_open failed for bus#%x\n", __func__, VR_BUS_ID);
#endif
    } else {
        break;
    }

    if ( 0 == Retry )
    {
      syslog(LOG_WARNING, "[%s] i2c_open failed for bus#%x\n", __func__, VR_BUS_ID);
#ifdef VR_DEBUG
      printf("[%s] i2c_open failed for bus#%x\n", __func__, VR_BUS_ID);
#endif
        goto error_exit;
    }
  }

  Retry = MAX_READ_RETRY;

  //Retry transmission
  while ( Retry )
  {
      RetVal = i2c_io(fd, addr, txbuf, txcount, rxbuf, rxcount);
      if ( RetVal != 0 )
      {
        Retry--;
        syslog(LOG_WARNING, "[%s] Read/Wrtie Bus#%x Slave address: %x failed.\n", __func__, VR_BUS_ID, addr);
#ifdef VR_DEBUG
        printf("[%s] Read/Wrtie Bus#%x Slave address: %x failed.\n", __func__, VR_BUS_ID, addr);
#endif
        msleep(500);
      } else {
        break;
      }
  }

  if ( RetVal != 0  )
  {
    syslog(LOG_WARNING, "[%s] Read/Wrtie Bus#%x Slave address: %x failed.\n", __func__, VR_BUS_ID, addr);
#ifdef VR_DEBUG
    printf("[%s] Read/Wrtie Bus#%x Slave address: %x failed.\n", __func__, VR_BUS_ID, addr);
#endif
  }

error_exit:

  close(fd);

  return RetVal;
}

/*
 * Board specific version match verfication
 *
*/
static int
pal_check_vr_fw_code_match_MB(int startindex, int endindex, uint8_t *BinData, uint8_t BoardInfo)
{
  int RetVal = -1;
  int j;
  uint8_t BOARD_SKU_ID;
  uint8_t FW_VersionUsed;
  uint8_t FWCodeStage;
  uint8_t BoardStage;
  uint8_t VersionMapper[]=
  {
    EVT1,//evt1
    EVT2,//evt2
    EVT3,//evt3
    DVT,   //dvt
  };

  //for evt3 and after version
  uint8_t DevStageMapper[]=
  {
    DS_IFX,
    SS_IFX,
    DS_Fairchild,
    SS_Fairchild,
  };

  //before evt3 version. evt1 and evt2
  uint8_t EVTStageMapper[]=
  {
    IFX,
    Fairchild,
  };

  //get board stage. evt1, evt2, evt3, or ...
  RetVal = pal_get_board_rev_id(&BoardStage);
  if ( RetVal < 0 )
  {
    syslog(LOG_WARNING, "[%s] failed to get rev id!!\n", __func__);

    return -1;
  }

  BoardStage = VersionMapper[BoardStage];

  //only need to FM_BOARD_SKU_ID3/4
  BOARD_SKU_ID = BoardInfo >> 3;

  for ( j = startindex; j < endindex; j = j + 4 )// 4 bytes
  {
#ifdef VR_DEBUG
    printf("[%s] %x %x %x %x\n", __func__, BinData[j], BinData[j+1], BinData[j+2], BinData[j+3]);
#endif
    //user data0 - 5e0c address. It represents that the vr fw code version
    //we need to check the vr fw code is match MB or not
    if ( ( 0x5e == BinData[j] ) && ( 0x0c == BinData[j+1] ) )
    {
      //it might to be 1,2,3, or 4
      FW_VersionUsed = BinData[j+2];

#ifdef VR_DEBUG
      printf("[%s] FW_VersionUsed: %x \n", __func__, FW_VersionUsed);
#endif
      //its for EVT1, EVT2, or other?
      FWCodeStage = BinData[j+3];

#ifdef VR_DEBUG
      printf("[%s] FWCodeStage: %x \n", __func__, FWCodeStage);
#endif
      break;
    }
  }

  //identify stage is match or not
  if ( BoardStage != FWCodeStage )
  {
    RetVal = -1;

    goto error_exit;
  }

  //identify the version after EVT2
  uint8_t SKUID;

  if ( FWCodeStage > EVT2 )
  {
    SKUID = DevStageMapper[FW_VersionUsed-1];
  }
  else
  {
    SKUID = EVTStageMapper[FW_VersionUsed-1];

    BOARD_SKU_ID = BOARD_SKU_ID & 0x1;
  }

  if ( SKUID == BOARD_SKU_ID )
  {
      return 0;
  }
  else
  {
    syslog(LOG_WARNING, "[%s] VR fw code does not match MB!\n", __func__);

    RetVal = -1;

    goto error_exit;
  }

error_exit:
  return RetVal;
}

static int
pal_check_integrity_of_vr_data(uint8_t SlaveAddr, uint8_t *ExpectedCRC, int StartIndex, int EndIndex, uint8_t *Data)
{
  int i = StartIndex;
  uint32_t CurrentData = 0x0;
  uint32_t CRC = 0x0;
  uint32_t Dout = 0xffffffff;

  for ( ; i < EndIndex; i = i + 4 )
  {
    //printf("[%d] %x %x %x %x\n", i, Data[i], Data[i+1], Data[i+2], Data[i+3]);

    CurrentData = (CurrentData & 0x0) | Data[i+2];//low byte

    CRC =  (VR_BIT0(Dout, CurrentData) & 0x1 ) + ((VR_BIT1(Dout, CurrentData) & 0x1 ) << 1 ) + ((VR_BIT2(Dout, CurrentData) & 0x1 ) << 2 ) + ((VR_BIT3(Dout, CurrentData) & 0x1 ) << 3 ) + ((VR_BIT4(Dout, CurrentData) & 0x1 ) << 4 ) + ((VR_BIT5(Dout, CurrentData) & 0x1 ) << 5 ) + ((VR_BIT6(Dout, CurrentData) & 0x1 ) << 6 ) + ((VR_BIT7(Dout, CurrentData) & 0x1 ) << 7 ) +
    ((VR_BIT8(Dout, CurrentData) & 0x1 ) << 8 ) + ((VR_BIT9(Dout, CurrentData) & 0x1 ) << 9 ) + ((VR_BIT10(Dout, CurrentData) & 0x1 ) << 10 ) + ((VR_BIT11(Dout, CurrentData) & 0x1 ) << 11 ) + ((VR_BIT12(Dout, CurrentData) & 0x1 ) << 12 ) + ((VR_BIT13(Dout, CurrentData) & 0x1 ) << 13 ) + ((VR_BIT14(Dout, CurrentData) & 0x1 ) << 14 ) + ((VR_BIT15(Dout, CurrentData) & 0x1 ) << 15 ) +
    ((VR_BIT16(Dout, CurrentData) & 0x1 ) << 16 ) + ((VR_BIT17(Dout, CurrentData) & 0x1 ) << 17 ) + ((VR_BIT18(Dout, CurrentData) & 0x1 ) << 18 ) + ((VR_BIT19(Dout, CurrentData) & 0x1 ) << 19 ) + ((VR_BIT20(Dout, CurrentData) & 0x1 ) << 20 ) + ((VR_BIT21(Dout, CurrentData) & 0x1 ) << 21 ) + ((VR_BIT22(Dout, CurrentData) & 0x1 ) << 22 ) + ((VR_BIT23(Dout, CurrentData) & 0x1 ) << 23 ) +
    ((VR_BIT24(Dout, CurrentData) & 0x1 ) << 24 ) + ((VR_BIT25(Dout, CurrentData) & 0x1 ) << 25 ) + ((VR_BIT26(Dout, CurrentData) & 0x1 ) << 26 ) + ((VR_BIT27(Dout, CurrentData) & 0x1 ) << 27 ) + ((VR_BIT28(Dout, CurrentData) & 0x1 ) << 28 ) + ((VR_BIT29(Dout, CurrentData) & 0x1 ) << 29 ) + ((VR_BIT30(Dout, CurrentData) & 0x1 ) << 30 ) + ((VR_BIT31(Dout, CurrentData) & 0x1 ) << 31 );

    Dout = CRC;

    CurrentData = (CurrentData & 0x0) | Data[i+3];//high byte

    CRC =  (VR_BIT0(Dout, CurrentData) & 0x1 ) + ((VR_BIT1(Dout, CurrentData) & 0x1 ) << 1 ) + ((VR_BIT2(Dout, CurrentData) & 0x1 ) << 2 ) + ((VR_BIT3(Dout, CurrentData) & 0x1 ) << 3 ) + ((VR_BIT4(Dout, CurrentData) & 0x1 ) << 4 ) + ((VR_BIT5(Dout, CurrentData) & 0x1 ) << 5 ) + ((VR_BIT6(Dout, CurrentData) & 0x1 ) << 6 ) + ((VR_BIT7(Dout, CurrentData) & 0x1 ) << 7 ) +
    ((VR_BIT8(Dout, CurrentData) & 0x1 ) << 8 ) + ((VR_BIT9(Dout, CurrentData) & 0x1 ) << 9 ) + ((VR_BIT10(Dout, CurrentData) & 0x1 ) << 10 ) + ((VR_BIT11(Dout, CurrentData) & 0x1 ) << 11 ) + ((VR_BIT12(Dout, CurrentData) & 0x1 ) << 12 ) + ((VR_BIT13(Dout, CurrentData) & 0x1 ) << 13 ) + ((VR_BIT14(Dout, CurrentData) & 0x1 ) << 14 ) + ((VR_BIT15(Dout, CurrentData) & 0x1 ) << 15 ) +
    ((VR_BIT16(Dout, CurrentData) & 0x1 ) << 16 ) + ((VR_BIT17(Dout, CurrentData) & 0x1 ) << 17 ) + ((VR_BIT18(Dout, CurrentData) & 0x1 ) << 18 ) + ((VR_BIT19(Dout, CurrentData) & 0x1 ) << 19 ) + ((VR_BIT20(Dout, CurrentData) & 0x1 ) << 20 ) + ((VR_BIT21(Dout, CurrentData) & 0x1 ) << 21 ) + ((VR_BIT22(Dout, CurrentData) & 0x1 ) << 22 ) + ((VR_BIT23(Dout, CurrentData) & 0x1 ) << 23 ) +
    ((VR_BIT24(Dout, CurrentData) & 0x1 ) << 24 ) + ((VR_BIT25(Dout, CurrentData) & 0x1 ) << 25 ) + ((VR_BIT26(Dout, CurrentData) & 0x1 ) << 26 ) + ((VR_BIT27(Dout, CurrentData) & 0x1 ) << 27 ) + ((VR_BIT28(Dout, CurrentData) & 0x1 ) << 28 ) + ((VR_BIT29(Dout, CurrentData) & 0x1 ) << 29 ) + ((VR_BIT30(Dout, CurrentData) & 0x1 ) << 30 ) + ((VR_BIT31(Dout, CurrentData) & 0x1 ) << 31 );

    Dout = CRC;
  }

  uint8_t ActualCRC32[4] = {0};

  ActualCRC32[0] = ( CRC & 0xff );
  ActualCRC32[1] = ( ( CRC >> 8 ) & 0xff );
  ActualCRC32[2] = ( ( CRC >> 16 ) & 0xff );
  ActualCRC32[3] = ( ( CRC >> 24 ) & 0xff );

  if ( (ActualCRC32[3] != ExpectedCRC[0]) || (ActualCRC32[2] != ExpectedCRC[1]) || (ActualCRC32[1] != ExpectedCRC[2]) || (ActualCRC32[0] != ExpectedCRC[3]) )
  {
    syslog(LOG_WARNING, "[%s] Slave: %x CRC not match !\n", __func__, SlaveAddr);
    syslog(LOG_WARNING, "[%s] Slave: %x CRC: %x %x %x %x!\n", __func__, SlaveAddr, ActualCRC32[0], ActualCRC32[1], ActualCRC32[2], ActualCRC32[3]);
    syslog(LOG_WARNING, "[%s] Slave: %x Expected CRC: %x %x %x %x!\n", __func__, SlaveAddr, ExpectedCRC[0], ExpectedCRC[1], ExpectedCRC[2], ExpectedCRC[3]);

    return -1;
  }
  else
  {
#ifdef VR_DEBUG
    printf("[%s] Slave: %x CRC match !\n", __func__, SlaveAddr);
#endif
  }

  return 0;
}

static int
pal_unlock_vr_register(char *BusName, uint8_t SlaveAddr)
{
  int RetVal;
  uint8_t TxBuf[3] = {0};

  /********* Set Page***********/
  TxBuf[0] = 0x00;
  TxBuf[1] = 0x3f;

  RetVal = pal_i2c_io_master_write(BusName, SlaveAddr, TxBuf, 2);
  if ( RetVal < 0 )
  {
    syslog(LOG_WARNING, "[%s] UnlockRegister: Change Register Page failed: 0x00 0x3f \n", __func__);
    goto error_exit;
  }

  /*********Write unlock data to 0x27***********/
  TxBuf[0] = 0x27;
  TxBuf[1] = 0x7c;
  TxBuf[2] = 0xb3;

  RetVal = pal_i2c_io_master_write(BusName, SlaveAddr, TxBuf, 3);
  if ( RetVal < 0 )
  {
    syslog(LOG_WARNING, "[%s] Unlock register failed: 0x27 0x7c 0xb3 \n", __func__);
    goto error_exit;
  }


  /*********Write unlock data to 0x2a***********/
  TxBuf[0] = 0x2a;
  TxBuf[1] = 0xb2;
  TxBuf[2] = 0x8a;

  RetVal = pal_i2c_io_master_write(BusName, SlaveAddr, TxBuf, 3);
  if ( RetVal < 0 )
  {
    syslog(LOG_WARNING, "[%s] Unlock register failed: 0x27, 0x7c, 0xb3 \n", __func__);
    goto error_exit;
  }

error_exit:

  return RetVal;
}

static int
pal_check_vr_chip_remaining_times(char *BusName, uint8_t SlaveAddr)
{
  int RetVal;
  int fd;
  uint16_t ChipRemainTimes = 0;
  uint8_t TxBuf[3] = {0};
  uint8_t RxBuf[3] = {0};

  //set page
  TxBuf[0] = 0x00;
  TxBuf[1] = 0x50;

  RetVal = pal_i2c_io_master_write(BusName, SlaveAddr, TxBuf, 2);
  if ( RetVal < 0 )
  {
    syslog(LOG_WARNING, "[%s] set page to 0x00 0x50 failed\n", __func__);
    goto error_exit;
  }

  //read two bytes from address 0x82
  TxBuf[0] = 0x82;
  RxBuf[0] = 0;
  RxBuf[1] = 0;

  RetVal = pal_i2c_io_rw(BusName, SlaveAddr, TxBuf, 1, RxBuf, 2);
  if ( RetVal < 0 )
  {
    syslog(LOG_WARNING, "[%s] CheckChipRemainingTimes: Read data from address 0x82 failed\n", __func__);
    goto error_exit;
  }

  /*get bit 6 ~ bit 11*/
  ChipRemainTimes = (RxBuf[1] << 8) + RxBuf[0];
  ChipRemainTimes = (ChipRemainTimes >> 6 ) & 0x3f;

  //set to page 20
  TxBuf[0] = 0x00;
  TxBuf[1] = 0x20;

  RetVal = pal_i2c_io_master_write(BusName, SlaveAddr, TxBuf, 2);
  if ( RetVal < 0 )
  {
    syslog(LOG_WARNING, "[%s] Change Register Page failed[End]: 0x00 0x20 failed\n", __func__);
    goto error_exit;
  }

  //the remaining time of chip is 0, we cannot flash vr
  if ( ChipRemainTimes == 0 )
  {
    RetVal = -1;
  }

error_exit:

  return RetVal;
}

static int
pal_write_data_to_vr(char *BusName, uint8_t SlaveAddr, uint8_t *Data)
{
  int RetVal;
  uint8_t RxBuf[3] = {0};
  uint8_t TxBuf[3] = {0};

#ifdef VR_DEBUG
  printf("[%s] %x %x %x %x \n", __func__, Data[0], Data[1], Data[2], Data[3]);
#endif
  /***set Page***/
  TxBuf[0] = 0x00;
  TxBuf[1] = (Data[0] >> 1) & 0x3f;

  RetVal = pal_i2c_io_master_write(BusName, SlaveAddr, TxBuf, 2 );
  if ( RetVal < 0 )
  {
    syslog(LOG_WARNING, "[%s] Change Register Page failed: 0x00, 0x3f \n", __func__);
    goto error_exit;
  }

#ifdef VR_DEBUG
  //Check Page is changed or not
  TxBuf[0] = 0x0;
  RxBuf[0] = 0x0;

  RetVal = pal_i2c_io_rw(BusName, SlaveAddr, TxBuf, 1, RxBuf, 1);
  if ( RetVal < 0 )
  {
    syslog(LOG_WARNING, "[%s] Read page failed: 0x0\n", __func__);
    goto error_exit;
  }

    /***set data to buf***/
    //read data from the target address. just for debugging
  TxBuf[0] = Data[1];
  RxBuf[0] = 0;
  RxBuf[1] = 0;

  RetVal = pal_i2c_io_rw(BusName, SlaveAddr, TxBuf, 1, RxBuf, 2);
  if ( RetVal < 0 )
  {
    syslog(LOG_WARNING, "[%s] Read failed: %x , %x, %x \n", __func__, Data[1], Data[2], Data[3]);
    goto error_exit;
  }

    printf("[%s] [Write]Page %x, Addr %x ,low byte %x, high byte %x\n", __func__, (Data[0] >> 1) & 0x3f, Data[1], Data[2], Data[3]);
#endif

    //write data to register
  TxBuf[0] = Data[1];//addr
  TxBuf[1] = Data[2];//low byte
  TxBuf[2] = Data[3];//high byte

  RetVal = pal_i2c_io_master_write(BusName, SlaveAddr, TxBuf, 3);
  if ( RetVal < 0 )
  {
    syslog(LOG_WARNING, "[%s]  Write failed: %x, %x, %x\n", __func__, TxBuf[0], TxBuf[1], TxBuf[2]);
    goto error_exit;
  }

  //double check -- read the address again
  TxBuf[0] = Data[1];
  RxBuf[0] = 0;
  RxBuf[1] = 0;

  RetVal = pal_i2c_io_rw(BusName, SlaveAddr, TxBuf, 1, RxBuf, 2);
  if ( RetVal < 0 )
  {
    syslog(LOG_WARNING, "[%s] Read failed: %x , %x, %x \n",__func__ ,Data[1], Data[2], Data[3]);
    goto error_exit;
  }

  //printf("[Read2]Page %x, Addr %x ,low byte %x, high byte %x\n", page, TxBuf[0], RxBuf[0], RxBuf[1]);

  if ( (RxBuf[0] != Data[2]) || (RxBuf[1] != Data[3]) )
  {
    syslog(LOG_WARNING, "[%s] The result between Read and Write is DIFFERENT!!!!\n",__func__);
    syslog(LOG_WARNING, "[%s] Write: Page %x, Addr %x, LowByte %x, HighByte %x \n",__func__ ,(Data[0] >> 1) & 0x3f ,Data[0] ,Data[1], Data[2] );
    syslog(LOG_WARNING, "[%s] Read:  Page %x, Addr %x, LowByte %x, HighByte %x \n",__func__ ,(Data[0] >> 1) & 0x3f ,TxBuf[0] ,RxBuf[0], RxBuf[1]);
    RetVal = -1;

      goto error_exit;
  }

error_exit:

  return RetVal;
}

static int
pal_write_vr_finish(char *BusName, uint8_t SlaveAddr)
{
  int RetVal;
  uint8_t TxBuf[3] = {0};
  uint8_t RxBuf[3] = {0};

  /*********Set Page***********/
  TxBuf[0] = 0x00;
  TxBuf[1] = 0x3f;

  RetVal = pal_i2c_io_master_write(BusName, SlaveAddr, TxBuf, 2);
  if ( RetVal < 0 )
  {
    syslog(LOG_WARNING, "[%s] Change Register Page failed: 0x00, 0x3f \n", __func__);
    goto error_exit;
  }

#ifdef VR_DEBUG
  //check page is changed or not
  TxBuf[0] = 0x00;
  RxBuf[0] = 0x00;

  //check page
  RetVal = pal_i2c_io_rw(BusName, SlaveAddr, TxBuf, 1, RxBuf, 1);
  if ( RetVal < 0 )
  {
    syslog(LOG_WARNING, "[%s] Check page failed: 0x3f \n",__func__);
    goto error_exit;
  }

  printf("[%s] Check Page: %x \n",__func__, RxBuf[0]);
#endif

  TxBuf[0] = 0x2a;
  TxBuf[1] = 0x00;
  TxBuf[2] = 0x00;

  //write word to the target address
  RetVal = pal_i2c_io_master_write(BusName, SlaveAddr, TxBuf,3);
  if ( RetVal < 0 )
  {
    syslog(LOG_WARNING, "[%s] Lock register failed: 0x2a, 0x00, 0x00 \n",__func__);
    goto error_exit;
  }

  //check the writing data from the target address
  TxBuf[0] = 0x2a;
  RxBuf[0] = 0;
  RxBuf[1] = 0;

  RetVal = pal_i2c_io_rw(BusName, SlaveAddr, TxBuf, 1, RxBuf, 2);
  if ( RetVal < 0 )
  {
    syslog(LOG_WARNING, "[%s] Check data failed: 0x2a, 0x00, 0x00 \n",__func__);
    goto error_exit;
  }

error_exit:

  return RetVal;
}

static int
pal_save_data_to_vr_NVM(char *BusName, uint8_t SlaveAddr)
{
  int RetVal;
  uint8_t TxBuf[3] = {0};
  uint8_t RxBuf[3] = {0};

  /*********Set Page***********/
  TxBuf[0] = 0x00;
  TxBuf[1] = 0x3f;

  RetVal = pal_i2c_io_master_write(BusName, SlaveAddr, TxBuf, 2);
  if ( RetVal < 0 )
  {
    syslog(LOG_WARNING, "[%s] Change Register Page failed: 0x00, 0x3f \n", __func__);
    goto error_exit;
  }

  TxBuf[0] = 0x00;
  RxBuf[0] = 0x00;

  /****Check Page is changed or not****/
  RetVal = pal_i2c_io_rw(BusName, SlaveAddr, TxBuf, 1, RxBuf, 1);
  if ( RetVal < 0 )
  {
    syslog(LOG_WARNING, "[%s] Check page failed: 0x3f \n", __func__);
    goto error_exit;
  }

  /****lock - fill buf****/
  TxBuf[0] = 0x29;
  TxBuf[1] = 0xd7;
  TxBuf[2] = 0xef;

  /****lock - write to the address****/
  RetVal = pal_i2c_io_master_write(BusName, SlaveAddr, TxBuf, 3);
  if ( RetVal < 0 )
  {
    syslog(LOG_WARNING, "[%s] Lock register failed: 0x29, 0xd7, 0xef \n", __func__);
    goto error_exit;
  }

  /****lock - check the data again****/
  RxBuf[0] = 0;
  RxBuf[1] = 0;

  RetVal = pal_i2c_io_rw(BusName, SlaveAddr, TxBuf, 1, RxBuf, 2);
  if ( RetVal < 0 )
  {
    syslog(LOG_WARNING, "[%s] Check data failed: 0x29, 0xd7, 0xef \n", __func__);
    goto error_exit;
  }

  /*********clear fault flag***********/

  TxBuf[0] = 0x2c;

  RetVal = pal_i2c_io_master_write(BusName, SlaveAddr, TxBuf, 1);
  if ( RetVal < 0 )
  {
    syslog(LOG_WARNING, "[%s] Clear fault failed: 0x2c \n",__func__);
    goto error_exit;
  }

  /*********upload data to NVM***********/
  //initiates an upload from the register to NVM
  TxBuf[0] = 0x34;

  RetVal = pal_i2c_io_master_write(BusName, SlaveAddr, TxBuf, 1);
  if ( RetVal < 0 )
  {
    syslog(LOG_WARNING, "[%s] Lock register failed: 0x34 \n", __func__);
    goto error_exit;
  }

  /*********Set Page***********/
  sleep(3);//avoid page failure

  TxBuf[0] = 0x00;
  TxBuf[1] = 0x60;

  RetVal = pal_i2c_io_master_write(BusName, SlaveAddr, TxBuf, 2);
  if ( RetVal < 0 )
  {
    syslog(LOG_WARNING, "[%s] Change Register Page failed: 0x00, 0x60 \n", __func__);
    goto error_exit;
  }

#ifdef VR_DEBUG
  RxBuf[0] = 0;
  TxBuf[0] = 0;

  //check page
  RetVal = pal_i2c_io_rw(BusName, SlaveAddr, TxBuf, 1, RxBuf, 1);
  if ( RetVal < 0 )
  {
    syslog(LOG_WARNING, "[%s] Check page failed: 0x60 \n", __func__);

    goto error_exit;
  }
#endif

  /*********Read Data***********/
  //read word from reg 0x01 and 0x02
  //read data from reg1
  TxBuf[0] = 0x01;
  RxBuf[0] = 0;
  RxBuf[1] = 0;

  RetVal = pal_i2c_io_rw(BusName, SlaveAddr, TxBuf, 1, RxBuf, 2);
  if ( RetVal < 0 )
  {
    syslog(LOG_WARNING, "[%s] Read data from page 0x60  addr 0x01 failed \n",__func__);
    goto error_exit;
  }

  //read data from reg2
  TxBuf[0] = 0x02;

  RxBuf[0] = 0;
  RxBuf[1] = 0;

  RetVal = pal_i2c_io_rw(BusName, SlaveAddr, TxBuf, 1, RxBuf, 2);
  if ( RetVal < 0 )
  {
    syslog(LOG_WARNING, "[%s] Read data from page 0x60  addr 0x02 failed \n",__func__);
    goto error_exit;
  }

  /*********Page***********/
  sleep(1); //avoid page failure

  TxBuf[0] = 0x00;
  TxBuf[1] = 0x3f;

  RetVal = pal_i2c_io_master_write(BusName, SlaveAddr, TxBuf, 2);
  if ( RetVal < 0 )
  {
    syslog(LOG_WARNING, "[%s] Change Register Page failed: 0x00, 0x3f \n",__func__);
    goto error_exit;
  }

#ifdef VR_DEBUG
  //check page
  TxBuf[0] = 0x0;
  RxBuf[0] = 0x0;

  RetVal = pal_i2c_io_rw(BusName, SlaveAddr, TxBuf, 1, RxBuf, 1);
  if ( RetVal < 0 )
  {
    syslog(LOG_WARNING, "[%s] Check page failed: 0x3f \n",__func__);
    goto error_exit;
  }

  //printf("[%s] Check Page: %x \n",__func__, RxBuf[0]);
#endif

  /*********End***********/
  TxBuf[0] = 0x29;
  TxBuf[1] = 0x00;
  TxBuf[2] = 0x00;

  RetVal = pal_i2c_io_master_write(BusName, SlaveAddr, TxBuf, 3);
  if ( RetVal < 0 )
  {
    syslog(LOG_WARNING, "[%s] Failed to write 0x29 0x00 0x00 \n",__func__);
    goto error_exit;
  }

  sleep(2);

#ifdef VR_DEBUG
  TxBuf[0] = 0x29;

  RxBuf[0] = 0x0;
  RxBuf[1] = 0x0;

  //check write data
  RetVal = pal_i2c_io_rw(BusName, SlaveAddr, TxBuf, 1, RxBuf, 2);
  if ( RetVal < 0 )
  {
    syslog(LOG_WARNING, "[%s] Check data failed: 0x29 0x00 0x00 \n",__func__);
    goto error_exit;
  }
#endif

error_exit:

  return RetVal;
}

static int
pal_check_vr_CRC(char *BusName, uint8_t SlaveAddr, uint8_t *ExpectedCRC)
{
  int RetVal=0;
  uint8_t TxBuf[3]={0};
  uint8_t RxBuf[4]={0};

  /****Set Page****/
  TxBuf[0] = 0x00;
  TxBuf[1] = 0x3f;

  RetVal = pal_i2c_io_master_write(BusName, SlaveAddr, TxBuf, 2);
  if ( RetVal < 0 )
  {
    syslog(LOG_WARNING, "[%s] Change Register Page failed: 0x00, 0x3f \n", __func__);
    goto error_exit;
  }

#ifdef VR_DEBUG
  /****Check Page changed or not****/
  TxBuf[0] = 0;
  RxBuf[0] = 0;

  RetVal = pal_i2c_io_rw(BusName, SlaveAddr, TxBuf, 1, RxBuf, 1);
  if ( RetVal < 0 )
  {
    syslog(LOG_WARNING, "[%s] read page failed: 0x3f \n", __func__);
    goto error_exit;
  }
#endif

  sleep(1);//force sleep

  /*********data***********/
  TxBuf[0] = 0x27;
  TxBuf[1] = 0x7c;
  TxBuf[2] = 0xb3;

  RetVal = pal_i2c_io_master_write(BusName, SlaveAddr, TxBuf, 3);
  if ( RetVal < 0 )
  {
    syslog(LOG_WARNING, "[%s] set data failed: 0x27 0x7c 0xb3 \n", __func__);
    goto error_exit;
  }

#ifdef VR_DEBUG
  //check data
  TxBuf[0] = 0x27;

  RxBuf[0] = 0;
  RxBuf[1] = 0;

  RetVal = pal_i2c_io_rw(BusName, SlaveAddr, TxBuf, 1, RxBuf, 2);
  if ( RetVal < 0 )
  {
    syslog(LOG_WARNING, "[%s] Check data failed: 0x27 0x7c 0xb3 \n", __func__);
    goto error_exit;
  }
#endif

  /*********data***********/
  TxBuf[0] = 0x32;

  RetVal = pal_i2c_io_master_write(BusName, SlaveAddr, TxBuf, 1);
  if ( RetVal < 0 )
  {
    syslog(LOG_WARNING, "[%s] set data failed: 0x32  \n", __func__);
    goto error_exit;
  }


  /*********Page***********/
  TxBuf[0] = 0x00;
  TxBuf[1] = 0x6f;

  RetVal = pal_i2c_io_master_write(BusName, SlaveAddr, TxBuf, 2);
  if ( RetVal < 0 )
  {
    syslog(LOG_WARNING, "[%s] Change Register Page failed: 0x00, 0x6f \n", __func__);
    goto error_exit;
  }

#ifdef VR_DEBUG
  /*********Check page changed or not***********/
  TxBuf[0] = 0x00;
  RxBuf[0] = 0x00;

  //check page
  RetVal = pal_i2c_io_rw(BusName, SlaveAddr, TxBuf, 1, RxBuf, 1);
  if ( RetVal < 0 )
  {
    syslog(LOG_WARNING, "[%s] Check page failed: 0x6f \n", __func__);
    goto error_exit;
  }
#endif

  /********read data*************/
  TxBuf[0] = 0x3d;

  RxBuf[0] = 0x00;
  RxBuf[1] = 0x00;

  RetVal = pal_i2c_io_rw(BusName, SlaveAddr, TxBuf, 1, &RxBuf[0], 2);
  if ( RetVal < 0 )
  {
    syslog(LOG_WARNING, "[%s] read addr failed: 0x3d \n", __func__);
    goto error_exit;
  }

  TxBuf[0] = 0x3e;
  RxBuf[2] = 0x00;
  RxBuf[3] = 0x00;

  RetVal = pal_i2c_io_rw(BusName, SlaveAddr, TxBuf, 1, &RxBuf[2], 2);
  if ( RetVal < 0 )
  {
    syslog(LOG_WARNING, "[%s] read addr failed: 0x3e \n",__func__);
    goto error_exit;
  }

  //check CRC match
  if ( (RxBuf[3] != ExpectedCRC[0]) || (RxBuf[2] != ExpectedCRC[1]) || (RxBuf[1] != ExpectedCRC[2]) || (RxBuf[0] != ExpectedCRC[3]))
  {
    syslog(LOG_WARNING, "[%s] CRC cannot MATCH!!\n",__func__);
    syslog(LOG_WARNING, "[%s] Expected CRC: %x %x %x %x\n",__func__, ExpectedCRC[0], ExpectedCRC[1], ExpectedCRC[2], ExpectedCRC[3]);
    syslog(LOG_WARNING, "[%s] Actual CRC: %x %x %x %x\n",__func__, RxBuf[3], RxBuf[2], RxBuf[1], RxBuf[0]);

    RetVal = -1;

    goto error_exit;
  }

error_exit:
  return RetVal;
}

int pal_vr_fw_update(uint8_t *BinData)
{
  VRBasicInfo vr_data[MAX_VR_CHIPS];//there are nine vr chips
  int VRDataLength = 0;
  int VR_counts = sizeof(vr_data)/ sizeof(VRBasicInfo);
  int i, j;
  int fd;
  int RetVal;
  int TotalProgress = 0;
  int CurrentProgress = 0;
  char BusName[64];
  uint8_t BoardInfo;
  uint8_t BOARD_SKU_ID0;

  //create a file to inform "read_vr function". do not send req to VR
  fd = open(VR_UPDATE_IN_PROGRESS, O_WRONLY | O_CREAT | O_TRUNC, 0700);
  if ( fd == -1 )
  {
    printf("[%s] Create %s file error",__func__, VR_UPDATE_IN_PROGRESS);
    RetVal = -1;
    goto error_exit;
  }

  close(fd);

  //wait for the sensord monitor cycle end
  sleep(3);

  snprintf(BusName, sizeof(BusName), "/dev/i2c-%d", VR_BUS_ID);

  //init vr array
  memset(vr_data, 0, sizeof(vr_data));

#ifdef VR_DEBUG
    printf("[%s] VR Counts:%d, Data_Start_Addr: %d\n", __func__, VR_counts, DATA_START_ADDR);
#endif

  //before updating vr, we parse data first.
  //DataLength = update data header + update data
  for ( i = 0 ; i < VR_counts; i++ )
  {
    vr_data[i].DataLength = (BinData[ (2*i) ] << 8 ) + BinData[ (2*i) + 1 ];
#ifdef VR_DEBUG
    printf("[%s] DataLength: %d \n", __func__, vr_data[i].DataLength);
#endif
  }

    //catch the basic info
  for ( i = 0 ; i < VR_counts; i++ )
  {
    /*need to identify the first one. and set the start address*/
    if ( 0 == i )
    {
      j = DATA_START_ADDR;

      vr_data[i].IndexStartAddr = j + 5;
      vr_data[i].SlaveAddr = BinData[j];

      // copy crc
      memcpy(vr_data[i].CRC, &BinData[j+1], 4);

      VRDataLength = vr_data[ i ].DataLength + DATA_START_ADDR;

      vr_data[i].IndexEndAddr = VRDataLength;

#ifdef VR_DEBUG
      printf("[%s]: SlaveAddr: %x\n", __func__, vr_data[i].SlaveAddr );
      printf("[%s]: CRC %x %x %x %x\n", __func__, vr_data[i].CRC[0], vr_data[i].CRC[1], vr_data[i].CRC[2], vr_data[i].CRC[3]);
#endif
    }
    else
    {
      j = VRDataLength;

      vr_data[i].IndexStartAddr = j + 5;
      vr_data[i].SlaveAddr = BinData[ j ];

      //copy crc
      memcpy(vr_data[i].CRC, &BinData[ j + 1 ], 4);

      VRDataLength = vr_data[ i ].DataLength + VRDataLength;

      vr_data[i].IndexEndAddr = VRDataLength;

#ifdef VR_DEBUG
      printf("[%s]: SlaveAddr: %x\n", __func__, vr_data[i].SlaveAddr );
      printf("[%s]: CRC: %x %x %x %x\n", __func__, vr_data[i].CRC[0], vr_data[i].CRC[1], vr_data[i].CRC[2], vr_data[i].CRC[3]);
#endif
    }
  }

  //calculate the reality data length
  for ( i = 0 ; i < VR_counts; i++ )
  {
    TotalProgress += (vr_data[i].IndexEndAddr - vr_data[i].IndexStartAddr);
  }

  /**Check Chip**/
  printf("Check Data...");

  //check the remaining times of flashing chip
  for ( i = 0 ; i < VR_counts; i++ )
  {
    RetVal = pal_check_vr_chip_remaining_times( BusName, vr_data[i].SlaveAddr );
    if ( RetVal < 0 )
    {
      //The Remaining Times of Chip is 0!
      goto error_exit;
    }
  }

  //use FM_BOARD_SKU_ID0 to identify ODM#1 MB to initiate matching check
  RetVal = pal_get_platform_id(&BoardInfo);
  if ( RetVal < 0 )
  {
      syslog(LOG_WARNING, "[%s] Get PlatformID Error!\n", __func__);
      goto error_exit;
  }

  BOARD_SKU_ID0 = BoardInfo & 0x1; //get FM_BOARD_SKU_ID0
  if ( BOARD_SKU_ID0 )
  {
    //get 0x5e0c-userdata(version) data to compare the
      RetVal = pal_check_vr_fw_code_match_MB(vr_data[0].IndexStartAddr, vr_data[0].IndexEndAddr, BinData, BoardInfo);
      if ( RetVal < 0 )
      {
          syslog(LOG_WARNING, "[%s] Please Check vr fw code!\n", __func__);
          goto error_exit;
      }
  }

  //check CRC
  for ( i = 0 ; i < VR_counts; i++ )
  {
    RetVal = pal_check_integrity_of_vr_data(vr_data[i].SlaveAddr, vr_data[i].CRC, vr_data[i].IndexStartAddr, vr_data[i].IndexEndAddr, BinData);
    if ( RetVal < 0 )
    {
      //the bin file is incomplete
      goto error_exit;
    }
  }

  /**Check Chip**/
  printf("Done\n");

  for ( i = 0; i < VR_counts; i++ )
  {

#ifdef VR_DEBUG
    printf("[%s][%d]pal_vr_fw_update: UnlockRegister \n", __func__, i);
#endif

    RetVal = pal_unlock_vr_register( BusName, vr_data[i].SlaveAddr );
    if ( RetVal < 0 )
    {
      //unlock register failed
      syslog(LOG_WARNING, "[%s] Failed Occurred at Unlocking Register\n", __func__);
      goto error_exit;
    }

    //the starting data index of the current vr
    j = vr_data[i].IndexStartAddr;

    //the end index of the current vr
    int EndAddr = vr_data[i].IndexEndAddr;

    for ( ; j < EndAddr; j = j + 4 )// 4 bytes
    {
      CurrentProgress += 4;

      printf("Writing Data: %d/%d (%.2f%%) \r",(CurrentProgress), TotalProgress, (((CurrentProgress)/(float)TotalProgress)*100));

      RetVal = pal_write_data_to_vr( BusName, vr_data[i].SlaveAddr, &BinData[j] );
      if ( RetVal < 0 )
      {
        syslog(LOG_WARNING, "[%s] Failed Occurred at Writing Data\n", __func__);
        goto error_exit;
      }

    }

    //After writing data, we need to store it
    RetVal = pal_write_vr_finish( BusName, vr_data[i].SlaveAddr );
    if ( RetVal < 0 )
    {
      syslog(LOG_WARNING, "[%s] Error at Locking register\n", __func__);
      goto error_exit;
    }

    RetVal = pal_save_data_to_vr_NVM( BusName, vr_data[i].SlaveAddr );
    if ( RetVal < 0 )
    {
      syslog(LOG_WARNING, "[%s] Error at Saving Data\n", __func__);
      goto error_exit;
    }

    RetVal = pal_check_vr_CRC ( BusName, vr_data[i].SlaveAddr, vr_data[i].CRC);
    if ( RetVal < 0 )
    {
      syslog(LOG_WARNING, "[%s] Error at CRC value from the register\n", __func__);
      goto error_exit;
    }

  }
  printf("\n");

  printf("Update VR Success!\n");

error_exit:
  if ( -1 == remove(VR_UPDATE_IN_PROGRESS) )
  {
    printf("[%s] Remove %s Error\n", __func__, VR_UPDATE_IN_PROGRESS);
    RetVal = -1;
  }
  return RetVal;
}
