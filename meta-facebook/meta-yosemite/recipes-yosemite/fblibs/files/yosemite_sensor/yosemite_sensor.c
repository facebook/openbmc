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
#include <dirent.h>
#include <sys/mman.h>
#include <openbmc/obmc-i2c.h>
#include <openbmc/obmc-sensor.h>
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
#define HSC_IN_POWER "power1_input"

#define UNIT_DIV 1000

#define I2C_DEV_NIC "/dev/i2c-11"
#define I2C_NIC_ADDR 0x1f
#define I2C_NIC_SENSOR_TEMP_REG 0x01

#define BIC_SENSOR_READ_NA 0x20

#define MAX_SENSOR_NUM 0xFF
#define ALL_BYTES 0xFF
#define LAST_REC_ID 0xFFFF

#define YOSEMITE_SDR_PATH "/tmp/sdr_%s.bin"
#define ADM1278_R_SENSE 0.5

static float hsc_r_sense = ADM1278_R_SENSE;

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
  SP_SENSOR_P12V_SLOT1,
  SP_SENSOR_P12V_SLOT2,
  SP_SENSOR_P12V_SLOT3,
  SP_SENSOR_P12V_SLOT4,
  SP_SENSOR_P3V3,
  SP_SENSOR_HSC_IN_VOLT,
  SP_SENSOR_HSC_OUT_CURR,
  SP_SENSOR_HSC_TEMP,
  SP_SENSOR_HSC_IN_POWER,
};

// List of NIC sensors to be monitored
const uint8_t nic_sensor_list[] = {
  MEZZ_SENSOR_TEMP,

  // PLDM numeric sensors
  NIC_SOC_TEMP,
  PORT_0_TEMP,
  PORT_0_LINK_SPEED,

  // PLDM state sensors
  NIC_HEALTH_STATE,
  PORT_0_LINK_STATE,
};

float spb_sensor_threshold[MAX_SENSOR_NUM][MAX_SENSOR_THRESHOLD + 1] = {0};
float nic_sensor_threshold[MAX_SENSOR_NUM][MAX_SENSOR_THRESHOLD + 1] = {0};


/*
PLDM sensor threshold are populated in shared memory object by ncsid
Retrieve these value and initialize global init_pldm_sensors
*/
static void
init_pldm_sensors() {
  int shm_fd = 0, i = 0;
  int shm_size = sizeof(pldm_sensor_t) * NUM_PLDM_SENSORS;
  pldm_sensor_t *pldm_sensors;

  /* open the shared memory object */
  shm_fd = shm_open(PLDM_SNR_INFO, O_RDONLY, 0666);
  if (shm_fd < 0) {
    return;
  }

  /* memory map the shared memory object */
  pldm_sensors = mmap(0, shm_size, PROT_READ, MAP_SHARED, shm_fd, 0);
  if (pldm_sensors < 0) {
    syslog(LOG_INFO, "init_pldm_sensors sensor read failed");
    close(shm_fd);
    return;
  }

  for ( i = 0; i < NUM_PLDM_SENSORS; ++i) {
    if (pldm_sensors[i].sensor_type == PLDM_SENSOR_TYPE_NUMERIC) {

      int pft_id = pldm_sensors[i].pltf_sensor_id;

      // make sure data is in range
      if (pft_id < PLDM_NUMERIC_SENSOR_START || pft_id > PLDM_SENSOR_END) {
        syslog(LOG_INFO, "init_pldm_sensors: invalid PLDM sensor 0x%x", pft_id);
        continue;
      }
      nic_sensor_threshold[pft_id][UNR_THRESH] = pldm_sensors[i].unr;
      nic_sensor_threshold[pft_id][UCR_THRESH] = pldm_sensors[i].ucr;
      nic_sensor_threshold[pft_id][UNC_THRESH] = pldm_sensors[i].unc;
      nic_sensor_threshold[pft_id][LNC_THRESH] = pldm_sensors[i].lnc;
      nic_sensor_threshold[pft_id][LCR_THRESH] = pldm_sensors[i].lcr;
      nic_sensor_threshold[pft_id][LNR_THRESH] = pldm_sensors[i].lnr;
    }
  }

  if (munmap(pldm_sensors, shm_size) != 0) {
    syslog(LOG_INFO, "init_pldm_sensors munmap failed");
  }
  close(shm_fd);
  return;
}


