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
#include <fcntl.h>
#include <errno.h>
#include <syslog.h>
#include "yosemite_sensor.h"

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

#define UNIT_DIV 1000

#define BIC_SENSOR_READ_NA 0x20

#define MAX_SENSOR_NUM 0xFF
#define ALL_BYTES 0xFF
#define LAST_REC_ID 0xFFFF

#define YOSEMITE_SDR_PATH "/tmp/sdr_%s.bin"

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
  /* Discrete sensors */
  //BIC_SENSOR_SYSTEM_STATUS,
  //BIC_SENSOR_SYS_BOOT_STAT,
  //BIC_SENSOR_CPU_DIMM_HOT,
  //BIC_SENSOR_PROC_FAIL,
  //BIC_SENSOR_VR_HOT,
  /* Event-only sensors */
  //BIC_SENSOR_POST_ERR,
  //BIC_SENSOR_SPS_FW_HLTH,
  //BIC_SENSOR_POWER_THRESH_EVENT,
  //BIC_SENSOR_MACHINE_CHK_ERR,
  //BIC_SENSOR_PCIE_ERR,
  //BIC_SENSOR_OTHER_IIO_ERR,
  //BIC_SENSOR_PROC_HOT_EXT,
  //BIC_SENSOR_POWER_ERR,
  //BIC_SENSOR_CAT_ERR,
};

// List of SPB sensors to be monitored
const uint8_t spb_sensor_list[] = {
  SP_SENSOR_INLET_TEMP,
  SP_SENSOR_OUTLET_TEMP,
  //SP_SENSOR_MEZZ_TEMP
  SP_SENSOR_FAN0_TACH,
  SP_SENSOR_FAN1_TACH,
  //SP_SENSOR_AIR_FLOW,
  SP_SENSOR_P5V,
  SP_SENSOR_P12V,
  SP_SENSOR_P3V3_STBY,
  SP_SENSOR_P12V_SLOT0,
  SP_SENSOR_P12V_SLOT1,
  SP_SENSOR_P12V_SLOT2,
  SP_SENSOR_P12V_SLOT3,
  SP_SENSOR_P3V3,
  SP_SENSOR_HSC_IN_VOLT,
  SP_SENSOR_HSC_OUT_CURR,
  SP_SENSOR_HSC_TEMP,
  SP_SENSOR_HSC_IN_POWER,
};

size_t bic_sensor_cnt = sizeof(bic_sensor_list)/sizeof(uint8_t);

size_t spb_sensor_cnt = sizeof(spb_sensor_list)/sizeof(uint8_t);

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

static sensor_info_t g_sinfo1[MAX_SENSOR_NUM] = {0};
static sensor_info_t g_sinfo2[MAX_SENSOR_NUM] = {0};
static sensor_info_t g_sinfo3[MAX_SENSOR_NUM] = {0};
static sensor_info_t g_sinfo4[MAX_SENSOR_NUM] = {0};
static sensor_info_t g_sinfo_spb[MAX_SENSOR_NUM] = {0};
static sensor_info_t g_sinfo_nic[MAX_SENSOR_NUM] = {0};

