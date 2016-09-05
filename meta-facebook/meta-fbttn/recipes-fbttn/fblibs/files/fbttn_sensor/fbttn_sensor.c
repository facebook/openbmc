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
#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>
#include <math.h>
#include <string.h>
#include <fcntl.h>
#include <errno.h>
#include <syslog.h>
#include <facebook/i2c-dev.h>
#include <facebook/i2c.h>
#include "fbttn_sensor.h"
//For Kernel 2.6 -> 4.1
#define MEZZ_TEMP_DEVICE "/sys/devices/platform/ast-i2c.8/i2c-8/8-001f/hwmon/hwmon*"

#define LARGEST_DEVICE_NAME 120

#define GPIO_VAL "/sys/class/gpio/gpio%d/value"

#define I2C_BUS_9_DIR "/sys/class/i2c-adapter/i2c-9/"
#define I2C_BUS_10_DIR "/sys/class/i2c-adapter/i2c-10/"

#define TACH_DIR "/sys/devices/platform/ast_pwm_tacho.0"
#define ADC_DIR "/sys/devices/platform/ast_adc.0"

#define SP_INLET_TEMP_DEVICE I2C_BUS_9_DIR "9-004e"
#define SP_OUTLET_TEMP_DEVICE I2C_BUS_9_DIR "9-004f"
#define HSC_DEVICE I2C_BUS_10_DIR "10-0040"

#define FAN_TACH_RPM "tacho%d_rpm"
#define ADC_VALUE "adc%d_value"
#define HSC_IN_VOLT "in1_input"
#define HSC_OUT_CURR "curr1_input"
#define HSC_TEMP "temp1_input"
#define HSC_IN_POWER "power1_input"

#define UNIT_DIV 1000

#define I2C_DEV_NIC "/dev/i2c-11"
#define I2C_NIC_ADDR 0x1f
#define I2C_NIC_SENSOR_TEMP_REG 0x01

#define BIC_SENSOR_READ_NA 0x20

#define MAX_SENSOR_NUM 0xFF
#define ALL_BYTES 0xFF
#define LAST_REC_ID 0xFFFF

#define FBTTN_SDR_PATH "/tmp/sdr_%s.bin"

// List of BIC sensors to be monitored
const uint8_t bic_sensor_list[] = {
  /* Threshold sensors */
  BIC_SENSOR_MB_OUTLET_TEMP,
  BIC_SENSOR_VCCIN_VR_TEMP,
  BIC_SENSOR_VCC_GBE_VR_TEMP,
  BIC_SENSOR_1V05PCH_VR_TEMP,
  BIC_SENSOR_SOC_TEMP,
  BIC_SENSOR_MB_INLET_TEMP,
  BIC_SENSOR_PCH_TEMP,
  BIC_SENSOR_SOC_THERM_MARGIN,
  BIC_SENSOR_VDDR_VR_TEMP,
  BIC_SENSOR_VCC_GBE_VR_CURR,
  BIC_SENSOR_1V05_PCH_VR_CURR,
  BIC_SENSOR_VCCIN_VR_POUT,
  BIC_SENSOR_VCCIN_VR_CURR,
  BIC_SENSOR_VCCIN_VR_VOL,
  BIC_SENSOR_INA230_POWER,
  BIC_SENSOR_INA230_VOL,
  BIC_SENSOR_SOC_PACKAGE_PWR,
  BIC_SENSOR_SOC_TJMAX,
  BIC_SENSOR_VDDR_VR_POUT,
  BIC_SENSOR_VDDR_VR_CURR,
  BIC_SENSOR_VDDR_VR_VOL,
  BIC_SENSOR_VCC_SCSUS_VR_CURR,
  BIC_SENSOR_VCC_SCSUS_VR_VOL,
  BIC_SENSOR_VCC_SCSUS_VR_TEMP,
  BIC_SENSOR_VCC_SCSUS_VR_POUT,
  BIC_SENSOR_VCC_GBE_VR_POUT,
  BIC_SENSOR_VCC_GBE_VR_VOL,
  BIC_SENSOR_1V05_PCH_VR_POUT,
  BIC_SENSOR_1V05_PCH_VR_VOL,
  BIC_SENSOR_SOC_DIMMA0_TEMP,
  BIC_SENSOR_SOC_DIMMA1_TEMP,
  BIC_SENSOR_SOC_DIMMB0_TEMP,
  BIC_SENSOR_SOC_DIMMB1_TEMP,
  BIC_SENSOR_P3V3_MB,
  BIC_SENSOR_P12V_MB,
  BIC_SENSOR_P1V05_PCH,
  BIC_SENSOR_P3V3_STBY_MB,
  BIC_SENSOR_P5V_STBY_MB,
  BIC_SENSOR_PV_BAT,
  BIC_SENSOR_PVDDR,
  BIC_SENSOR_PVCC_GBE,
};