static void
sensor_thresh_array_init() {
  static bool init_done = false;

  if (init_done)
    return;

  spb_sensor_threshold[SP_SENSOR_INLET_TEMP][UCR_THRESH] = 40;
  spb_sensor_threshold[SP_SENSOR_OUTLET_TEMP][UCR_THRESH] = 70;
  spb_sensor_threshold[SP_SENSOR_FAN0_TACH][UCR_THRESH] = 11500;
  spb_sensor_threshold[SP_SENSOR_FAN0_TACH][UNC_THRESH] = 8500;
  spb_sensor_threshold[SP_SENSOR_FAN0_TACH][LCR_THRESH] = 500;
  spb_sensor_threshold[SP_SENSOR_FAN1_TACH][UCR_THRESH] = 11500;
  spb_sensor_threshold[SP_SENSOR_FAN1_TACH][UNC_THRESH] = 8500;
  spb_sensor_threshold[SP_SENSOR_FAN1_TACH][LCR_THRESH] = 500;
  //spb_sensor_threshold[SP_SENSOR_AIR_FLOW][UCR_THRESH] =  {75.0, 0, 0, 0, 0, 0, 0, 0};
  spb_sensor_threshold[SP_SENSOR_P5V][UCR_THRESH] = 5.493;
  spb_sensor_threshold[SP_SENSOR_P5V][LCR_THRESH] = 4.501;
  spb_sensor_threshold[SP_SENSOR_P12V][UCR_THRESH] = 13.216;
  spb_sensor_threshold[SP_SENSOR_P12V][LCR_THRESH] = 11.269;
  spb_sensor_threshold[SP_SENSOR_P3V3_STBY][UCR_THRESH] = 3.625;
  spb_sensor_threshold[SP_SENSOR_P3V3_STBY][LCR_THRESH] = 2.973;
  spb_sensor_threshold[SP_SENSOR_P12V_SLOT1][UCR_THRESH] = 13.216;
  spb_sensor_threshold[SP_SENSOR_P12V_SLOT1][LCR_THRESH] = 11.269;
  spb_sensor_threshold[SP_SENSOR_P12V_SLOT2][UCR_THRESH] = 13.216;
  spb_sensor_threshold[SP_SENSOR_P12V_SLOT2][LCR_THRESH] = 11.269;
  spb_sensor_threshold[SP_SENSOR_P12V_SLOT3][UCR_THRESH] = 13.216;
  spb_sensor_threshold[SP_SENSOR_P12V_SLOT3][LCR_THRESH] = 11.269;
  spb_sensor_threshold[SP_SENSOR_P12V_SLOT4][UCR_THRESH] = 13.216;
  spb_sensor_threshold[SP_SENSOR_P12V_SLOT4][LCR_THRESH] = 11.269;
  spb_sensor_threshold[SP_SENSOR_P3V3][UCR_THRESH] = 3.625;
  spb_sensor_threshold[SP_SENSOR_P3V3][LCR_THRESH] = 2.973;
  spb_sensor_threshold[SP_SENSOR_HSC_IN_VOLT][UCR_THRESH] = 13.2;
  spb_sensor_threshold[SP_SENSOR_HSC_IN_VOLT][LCR_THRESH] = 10.8;
  spb_sensor_threshold[SP_SENSOR_HSC_OUT_CURR][UCR_THRESH] = 47.705;
  spb_sensor_threshold[SP_SENSOR_HSC_TEMP][UCR_THRESH] = 120;
  spb_sensor_threshold[SP_SENSOR_HSC_IN_POWER][UCR_THRESH] = 525;

  nic_sensor_threshold[MEZZ_SENSOR_TEMP][UCR_THRESH] = 95;
  init_pldm_sensors();
  init_done = true;
}

size_t bic_sensor_cnt = sizeof(bic_sensor_list)/sizeof(uint8_t);

size_t bic_discrete_cnt = sizeof(bic_discrete_list)/sizeof(uint8_t);

