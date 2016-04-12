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
#include "lightning_sensor.h"

#define LARGEST_DEVICE_NAME 128

#define I2C_DEV_PEB       "/dev/i2c-4"
#define I2C_DEV_PDPB      "/dev/i2c-6"
#define I2C_DEV_FCB       "/dev/i2c-5"

#define I2C_ADDR_PEB_HSC 0x11

#define I2C_ADDR_PDPB_ADC  0x48

#define I2C_ADDR_FCB_HSC  0x22
#define I2C_ADDR_NCT7904  0x2d

#define ADC_DIR "/sys/devices/platform/ast_adc.0"
#define ADC_VALUE "adc%d_value"

#define UNIT_DIV 1000

#define TPM75_TEMP_RESOLUTION 0.0625

#define ADS1015_DEFAULT_CONFIG 0xe383

#define MAX_SENSOR_NUM 0xFF
#define ALL_BYTES 0xFF
#define LAST_REC_ID 0xFFFF

enum temp_sensor_type {
  LOCAL_SENSOR = 0,
  REMOTE_SENSOR,
};
enum tmp75_peb_sensors {
  PEB_TMP75_U136 = 0x4d,
  PEB_TMP75_U134 = 0x4a,
  PEB_TMP421_U15 = 0x4c,
  PEB_MAX6654_U4 = 0x18,
};

enum tmp75_pdpb_sensors {
  PDPB_TMP75_U47 = 0x49,
  PDPB_TMP75_U48 = 0x4a,
  PDPB_TMP75_U49 = 0x4b,
  PDPB_TMP75_U51 = 0x4c,
};

enum nct7904_registers {
  NCT7904_TEMP_CH1 = 0x42,
  NCT7904_TEMP_CH2 = 0x46,
  NCT7904_VSEN6 = 0x4A,
  NCT7904_VSEN7 = 0x4C,
  NCT7904_VSEN9 = 0x50,
  NCT7904_3VDD = 0x5C,
  NCT7904_BANK_SEL = 0xFF,
};

enum hsc_controllers {
  HSC_ADM1276 = 0,
  HSC_ADM1278,
};

enum hsc_commands {
  HSC_IN_VOLT = 0x88,
  HSC_OUT_CURR = 0x8c,
  HSC_IN_POWER = 0x97,
  HSC_TEMP = 0x8D,
};

enum ads1015_registers {
  ADS1015_CONVERSION = 0,
  ADS1015_CONFIG,
};

enum ads1015_channels {
  ADS1015_CHANNEL0 = 0,
  ADS1015_CHANNEL1,
  ADS1015_CHANNEL2,
  ADS1015_CHANNEL3,
  ADS1015_CHANNEL4,
  ADS1015_CHANNEL5,
  ADS1015_CHANNEL6,
  ADS1015_CHANNEL7,
};

enum adc_pins {
  ADC_PIN0 = 0,
  ADC_PIN1,
  ADC_PIN2,
  ADC_PIN3,
  ADC_PIN4,
  ADC_PIN5,
  ADC_PIN6,
  ADC_PIN7,
};

// List of PEB sensors to be monitored
const uint8_t peb_sensor_list[] = {
  PEB_SENSOR_ADC_P12V,
  PEB_SENSOR_ADC_P5V,
  PEB_SENSOR_ADC_P3V3_STBY,
  PEB_SENSOR_ADC_P1V8_STBY,
  PEB_SENSOR_ADC_P1V53,
  PEB_SENSOR_ADC_P0V9,
  PEB_SENSOR_ADC_P0V9_E,
  PEB_SENSOR_ADC_P1V26,
  PEB_SENSOR_HSC_IN_VOLT,
  PEB_SENSOR_HSC_OUT_CURR,
  PEB_SENSOR_HSC_IN_POWER,
  PEB_SENSOR_PCIE_SW_TEMP,
  PEB_SENSOR_PCIE_SW_FRONT_TEMP,
  PEB_SENSOR_PCIE_SW_REAR_TEMP,
  PEB_SENSOR_LEFT_CONN_TEMP,
  PEB_SENSOR_RIGHT_CONN_TEMP,
  PEB_SENSOR_BMC_TEMP,
  PEB_SENSOR_HSC_TEMP,
};

// List of PDPB sensors to be monitored
const uint8_t pdpb_sensor_list[] = {
  PDPB_SENSOR_P12V,
  PDPB_SENSOR_P3V3,
  PDPB_SENSOR_P2V5,
  PDPB_SENSOR_P12V_SSD,
  PDPB_SENSOR_LEFT_REAR_TEMP,
  PDPB_SENSOR_LEFT_FRONT_TEMP,
  PDPB_SENSOR_RIGHT_REAR_TEMP,
  PDPB_SENSOR_RIGHT_FRONT_TEMP,
  PDPB_SENSOR_FLASH_TEMP_0,
  PDPB_SENSOR_FLASH_TEMP_1,
  PDPB_SENSOR_FLASH_TEMP_2,
  PDPB_SENSOR_FLASH_TEMP_3,
  PDPB_SENSOR_FLASH_TEMP_4,
  PDPB_SENSOR_FLASH_TEMP_5,
  PDPB_SENSOR_FLASH_TEMP_6,
  PDPB_SENSOR_FLASH_TEMP_7,
  PDPB_SENSOR_FLASH_TEMP_8,
  PDPB_SENSOR_FLASH_TEMP_9,
  PDPB_SENSOR_FLASH_TEMP_10,
  PDPB_SENSOR_FLASH_TEMP_11,
  PDPB_SENSOR_FLASH_TEMP_12,
  PDPB_SENSOR_FLASH_TEMP_13,
  PDPB_SENSOR_FLASH_TEMP_14,
};

