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
#include "pal.h"
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

#define PAGE_SIZE  0x1000
#define AST_SCU_BASE 0x1e6e2000
#define PIN_CTRL1_OFFSET 0x80
#define PIN_CTRL2_OFFSET 0x84
#define AST_WDT_BASE 0x1e785000
#define WDT_OFFSET 0x10

#define AST_LPC_BASE 0x1e789000
#define HICRA_OFFSET 0x9C
#define HICRA_MASK_UART1 0x70000
#define HICRA_MASK_UART3 0x1C00000
#define UART1_TO_UART3 0x5
#define UART3_TO_UART1 0x5
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

#define READING_NA -2

static uint8_t gpio_rst_btn[] = { 0, 57, 56, 59, 58 };
const static uint8_t gpio_id_led[] = { 0, 41, 40, 43, 42 };
const static uint8_t gpio_prsnt[] = { 0, 61, 60, 63, 62 };
const char pal_fru_list[] = "all, mb, nic";
const char pal_server_list[] = "mb";

size_t pal_pwm_cnt = 2;
size_t pal_tach_cnt = 2;
const char pal_pwm_list[] = "0, 1";
const char pal_tach_list[] = "0, 1";

char * key_list[] = {
"pwr_server_last_state",
"sysfw_ver_server",
"identify_sled",
"timestamp_sled",
"server_por_cfg",
"server_sensor_health",
"nic_sensor_health",
"server_sel_error",
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
  /* Add more def values for the correspoding keys*/
  LAST_KEY /* Same as last entry of the key_list */
};

// List of MB sensors to be monitored
const uint8_t mb_sensor_list[] = {
  MB_SENSOR_INLET_TEMP,
  MB_SENSOR_OUTLET_TEMP,
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
  MB_SENSOR_VR_CPU0_VDDQ_ABC_TEMP,
  MB_SENSOR_VR_CPU0_VDDQ_ABC_CURR,
  MB_SENSOR_VR_CPU0_VDDQ_ABC_VOLT,
  MB_SENSOR_VR_CPU0_VDDQ_ABC_POWER,
  MB_SENSOR_VR_CPU0_VDDQ_DEF_TEMP,
  MB_SENSOR_VR_CPU0_VDDQ_DEF_CURR,
  MB_SENSOR_VR_CPU0_VDDQ_DEF_VOLT,
  MB_SENSOR_VR_CPU0_VDDQ_DEF_POWER,
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
  MB_SENSOR_VR_CPU1_VDDQ_GHJ_TEMP,
  MB_SENSOR_VR_CPU1_VDDQ_GHJ_CURR,
  MB_SENSOR_VR_CPU1_VDDQ_GHJ_VOLT,
  MB_SENSOR_VR_CPU1_VDDQ_GHJ_POWER,
  MB_SENSOR_VR_CPU1_VDDQ_KLM_TEMP,
  MB_SENSOR_VR_CPU1_VDDQ_KLM_CURR,
  MB_SENSOR_VR_CPU1_VDDQ_KLM_VOLT,
  MB_SENSOR_VR_CPU1_VDDQ_KLM_POWER,
  MB_SENSOR_VR_PCH_PVNN_TEMP,
  MB_SENSOR_VR_PCH_PVNN_CURR,
  MB_SENSOR_VR_PCH_PVNN_VOLT,
  MB_SENSOR_VR_PCH_PVNN_POWER,
  MB_SENSOR_VR_PCH_P1V05_TEMP,
  MB_SENSOR_VR_PCH_P1V05_CURR,
  MB_SENSOR_VR_PCH_P1V05_VOLT,
  MB_SENSOR_VR_PCH_P1V05_POWER,
};

// List of NIC sensors to be monitored
const uint8_t nic_sensor_list[] = {
  MEZZ_SENSOR_TEMP,
};

float mb_sensor_threshold[MAX_SENSOR_NUM][MAX_SENSOR_THRESHOLD + 1] = {0};
float nic_sensor_threshold[MAX_SENSOR_NUM][MAX_SENSOR_THRESHOLD + 1] = {0};

size_t mb_sensor_cnt = sizeof(mb_sensor_list)/sizeof(uint8_t);
size_t nic_sensor_cnt = sizeof(nic_sensor_list)/sizeof(uint8_t);