size_t spb_sensor_cnt = sizeof(spb_sensor_list)/sizeof(uint8_t);

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
get_current_dir(const char *device, char *dir_name) {
  char full_name[LARGEST_DEVICE_NAME + 1];
  DIR *dir = NULL;
  struct dirent *ent;

  snprintf(full_name, sizeof(full_name), "%s/hwmon", device);
  dir = opendir(full_name);
  if (dir == NULL) {
    goto close_dir_out;
  }
  while ((ent = readdir(dir)) != NULL) {
    if (strstr(ent->d_name, "hwmon")) {
      // found the correct 'hwmon??' directory
      snprintf(dir_name, sizeof(full_name), "%s/hwmon/%s/",
      device, ent->d_name);
      goto close_dir_out;
    }
  }

close_dir_out:
  if (dir != NULL) {
    if (closedir(dir)) {
      syslog(LOG_ERR, "%s closedir failed, errno=%s\n",
              __FUNCTION__, strerror(errno));
    }
  }
  return 0;
}


static int
read_temp_attr(const char *device, const char *attr, float *value) {
  char full_dir_name[LARGEST_DEVICE_NAME + 1];
  char dir_name[LARGEST_DEVICE_NAME + 1];
  int tmp;

  // Get current working directory
  if (get_current_dir(device, dir_name))
  {
    return -1;
  }
  snprintf(
      full_dir_name, LARGEST_DEVICE_NAME, "%s/%s", dir_name, attr);


  if (read_device(full_dir_name, &tmp)) {
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
read_hsc_value(const char* attr, const char *device, float r_sense, float *value) {
  char full_dir_name[LARGEST_DEVICE_NAME];
  char dir_name[LARGEST_DEVICE_NAME + 1];
  int tmp;

  // Get current working directory
  if (get_current_dir(device, dir_name))
  {
    return -1;
  }
  snprintf(
      full_dir_name, LARGEST_DEVICE_NAME, "%s/%s", dir_name, attr);

  if(read_device(full_dir_name, &tmp)) {
    return -1;
  }

  if ((strcmp(attr, HSC_OUT_CURR) == 0) || (strcmp(attr, HSC_IN_POWER) == 0)) {
    *value = ((float) tmp)/r_sense/UNIT_DIV;
  }
  else {
    *value = ((float) tmp)/UNIT_DIV;
  }

  return 0;
}

static int
read_nic_temp(uint8_t snr_num, float *value) {
  char command[64];
  int dev;
  int ret;
  uint8_t tmp_val;

  if (snr_num == MEZZ_SENSOR_TEMP) {
    dev = open(I2C_DEV_NIC, O_RDWR);
    if (dev < 0) {
      syslog(LOG_ERR, "open() failed for read_nic_temp, errno=%s",
             strerror(errno));
      return -1;
    }
    /* Assign the i2c device address */
    ret = ioctl(dev, I2C_SLAVE, I2C_NIC_ADDR);
    if (ret < 0) {
      syslog(LOG_ERR, "read_nic_temp: ioctl() assigning i2c addr failed");
    }

    tmp_val = i2c_smbus_read_byte_data(dev, I2C_NIC_SENSOR_TEMP_REG);

    close(dev);

    // TODO: This is a HACK till we find the actual root cause
    // This condition implies that the I2C bus is busy
    if (tmp_val == 0xFF) {
      syslog(LOG_INFO, "read_nic_temp: value 0xFF - i2c bus is busy");
      return -1;
    }

    *value = (float) tmp_val;
  }

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
yosemite_sensor_sdr_path(uint8_t fru, char *path) {

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
#ifdef DEBUG
    syslog(LOG_WARNING, "yosemite_sensor_sdr_path: Wrong Slot ID\n");
#endif
    return -1;
}

sprintf(path, YOSEMITE_SDR_PATH, fru_name);

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
yosemite_sensor_sdr_init(uint8_t fru, sensor_info_t *sinfo) {
  int fd;
  uint8_t buf[MAX_SDR_LEN] = {0};
  uint8_t bytes_rd = 0;
  uint8_t sn = 0;
  char path[64] = {0};

  switch(fru) {
    case FRU_SLOT1:
    case FRU_SLOT2:
    case FRU_SLOT3:
    case FRU_SLOT4:
      if (yosemite_sensor_sdr_path(fru, path) < 0) {
#ifdef DEBUG
        syslog(LOG_WARNING, "yosemite_sensor_sdr_init: get_fru_sdr_path failed\n");
#endif
        return ERR_NOT_READY;
      }

      if (sdr_init(path, sinfo) < 0) {
#ifdef DEBUG
        syslog(LOG_ERR, "yosemite_sensor_sdr_init: sdr_init failed for FRU %d", fru);
#endif
      }
      break;

    case FRU_SPB:
    case FRU_NIC:
      return -1;
      break;
  }

  return 0;
}

static int
yosemite_sdr_init(uint8_t fru) {

  static bool init_done[MAX_NUM_FRUS] = {false};

  if (!init_done[fru - 1]) {

    sensor_info_t *sinfo = g_sinfo[fru-1];

    if (yosemite_sensor_sdr_init(fru, sinfo) < 0)
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
yosemite_sensor_units(uint8_t fru, uint8_t sensor_num, char *units) {
  uint8_t op, modifier;
  sensor_info_t *sinfo;

    if (is_server_prsnt(fru) && (yosemite_sdr_init(fru) != 0)) {
      return -1;
    }

  switch(fru) {
    case FRU_SLOT1:
    case FRU_SLOT2:
    case FRU_SLOT3:
    case FRU_SLOT4:
      sprintf(units, "");
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
        case SP_SENSOR_P12V_SLOT1:
        case SP_SENSOR_P12V_SLOT2:
        case SP_SENSOR_P12V_SLOT3:
        case SP_SENSOR_P12V_SLOT4:
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
      switch(sensor_num) {
        case MEZZ_SENSOR_TEMP:
          sprintf(units, "C");
          break;
        case NIC_SOC_TEMP:
          sprintf(units, "C");
          break;
        case PORT_0_TEMP:
          sprintf(units, "C");
          break;
        case PORT_0_LINK_SPEED:
          sprintf(units, "100Mbps");
          break;
        case NIC_HEALTH_STATE:
          strcpy(units, "");
          break;
        case PORT_0_LINK_STATE:
          strcpy(units, "");
          break;
      }
      break;
  }
  return 0;
}

int
yosemite_sensor_threshold(uint8_t fru, uint8_t sensor_num, uint8_t thresh, float *value) {

  sensor_thresh_array_init();

  switch(fru) {
    case FRU_SLOT1:
    case FRU_SLOT2:
    case FRU_SLOT3:
    case FRU_SLOT4:
      break;
    case FRU_SPB:
      *value = spb_sensor_threshold[sensor_num][thresh];
      break;
    case FRU_NIC:
      *value = nic_sensor_threshold[sensor_num][thresh];
      break;
  }
  return 0;
}

/* Get the name for the sensor */
int
yosemite_sensor_name(uint8_t fru, uint8_t sensor_num, char *name) {

  switch(fru) {
    case FRU_SLOT1:
    case FRU_SLOT2:
    case FRU_SLOT3:
    case FRU_SLOT4:
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

    case FRU_SPB:
      switch(sensor_num) {
        case SP_SENSOR_INLET_TEMP:
          sprintf(name, "SP_INLET_TEMP");
          break;
        case SP_SENSOR_OUTLET_TEMP:
          sprintf(name, "SP_OUTLET_TEMP");
          break;
        case SP_SENSOR_MEZZ_TEMP:
          sprintf(name, "SP_MEZZ_TEMP");
          break;
        case SP_SENSOR_FAN0_TACH:
          sprintf(name, "SP_FAN0_TACH");
          break;
        case SP_SENSOR_FAN1_TACH:
          sprintf(name, "SP_FAN1_TACH");
          break;
        case SP_SENSOR_AIR_FLOW:
          sprintf(name, "SP_AIR_FLOW");
          break;
        case SP_SENSOR_P5V:
          sprintf(name, "SP_P5V");
          break;
        case SP_SENSOR_P12V:
          sprintf(name, "SP_P12V");
          break;
        case SP_SENSOR_P3V3_STBY:
          sprintf(name, "SP_P3V3_STBY");
          break;
        case SP_SENSOR_P12V_SLOT1:
          sprintf(name, "SP_P12V_SLOT1");
          break;
        case SP_SENSOR_P12V_SLOT2:
          sprintf(name, "SP_P12V_SLOT2");
          break;
        case SP_SENSOR_P12V_SLOT3:
          sprintf(name, "SP_P12V_SLOT3");
          break;
        case SP_SENSOR_P12V_SLOT4:
          sprintf(name, "SP_P12V_SLOT4");
          break;
        case SP_SENSOR_P3V3:
          sprintf(name, "SP_P3V3");
          break;
        case SP_SENSOR_HSC_IN_VOLT:
          sprintf(name, "SP_HSC_IN_VOLT");
          break;
        case SP_SENSOR_HSC_OUT_CURR:
          sprintf(name, "SP_HSC_OUT_CURR");
          break;
        case SP_SENSOR_HSC_TEMP:
          sprintf(name, "SP_HSC_TEMP");
          break;
        case SP_SENSOR_HSC_IN_POWER:
          sprintf(name, "SP_HSC_IN_POWER");
          break;
      }
      break;
    case FRU_NIC:
      switch(sensor_num) {
        case MEZZ_SENSOR_TEMP:
          sprintf(name, "MEZZ_SENSOR_TEMP");
          break;
        case NIC_SOC_TEMP:
          sprintf(name, "NIC_SOC_TEMP (PLDM)");
          break;
        case PORT_0_TEMP:
          sprintf(name, "PORT_0_TEMP (PLDM)");
          break;
        case PORT_0_LINK_SPEED:
          sprintf(name, "PORT_0_LINK_SPEED (PLDM)");
          break;
        case NIC_HEALTH_STATE:
          sprintf(name, "NIC_HEALTH_STATE (PLDM)");
          break;
        case PORT_0_LINK_STATE:
          sprintf(name, "PORT_0_LINK_STATE (PLDM)");
          break;
      }
      break;
  }
  return 0;
}


int
yosemite_sensor_read(uint8_t fru, uint8_t sensor_num, void *value) {

  float volt;
  float curr;
  int ret;
  bool discrete;
  int i;

  switch (fru) {
    case FRU_SLOT1:
    case FRU_SLOT2:
    case FRU_SLOT3:
    case FRU_SLOT4:

      if (!(is_server_prsnt(fru))) {
        return -1;
      }

      ret = yosemite_sdr_init(fru);
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

    case FRU_SPB:
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
 			  case SP_SENSOR_P12V_SLOT1:
 			    return read_adc_value(ADC_PIN4, ADC_VALUE, (float*) value);
 			  case SP_SENSOR_P12V_SLOT2:
 			    return read_adc_value(ADC_PIN3, ADC_VALUE, (float*) value);
 			  case SP_SENSOR_P12V_SLOT3:
          return read_adc_value(ADC_PIN6, ADC_VALUE, (float*) value);
        case SP_SENSOR_P12V_SLOT4:
          return read_adc_value(ADC_PIN5, ADC_VALUE, (float*) value);
        case SP_SENSOR_P3V3:
          return read_adc_value(ADC_PIN7, ADC_VALUE, (float*) value);

        // Hot Swap Controller
        case SP_SENSOR_HSC_IN_VOLT:
          return read_hsc_value(HSC_IN_VOLT, HSC_DEVICE, hsc_r_sense, (float*) value);
        case SP_SENSOR_HSC_OUT_CURR:
          return read_hsc_value(HSC_OUT_CURR, HSC_DEVICE, hsc_r_sense, (float*) value);
        case SP_SENSOR_HSC_TEMP:
          return read_hsc_value(HSC_TEMP, HSC_DEVICE, hsc_r_sense, (float*) value);
        case SP_SENSOR_HSC_IN_POWER:
          return read_hsc_value(HSC_IN_POWER, HSC_DEVICE, hsc_r_sense, (float*) value);
      }
      break;

    case FRU_NIC:
      switch(sensor_num) {
      // Mezz Temp
        case MEZZ_SENSOR_TEMP:
          return read_nic_temp(MEZZ_SENSOR_TEMP, (float*) value);
      }
      break;
  }
  return EER_UNHANDLED;
}