// List of NIC sensors to be monitored
const uint8_t fcb_sensor_list[] = {
  FCB_SENSOR_P12V_AUX,
  FCB_SENSOR_P12VL,
  FCB_SENSOR_P12VU,
  FCB_SENSOR_P3V3,
  FCB_SENSOR_HSC_IN_VOLT,
  FCB_SENSOR_HSC_OUT_CURR,
  FCB_SENSOR_HSC_IN_POWER,
  FCB_SENSOR_BJT_TEMP_1,
  FCB_SENSOR_BJT_TEMP_2,
};

static sensor_info_t g_sinfo[MAX_NUM_FRUS][MAX_SENSOR_NUM] = {0};

float peb_sensor_threshold[MAX_SENSOR_NUM][MAX_SENSOR_THRESHOLD + 1] = {0};
float pdpb_sensor_threshold[MAX_SENSOR_NUM][MAX_SENSOR_THRESHOLD + 1] = {0};
float fcb_sensor_threshold[MAX_SENSOR_NUM][MAX_SENSOR_THRESHOLD + 1] = {0};

static void
sensor_thresh_array_init() {

  static bool init_done = false;

  if (init_done)
    return;

  int i;

  // PEB SENSOR
  peb_sensor_threshold[PEB_SENSOR_HSC_IN_VOLT][UCR_THRESH] = 14.34; // TODO: HACK Value
  peb_sensor_threshold[PEB_SENSOR_HSC_OUT_CURR][UCR_THRESH] = 10.5; // TODO: HACK Value
  peb_sensor_threshold[PEB_SENSOR_HSC_TEMP][UCR_THRESH] = 50; // TODO: HACK value
  peb_sensor_threshold[PEB_SENSOR_HSC_IN_POWER][UCR_THRESH] = 150; // TODO: HACK Value
  peb_sensor_threshold[PEB_SENSOR_ADC_P12V][UCR_THRESH] = 16.25; // Abnormal
  peb_sensor_threshold[PEB_SENSOR_ADC_P5V][UCR_THRESH] = 10.66; // Abnormal
  peb_sensor_threshold[PEB_SENSOR_ADC_P3V3_STBY][UCR_THRESH] = 4.95; // Abnormal
  peb_sensor_threshold[PEB_SENSOR_ADC_P1V8_STBY][UCR_THRESH] = 2.46; // Abnormal
  peb_sensor_threshold[PEB_SENSOR_ADC_P1V53][UCR_THRESH] = 2.46; // Abnormal
  peb_sensor_threshold[PEB_SENSOR_ADC_P0V9][UCR_THRESH] = 2.46; // Abnormal
  peb_sensor_threshold[PEB_SENSOR_ADC_P0V9_E][UCR_THRESH] = 2.46; // Abnormal
  peb_sensor_threshold[PEB_SENSOR_ADC_P1V26][UCR_THRESH] = 2.46; // Abnormal

  // PEB TEMP SENSORS
  peb_sensor_threshold[PEB_SENSOR_PCIE_SW_TEMP][UCR_THRESH] = 100;
  peb_sensor_threshold[PEB_SENSOR_PCIE_SW_FRONT_TEMP][UCR_THRESH] = 50;
  peb_sensor_threshold[PEB_SENSOR_PCIE_SW_REAR_TEMP][UCR_THRESH] = 50;
  peb_sensor_threshold[PEB_SENSOR_LEFT_CONN_TEMP][UCR_THRESH] = 50;
  peb_sensor_threshold[PEB_SENSOR_RIGHT_CONN_TEMP][UCR_THRESH] = 50;
  peb_sensor_threshold[PEB_SENSOR_BMC_TEMP][UCR_THRESH] = 50;

  peb_sensor_threshold[PEB_SENSOR_PCIE_SW_TEMP][UNC_THRESH] = 95;
  peb_sensor_threshold[PEB_SENSOR_PCIE_SW_FRONT_TEMP][UNC_THRESH] = 45;
  peb_sensor_threshold[PEB_SENSOR_PCIE_SW_REAR_TEMP][UNC_THRESH] = 45;
  peb_sensor_threshold[PEB_SENSOR_LEFT_CONN_TEMP][UNC_THRESH] = 45;
  peb_sensor_threshold[PEB_SENSOR_RIGHT_CONN_TEMP][UNC_THRESH] = 45;
  peb_sensor_threshold[PEB_SENSOR_BMC_TEMP][UNC_THRESH] = 45;

  peb_sensor_threshold[PEB_SENSOR_PCIE_SW_TEMP][LNC_THRESH] = 10;
  peb_sensor_threshold[PEB_SENSOR_PCIE_SW_FRONT_TEMP][LNC_THRESH] = 10;
  peb_sensor_threshold[PEB_SENSOR_PCIE_SW_REAR_TEMP][LNC_THRESH] = 10;
  peb_sensor_threshold[PEB_SENSOR_LEFT_CONN_TEMP][LNC_THRESH] = 10;
  peb_sensor_threshold[PEB_SENSOR_RIGHT_CONN_TEMP][LNC_THRESH] = 10;
  peb_sensor_threshold[PEB_SENSOR_BMC_TEMP][LNC_THRESH] = 10;

  peb_sensor_threshold[PEB_SENSOR_PCIE_SW_TEMP][LCR_THRESH] = 5;
  peb_sensor_threshold[PEB_SENSOR_PCIE_SW_FRONT_TEMP][LCR_THRESH] = 5;
  peb_sensor_threshold[PEB_SENSOR_PCIE_SW_REAR_TEMP][LCR_THRESH] = 5;
  peb_sensor_threshold[PEB_SENSOR_LEFT_CONN_TEMP][LCR_THRESH] = 5;
  peb_sensor_threshold[PEB_SENSOR_RIGHT_CONN_TEMP][LCR_THRESH] = 5;
  peb_sensor_threshold[PEB_SENSOR_BMC_TEMP][LCR_THRESH] = 5;

  // PDPB SENSORS
  pdpb_sensor_threshold[PDPB_SENSOR_P12V][UCR_THRESH] = 13.728;
  pdpb_sensor_threshold[PDPB_SENSOR_P3V3][UCR_THRESH] = 4.192;
  pdpb_sensor_threshold[PDPB_SENSOR_P2V5][UCR_THRESH] = 2.72;
  pdpb_sensor_threshold[PDPB_SENSOR_P12V_SSD][UCR_THRESH] = 13.728;

  // PDPB TEMP SENSORS
  pdpb_sensor_threshold[PDPB_SENSOR_LEFT_REAR_TEMP][UCR_THRESH] = 55;
  pdpb_sensor_threshold[PDPB_SENSOR_LEFT_FRONT_TEMP][UCR_THRESH] = 55;
  pdpb_sensor_threshold[PDPB_SENSOR_RIGHT_REAR_TEMP][UCR_THRESH] = 55;
  pdpb_sensor_threshold[PDPB_SENSOR_RIGHT_FRONT_TEMP][UCR_THRESH] = 55;

  pdpb_sensor_threshold[PDPB_SENSOR_LEFT_REAR_TEMP][UNC_THRESH] = 50;
  pdpb_sensor_threshold[PDPB_SENSOR_LEFT_FRONT_TEMP][UNC_THRESH] = 50;
  pdpb_sensor_threshold[PDPB_SENSOR_RIGHT_REAR_TEMP][UNC_THRESH] = 50;
  pdpb_sensor_threshold[PDPB_SENSOR_RIGHT_FRONT_TEMP][UNC_THRESH] = 50;

  pdpb_sensor_threshold[PDPB_SENSOR_LEFT_REAR_TEMP][LNC_THRESH] = 10;
  pdpb_sensor_threshold[PDPB_SENSOR_LEFT_FRONT_TEMP][LNC_THRESH] = 10;
  pdpb_sensor_threshold[PDPB_SENSOR_RIGHT_REAR_TEMP][LNC_THRESH] = 10;
  pdpb_sensor_threshold[PDPB_SENSOR_RIGHT_FRONT_TEMP][LNC_THRESH] = 10;

  pdpb_sensor_threshold[PDPB_SENSOR_LEFT_REAR_TEMP][LCR_THRESH] = 5;
  pdpb_sensor_threshold[PDPB_SENSOR_LEFT_FRONT_TEMP][LCR_THRESH] = 5;
  pdpb_sensor_threshold[PDPB_SENSOR_RIGHT_REAR_TEMP][LCR_THRESH] = 5;
  pdpb_sensor_threshold[PDPB_SENSOR_RIGHT_FRONT_TEMP][LCR_THRESH] = 5;

  for (i = 0; i < lightning_flash_cnt; i++) {
    pdpb_sensor_threshold[PDPB_SENSOR_FLASH_TEMP_0 + i][UCR_THRESH] = 70;
    pdpb_sensor_threshold[PDPB_SENSOR_FLASH_TEMP_0 + i][UNC_THRESH] = 65;
    pdpb_sensor_threshold[PDPB_SENSOR_FLASH_TEMP_0 + i][LNC_THRESH] = 10;
    pdpb_sensor_threshold[PDPB_SENSOR_FLASH_TEMP_0 + i][LCR_THRESH] = 5;
  }

  // FCB SENSORS
  fcb_sensor_threshold[FCB_SENSOR_P12V_AUX][UCR_THRESH] = 13.75;
  fcb_sensor_threshold[FCB_SENSOR_P12VL][UCR_THRESH] = 13.75;
  fcb_sensor_threshold[FCB_SENSOR_P12VU][UCR_THRESH] = 13.75;
  fcb_sensor_threshold[FCB_SENSOR_P3V3][UCR_THRESH] = 3.63;
  fcb_sensor_threshold[FCB_SENSOR_HSC_IN_VOLT][UCR_THRESH] = 100; // Missing
  fcb_sensor_threshold[FCB_SENSOR_HSC_OUT_CURR][UCR_THRESH] = 60;
  fcb_sensor_threshold[FCB_SENSOR_HSC_IN_POWER][UCR_THRESH] = 1020;

  // FCB TEMP SENSORS
  fcb_sensor_threshold[FCB_SENSOR_BJT_TEMP_1][UCR_THRESH] = 55;
  fcb_sensor_threshold[FCB_SENSOR_BJT_TEMP_2][UCR_THRESH] = 55;

  fcb_sensor_threshold[FCB_SENSOR_BJT_TEMP_1][UNC_THRESH] = 50;
  fcb_sensor_threshold[FCB_SENSOR_BJT_TEMP_2][UNC_THRESH] = 50;

  fcb_sensor_threshold[FCB_SENSOR_BJT_TEMP_1][LNC_THRESH] = 10;
  fcb_sensor_threshold[FCB_SENSOR_BJT_TEMP_2][LNC_THRESH] = 10;

  fcb_sensor_threshold[FCB_SENSOR_BJT_TEMP_1][LCR_THRESH] = 5;
  fcb_sensor_threshold[FCB_SENSOR_BJT_TEMP_2][LCR_THRESH] = 5;

  init_done = true;
}