const uint8_t bic_discrete_list[] = {
  /* Discrete sensors */
  BIC_SENSOR_SYSTEM_STATUS,
  BIC_SENSOR_VR_HOT,
  BIC_SENSOR_CPU_DIMM_HOT,
};

// List of IOM Type VII sensors to be monitored
const uint8_t iom_sensor_list[] = {
  /* Threshold sensors */
  IOM_SENSOR_INTEL_TEMP,
  IOM_SENSOR_HSC_POWER,
  IOM_SENSOR_HSC_VOLT,
  IOM_SENSOR_HSC_CURR,
  IOM_SENSOR_M2_1_TEMP,
  IOM_SENSOR_M2_2_TEMP,
  IOM_SENSOR_ADC_12V,
  IOM_SENSOR_ADC_P5V_STBY,
  IOM_SENSOR_ADC_P3V3_STBY,
  IOM_SENSOR_ADC_P1V8_STBY,
  IOM_SENSOR_ADC_P2V5_STBY,
  IOM_SENSOR_ADC_P1V2_STBY,
  IOM_SENSOR_ADC_P1V15_STBY,
  IOM_SENSOR_ADC_P3V3,
  IOM_SENSOR_ADC_P1V8,
  IOM_SENSOR_ADC_P1V5,
  IOM_SENSOR_ADC_P0V975,
};

// List of DPB sensors to be monitored
const uint8_t dpb_sensor_list[] = {
  /* Threshold sensors */
  DPB_SENSOR_12V_POWER_CLIP,
  DPB_SENSOR_P12V_CLIP,
  DPB_SENSOR_12V_CURR_CLIP,
  DPB_SENSOR_FAN0_FRONT,
  DPB_SENSOR_FAN1_FRONT,
  DPB_SENSOR_FAN2_FRONT,
  DPB_SENSOR_FAN3_FRONT,
  DPB_SENSOR_FAN0_REAR,
  DPB_SENSOR_FAN1_REAR,
  DPB_SENSOR_FAN2_REAR,
  DPB_SENSOR_FAN3_REAR,
};

const uint8_t dpb_discrete_list[] = {
  /* Discrete sensors */
  DPB_SENSOR_FAN_STATUS,
};

// List of NIC sensors to be monitored
const uint8_t nic_sensor_list[] = {
  MEZZ_SENSOR_TEMP,
};

float dpb_sensor_threshold[MAX_SENSOR_NUM][MAX_SENSOR_THRESHOLD + 1] = {0};
float iom_sensor_threshold[MAX_SENSOR_NUM][MAX_SENSOR_THRESHOLD + 1] = {0};
float nic_sensor_threshold[MAX_SENSOR_NUM][MAX_SENSOR_THRESHOLD + 1] = {0};

static void
sensor_thresh_array_init() {
  static bool init_done = false;

  if (init_done)
    return;

  nic_sensor_threshold[MEZZ_SENSOR_TEMP][UCR_THRESH] = 95;

  init_done = true;
}

size_t bic_sensor_cnt = sizeof(bic_sensor_list)/sizeof(uint8_t);

size_t bic_discrete_cnt = sizeof(bic_discrete_list)/sizeof(uint8_t);

size_t iom_sensor_cnt = sizeof(iom_sensor_list)/sizeof(uint8_t);