static void
sensor_thresh_array_init() {
  static bool init_done = false;

  if (init_done)
    return;

  mb_sensor_threshold[MB_SENSOR_INLET_TEMP][UCR_THRESH] = 40;
  mb_sensor_threshold[MB_SENSOR_OUTLET_TEMP][UCR_THRESH] = 75;
  mb_sensor_threshold[MB_SENSOR_FAN0_TACH][UCR_THRESH] = 11000;
  mb_sensor_threshold[MB_SENSOR_FAN0_TACH][LCR_THRESH] = 500;
  mb_sensor_threshold[MB_SENSOR_FAN1_TACH][UCR_THRESH] = 11000;
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

  mb_sensor_threshold[MB_SENSOR_VR_CPU0_VCCIN_TEMP][UCR_THRESH] = 85;
  mb_sensor_threshold[MB_SENSOR_VR_CPU0_VCCIN_CURR][UCR_THRESH] = 228;
  mb_sensor_threshold[MB_SENSOR_VR_CPU0_VCCIN_POWER][UCR_THRESH] = 414;
  mb_sensor_threshold[MB_SENSOR_VR_CPU0_VCCIN_VOLT][LCR_THRESH] = 1.5;
  mb_sensor_threshold[MB_SENSOR_VR_CPU0_VCCIN_VOLT][UCR_THRESH] = 2.0;

  mb_sensor_threshold[MB_SENSOR_VR_CPU0_VSA_TEMP][UCR_THRESH] = 85;
  mb_sensor_threshold[MB_SENSOR_VR_CPU0_VSA_CURR][UCR_THRESH] = 20;
  mb_sensor_threshold[MB_SENSOR_VR_CPU0_VSA_POWER][UCR_THRESH] = 18;
  mb_sensor_threshold[MB_SENSOR_VR_CPU0_VSA_VOLT][LCR_THRESH] = 0.7;
  mb_sensor_threshold[MB_SENSOR_VR_CPU0_VSA_VOLT][UCR_THRESH] = 1.2;


  mb_sensor_threshold[MB_SENSOR_VR_CPU0_VCCIO_TEMP][UCR_THRESH] = 85;
  mb_sensor_threshold[MB_SENSOR_VR_CPU0_VCCIO_CURR][UCR_THRESH] = 21;
  mb_sensor_threshold[MB_SENSOR_VR_CPU0_VCCIO_POWER][UCR_THRESH] = 22;
  mb_sensor_threshold[MB_SENSOR_VR_CPU0_VCCIO_VOLT][LCR_THRESH] = 0.8;
  mb_sensor_threshold[MB_SENSOR_VR_CPU0_VCCIO_VOLT][UCR_THRESH] = 1.1;


  mb_sensor_threshold[MB_SENSOR_VR_CPU0_VDDQ_ABC_TEMP][UCR_THRESH] = 83;
  mb_sensor_threshold[MB_SENSOR_VR_CPU0_VDDQ_ABC_CURR][UCR_THRESH] = 72;
  mb_sensor_threshold[MB_SENSOR_VR_CPU0_VDDQ_ABC_POWER][UCR_THRESH] = 88;
  mb_sensor_threshold[MB_SENSOR_VR_CPU0_VDDQ_ABC_VOLT][LCR_THRESH] = 1.08;
  mb_sensor_threshold[MB_SENSOR_VR_CPU0_VDDQ_ABC_VOLT][UCR_THRESH] = 1.32;

  mb_sensor_threshold[MB_SENSOR_VR_CPU0_VDDQ_DEF_TEMP][UCR_THRESH] = 83;
  mb_sensor_threshold[MB_SENSOR_VR_CPU0_VDDQ_DEF_CURR][UCR_THRESH] = 72;
  mb_sensor_threshold[MB_SENSOR_VR_CPU0_VDDQ_DEF_POWER][UCR_THRESH] = 88;
  mb_sensor_threshold[MB_SENSOR_VR_CPU0_VDDQ_DEF_VOLT][LCR_THRESH] = 1.08;
  mb_sensor_threshold[MB_SENSOR_VR_CPU0_VDDQ_DEF_VOLT][UCR_THRESH] = 1.32;

  mb_sensor_threshold[MB_SENSOR_VR_CPU1_VCCIN_TEMP][UCR_THRESH] = 85;
  mb_sensor_threshold[MB_SENSOR_VR_CPU1_VCCIN_CURR][UCR_THRESH] = 228;
  mb_sensor_threshold[MB_SENSOR_VR_CPU1_VCCIN_POWER][UCR_THRESH] = 414;
  mb_sensor_threshold[MB_SENSOR_VR_CPU1_VCCIN_VOLT][LCR_THRESH] = 1.5;
  mb_sensor_threshold[MB_SENSOR_VR_CPU1_VCCIN_VOLT][UCR_THRESH] = 2.0;

  mb_sensor_threshold[MB_SENSOR_VR_CPU1_VSA_TEMP][UCR_THRESH] = 85;
  mb_sensor_threshold[MB_SENSOR_VR_CPU1_VSA_CURR][UCR_THRESH] = 20;
  mb_sensor_threshold[MB_SENSOR_VR_CPU1_VSA_POWER][UCR_THRESH] = 18;
  mb_sensor_threshold[MB_SENSOR_VR_CPU1_VSA_VOLT][LCR_THRESH] = 0.7;
  mb_sensor_threshold[MB_SENSOR_VR_CPU1_VSA_VOLT][UCR_THRESH] = 1.2;

  mb_sensor_threshold[MB_SENSOR_VR_CPU1_VCCIO_TEMP][UCR_THRESH] = 85;
  mb_sensor_threshold[MB_SENSOR_VR_CPU1_VCCIO_CURR][UCR_THRESH] = 21;
  mb_sensor_threshold[MB_SENSOR_VR_CPU1_VCCIO_POWER][UCR_THRESH] = 22;
  mb_sensor_threshold[MB_SENSOR_VR_CPU1_VCCIO_VOLT][LCR_THRESH] = 0.8;
  mb_sensor_threshold[MB_SENSOR_VR_CPU1_VCCIO_VOLT][UCR_THRESH] = 1.1;

  mb_sensor_threshold[MB_SENSOR_VR_CPU1_VDDQ_GHJ_TEMP][UCR_THRESH] = 83;
  mb_sensor_threshold[MB_SENSOR_VR_CPU1_VDDQ_GHJ_CURR][UCR_THRESH] = 72;
  mb_sensor_threshold[MB_SENSOR_VR_CPU1_VDDQ_GHJ_POWER][UCR_THRESH] = 88;
  mb_sensor_threshold[MB_SENSOR_VR_CPU1_VDDQ_GHJ_VOLT][LCR_THRESH] = 1.08;
  mb_sensor_threshold[MB_SENSOR_VR_CPU1_VDDQ_GHJ_VOLT][UCR_THRESH] = 1.32;

  mb_sensor_threshold[MB_SENSOR_VR_CPU1_VDDQ_KLM_TEMP][UCR_THRESH] = 83;
  mb_sensor_threshold[MB_SENSOR_VR_CPU1_VDDQ_KLM_CURR][UCR_THRESH] = 72;
  mb_sensor_threshold[MB_SENSOR_VR_CPU1_VDDQ_KLM_POWER][UCR_THRESH] = 88;
  mb_sensor_threshold[MB_SENSOR_VR_CPU1_VDDQ_KLM_VOLT][LCR_THRESH] = 1.08;
  mb_sensor_threshold[MB_SENSOR_VR_CPU1_VDDQ_KLM_VOLT][UCR_THRESH] = 1.32;

  mb_sensor_threshold[MB_SENSOR_VR_PCH_PVNN_TEMP][UCR_THRESH] = 85;
  mb_sensor_threshold[MB_SENSOR_VR_PCH_PVNN_CURR][UCR_THRESH] = 26;
  mb_sensor_threshold[MB_SENSOR_VR_PCH_PVNN_POWER][UCR_THRESH] = 26;
  mb_sensor_threshold[MB_SENSOR_VR_PCH_PVNN_VOLT][LCR_THRESH] = 0.76;
  mb_sensor_threshold[MB_SENSOR_VR_PCH_PVNN_VOLT][UCR_THRESH] = 1.1;

  mb_sensor_threshold[MB_SENSOR_VR_PCH_P1V05_TEMP][UCR_THRESH] = 85;
  mb_sensor_threshold[MB_SENSOR_VR_PCH_P1V05_CURR][UCR_THRESH] = 20;
  mb_sensor_threshold[MB_SENSOR_VR_PCH_P1V05_POWER][UCR_THRESH] = 22;
  mb_sensor_threshold[MB_SENSOR_VR_PCH_P1V05_VOLT][LCR_THRESH] = 0.94;
  mb_sensor_threshold[MB_SENSOR_VR_PCH_P1V05_VOLT][UCR_THRESH] = 1.15;

  nic_sensor_threshold[MEZZ_SENSOR_TEMP][UCR_THRESH] = 95;

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
    syslog(LOG_ERR, "Failed to do raw io");
    return -1;
  }

  return 0;
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
read_temp(const char *device, float *value) {
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
      full_name, LARGEST_DEVICE_NAME, "%s/temp1_input", dir_name);

  if (read_device(full_name, &tmp)) {
    return -1;
  }

  *value = ((float)tmp)/UNIT_DIV;

  return 0;
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
read_vr_volt(uint8_t vr, uint8_t loop, float *value) {
  int fd;
  char fn[32];
  int ret;
  uint8_t tcount, rcount;
  uint8_t tbuf[16] = {0};
  uint8_t rbuf[16] = {0};

  snprintf(fn, sizeof(fn), "/dev/i2c-%d", VR_BUS_ID);
  fd = open(fn, O_RDWR);
  if (fd < 0) {
    syslog(LOG_WARNING, "pal_get_vr_volt: i2c_open failed for bus#%x\n", VR_BUS_ID);
    goto error_exit;
  }

  // Set the page to read Voltage
  tbuf[0] = 0x00;
  tbuf[1] = loop;

  tcount = 2;
  rcount = 0;

  ret = i2c_io(fd, vr, tbuf, tcount, rbuf, rcount);
  if (ret) {
    syslog(LOG_WARNING, "pal_get_vr_volt: i2c_io failed for bus#%x, dev#%x\n", VR_BUS_ID, vr);
    goto error_exit;
  }

  // Read 2 bytes from Voltage register
  tbuf[0] = VR_TELEMETRY_VOLT;

  tcount = 1;
  rcount = 2;

  ret = i2c_io(fd, vr, tbuf, tcount, rbuf, rcount);
  if (ret) {
    syslog(LOG_WARNING, "pal_get_vr_volt: i2c_io failed for bus#%x, dev#%x\n", VR_BUS_ID, vr);
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
  int ret;
  uint8_t tcount, rcount;
  uint8_t tbuf[16] = {0};
  uint8_t rbuf[16] = {0};

  snprintf(fn, sizeof(fn), "/dev/i2c-%d", VR_BUS_ID);
  fd = open(fn, O_RDWR);
  if (fd < 0) {
    syslog(LOG_WARNING, "pal_get_vr_curr: i2c_open failed for bus#%x\n", VR_BUS_ID);
    goto error_exit;
  }

  // Set the page to read Voltage
  tbuf[0] = 0x00;
  tbuf[1] = loop;

  tcount = 2;
  rcount = 0;

  ret = i2c_io(fd, vr, tbuf, tcount, rbuf, rcount);
  if (ret) {
    syslog(LOG_WARNING, "pal_get_vr_curr: i2c_io failed for bus#%x, dev#%x\n", VR_BUS_ID, vr);
    goto error_exit;
  }

  // Read 2 bytes from Voltage register
  tbuf[0] = VR_TELEMETRY_CURR;

  tcount = 1;
  rcount = 2;

  ret = i2c_io(fd, vr, tbuf, tcount, rbuf, rcount);
  if (ret) {
    syslog(LOG_WARNING, "pal_get_vr_curr: i2c_io failed for bus#%x, dev#%x\n", VR_BUS_ID, vr);
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
  int ret;
  uint8_t tcount, rcount;
  uint8_t tbuf[16] = {0};
  uint8_t rbuf[16] = {0};

  snprintf(fn, sizeof(fn), "/dev/i2c-%d", VR_BUS_ID);
  fd = open(fn, O_RDWR);
  if (fd < 0) {
    syslog(LOG_WARNING, "pal_get_vr_power: i2c_open failed for bus#%x\n", VR_BUS_ID);
    goto error_exit;
  }

  // Set the page to read Power
  tbuf[0] = 0x00;
  tbuf[1] = loop;

  tcount = 2;
  rcount = 0;

  ret = i2c_io(fd, vr, tbuf, tcount, rbuf, rcount);
  if (ret) {
    syslog(LOG_WARNING, "pal_get_vr_power: i2c_io failed for bus#%x, dev#%x\n", VR_BUS_ID, vr);
    goto error_exit;
  }

  // Read 2 bytes from Power register
  tbuf[0] = VR_TELEMETRY_POWER;

  tcount = 1;
  rcount = 2;

  ret = i2c_io(fd, vr, tbuf, tcount, rbuf, rcount);
  if (ret) {
    syslog(LOG_WARNING, "pal_get_vr_power: i2c_io failed for bus#%x, dev#%x\n", VR_BUS_ID, vr);
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
  int ret;
  uint8_t tcount, rcount;
  uint8_t tbuf[16] = {0};
  uint8_t rbuf[16] = {0};

  snprintf(fn, sizeof(fn), "/dev/i2c-%d", VR_BUS_ID);
  fd = open(fn, O_RDWR);
  if (fd < 0) {
    syslog(LOG_WARNING, "pal_get_vr_temp: i2c_open failed for bus#%x\n", VR_BUS_ID);
    goto error_exit;
  }

  // Set the page to read Temperature
  tbuf[0] = 0x00;
  tbuf[1] = loop;

  tcount = 2;
  rcount = 0;

  ret = i2c_io(fd, vr, tbuf, tcount, rbuf, rcount);
  if (ret) {
    syslog(LOG_WARNING, "pal_get_vr_temp: i2c_io failed for bus#%x, dev#%x\n", VR_BUS_ID, vr);
    goto error_exit;
  }

  // Read 2 bytes from Power register
  tbuf[0] = VR_TELEMETRY_TEMP;

  tcount = 1;
  rcount = 2;

  ret = i2c_io(fd, vr, tbuf, tcount, rbuf, rcount);
  if (ret) {
    syslog(LOG_WARNING, "pal_get_vr_temp: i2c_io failed for bus#%x, dev#%x\n", VR_BUS_ID, vr);
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
  // Clear bits for UART1 and UART3 routing
  ctrl &= (~HICRA_MASK_UART1);
  ctrl &= (~HICRA_MASK_UART3);

  // Route UART1 to UART3 for SoL purpose
  ctrl |= (UART1_TO_UART3 << 22);

  switch(fru) {
  case UART_TO_DEBUG:
    // Route DEBUG to UART1 for TXD control
    ctrl |= (DEBUG_TO_UART1 << 16);
    break;
  default:
    // Route DEBUG to UART1 for TXD control
    ctrl |= (UART3_TO_UART1 << 16);
    break;
  }

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

    default:
      return -1;
  }

  return 0;
}

int
pal_sled_cycle(void) {
  // Remove the adm1275 module as the HSC device is busy
  system("rmmod adm1275");

  // Send command to HSC power cycle
  system("i2cset -y 7 0x11 0xd9 c");

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

  switch(fru) {
  case FRU_MB:
    sprintf(key, "mb_sensor%d", sensor_num);
    if (is_server_off()) {
      // Power is OFF, so only some of the sensors can be read
      switch(sensor_num) {
      // Temp. Sensors
      case MB_SENSOR_INLET_TEMP:
        ret = read_temp(MB_INLET_TEMP_DEVICE, (float*) value);
        break;
      case MB_SENSOR_OUTLET_TEMP:
        ret = read_temp(MB_OUTLET_TEMP_DEVICE, (float*) value);
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
        ret = read_hsc_value(HSC_IN_VOLT, (float*) value);
        break;
      case MB_SENSOR_HSC_OUT_CURR:
        ret = read_hsc_value(HSC_OUT_CURR, (float*) value);
        break;
      case MB_SENSOR_HSC_IN_POWER:
        if (ret = read_hsc_value(HSC_IN_VOLT, &volt)) {
          return -1;
        }
        if (ret = read_hsc_value(HSC_OUT_CURR, &curr)) {
          return -1;
        }
        * (float*) value = volt * curr;
        break;
      default:
        ret = READING_NA;
        break;
      }
    } else {
      switch(sensor_num) {
      // Temp. Sensors
      case MB_SENSOR_INLET_TEMP:
        ret = read_temp(MB_INLET_TEMP_DEVICE, (float*) value);
        break;
      case MB_SENSOR_OUTLET_TEMP:
        ret = read_temp(MB_OUTLET_TEMP_DEVICE, (float*) value);
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
        ret = read_hsc_value(HSC_IN_VOLT, (float*) value);
        break;
      case MB_SENSOR_HSC_OUT_CURR:
        ret = read_hsc_value(HSC_OUT_CURR, (float*) value);
        break;
      case MB_SENSOR_HSC_IN_POWER:
        if (ret = read_hsc_value(HSC_IN_VOLT, &volt)) {
          return -1;
        }
        if (ret = read_hsc_value(HSC_OUT_CURR, &curr)) {
          return -1;
        }
        * (float*) value = volt * curr;
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
      case MB_SENSOR_VR_CPU0_VDDQ_ABC_TEMP:
        ret = read_vr_temp(VR_CPU0_VDDQ_ABC, VR_LOOP_PAGE_0, (float*) value);
        break;
      case MB_SENSOR_VR_CPU0_VDDQ_ABC_CURR:
        ret = read_vr_curr(VR_CPU0_VDDQ_ABC, VR_LOOP_PAGE_0, (float*) value);
        break;
      case MB_SENSOR_VR_CPU0_VDDQ_ABC_VOLT:
        ret = read_vr_volt(VR_CPU0_VDDQ_ABC, VR_LOOP_PAGE_0, (float*) value);
        break;
      case MB_SENSOR_VR_CPU0_VDDQ_ABC_POWER:
        ret = read_vr_power(VR_CPU0_VDDQ_ABC, VR_LOOP_PAGE_0, (float*) value);
        break;
      case MB_SENSOR_VR_CPU0_VDDQ_DEF_TEMP:
        ret = read_vr_temp(VR_CPU0_VDDQ_DEF, VR_LOOP_PAGE_0, (float*) value);
        break;
      case MB_SENSOR_VR_CPU0_VDDQ_DEF_CURR:
        ret = read_vr_curr(VR_CPU0_VDDQ_DEF, VR_LOOP_PAGE_0, (float*) value);
        break;
      case MB_SENSOR_VR_CPU0_VDDQ_DEF_VOLT:
        ret = read_vr_volt(VR_CPU0_VDDQ_DEF, VR_LOOP_PAGE_0, (float*) value);
        break;
      case MB_SENSOR_VR_CPU0_VDDQ_DEF_POWER:
        ret = read_vr_power(VR_CPU0_VDDQ_DEF, VR_LOOP_PAGE_0, (float*) value);
        break;
      case MB_SENSOR_VR_CPU1_VCCIN_TEMP:
        ret = read_vr_temp(VR_CPU1_VCCIN, VR_LOOP_PAGE_0, (float*) value);
        break;
      case MB_SENSOR_VR_CPU1_VCCIN_CURR:
        ret = read_vr_curr(VR_CPU1_VCCIN, VR_LOOP_PAGE_0, (float*) value);
        break;
      case MB_SENSOR_VR_CPU1_VCCIN_VOLT:
        ret = read_vr_volt(VR_CPU1_VCCIN, VR_LOOP_PAGE_0, (float*) value);
        break;
      case MB_SENSOR_VR_CPU1_VCCIN_POWER:
        ret = read_vr_power(VR_CPU1_VCCIN, VR_LOOP_PAGE_0, (float*) value);
        break;
      case MB_SENSOR_VR_CPU1_VSA_TEMP:
        ret = read_vr_temp(VR_CPU1_VSA, VR_LOOP_PAGE_1, (float*) value);
        break;
      case MB_SENSOR_VR_CPU1_VSA_CURR:
        ret = read_vr_curr(VR_CPU1_VSA, VR_LOOP_PAGE_1, (float*) value);
        break;
      case MB_SENSOR_VR_CPU1_VSA_VOLT:
        ret = read_vr_volt(VR_CPU1_VSA, VR_LOOP_PAGE_1, (float*) value);
        break;
      case MB_SENSOR_VR_CPU1_VSA_POWER:
        ret = read_vr_power(VR_CPU1_VSA, VR_LOOP_PAGE_1, (float*) value);
        break;
      case MB_SENSOR_VR_CPU1_VCCIO_TEMP:
        ret = read_vr_temp(VR_CPU1_VCCIO, VR_LOOP_PAGE_0, (float*) value);
        break;
      case MB_SENSOR_VR_CPU1_VCCIO_CURR:
        ret = read_vr_curr(VR_CPU1_VCCIO, VR_LOOP_PAGE_0, (float*) value);
        break;
      case MB_SENSOR_VR_CPU1_VCCIO_VOLT:
        ret = read_vr_volt(VR_CPU1_VCCIO, VR_LOOP_PAGE_0, (float*) value);
        break;
      case MB_SENSOR_VR_CPU1_VCCIO_POWER:
        ret = read_vr_power(VR_CPU1_VCCIO, VR_LOOP_PAGE_0, (float*) value);
        break;
      case MB_SENSOR_VR_CPU1_VDDQ_GHJ_TEMP:
        ret = read_vr_temp(VR_CPU1_VDDQ_GHJ, VR_LOOP_PAGE_0, (float*) value);
        break;
      case MB_SENSOR_VR_CPU1_VDDQ_GHJ_CURR:
        ret = read_vr_curr(VR_CPU1_VDDQ_GHJ, VR_LOOP_PAGE_0, (float*) value);
        break;
      case MB_SENSOR_VR_CPU1_VDDQ_GHJ_VOLT:
        ret = read_vr_volt(VR_CPU1_VDDQ_GHJ, VR_LOOP_PAGE_0, (float*) value);
        break;
      case MB_SENSOR_VR_CPU1_VDDQ_GHJ_POWER:
        ret = read_vr_power(VR_CPU1_VDDQ_GHJ, VR_LOOP_PAGE_0, (float*) value);
        break;
      case MB_SENSOR_VR_CPU1_VDDQ_KLM_TEMP:
        ret = read_vr_temp(VR_CPU1_VDDQ_KLM, VR_LOOP_PAGE_0, (float*) value);
        break;
      case MB_SENSOR_VR_CPU1_VDDQ_KLM_CURR:
        ret = read_vr_curr(VR_CPU1_VDDQ_KLM, VR_LOOP_PAGE_0, (float*) value);
        break;
      case MB_SENSOR_VR_CPU1_VDDQ_KLM_VOLT:
        ret = read_vr_volt(VR_CPU1_VDDQ_KLM, VR_LOOP_PAGE_0, (float*) value);
        break;
      case MB_SENSOR_VR_CPU1_VDDQ_KLM_POWER:
        ret = read_vr_power(VR_CPU1_VDDQ_KLM, VR_LOOP_PAGE_0, (float*) value);
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
    if (ret == READING_NA) {
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
  }
  else {
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
    case MB_SENSOR_VR_CPU0_VDDQ_ABC_TEMP:
      sprintf(name, "MB_VR_CPU0_VDDQ_ABC_TEMP");
      break;
    case MB_SENSOR_VR_CPU0_VDDQ_ABC_CURR:
      sprintf(name, "MB_VR_CPU0_VDDQ_ABC_CURR");
      break;
    case MB_SENSOR_VR_CPU0_VDDQ_ABC_VOLT:
      sprintf(name, "MB_VR_CPU0_VDDQ_ABC_VOLT");
      break;
    case MB_SENSOR_VR_CPU0_VDDQ_ABC_POWER:
      sprintf(name, "MB_VR_CPU0_VDDQ_ABC_POWER");
      break;
    case MB_SENSOR_VR_CPU0_VDDQ_DEF_TEMP:
      sprintf(name, "MB_VR_CPU0_VDDQ_DEF_TEMP");
      break;
    case MB_SENSOR_VR_CPU0_VDDQ_DEF_CURR:
      sprintf(name, "MB_VR_CPU0_VDDQ_DEF_CURR");
      break;
    case MB_SENSOR_VR_CPU0_VDDQ_DEF_VOLT:
      sprintf(name, "MB_VR_CPU0_VDDQ_DEF_VOLT");
      break;
    case MB_SENSOR_VR_CPU0_VDDQ_DEF_POWER:
      sprintf(name, "MB_VR_CPU0_VDDQ_DEF_POWER");
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
    case MB_SENSOR_VR_CPU1_VDDQ_GHJ_TEMP:
      sprintf(name, "MB_VR_CPU1_VDDQ_GHJ_TEMP");
      break;
    case MB_SENSOR_VR_CPU1_VDDQ_GHJ_CURR:
      sprintf(name, "MB_VR_CPU1_VDDQ_GHJ_CURR");
      break;
    case MB_SENSOR_VR_CPU1_VDDQ_GHJ_VOLT:
      sprintf(name, "MB_VR_CPU1_VDDQ_GHJ_VOLT");
      break;
    case MB_SENSOR_VR_CPU1_VDDQ_GHJ_POWER:
      sprintf(name, "MB_VR_CPU1_VDDQ_GHJ_POWER");
      break;
    case MB_SENSOR_VR_CPU1_VDDQ_KLM_TEMP:
      sprintf(name, "MB_VR_CPU1_VDDQ_KLM_TEMP");
      break;
    case MB_SENSOR_VR_CPU1_VDDQ_KLM_CURR:
      sprintf(name, "MB_VR_CPU1_VDDQ_KLM_CURR");
      break;
    case MB_SENSOR_VR_CPU1_VDDQ_KLM_VOLT:
      sprintf(name, "MB_VR_CPU1_VDDQ_KLM_VOLT");
      break;
    case MB_SENSOR_VR_CPU1_VDDQ_KLM_POWER:
      sprintf(name, "MB_VR_CPU1_VDDQ_KLM_POWER");
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
    case MB_SENSOR_VR_CPU0_VCCIN_TEMP:
    case MB_SENSOR_VR_CPU0_VSA_TEMP:
    case MB_SENSOR_VR_CPU0_VCCIO_TEMP:
    case MB_SENSOR_VR_CPU0_VDDQ_ABC_TEMP:
    case MB_SENSOR_VR_CPU0_VDDQ_DEF_TEMP:
    case MB_SENSOR_VR_CPU1_VCCIN_TEMP:
    case MB_SENSOR_VR_CPU1_VSA_TEMP:
    case MB_SENSOR_VR_CPU1_VCCIO_TEMP:
    case MB_SENSOR_VR_CPU1_VDDQ_GHJ_TEMP:
    case MB_SENSOR_VR_CPU1_VDDQ_KLM_TEMP:
    case MB_SENSOR_VR_PCH_PVNN_TEMP:
    case MB_SENSOR_VR_PCH_P1V05_TEMP:
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
    case MB_SENSOR_VR_CPU0_VDDQ_ABC_VOLT:
    case MB_SENSOR_VR_CPU0_VDDQ_DEF_VOLT:
    case MB_SENSOR_VR_CPU1_VCCIN_VOLT:
    case MB_SENSOR_VR_CPU1_VSA_VOLT:
    case MB_SENSOR_VR_CPU1_VCCIO_VOLT:
    case MB_SENSOR_VR_CPU1_VDDQ_GHJ_VOLT:
    case MB_SENSOR_VR_CPU1_VDDQ_KLM_VOLT:
    case MB_SENSOR_VR_PCH_PVNN_VOLT:
    case MB_SENSOR_VR_PCH_P1V05_VOLT:
      sprintf(units, "Volts");
      break;
    case MB_SENSOR_HSC_OUT_CURR:
    case MB_SENSOR_VR_CPU0_VCCIN_CURR:
    case MB_SENSOR_VR_CPU0_VSA_CURR:
    case MB_SENSOR_VR_CPU0_VCCIO_CURR:
    case MB_SENSOR_VR_CPU0_VDDQ_ABC_CURR:
    case MB_SENSOR_VR_CPU0_VDDQ_DEF_CURR:
    case MB_SENSOR_VR_CPU1_VCCIN_CURR:
    case MB_SENSOR_VR_CPU1_VSA_CURR:
    case MB_SENSOR_VR_CPU1_VCCIO_CURR:
    case MB_SENSOR_VR_CPU1_VDDQ_GHJ_CURR:
    case MB_SENSOR_VR_CPU1_VDDQ_KLM_CURR:
    case MB_SENSOR_VR_PCH_PVNN_CURR:
    case MB_SENSOR_VR_PCH_P1V05_CURR:
      sprintf(units, "Amps");
      break;
    case MB_SENSOR_HSC_IN_POWER:
    case MB_SENSOR_VR_CPU0_VCCIN_POWER:
    case MB_SENSOR_VR_CPU0_VSA_POWER:
    case MB_SENSOR_VR_CPU0_VCCIO_POWER:
    case MB_SENSOR_VR_CPU0_VDDQ_ABC_POWER:
    case MB_SENSOR_VR_CPU0_VDDQ_DEF_POWER:
    case MB_SENSOR_VR_CPU1_VCCIN_POWER:
    case MB_SENSOR_VR_CPU1_VSA_POWER:
    case MB_SENSOR_VR_CPU1_VCCIO_POWER:
    case MB_SENSOR_VR_CPU1_VDDQ_GHJ_POWER:
    case MB_SENSOR_VR_CPU1_VDDQ_KLM_POWER:
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
  sprintf(devtty, "/dev/ttyS3");
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

  return 0;
}

int
pal_get_sys_guid(uint8_t slot, char *guid) {
  int ret;

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
pal_get_vr_ver(uint8_t vr, uint8_t *ver) {
  int fd;
  char fn[32];
  int ret;
  uint8_t tcount, rcount;
  uint8_t tbuf[16] = {0};
  uint8_t rbuf[16] = {0};

  snprintf(fn, sizeof(fn), "/dev/i2c-%d", VR_BUS_ID);
  fd = open(fn, O_RDWR);
  if (fd < 0) {
    syslog(LOG_WARNING, "pal_get_vr_ver: i2c_open failed for bus#%x\n", VR_BUS_ID);
    goto error_exit;
  }

  // Set the page to read FW Version
  tbuf[0] = 0x00;
  tbuf[1] = VR_FW_PAGE;

  tcount = 2;
  rcount = 0;

  ret = i2c_io(fd, vr, tbuf, tcount, rbuf, rcount);
  if (ret) {
    syslog(LOG_WARNING, "pal_get_vr_ver: i2c_io failed for bus#%x, dev#%x\n", VR_BUS_ID, vr);
    goto error_exit;
  }

  // Read 2 bytes from first register
  tbuf[0] = VR_FW_REG1;

  tcount = 1;
  rcount = 2;

  ret = i2c_io(fd, vr, tbuf, tcount, rbuf, rcount);
  if (ret) {
    syslog(LOG_WARNING, "pal_get_vr_ver: i2c_io failed for bus#%x, dev#%x\n", VR_BUS_ID, vr);
    goto error_exit;
  }

  ver[0] = rbuf[1];
  ver[1] = rbuf[0];

  tbuf[0] = VR_FW_REG2;

  tcount = 1;
  rcount = 2;

  ret = i2c_io(fd, vr, tbuf, tcount, rbuf, rcount);
  if (ret) {
    syslog(LOG_WARNING, "pal_get_vr_ver: i2c_io failed for bus#%x, dev#%x\n", VR_BUS_ID, vr);
    goto error_exit;
  }

  ver[2] = rbuf[1];
  ver[3] = rbuf[0];

error_exit:
  if (fd > 0) {
    close(fd);
  }

  return ret;
}

int
pal_get_vr_checksum(uint8_t vr, uint8_t *checksum) {
  int fd;
  char fn[32];
  int ret;
  uint8_t tcount, rcount;
  uint8_t tbuf[16] = {0};
  uint8_t rbuf[16] = {0};

  snprintf(fn, sizeof(fn), "/dev/i2c-%d", VR_BUS_ID);
  fd = open(fn, O_RDWR);
  if (fd < 0) {
    syslog(LOG_WARNING, "pal_get_vr_checksum: i2c_open failed for bus#%x\n", VR_BUS_ID);
    goto error_exit;
  }

  // Set the page to read FW checksum
  tbuf[0] = 0x00;
  tbuf[1] = VR_FW_PAGE_2;

  tcount = 2;
  rcount = 0;

  ret = i2c_io(fd, vr, tbuf, tcount, rbuf, rcount);
  if (ret) {
    syslog(LOG_WARNING, "pal_get_vr_checksum: i2c_io failed for bus#%x, dev#%x\n", VR_BUS_ID, vr);
    goto error_exit;
  }

  // Read 2 bytes from first register
  tbuf[0] = VR_FW_REG4;

  tcount = 1;
  rcount = 2;

  ret = i2c_io(fd, vr, tbuf, tcount, rbuf, rcount);
  if (ret) {
    syslog(LOG_WARNING, "pal_get_vr_checksum: i2c_io failed for bus#%x, dev#%x\n", VR_BUS_ID, vr);
    goto error_exit;
  }

  checksum[0] = rbuf[1];
  checksum[1] = rbuf[0];

  tbuf[0] = VR_FW_REG3;

  tcount = 1;
  rcount = 2;

  ret = i2c_io(fd, vr, tbuf, tcount, rbuf, rcount);
  if (ret) {
    syslog(LOG_WARNING, "pal_get_vr_checksum: i2c_io failed for bus#%x, dev#%x\n", VR_BUS_ID, vr);
    goto error_exit;
  }

  checksum[2] = rbuf[1];
  checksum[3] = rbuf[0];

error_exit:
  if (fd > 0) {
    close(fd);
  }

  return ret;
}

int
pal_get_vr_deviceId(uint8_t vr, uint8_t *deviceId) {
  int fd;
  char fn[32];
  int ret;
  uint8_t tcount, rcount;
  uint8_t tbuf[16] = {0};
  uint8_t rbuf[16] = {0};

  snprintf(fn, sizeof(fn), "/dev/i2c-%d", VR_BUS_ID);
  fd = open(fn, O_RDWR);
  if (fd < 0) {
    syslog(LOG_WARNING, "pal_get_vr_deviceId: i2c_open failed for bus#%x\n", VR_BUS_ID);
    goto error_exit;
  }

  // Set the page to read FW deviceId
  tbuf[0] = 0x00;
  tbuf[1] = VR_FW_PAGE_3;

  tcount = 2;
  rcount = 0;

  ret = i2c_io(fd, vr, tbuf, tcount, rbuf, rcount);
  if (ret) {
    syslog(LOG_WARNING, "pal_get_vr_deviceId: i2c_io failed for bus#%x, dev#%x\n", VR_BUS_ID, vr);
    goto error_exit;
  }

  // Read 2 bytes from first register
  tbuf[0] = VR_FW_REG5;

  tcount = 1;
  rcount = 2;

  ret = i2c_io(fd, vr, tbuf, tcount, rbuf, rcount);
  if (ret) {
    syslog(LOG_WARNING, "pal_get_vr_deviceId: i2c_io failed for bus#%x, dev#%x\n", VR_BUS_ID, vr);
    goto error_exit;
  }

  deviceId[0] = rbuf[1];
  deviceId[1] = rbuf[0];

error_exit:
  if (fd > 0) {
    close(fd);
  }

  return ret;
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
    case PWR_ERR:
      sprintf(name, "PWR_ERR");
      break;
    case CATERR:
      sprintf(name, "CATERR");
      break;
    case CPU_DIMM_HOT:
      sprintf(name, "CPU_DIMM_HOT");
      break;
    case CPU0_THERM_STATUS:
      sprintf(name, "CPU0_THERM_STATUS");
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

  char *ed = event_data;
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
      sprintf(error_log, "");
      if ((ed[0] & 0x0F) == 0x0)
        strcat(error_log, "Correctable");
      else if ((ed[0] & 0x0F) == 0x1)
        strcat(error_log, "Uncorrectable");
      else if ((ed[0] & 0x0F) == 0x5)
        strcat(error_log, "Correctable ECC error Logging Limit Reached");
      else
        strcat(error_log, "Unknown");

      if (((ed[1] & 0xC) >> 2) == 0x0) {
        /* All Info Valid */
        sprintf(temp_log, " (CPU# %d, CHN# %d, DIMM# %d)",
            (ed[2] & 0xE0) >> 5, (ed[2] & 0x18) >> 3, ed[2] & 0x7);
      } else if (((ed[1] & 0xC) >> 2) == 0x1) {
        /* DIMM info not valid */
        sprintf(temp_log, " (CPU# %d, CHN# %d)",
            (ed[2] & 0xE0) >> 5, (ed[2] & 0x18) >> 3);
      } else if (((ed[1] & 0xC) >> 2) == 0x2) {
        /* CHN info not valid */
        sprintf(temp_log, " (CPU# %d, DIMM# %d)",
            (ed[2] & 0xE0) >> 5, ed[2] & 0x7);
      } else if (((ed[1] & 0xC) >> 2) == 0x3) {
        /* CPU info not valid */
        sprintf(temp_log, " (CHN# %d, DIMM# %d)",
            (ed[2] & 0x18) >> 3, ed[2] & 0x7);
      }
      strcat(error_log, temp_log);

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
        strcat(error_log, "IERR");
      else if (ed[0] == 0xB)
        strcat(error_log, "MCERR");
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

    case SPS_FW_HEALTH:
      sprintf(error_log, "");
      if (event_data[0] == 0xDC && ed[1] == 0x06) {
        strcat(error_log, "FW UPDATE");
        return 1;
      } else
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