size_t peb_sensor_cnt = sizeof(peb_sensor_list)/sizeof(uint8_t);

size_t pdpb_sensor_cnt = sizeof(pdpb_sensor_list)/sizeof(uint8_t);

size_t fcb_sensor_cnt = sizeof(fcb_sensor_list)/sizeof(uint8_t);

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
read_flash_temp(uint8_t flash_num, float *value) {

  return lightning_flash_temp_read(lightning_flash_list[flash_num], value);
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
read_adc_value(const int pin, const char *device, float *value) {
  char device_name[LARGEST_DEVICE_NAME];
  char full_name[LARGEST_DEVICE_NAME];

  snprintf(device_name, LARGEST_DEVICE_NAME, device, pin);
  snprintf(full_name, LARGEST_DEVICE_NAME, "%s/%s", ADC_DIR, device_name);
  return read_device_float(full_name, value);
}


static int
read_tmp75_value(char *device, uint8_t addr, uint8_t type, float *value) {

  int dev;
  int ret;
  int32_t res;

  dev = open(device, O_RDWR);
  if (dev < 0) {
    syslog(LOG_ERR, "read_tmp75_value: open() failed");
    return -1;
  }

  /* Assign the i2c device address */
  ret = ioctl(dev, I2C_SLAVE, addr);
  if (ret < 0) {
    syslog(LOG_ERR, "read_tmp75_value: ioctl() assigning i2c addr failed");
    close(dev);
    return -1;
  }

  /* Read the Temperature Register result based on whether it is internal or external sensor */
  res = i2c_smbus_read_word_data(dev, type);
  if (res < 0) {
    close(dev);
    syslog(LOG_ERR, "read_tmp75_value: i2c_smbus_read_word_data failed");
    return -1;
  }

  close(dev);
  /* Result is read as MSB byte first and LSB byte second.
   * Result is 12bit with res[11:4]  == MSB[7:0] and res[3:0] = LSB */
  res = ((res & 0x0FF) << 4) | ((res & 0xF000) >> 12);

   /* Resolution is 0.0625 deg C/bit */
  if (res <= 0x7FF) {
    /* Temperature is positive  */
    *value = (float) res * TPM75_TEMP_RESOLUTION;
  } else if (res >= 0xC90) {
    /* Temperature is negative */
    *value = (float) (0xFFF - res + 1) * (-1) * TPM75_TEMP_RESOLUTION;
  } else {
    /* Out of range [128C to -55C] */
    syslog(LOG_WARNING, "read_tmp75_value: invalid res value = 0x%X", res);
    return -1;
  }

  return 0;
}

static int
read_hsc_value(uint8_t reg, char *device, uint8_t addr, uint8_t cntlr, float *value) {

  int dev;
  int ret;
  int32_t res;

  dev = open(device, O_RDWR);
  if (dev < 0) {
    syslog(LOG_ERR, "read_hsc_value: open() failed");
    return -1;
  }

  /* Assign the i2c device address */
  ret = ioctl(dev, I2C_SLAVE, addr);
  if (ret < 0) {
    syslog(LOG_ERR, "read_hsc_value: ioctl() assigning i2c addr failed");
    close(dev);
    return -1;
  }

  /* Read the er result based on whether it is internal or external sensor */
  res = i2c_smbus_read_word_data(dev, reg);
  if (res < 0) {
    close(dev);
    syslog(LOG_ERR, "read_hsc_value: i2c_smbus_read_word_data failed");
    return -1;
  }

  close(dev);

  // All Parameters use only bits [11:0]
  res &= 0xFFF;

  switch(reg) {
    case HSC_IN_VOLT:
      if (cntlr == HSC_ADM1276 || cntlr == HSC_ADM1278) {
        *value = (1.0/19599) * ((res * 100) - 0);
      }
      break;
    case HSC_OUT_CURR:
      if (cntlr == HSC_ADM1276) {
        *value = ((res * 10) - 20475) / (807 * 0.1667);
      } else if (cntlr == HSC_ADM1278) {
        *value = (1.0/1600) * ((res * 10) - 20475);
      }
      break;
    case HSC_IN_POWER:
      if (cntlr == HSC_ADM1276) {
        *value = ((res * 100) - 0) / (6043 * 0.1667);
      } else if (cntlr == HSC_ADM1278) {
        *value = (1.0/12246) * ((res * 100) - 0);
      }
      break;
    case HSC_TEMP:
      if (cntlr == HSC_ADM1278) {
        *value = (1.0/42) * ((res * 10) - 31880);
      }
      break;
    default:
      syslog(LOG_ERR, "read_hsc_value: wrong param");
      return -1;
  }

  return 0;
}


static int
read_nct7904_value(uint8_t reg, char *device, uint8_t addr, float *value) {

  int dev;
  int ret;
  int res_h;
  int res_l;
  int bank;
  uint16_t res;
  float multipler;

  dev = open(device, O_RDWR);
  if (dev < 0) {
    syslog(LOG_ERR, "read_nct7904_value: open() failed");
    return -1;
  }

  /* Assign the i2c device address */
  ret = ioctl(dev, I2C_SLAVE, addr);
  if (ret < 0) {
    syslog(LOG_ERR, "read_nct7904_value: ioctl() assigning i2c addr failed");
  }

  /* Read the Bank Register and set it to 0 */
  bank = i2c_smbus_read_byte_data(dev, NCT7904_BANK_SEL);
  if (bank != 0x0) {
    syslog(LOG_INFO, "read_nct7904_value: Bank Register set to %d", bank);
    if (i2c_smbus_write_byte_data(dev, NCT7904_BANK_SEL, 0) < 0) {
      syslog(LOG_ERR, "read_nct7904_value: i2c_smbus_write_byte_data: "
          "selecting Bank 0 failed");
      return -1;
    }
  }

  /* Read the MSB byte for the value */
  res_h = i2c_smbus_read_byte_data(dev, reg);

  /* Read the LSB byte for the value */
  res_l = i2c_smbus_read_byte_data(dev, reg + 1);

  close(dev);

  /*
   * Result is 11 bits
   * res[10:3] = res_h[7:0]
   * res[2:0] = res_l[2:0]
   */
  res = (res_h << 3) | (res_l & 0x7);

  switch(reg) {
    case NCT7904_TEMP_CH1:
      multipler = (0.125 /* NCT7904 Section 6.4 */);
      break;
    case NCT7904_TEMP_CH2:
      multipler = (0.125 /* NCT7904 Section 6.4 */);
      break;
    case NCT7904_VSEN9:
      multipler = (12 /* Voltage Divider*/ * 0.002 /* NCT7904 Section 6.4 */);
      break;
    case NCT7904_VSEN6:
      multipler = (12 /* Voltage Divider*/ * 0.002 /* NCT7904 Section 6.4 */);
      break;
    case NCT7904_VSEN7:
      multipler = (12 /* Voltage Divider*/ * 0.002 /* NCT7904 Section 6.4 */);
      break;
    case NCT7904_3VDD:
      multipler = (0.006 /* NCT7904 Section 6.4 */);
      break;
  }

  *value = (float) (res * multipler);

  return 0;
}

static int
read_ads1015_value(uint8_t channel, char *device, uint8_t addr, float *value) {

  int dev;
  int ret;
  int32_t config;
  int32_t res;

  dev = open(device, O_RDWR);
  if (dev < 0) {
    syslog(LOG_ERR, "read_ads1015_value: open() failed");
    return -1;
  }

  /* Assign the i2c device address */
  ret = ioctl(dev, I2C_SLAVE, addr);
  if (ret < 0) {
    syslog(LOG_ERR, "read_ads1015_value: ioctl() assigning i2c addr failed");
    close(dev);
    return -1;
  }

  /*
   * config
   * Byte 0: OS[7]  CHANNEL[6:4] PGA[3:1] MODE [0]
   * Byte 1: Data Rate[7:5] COMPARATOR[4:0]
   */
  config = ADS1015_DEFAULT_CONFIG;
  config |= (channel & 0x7) << 4;

  /* Write the config in the CONFIG register */
  ret = i2c_smbus_write_word_data(dev, ADS1015_CONFIG, config);
  if (ret < 0) {
    syslog(LOG_ERR, "read_ads1015_value: i2c_smbus_write_word_data failed");
    close(dev);
    return -1;
  }
  /* Wait for the conversion to finish */
  msleep(5);

  /* Read the CONFIG register to check if the conversion completed. */
  config = i2c_smbus_read_word_data(dev, ADS1015_CONFIG);
  if (!(config & (1 << 15))) {
    close(dev);
    return -1;
  } else if (config < 0) {
    close(dev);
    syslog(LOG_ERR, "read_ads1015_value: i2c_smbus_read_word_data failed");
    return -1;
  }

  /* Read the CONVERSION result */
  res = i2c_smbus_read_word_data(dev, ADS1015_CONVERSION);
  if (res < 0) {
    close(dev);
    syslog(LOG_ERR, "read_ads1015_value: i2c_smbus_read_word_data failed");
    return -1;
  }

  close(dev);

  /* Result is read as MSB byte first and LSB byte second. */
  res = ((res & 0x0FF) << 8) | ((res & 0xFF00) >> 8);

  /*
   * Based on the config PGA, COMP MODE, DATA RATE value,
   * Voltage in Volts = res [15:4] * Reference volt / 2048
   */
  *value = (float) (res >> 4) * (4.096 / 2048);

  return 0;
}

int
lightning_sensor_sdr_path(uint8_t fru, char *path) {
  return 0;
}

int
lightning_sensor_sdr_init(uint8_t fru, sensor_info_t *sinfo) {
  return -1;
}

/* Get the units for the sensor */
int
lightning_sensor_units(uint8_t fru, uint8_t sensor_num, char *units) {
  uint8_t op, modifier;
  sensor_info_t *sinfo;

  switch(fru) {
    case FRU_PEB:
      switch(sensor_num) {
        case PEB_SENSOR_PCIE_SW_TEMP:
        case PEB_SENSOR_PCIE_SW_FRONT_TEMP:
        case PEB_SENSOR_PCIE_SW_REAR_TEMP:
        case PEB_SENSOR_LEFT_CONN_TEMP:
        case PEB_SENSOR_RIGHT_CONN_TEMP:
        case PEB_SENSOR_BMC_TEMP:
          sprintf(units, "C");
          break;
        case PEB_SENSOR_HSC_IN_VOLT:
          sprintf(units, "Volts");
          break;
        case PEB_SENSOR_HSC_OUT_CURR:
          sprintf(units, "Amps");
          break;
        case PEB_SENSOR_HSC_TEMP:
          sprintf(units, "C");
          break;
        case PEB_SENSOR_HSC_IN_POWER:
          sprintf(units, "Watts");
          break;
        case PEB_SENSOR_ADC_P12V:
        case PEB_SENSOR_ADC_P5V:
        case PEB_SENSOR_ADC_P3V3_STBY:
        case PEB_SENSOR_ADC_P1V8_STBY:
        case PEB_SENSOR_ADC_P1V53:
        case PEB_SENSOR_ADC_P0V9:
        case PEB_SENSOR_ADC_P0V9_E:
        case PEB_SENSOR_ADC_P1V26:
          sprintf(units, "Volts");
          break;
        default:
          sprintf(units, "");
          break;
      }
      break;

    case FRU_PDPB:

      if (sensor_num >= PDPB_SENSOR_FLASH_TEMP_0 &&
          sensor_num < (PDPB_SENSOR_FLASH_TEMP_0 + lightning_flash_cnt)) {
        sprintf(units, "C");
        break;
      }

      switch(sensor_num) {
        case PDPB_SENSOR_LEFT_REAR_TEMP:
        case PDPB_SENSOR_LEFT_FRONT_TEMP:
        case PDPB_SENSOR_RIGHT_REAR_TEMP:
        case PDPB_SENSOR_RIGHT_FRONT_TEMP:
          sprintf(units, "C");
          break;
        case PDPB_SENSOR_P12V:
        case PDPB_SENSOR_P3V3:
        case PDPB_SENSOR_P2V5:
        case PDPB_SENSOR_P12V_SSD:
          sprintf(units, "Volts");
          break;
        default:
          sprintf(units, "");
          break;
      }
      break;

    case FRU_FCB:
      switch(sensor_num) {
        case FCB_SENSOR_P12V_AUX:
        case FCB_SENSOR_P12VL:
        case FCB_SENSOR_P12VU:
        case FCB_SENSOR_P3V3:
          sprintf(units, "Volts");
          break;
        case FCB_SENSOR_HSC_IN_VOLT:
          sprintf(units, "Volts");
          break;
        case FCB_SENSOR_HSC_OUT_CURR:
          sprintf(units, "Amps");
          break;
        case FCB_SENSOR_HSC_IN_POWER:
          sprintf(units, "Watts");
          break;
        case FCB_SENSOR_BJT_TEMP_1:
        case FCB_SENSOR_BJT_TEMP_2:
          sprintf(units, "C");
          break;

        default:
          sprintf(units, "");
          break;
      }
      break;
  }
  return 0;
}

int
lightning_sensor_threshold(uint8_t fru, uint8_t sensor_num, uint8_t thresh, float *value) {

  sensor_thresh_array_init();

  switch(fru) {
    case FRU_PEB:
      *value = peb_sensor_threshold[sensor_num][thresh];
      break;
    case FRU_PDPB:
      *value = pdpb_sensor_threshold[sensor_num][thresh];
      break;
    case FRU_FCB:
      *value = fcb_sensor_threshold[sensor_num][thresh];
      break;
  }
  return 0;
}

/* Get the name for the sensor */
int
lightning_sensor_name(uint8_t fru, uint8_t sensor_num, char *name) {

  switch(fru) {
    case FRU_PEB:
      switch(sensor_num) {
        case PEB_SENSOR_PCIE_SW_TEMP:
          sprintf(name, "PCIE_SW_TEMP");
          break;
        case PEB_SENSOR_PCIE_SW_FRONT_TEMP:
          sprintf(name, "SW_FRONT_TEMP");
          break;
        case PEB_SENSOR_PCIE_SW_REAR_TEMP:
          sprintf(name, "SW_REAR_TEMP");
          break;
        case PEB_SENSOR_LEFT_CONN_TEMP:
          sprintf(name, "LEFT_CONN_TEMP");
          break;
        case PEB_SENSOR_RIGHT_CONN_TEMP:
          sprintf(name, "RIGHT_CONN_TEMP");
          break;
        case PEB_SENSOR_BMC_TEMP:
          sprintf(name, "BMC_TEMP");
          break;
        case PEB_SENSOR_HSC_IN_VOLT:
          sprintf(name, "PEB_HSC_IN_VOLT");
          break;
        case PEB_SENSOR_HSC_OUT_CURR:
          sprintf(name, "PEB_HSC_OUT_CURR");
          break;
        case PEB_SENSOR_HSC_TEMP:
          sprintf(name, "PEB_HSC_TEMP");
          break;
        case PEB_SENSOR_HSC_IN_POWER:
          sprintf(name, "PEB_HSC_IN_POWER");
          break;
        case PEB_SENSOR_ADC_P12V:
          sprintf(name, "PEB_P12V");
          break;
        case PEB_SENSOR_ADC_P5V:
          sprintf(name, "PEB_P5V");
          break;
        case PEB_SENSOR_ADC_P3V3_STBY:
          sprintf(name, "PEB_P3V3");
          break;
        case PEB_SENSOR_ADC_P1V8_STBY:
          sprintf(name, "PEB_P1V8");
          break;
        case PEB_SENSOR_ADC_P1V53:
          sprintf(name, "PEB_P1V53");
          break;
        case PEB_SENSOR_ADC_P0V9:
          sprintf(name, "PEB_P0V9");
          break;
        case PEB_SENSOR_ADC_P0V9_E:
          sprintf(name, "PEB_P0V9_E");
          break;
        case PEB_SENSOR_ADC_P1V26:
          sprintf(name, "PEB_P1V26");
          break;
        default:
          sprintf(name, "");
          break;
      }
      break;

    case FRU_PDPB:

      if (sensor_num >= PDPB_SENSOR_FLASH_TEMP_0 &&
          sensor_num < (PDPB_SENSOR_FLASH_TEMP_0 + lightning_flash_cnt)) {
        sprintf(name, "SSD_%d_TEMP", sensor_num - PDPB_SENSOR_FLASH_TEMP_0);
        break;
      }

      switch(sensor_num) {
        case PDPB_SENSOR_LEFT_REAR_TEMP:
          sprintf(name, "LEFT_REAR_TEMP");
          break;
        case PDPB_SENSOR_LEFT_FRONT_TEMP:
          sprintf(name, "LEFT_FRONT_TEMP");
          break;
        case PDPB_SENSOR_RIGHT_REAR_TEMP:
          sprintf(name, "RIGHT_REAR_TEMP");
          break;
        case PDPB_SENSOR_RIGHT_FRONT_TEMP:
          sprintf(name, "RIGHT_FRONT_TEMP");
          break;
        case PDPB_SENSOR_P12V:
          sprintf(name, "PDPB_P12V");
          break;
        case PDPB_SENSOR_P3V3:
          sprintf(name, "PDPB_P3V3");
          break;
        case PDPB_SENSOR_P2V5:
          sprintf(name, "PDPB_P2V5");
          break;
        case PDPB_SENSOR_P12V_SSD:
          sprintf(name, "PDPB_P12V_SSD");
          break;
        default:
          sprintf(name, "");
          break;
      }
      break;

    case FRU_FCB:
      switch(sensor_num) {
        case FCB_SENSOR_P12V_AUX:
          sprintf(name, "FCB_P12V_AUX");
          break;
        case FCB_SENSOR_P12VL:
          sprintf(name, "FCB_P12VL");
          break;
        case FCB_SENSOR_P12VU:
          sprintf(name, "FCB_P12VU");
          break;
        case FCB_SENSOR_P3V3:
          sprintf(name, "FCB_P3V3");
          break;
        case FCB_SENSOR_HSC_IN_VOLT:
          sprintf(name, "FCB_HSC_IN_VOLT");
          break;
        case FCB_SENSOR_HSC_OUT_CURR:
          sprintf(name, "FCB_HSC_OUT_CURR");
          break;
        case FCB_SENSOR_HSC_IN_POWER:
          sprintf(name, "FCB_HSC_IN_POWER");
          break;
        case FCB_SENSOR_BJT_TEMP_1:
          sprintf(name, "FCB_BJT_TEMP_1");
          break;
        case FCB_SENSOR_BJT_TEMP_2:
          sprintf(name, "FCB_BJT_TEMP_2");
          break;
        default:
          sprintf(name, "");
          break;
      }
      break;
  }
  return 0;
}

int
lightning_sensor_read(uint8_t fru, uint8_t sensor_num, void *value) {

  switch (fru) {
    case FRU_PEB:
      switch(sensor_num) {
        // Temperature Sensors
        case PEB_SENSOR_PCIE_SW_TEMP:
          return read_tmp75_value(I2C_DEV_PEB, PEB_MAX6654_U4, REMOTE_SENSOR, (float*) value);
        case PEB_SENSOR_PCIE_SW_FRONT_TEMP:
          return read_tmp75_value(I2C_DEV_PEB, PEB_MAX6654_U4, LOCAL_SENSOR, (float*) value);
        case PEB_SENSOR_PCIE_SW_REAR_TEMP:
          return read_tmp75_value(I2C_DEV_PEB, PEB_TMP75_U136, LOCAL_SENSOR, (float*) value);
        case PEB_SENSOR_LEFT_CONN_TEMP:
          return read_tmp75_value(I2C_DEV_PEB, PEB_TMP421_U15, REMOTE_SENSOR, (float*) value);
        case PEB_SENSOR_RIGHT_CONN_TEMP:
          return read_tmp75_value(I2C_DEV_PEB, PEB_TMP421_U15, LOCAL_SENSOR, (float*) value);
        case PEB_SENSOR_BMC_TEMP:
          return read_tmp75_value(I2C_DEV_PEB, PEB_TMP75_U134, LOCAL_SENSOR, (float*) value);

        // Hot Swap Controller
        case PEB_SENSOR_HSC_IN_VOLT:
          return read_hsc_value(HSC_IN_VOLT, I2C_DEV_PEB, I2C_ADDR_PEB_HSC, HSC_ADM1278, (float*) value);
        case PEB_SENSOR_HSC_OUT_CURR:
          return read_hsc_value(HSC_OUT_CURR, I2C_DEV_PEB, I2C_ADDR_PEB_HSC, HSC_ADM1278, (float*) value);
        case PEB_SENSOR_HSC_TEMP:
          return read_hsc_value(HSC_TEMP, I2C_DEV_PEB, I2C_ADDR_PEB_HSC, HSC_ADM1278, (float*) value);
        case PEB_SENSOR_HSC_IN_POWER:
          return read_hsc_value(HSC_IN_POWER, I2C_DEV_PEB, I2C_ADDR_PEB_HSC, HSC_ADM1278, (float*) value);

        // ADC Voltages
        case PEB_SENSOR_ADC_P12V:
          return read_adc_value(ADC_PIN0, ADC_VALUE, (float*) value);
        case PEB_SENSOR_ADC_P5V:
          return read_adc_value(ADC_PIN1, ADC_VALUE, (float*) value);
        case PEB_SENSOR_ADC_P3V3_STBY:
          return read_adc_value(ADC_PIN2, ADC_VALUE, (float*) value);
        case PEB_SENSOR_ADC_P1V8_STBY:
          return read_adc_value(ADC_PIN3, ADC_VALUE, (float*) value);
        case PEB_SENSOR_ADC_P1V53:
          return read_adc_value(ADC_PIN4, ADC_VALUE, (float*) value);
        case PEB_SENSOR_ADC_P0V9:
          return read_adc_value(ADC_PIN5, ADC_VALUE, (float*) value);
        case PEB_SENSOR_ADC_P0V9_E:
          return read_adc_value(ADC_PIN6, ADC_VALUE, (float*) value);
        case PEB_SENSOR_ADC_P1V26:
          return read_adc_value(ADC_PIN7, ADC_VALUE, (float*) value);
        default:
          return -1;
      }
      break;

    case FRU_PDPB:

        if (sensor_num >= PDPB_SENSOR_FLASH_TEMP_0 &&
            sensor_num < (PDPB_SENSOR_FLASH_TEMP_0 + lightning_flash_cnt)) {
          return read_flash_temp(sensor_num - PDPB_SENSOR_FLASH_TEMP_0, (float*) value);
        }

      switch(sensor_num) {
        // Temp
        case PDPB_SENSOR_LEFT_REAR_TEMP:
          return read_tmp75_value(I2C_DEV_PDPB, PDPB_TMP75_U47, LOCAL_SENSOR, (float*) value);
        case PDPB_SENSOR_LEFT_FRONT_TEMP:
          return read_tmp75_value(I2C_DEV_PDPB, PDPB_TMP75_U49, LOCAL_SENSOR, (float*) value);
        case PDPB_SENSOR_RIGHT_REAR_TEMP:
          return read_tmp75_value(I2C_DEV_PDPB, PDPB_TMP75_U48, LOCAL_SENSOR, (float*) value);
        case PDPB_SENSOR_RIGHT_FRONT_TEMP:
          return read_tmp75_value(I2C_DEV_PDPB, PDPB_TMP75_U51, LOCAL_SENSOR, (float*) value);

        // Voltage
        case PDPB_SENSOR_P12V:
          if (read_ads1015_value(ADS1015_CHANNEL4, I2C_DEV_PDPB, I2C_ADDR_PDPB_ADC, (float*) value) < 0) {
            return -1;
          }
          *((float *) value) = *((float *) value) * ((10 + 2) / 2); // Voltage Divider Circuit
          return 0;

        case PDPB_SENSOR_P3V3:
          return read_ads1015_value(ADS1015_CHANNEL5, I2C_DEV_PDPB, I2C_ADDR_PDPB_ADC, (float*) value);
        case PDPB_SENSOR_P2V5:
          return read_ads1015_value(ADS1015_CHANNEL6, I2C_DEV_PDPB, I2C_ADDR_PDPB_ADC, (float*) value);
        case PDPB_SENSOR_P12V_SSD:
          if (read_ads1015_value(ADS1015_CHANNEL7, I2C_DEV_PDPB, I2C_ADDR_PDPB_ADC, (float*) value) < 0) {
            return -1;
          }
          *((float *) value) = *((float *) value) * ((10 + 2) / 2); // Voltage Divider Circuit
          return 0;

        default:
          return -1;

      }
      break;

    case FRU_FCB:
      switch(sensor_num) {
        // Voltage
        case FCB_SENSOR_P12V_AUX:
          return read_nct7904_value(NCT7904_VSEN9, I2C_DEV_FCB, I2C_ADDR_NCT7904, (float*) value);
        case FCB_SENSOR_P12VL:
          return read_nct7904_value(NCT7904_VSEN6, I2C_DEV_FCB, I2C_ADDR_NCT7904, (float*) value);
        case FCB_SENSOR_P12VU:
          return read_nct7904_value(NCT7904_VSEN7, I2C_DEV_FCB, I2C_ADDR_NCT7904, (float*) value);
        case FCB_SENSOR_P3V3:
          return read_nct7904_value(NCT7904_3VDD, I2C_DEV_FCB, I2C_ADDR_NCT7904, (float*) value);

        // Hot Swap Controller
        case FCB_SENSOR_HSC_IN_VOLT:
          return read_hsc_value(HSC_IN_VOLT, I2C_DEV_FCB, I2C_ADDR_FCB_HSC, HSC_ADM1276, (float*) value);
        case FCB_SENSOR_HSC_OUT_CURR:
          return read_hsc_value(HSC_OUT_CURR, I2C_DEV_FCB, I2C_ADDR_FCB_HSC, HSC_ADM1276, (float*) value);
        case FCB_SENSOR_HSC_IN_POWER:
          return read_hsc_value(HSC_IN_POWER, I2C_DEV_FCB, I2C_ADDR_FCB_HSC, HSC_ADM1276, (float*) value);

        case FCB_SENSOR_BJT_TEMP_1:
          return read_nct7904_value(NCT7904_TEMP_CH1, I2C_DEV_FCB, I2C_ADDR_NCT7904, (float*) value);
          break;
        case FCB_SENSOR_BJT_TEMP_2:
          return read_nct7904_value(NCT7904_TEMP_CH2, I2C_DEV_FCB, I2C_ADDR_NCT7904, (float*) value);
          break;
        default:
          return -1;
      }
      break;
  }
}