size_t dpb_sensor_cnt = sizeof(dpb_sensor_list)/sizeof(uint8_t);

size_t dpb_discrete_cnt = sizeof(dpb_discrete_list)/sizeof(uint8_t);

size_t nic_sensor_cnt = sizeof(nic_sensor_list)/sizeof(uint8_t);

enum {
  FAN0 = 0,
  FAN1,
};

enum {
  ADC_PIN0 = 0,
  ADC_PIN1,
  ADC_PIN2,
  ADC_PIN3,
  ADC_PIN4,
  ADC_PIN5,
  ADC_PIN6,
  ADC_PIN7,
};

static sensor_info_t g_sinfo[MAX_NUM_FRUS][MAX_SENSOR_NUM] = {0};

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
read_temp(const char *device, float *value) {
  char full_name[LARGEST_DEVICE_NAME + 1];
  int tmp;

  snprintf(
      full_name, LARGEST_DEVICE_NAME, "%s/temp1_input", device);
  if (read_device(full_name, &tmp)) {
    return -1;
  }

  *value = ((float)tmp)/UNIT_DIV;

  return 0;
}

static int
read_fan_value(const int fan, const char *device, float *value) {
  char device_name[LARGEST_DEVICE_NAME];
  char full_name[LARGEST_DEVICE_NAME];

  snprintf(device_name, LARGEST_DEVICE_NAME, device, fan);
  snprintf(full_name, LARGEST_DEVICE_NAME, "%s/%s", TACH_DIR, device_name);
  return read_device_float(full_name, value);
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
  int tmp;

  snprintf(full_name, LARGEST_DEVICE_NAME, "%s/%s", HSC_DEVICE, device);
  if(read_device(full_name, &tmp)) {
    return -1;
  }

  *value = ((float) tmp)/UNIT_DIV;

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
bic_read_sensor_wrapper(uint8_t fru, uint8_t sensor_num, bool discrete,
    void *value) {

  int ret;
  sdr_full_t *sdr;
  ipmi_sensor_reading_t sensor;

  ret = bic_read_sensor(fru, sensor_num, &sensor);
  if (ret) {
    return ret;
  }

  if (sensor.flags & BIC_SENSOR_READ_NA) {
#ifdef DEBUG
    syslog(LOG_ERR, "bic_read_sensor_wrapper: Reading Not Available");
    syslog(LOG_ERR, "bic_read_sensor_wrapper: sensor_num: 0x%X, flag: 0x%X",
        sensor_num, sensor.flags);
#endif
    return -1;
  }

  if (discrete) {
    *(float *) value = (float) sensor.status;
    return 0;
  }

  sdr = &g_sinfo[fru-1][sensor_num].sdr;

  // If the SDR is not type1, no need for conversion
  if (sdr->type !=1) {
    *(float *) value = sensor.value;
    return 0;
  }

  // y = (mx + b * 10^b_exp) * 10^r_exp
  uint8_t x;
  uint8_t m_lsb, m_msb, m;
  uint8_t b_lsb, b_msb, b;
  int8_t b_exp, r_exp;

  x = sensor.value;

  m_lsb = sdr->m_val;
  m_msb = sdr->m_tolerance >> 6;
  m = (m_msb << 8) | m_lsb;

  b_lsb = sdr->b_val;
  b_msb = sdr->b_accuracy >> 6;
  b = (b_msb << 8) | b_lsb;

  // exponents are 2's complement 4-bit number
  b_exp = sdr->rb_exp & 0xF;
  if (b_exp > 7) {
    b_exp = (~b_exp + 1) & 0xF;
    b_exp = -b_exp;
  }
  r_exp = (sdr->rb_exp >> 4) & 0xF;
  if (r_exp > 7) {
    r_exp = (~r_exp + 1) & 0xF;
    r_exp = -r_exp;
  }

  //printf("m:%d, x:%d, b:%d, b_exp:%d, r_exp:%d\n", m, x, b, b_exp, r_exp);

  * (float *) value = ((m * x) + (b * pow(10, b_exp))) * (pow(10, r_exp));

  if ((sensor_num == BIC_SENSOR_SOC_THERM_MARGIN) && (* (float *) value > 0)) {
   * (float *) value -= (float) THERMAL_CONSTANT;
  }

  return 0;
}

int
fbttn_sensor_sdr_path(uint8_t fru, char *path) {

char fru_name[16] = {0};

switch(fru) {
  case FRU_SLOT1:
    sprintf(fru_name, "%s", "slot1");
    break;
  case FRU_IOM:
    sprintf(fru_name, "%s", "iom");
    break;
  case FRU_DPB:
    sprintf(fru_name, "%s", "dpb");
    break;
  case FRU_NIC:
    sprintf(fru_name, "%s", "nic");
    break;
  default:
#ifdef DEBUG
    syslog(LOG_WARNING, "fbttn_sensor_sdr_path: Wrong Slot ID\n");
#endif
    return -1;
}

sprintf(path, FBTTN_SDR_PATH, fru_name);

if (access(path, F_OK) == -1) {
  return -1;
}

return 0;
}

/* Populates all sensor_info_t struct using the path to SDR dump */
int
sdr_init(char *path, sensor_info_t *sinfo) {
int fd;
uint8_t buf[MAX_SDR_LEN] = {0};
uint8_t bytes_rd = 0;
uint8_t snr_num = 0;
sdr_full_t *sdr;

while (access(path, F_OK) == -1) {
  sleep(5);
}

fd = open(path, O_RDONLY);
if (fd < 0) {
  syslog(LOG_ERR, "sdr_init: open failed for %s\n", path);
  return -1;
}

while ((bytes_rd = read(fd, buf, sizeof(sdr_full_t))) > 0) {
  if (bytes_rd != sizeof(sdr_full_t)) {
    syslog(LOG_ERR, "sdr_init: read returns %d bytes\n", bytes_rd);
    return -1;
  }

  sdr = (sdr_full_t *) buf;
  snr_num = sdr->sensor_num;
  sinfo[snr_num].valid = true;
  memcpy(&sinfo[snr_num].sdr, sdr, sizeof(sdr_full_t));
}

return 0;
}

int
fbttn_sensor_sdr_init(uint8_t fru, sensor_info_t *sinfo) {
  int fd;
  uint8_t buf[MAX_SDR_LEN] = {0};
  uint8_t bytes_rd = 0;
  uint8_t sn = 0;
  char path[64] = {0};

  switch(fru) {
    case FRU_SLOT1:
      if (fbttn_sensor_sdr_path(fru, path) < 0) {
#ifdef DEBUG
        syslog(LOG_WARNING, "fbttn_sensor_sdr_init: get_fru_sdr_path failed\n");
#endif
        return ERR_NOT_READY;
      }

      if (sdr_init(path, sinfo) < 0) {
#ifdef DEBUG
        syslog(LOG_ERR, "fbttn_sensor_sdr_init: sdr_init failed for FRU %d", fru);
#endif
      }
      break;

    case FRU_IOM:
    case FRU_DPB:
    case FRU_NIC:
      return -1;
      break;
  }

  return 0;
}

static int
fbttn_sdr_init(uint8_t fru) {

  static bool init_done[MAX_NUM_FRUS] = {false};

  if (!init_done[fru - 1]) {

    sensor_info_t *sinfo = g_sinfo[fru-1];

    if (fbttn_sensor_sdr_init(fru, sinfo) < 0)
      return ERR_NOT_READY;

    init_done[fru - 1] = true;
  }

  return 0;
}

static bool
is_server_prsnt(uint8_t fru) {
  uint8_t gpio;
  int val;
  char path[64] = {0};

  switch(fru) {
  case 1:
    gpio = 61;
    break;
  case 2:
    gpio = 60;
    break;
  case 3:
    gpio = 63;
    break;
  case 4:
    gpio = 62;
    break;
  default:
    return 0;
  }

  sprintf(path, GPIO_VAL, gpio);

  if (read_device(path, &val)) {
    return -1;
  }

  if (val == 0x0) {
    return 1;
  } else {
    return 0;
  }
}

/* Get the units for the sensor */
int
fbttn_sensor_units(uint8_t fru, uint8_t sensor_num, char *units) {
  uint8_t op, modifier;
  sensor_info_t *sinfo;

    if (is_server_prsnt(fru) && (fbttn_sdr_init(fru) != 0)) {
      return -1;
    }

  switch(fru) {
    case FRU_SLOT1:
      sprintf(units, "");
      break;

    case FRU_IOM:
      switch(sensor_num) {
        case IOM_SENSOR_INTEL_TEMP:
          sprintf(units, "C");
          break;
        case IOM_SENSOR_HSC_POWER:
          sprintf(units, "Watts");
          break;
        case IOM_SENSOR_HSC_VOLT:
          sprintf(units, "Volts");
          break;
        case IOM_SENSOR_HSC_CURR:
          sprintf(units, "Amps");
          break;
        case IOM_SENSOR_M2_1_TEMP:
          sprintf(units, "C");
          break;
        case IOM_SENSOR_M2_2_TEMP:
          sprintf(units, "C");
          break;
        case IOM_SENSOR_ADC_12V:
          sprintf(units, "Volts");
          break;
        case IOM_SENSOR_ADC_P5V_STBY:
          sprintf(units, "Volts");
          break;
        case IOM_SENSOR_ADC_P3V3_STBY:
          sprintf(units, "Volts");
          break;
        case IOM_SENSOR_ADC_P1V8_STBY:
          sprintf(units, "Volts");
          break;
        case IOM_SENSOR_ADC_P2V5_STBY:
          sprintf(units, "Volts");
          break;
        case IOM_SENSOR_ADC_P1V2_STBY:
          sprintf(units, "Volts");
          break;
        case IOM_SENSOR_ADC_P1V15_STBY:
          sprintf(units, "Volts");
          break;
        case IOM_SENSOR_ADC_P3V3:
          sprintf(units, "Volts");
          break;
        case IOM_SENSOR_ADC_P1V8:
          sprintf(units, "Volts");
          break;
        case IOM_SENSOR_ADC_P1V5:
          sprintf(units, "Volts");
          break;
        case IOM_SENSOR_ADC_P0V975:
          sprintf(units, "Volts");
          break;
        case IOM_SENSOR_ADC_P3V3_M2:
          sprintf(units, "Volts");
          break;
      }
      break;

    case FRU_DPB:
      switch(sensor_num) {
        case DPB_SENSOR_12V_POWER_CLIP:
          sprintf(units, "Watts");
          break;
        case DPB_SENSOR_P12V_CLIP:
          sprintf(units, "Volts");
          break;
        case DPB_SENSOR_12V_CURR_CLIP:
          sprintf(units, "Amps");
          break;
        case DPB_SENSOR_FAN0_FRONT:
          sprintf(units, "RPM");
          break;
        case DPB_SENSOR_FAN1_FRONT:
          sprintf(units, "RPM");
          break;
        case DPB_SENSOR_FAN2_FRONT:
          sprintf(units, "RPM");
          break;
        case DPB_SENSOR_FAN3_FRONT:
          sprintf(units, "RPM");
          break;
        case DPB_SENSOR_FAN0_REAR:
          sprintf(units, "RPM");
          break;
        case DPB_SENSOR_FAN1_REAR:
          sprintf(units, "RPM");
          break;
        case DPB_SENSOR_FAN2_REAR:
          sprintf(units, "RPM");
          break;
        case DPB_SENSOR_FAN3_REAR:
          sprintf(units, "RPM");
          break;
        case DPB_SENSOR_FAN_STATUS:
          sprintf(units, "");
          break;
      }
      break;

    case FRU_NIC:
      switch(sensor_num) {
        case MEZZ_SENSOR_TEMP:
          sprintf(units, "C");
          break;
      }
      break;
  }
  return 0;
}

int
fbttn_sensor_threshold(uint8_t fru, uint8_t sensor_num, uint8_t thresh, float *value) {

  sensor_thresh_array_init();

  switch(fru) {
    case FRU_SLOT1:
      break;
    case FRU_IOM:
      *value = iom_sensor_threshold[sensor_num][thresh];
      break;
    case FRU_DPB:
      *value = dpb_sensor_threshold[sensor_num][thresh];
      break;
    case FRU_NIC:
      *value = nic_sensor_threshold[sensor_num][thresh];
      break;
  }
  return 0;
}

/* Get the name for the sensor */
int
fbttn_sensor_name(uint8_t fru, uint8_t sensor_num, char *name) {

  switch(fru) {
    case FRU_SLOT1:
      switch(sensor_num) {
        case BIC_SENSOR_SYSTEM_STATUS:
          sprintf(name, "SYSTEM_STATUS");
          break;
        case BIC_SENSOR_SYS_BOOT_STAT:
          sprintf(name, "SYS_BOOT_STAT");
          break;
        case BIC_SENSOR_CPU_DIMM_HOT:
          sprintf(name, "CPU_DIMM_HOT");
          break;
        case BIC_SENSOR_PROC_FAIL:
          sprintf(name, "PROC_FAIL");
          break;
        case BIC_SENSOR_VR_HOT:
          sprintf(name, "VR_HOT");
          break;
        default:
          sprintf(name, "");
          break;
      }
      break;

    case FRU_IOM:
      switch(sensor_num) {
        case IOM_SENSOR_INTEL_TEMP:
          sprintf(name, "IOM_INTEL_TEMP");
          break;
        case IOM_SENSOR_HSC_POWER:
          sprintf(name, "IOM_HSC_POWER");
          break;
        case IOM_SENSOR_HSC_VOLT:
          sprintf(name, "IOM_HSC_VOLT");
          break;
        case IOM_SENSOR_HSC_CURR:
          sprintf(name, "IOM_HSC_CURR");
          break;
        case IOM_SENSOR_M2_1_TEMP:
          sprintf(name, "M2_1_TEMP");
          break;
        case IOM_SENSOR_M2_2_TEMP:
          sprintf(name, "M2_2_TEMP");
          break;
        case IOM_SENSOR_ADC_12V:
          sprintf(name, "ADC_12V");
          break;
        case IOM_SENSOR_ADC_P5V_STBY:
          sprintf(name, "ADC_P5V_STBY");
          break;
        case IOM_SENSOR_ADC_P3V3_STBY:
          sprintf(name, "ADC_P3V3_STBY");
          break;
        case IOM_SENSOR_ADC_P1V8_STBY:
          sprintf(name, "ADC_P1V8_STBY");
          break;
        case IOM_SENSOR_ADC_P2V5_STBY:
          sprintf(name, "ADC_P2V5_STBY");
          break;
        case IOM_SENSOR_ADC_P1V2_STBY:
          sprintf(name, "ADC_P1V2_STBY");
          break;
        case IOM_SENSOR_ADC_P1V15_STBY:
          sprintf(name, "ADC_P1V15_STBY");
          break;
        case IOM_SENSOR_ADC_P3V3:
          sprintf(name, "ADC_P3V3");
          break;
        case IOM_SENSOR_ADC_P1V8:
          sprintf(name, "ADC_P1V8");
          break;
        case IOM_SENSOR_ADC_P1V5:
          sprintf(name, "ADC_P1V5");
          break;
        case IOM_SENSOR_ADC_P0V975:
          sprintf(name, "ADC_P0V975");
          break;
        case IOM_SENSOR_ADC_P3V3_M2:
          sprintf(name, "ADC_P3V3_M2");
          break;
      }
      break;

    case FRU_DPB:
      switch(sensor_num) {
        case DPB_SENSOR_12V_POWER_CLIP:
          sprintf(name, "12V_POWER_CLIP");
          break;
        case DPB_SENSOR_P12V_CLIP:
          sprintf(name, "P12V_CLIP");
          break;
        case DPB_SENSOR_12V_CURR_CLIP:
          sprintf(name, "12V_CURR_CLIP");
          break;
        case DPB_SENSOR_FAN0_FRONT:
          sprintf(name, "FAN0_FRONT");
          break;
        case DPB_SENSOR_FAN1_FRONT:
          sprintf(name, "FAN1_FRONT");
          break;
        case DPB_SENSOR_FAN2_FRONT:
          sprintf(name, "FAN2_FRONT");
          break;
        case DPB_SENSOR_FAN3_FRONT:
          sprintf(name, "FAN3_FRONT");
          break;
        case DPB_SENSOR_FAN0_REAR:
          sprintf(name, "FAN0_REAR");
          break;
        case DPB_SENSOR_FAN1_REAR:
          sprintf(name, "FAN1_REAR");
          break;
        case DPB_SENSOR_FAN2_REAR:
          sprintf(name, "FAN2_REAR");
          break;
        case DPB_SENSOR_FAN3_REAR:
          sprintf(name, "FAN3_REAR");
          break;
        case DPB_SENSOR_FAN_STATUS:
          sprintf(name, "FAN_STATUS");
          break;
      }
      break;

    case FRU_NIC:
      switch(sensor_num) {
        case MEZZ_SENSOR_TEMP:
          sprintf(name, "MEZZ_SENSOR_TEMP");
          break;
      }
      break;
  }
  return 0;
}


int
fbttn_sensor_read(uint8_t fru, uint8_t sensor_num, void *value) {

  float volt;
  float curr;
  int ret;
  bool discrete;
  int i;

  switch (fru) {
    case FRU_SLOT1:

      if (!(is_server_prsnt(fru))) {
        return -1;
      }

      ret = fbttn_sdr_init(fru);
      if (ret < 0) {
        return ret;
      }

      discrete = false;

      i = 0;
      while (i < bic_discrete_cnt) {
        if (sensor_num == bic_discrete_list[i++]) {
          discrete = true;
          break;
        }
      }

      return bic_read_sensor_wrapper(fru, sensor_num, discrete, value);

    // TODO: Add read functions for every Type VII sensors.
    case FRU_IOM:
      switch(sensor_num) {
        case IOM_SENSOR_INTEL_TEMP:
        case IOM_SENSOR_HSC_POWER:
        case IOM_SENSOR_HSC_VOLT:
        case IOM_SENSOR_HSC_CURR:
        case IOM_SENSOR_M2_1_TEMP:
        case IOM_SENSOR_M2_2_TEMP:
        case IOM_SENSOR_ADC_12V:
        case IOM_SENSOR_ADC_P5V_STBY:
        case IOM_SENSOR_ADC_P3V3_STBY:
        case IOM_SENSOR_ADC_P1V8_STBY:
        case IOM_SENSOR_ADC_P2V5_STBY:
        case IOM_SENSOR_ADC_P1V2_STBY:
        case IOM_SENSOR_ADC_P1V15_STBY:
        case IOM_SENSOR_ADC_P3V3:
        case IOM_SENSOR_ADC_P1V8:
        case IOM_SENSOR_ADC_P1V5:
        case IOM_SENSOR_ADC_P0V975:
          *(float *) value = 0;
          return 0;
      }
      break;

    // TODO: Add read functions for every DPB sensors.
    case FRU_DPB:
      switch(sensor_num) {
        case DPB_SENSOR_12V_POWER_CLIP:
        case DPB_SENSOR_P12V_CLIP:
        case DPB_SENSOR_12V_CURR_CLIP:
        case DPB_SENSOR_FAN0_FRONT:
        case DPB_SENSOR_FAN1_FRONT:
        case DPB_SENSOR_FAN2_FRONT:
        case DPB_SENSOR_FAN3_FRONT:
        case DPB_SENSOR_FAN0_REAR:
        case DPB_SENSOR_FAN1_REAR:
        case DPB_SENSOR_FAN2_REAR:
        case DPB_SENSOR_FAN3_REAR:
        case DPB_SENSOR_FAN_STATUS:
          *(float *) value = 0;
          return 0;
      }
      break;

    case FRU_NIC:
      switch(sensor_num) {
      // Mezz Temp
        case MEZZ_SENSOR_TEMP:
          return read_nic_temp(MEZZ_TEMP_DEVICE, (float*) value);
      }
      break;
  }
}