static int
read_device(const char *device, int *value) {
  FILE *fp;
  int rc;

  fp = fopen(device, "r");
  if (!fp) {
    int err = errno;

    syslog(LOG_INFO, "failed to open device %s", device);
    return err;
  }

  rc = fscanf(fp, "%d", value);
  fclose(fp);

  if (rc != 1) {
    syslog(LOG_INFO, "failed to read device %s", device);
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

    syslog(LOG_INFO, "failed to open device %s", device);
    return err;
  }

  rc = fscanf(fp, "%s", tmp);
  fclose(fp);

  if (rc != 1) {
    syslog(LOG_INFO, "failed to read device %s", device);
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
bic_read_sensor_wrapper(uint8_t slot_id, uint8_t sensor_num, void *value) {
  int ret;
  ipmi_sensor_reading_t sensor;

  ret = bic_read_sensor(slot_id, sensor_num, &sensor);
  if (ret) {
    return ret;
  }

  if (sensor.flags & BIC_SENSOR_READ_NA) {
    syslog(LOG_ERR, "bic_read_sensor_wrapper: Reading Not Available");
    syslog(LOG_ERR, "bic_read_sensor_wrapper: sensor_num: 0x%X, flag: 0x%X",
        sensor_num, sensor.flags);
    return -1;
  }

  if (sensor.status) {
    //printf("bic_read_sensor_wrapper: Status Asserted: 0x%X\n", sensor.status);
  }

  // Check SDR to convert raw value to actual
  sdr_full_t *sdr;

  switch (slot_id) {
  case 1:
    sdr = &g_sinfo1[sensor_num].sdr;
    break;
  case 2:
    sdr = &g_sinfo2[sensor_num].sdr;
    break;
  case 3:
    sdr = &g_sinfo3[sensor_num].sdr;
    break;
  case 4:
    sdr = &g_sinfo4[sensor_num].sdr;
    break;
  default:
    syslog(LOG_ALERT, "bic_read_sensor_wrapper: Wrong Slot ID\n");
    return -1;
  }

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

  return 0;
}

/* Returns the all the SDRs for the particular fru# */
static sensor_info_t *
get_struct_sensor_info(uint8_t fru) {
  sensor_info_t *sinfo;
  switch(fru) {
  case FRU_SLOT1:
    sinfo = g_sinfo1;
    break;
  case FRU_SLOT2:
    sinfo = g_sinfo2;
    break;
  case FRU_SLOT3:
    sinfo = g_sinfo3;
    break;
  case FRU_SLOT4:
    sinfo = g_sinfo4;
    break;
  case FRU_SPB:
    sinfo = g_sinfo_spb;
    break;
  case FRU_NIC:
    sinfo = g_sinfo_nic;
    break;
  default:
    syslog(LOG_ALERT, "yosemite_sdr_init: Wrong Slot ID\n");
    return NULL;
  }
  return sinfo;
}

int
get_fru_sdr_path(uint8_t fru, char *path) {

  char fru_name[16] = {0};

  switch(fru) {
    case FRU_SLOT1:
      sprintf(fru_name, "%s", "slot1");
      break;
    case FRU_SLOT2:
      sprintf(fru_name, "%s", "slot2");
      break;
    case FRU_SLOT3:
      sprintf(fru_name, "%s", "slot3");
      break;
    case FRU_SLOT4:
      sprintf(fru_name, "%s", "slot4");
      break;
    case FRU_SPB:
      sprintf(fru_name, "%s", "spb");
      break;
    case FRU_NIC:
      sprintf(fru_name, "%s", "nic");
      break;
    default:
      syslog(LOG_ALERT, "yosemite_sdr_init: Wrong Slot ID\n");
      return -1;
  }

  sprintf(path, YOSEMITE_SDR_PATH, fru_name);

  return 0;
}

static int
yosemite_sdr_init(uint8_t fru) {
  int fd;
  uint8_t buf[MAX_SDR_LEN] = {0};
  uint8_t bytes_rd = 0;
  uint8_t sn = 0;
  char path[64] = {0};
  sensor_info_t *sinfo;

  if (get_fru_sdr_path(fru, path) < 0) {
    syslog(LOG_ALERT, "yosemite_sdr_init: get_fru_sdr_path failed\n");
    return -1;
  }
  sinfo = get_struct_sensor_info(fru);
  if (sinfo == NULL) {
    syslog(LOG_ALERT, "yosemite_sdr_init: get_struct_sensor_info failed\n");
    return -1;
  }

  if (sdr_init(path, sinfo) < 0) {
    syslog(LOG_ERR, "yosemite_sdr_init: sdr_init failed for FRU %d", fru);
  }

  return 0;
}

static bool
is_server_prsnt(uint8_t slot_id) {
  uint8_t gpio;
  int val;
  char path[64] = {0};

  switch(slot_id) {
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
yosemite_sensor_units(uint8_t fru, uint8_t sensor_num, char *units) {
  static bool init_done = false;
  uint8_t op, modifier;
  sensor_info_t *sinfo;

  if (!init_done) {

    if (is_server_prsnt(1) && (yosemite_sdr_init(FRU_SLOT1) != 0)) {
      return -1;
    }
    if (is_server_prsnt(2) && (yosemite_sdr_init(FRU_SLOT2) != 0)) {
      return -1;
    }
    if (is_server_prsnt(3) && (yosemite_sdr_init(FRU_SLOT3) != 0)) {
      return -1;
    }
    if (is_server_prsnt(4) && (yosemite_sdr_init(FRU_SLOT4) != 0)) {
      return -1;
    }
    init_done = true;
  }

  switch(fru) {
    case FRU_SLOT1:
    case FRU_SLOT2:
    case FRU_SLOT3:
    case FRU_SLOT4:
      sinfo = get_struct_sensor_info(fru);
      if (sinfo == NULL) {
        syslog(LOG_ALERT, "yosemite_sensor_units: get_struct_sensor_info failed\n");
        return -1;
      }

      if (sdr_get_sensor_units(&sinfo[sensor_num].sdr, &op, &modifier, units)) {
        syslog(LOG_ALERT, "yosemite_sensor_units: FRU %d: num 0x%2X: reading units"
            " from SDR failed.", fru, sensor_num);
        return -1;
      }
    break;
    case FRU_SPB:
      switch(sensor_num) {
        case SP_SENSOR_INLET_TEMP:
          sprintf(units, "C");
          break;
        case SP_SENSOR_OUTLET_TEMP:
          sprintf(units, "C");
          break;
        case SP_SENSOR_MEZZ_TEMP:
          sprintf(units, "C");
          break;
        case SP_SENSOR_FAN0_TACH:
          sprintf(units, "RPM");
          break;
        case SP_SENSOR_FAN1_TACH:
          sprintf(units, "RPM");
          break;
        case SP_SENSOR_AIR_FLOW:
          sprintf(units, "");
          break;
        case SP_SENSOR_P5V:
          sprintf(units, "Volts");
          break;
        case SP_SENSOR_P12V:
          sprintf(units, "Volts");
          break;
        case SP_SENSOR_P3V3_STBY:
          sprintf(units, "Volts");
          break;
        case SP_SENSOR_P12V_SLOT0:
          sprintf(units, "Volts");
          break;
        case SP_SENSOR_P12V_SLOT1:
          sprintf(units, "Volts");
          break;
        case SP_SENSOR_P12V_SLOT2:
          sprintf(units, "Volts");
          break;
        case SP_SENSOR_P12V_SLOT3:
          sprintf(units, "Volts");
          break;
        case SP_SENSOR_P3V3:
          sprintf(units, "Volts");
          break;
        case SP_SENSOR_HSC_IN_VOLT:
          sprintf(units, "Volts");
          break;
        case SP_SENSOR_HSC_OUT_CURR:
          sprintf(units, "Amps");
          break;
        case SP_SENSOR_HSC_TEMP:
          sprintf(units, "C");
          break;
        case SP_SENSOR_HSC_IN_POWER:
          sprintf(units, "Watts");
          break;
      }
      break;
    case FRU_NIC:
      sprintf(units, "");
    break;
  }
  return 0;
}

/* Get the name for the sensor */
int
yosemite_sensor_name(uint8_t fru, uint8_t sensor_num, char *name) {
  static bool init_done = false;
  uint8_t op, modifier;
  sensor_info_t *sinfo;

  if (!init_done) {

    if (is_server_prsnt(1) && (yosemite_sdr_init(FRU_SLOT1) != 0)) {
      return -1;
    }
    if (is_server_prsnt(2) && (yosemite_sdr_init(FRU_SLOT2) != 0)) {
      return -1;
    }
    if (is_server_prsnt(3) && (yosemite_sdr_init(FRU_SLOT3) != 0)) {
      return -1;
    }
    if (is_server_prsnt(4) && (yosemite_sdr_init(FRU_SLOT4) != 0)) {
      return -1;
    }
    init_done = true;
  }

  switch(fru) {
    case FRU_SLOT1:
    case FRU_SLOT2:
    case FRU_SLOT3:
    case FRU_SLOT4:
      sinfo = get_struct_sensor_info(fru);
      if (sinfo == NULL) {
        syslog(LOG_ALERT, "yosemite_sensor_name: get_struct_sensor_info failed\n");
        return -1;
      }

      if (sdr_get_sensor_name(&sinfo[sensor_num].sdr, name)) {
        syslog(LOG_ALERT, "yosemite_sensor_name: FRU %d: num 0x%2X: reading units"
            " from SDR failed.", fru, sensor_num);
        return -1;
      }

      break;
    case FRU_SPB:
      switch(sensor_num) {
        case SP_SENSOR_INLET_TEMP:
          sprintf(name, "SP_SENSOR_INLET_TEMP");
          break;
        case SP_SENSOR_OUTLET_TEMP:
          sprintf(name, "SP_SENSOR_OUTLET_TEMP");
          break;
        case SP_SENSOR_MEZZ_TEMP:
          sprintf(name, "SP_SENSOR_MEZZ_TEMP");
          break;
        case SP_SENSOR_FAN0_TACH:
          sprintf(name, "SP_SENSOR_FAN0_TACH");
          break;
        case SP_SENSOR_FAN1_TACH:
          sprintf(name, "SP_SENSOR_FAN1_TACH");
          break;
        case SP_SENSOR_AIR_FLOW:
          sprintf(name, "SP_SENSOR_AIR_FLOW");
          break;
        case SP_SENSOR_P5V:
          sprintf(name, "SP_SENSOR_P5V");
          break;
        case SP_SENSOR_P12V:
          sprintf(name, "SP_SENSOR_P12V");
          break;
        case SP_SENSOR_P3V3_STBY:
          sprintf(name, "SP_SENSOR_P3V3_STBY");
          break;
        case SP_SENSOR_P12V_SLOT0:
          sprintf(name, "SP_SENSOR_P12V_SLOT0");
          break;
        case SP_SENSOR_P12V_SLOT1:
          sprintf(name, "SP_SENSOR_P12V_SLOT1");
          break;
        case SP_SENSOR_P12V_SLOT2:
          sprintf(name, "SP_SENSOR_P12V_SLOT2");
          break;
        case SP_SENSOR_P12V_SLOT3:
          sprintf(name, "SP_SENSOR_P12V_SLOT3");
          break;
        case SP_SENSOR_P3V3:
          sprintf(name, "SP_SENSOR_P3V3");
          break;
        case SP_SENSOR_HSC_IN_VOLT:
          sprintf(name, "SP_SENSOR_HSC_IN_VOLT");
          break;
        case SP_SENSOR_HSC_OUT_CURR:
          sprintf(name, "SP_SENSOR_HSC_OUT_CURR");
          break;
        case SP_SENSOR_HSC_TEMP:
          sprintf(name, "SP_SENSOR_HSC_TEMP");
          break;
        case SP_SENSOR_HSC_IN_POWER:
          sprintf(name, "SP_SENSOR_HSC_IN_POWER");
          break;
      }
      break;
    case FRU_NIC:
      sprintf(name, "");
      break;
  }
  return 0;
}


int
yosemite_sensor_read(uint8_t slot_id, uint8_t sensor_num, void *value) {
  static bool init_done = false;
  float volt;
  float curr;

  if (!init_done) {

    if (is_server_prsnt(1) && (yosemite_sdr_init(FRU_SLOT1) != 0)) {
      return -1;
    }

    if (is_server_prsnt(2) && (yosemite_sdr_init(FRU_SLOT2) != 0)) {
      return -1;
    }

    if (is_server_prsnt(3) && (yosemite_sdr_init(FRU_SLOT3) != 0)) {
      return -1;
    }

    if (is_server_prsnt(4) && (yosemite_sdr_init(FRU_SLOT4) != 0)) {
      return -1;
    }

    init_done = true;
  }

  switch(sensor_num) {
  // Inlet, Outlet Temp

  case SP_SENSOR_INLET_TEMP:
    return read_temp(SP_INLET_TEMP_DEVICE, (float*) value);
  case SP_SENSOR_OUTLET_TEMP:
    return read_temp(SP_OUTLET_TEMP_DEVICE, (float*) value);

  // Fan Tach Values
  case SP_SENSOR_FAN0_TACH:
    return read_fan_value(FAN0, FAN_TACH_RPM, (float*) value);
  case SP_SENSOR_FAN1_TACH:
    return read_fan_value(FAN1, FAN_TACH_RPM, (float*) value);

  // Various Voltages
  case SP_SENSOR_P5V:
    return read_adc_value(ADC_PIN0, ADC_VALUE, (float*) value);
  case SP_SENSOR_P12V:
    return read_adc_value(ADC_PIN1, ADC_VALUE, (float*) value);
  case SP_SENSOR_P3V3_STBY:
    return read_adc_value(ADC_PIN2, ADC_VALUE, (float*) value);
  case SP_SENSOR_P12V_SLOT0:
    return read_adc_value(ADC_PIN3, ADC_VALUE, (float*) value);
  case SP_SENSOR_P12V_SLOT1:
    return read_adc_value(ADC_PIN4, ADC_VALUE, (float*) value);
  case SP_SENSOR_P12V_SLOT2:
    return read_adc_value(ADC_PIN5, ADC_VALUE, (float*) value);
  case SP_SENSOR_P12V_SLOT3:
    return read_adc_value(ADC_PIN6, ADC_VALUE, (float*) value);
  case SP_SENSOR_P3V3:
    return read_adc_value(ADC_PIN7, ADC_VALUE, (float*) value);

  // Hot Swap Controller
  case SP_SENSOR_HSC_IN_VOLT:
    return read_hsc_value(HSC_IN_VOLT, (float*) value);
  case SP_SENSOR_HSC_OUT_CURR:
    return read_hsc_value(HSC_OUT_CURR, (float*) value);
  case SP_SENSOR_HSC_TEMP:
    return read_hsc_value(HSC_TEMP, (float*) value);
  case SP_SENSOR_HSC_IN_POWER:
    if (read_hsc_value(HSC_IN_VOLT, &volt)) {
      return -1;
    }

    if (read_hsc_value(HSC_OUT_CURR, &curr)) {
      return -1;
    }

    * (float*) value = volt * curr;
    return 0;
  default:
    // For all others we assume the sensors are on Monolake
    return bic_read_sensor_wrapper(slot_id, sensor_num, value);
  }
}

