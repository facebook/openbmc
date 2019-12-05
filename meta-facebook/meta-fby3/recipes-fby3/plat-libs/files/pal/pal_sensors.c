#include <stdio.h>
#include <math.h>
#include <stdint.h>
#include <stdbool.h>
#include <fcntl.h>
#include <errno.h>
#include <syslog.h>
#include <sys/mman.h>
#include <string.h>
#include <ctype.h>
#include <openbmc/kv.h>
#include <openbmc/libgpio.h>
#include <openbmc/obmc-i2c.h>
#include <openbmc/ipmb.h>
#include <openbmc/obmc-sensors.h>
#include "pal.h"

//#define DEBUG

#define FAN_DIR "/sys/bus/platform/devices/1e786000.pwm-tacho-controller/hwmon/hwmon0"


size_t pal_pwm_cnt = 4;
size_t pal_tach_cnt = 8;
const char pal_pwm_list[] = "0, 1, 2, 4";
const char pal_tach_list[] = "0..7";

const uint8_t bmc_sensor_list[] = {
  BMC_SENSOR_OUTLET_TEMP,
  BMC_SENSOR_INLET_TEMP,
  BMC_SENSOR_P5V,
  BMC_SENSOR_P12V,
  BMC_SENSOR_P3V3_STBY,
  BMC_SENSOR_P1V15_BMC_STBY,
  BMC_SENSOR_P1V2_BMC_STBY,
  BMC_SENSOR_P2V5_BMC_STBY,
  BMC_SENSOR_HSC_TEMP,
  BMC_SENSOR_HSC_IN_VOLT,
};

const uint8_t nic_sensor_list[] = {
  NIC_SENSOR_MEZZ_TEMP,
};

// List of MB discrete sensors to be monitored
const uint8_t bmc_discrete_sensor_list[] = {
};
 
//ADM1278
PAL_ADM1278_INFO adm1278_info_list[] = {
  {ADM1278_VOLTAGE, 19599, 0, 100},
  {ADM1278_CURRENT, 800 * ADM1278_RSENSE, 20475, 10},
  {ADM1278_POWER, 6123 * ADM1278_RSENSE, 0, 100},
  {ADM1278_TEMP, 42, 31880, 10},
};

//HSC
PAL_HSC_INFO hsc_info_list[] = {
  {HSC_ID0, ADM1278_SLAVE_ADDR, adm1278_info_list},
};

size_t bmc_sensor_cnt = sizeof(bmc_sensor_list)/sizeof(uint8_t);
size_t nic_sensor_cnt = sizeof(nic_sensor_list)/sizeof(uint8_t);

int
pal_get_fru_sensor_list(uint8_t fru, uint8_t **sensor_list, int *cnt) {
  switch(fru) {
  case FRU_BMC:
    *sensor_list = (uint8_t *) bmc_sensor_list;
    *cnt = bmc_sensor_cnt;
    break;
  case FRU_NIC:
    *sensor_list = (uint8_t *) nic_sensor_list;
    *cnt = nic_sensor_cnt;
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

int
pal_get_fru_discrete_list(uint8_t fru, uint8_t **sensor_list, int *cnt) {
  switch(fru) {
    case FRU_BMC:
    default:
      if (fru > MAX_NUM_FRUS)
        return -1;
      // Nothing to read yet.
      *sensor_list = NULL;
      *cnt = 0;
  }
    return 0;
}

uint8_t
pal_get_fan_source(uint8_t fan_num) {

  switch (fan_num)
  {
    case FAN_0:
    case FAN_1:
      return PWM_0;

    case FAN_2:
    case FAN_3:
      return PWM_1;

    case FAN_4:
    case FAN_5:
      return PWM_2;

    case FAN_6:
    case FAN_7:
      return PWM_3;

    default:
      syslog(LOG_WARNING, "[%s] Catch unknown fan number - %d\n", __func__, fan_num);
      break;
  }

  return 0xff;
}

int pal_set_fan_speed(uint8_t fan, uint8_t pwm)
{
  char label[32] = {0};
  uint8_t pwm_num = fan;

  if (pwm_num > pal_pwm_cnt ||
      snprintf(label, sizeof(label), "pwm%d", pwm_num + 1) > sizeof(label)) {
    return -1;
  }
  return sensors_write_fan(label, (float)pwm);
}

int pal_get_fan_speed(uint8_t fan, int *rpm)
{
  char label[32] = {0};
  float value;
  int ret;

  if (fan > pal_tach_cnt ||
      snprintf(label, sizeof(label), "fan%d", fan + 1) > sizeof(label)) {
    syslog(LOG_WARNING, "%s: invalid fan#:%d", __func__, fan);
    return -1;
  }
  ret = sensors_read_fan(label, &value);
  *rpm = (int)value;
  syslog(LOG_WARNING, "%s() fan%d: %d", __func__, fan, *rpm);
  return ret;
}

int pal_get_fan_name(uint8_t num, char *name)
{
  if (num > pal_tach_cnt) {
    syslog(LOG_WARNING, "%s: invalid fan#:%d", __func__, num);
    return -1;
  }

  sprintf(name, "Fan %d",num);

  return 0;
}

int pal_get_pwm_value(uint8_t fan, uint8_t *pwm)
{
  char label[32] = {0};
  float value;
  int ret;
  uint8_t fan_src = 0;

  fan_src = pal_get_fan_source(fan);

  if (fan >= pal_tach_cnt || fan_src == 0xff ||
      snprintf(label, sizeof(label), "pwm%d", fan_src + 1) > sizeof(label)) {
    syslog(LOG_WARNING, "%s: invalid fan#:%d", __func__, fan);
    return -1;
  }

  ret = sensors_read_fan(label, &value);
  if (ret == 0)
    *pwm = (int)value;
  return ret;
}

static int
read_sysfs_temp(const char *chip, const char *label, float *value) {
  return sensors_read(chip, label, value);
}

static int
read_sysfs_volt(const char *label, float *value) {
  return sensors_read_adc(label, value);
}

static void
get_adm1278_info(uint8_t hsc_id, uint8_t type, uint8_t *addr, float* m, float* b, float* r) {

 *addr = hsc_info_list[hsc_id].slv_addr;
 *m = hsc_info_list[hsc_id].info[type].m;
 *b = hsc_info_list[hsc_id].info[type].b;
 *r = hsc_info_list[hsc_id].info[type].r;

 return;
}

static int
read_hsc_temp(float *value) {
  uint8_t tbuf[1] = {0x00};
  uint8_t rbuf[2] = {0x00};
  uint8_t tlen = 0;
  uint8_t rlen = 0;
  uint8_t addr = 0;
  float m, b, r;
  int retry = 3;
  int ret = -1;
  int fd = 0;

  fd = open("/dev/i2c-11", O_RDWR);
  if (fd < 0) {
    syslog(LOG_WARNING, "Failed to open bus 11");
    goto error_exit;
  }

  get_adm1278_info(HSC_ID0, ADM1278_TEMP, &addr, &m, &b, &r);

  tbuf[0] = PMBUS_READ_TEMP1;
  tlen = 1;
  rlen = 2;

  while ( ret < 0 && retry-- > 0 ) {
    ret = i2c_rdwr_msg_transfer(fd, addr, tbuf, tlen, rbuf, rlen);
  }

  if ( ret < 0 ) {
    ret = READING_NA;
    goto error_exit;
  }

  *value = ((float)(rbuf[1] << 8 | rbuf[0]) * r - b) / m;
error_exit:
  if ( fd > 0 ) close(fd);

  return ret;
}

static int
read_hsc_vin(float *value) {
  uint8_t tbuf[1] = {0x00};
  uint8_t rbuf[2] = {0x00};
  uint8_t tlen = 0;
  uint8_t rlen = 0;
  uint8_t addr = 0;
  float m, b, r;
  int retry = 3;
  int ret = -1;
  int fd = 0;

  fd = open("/dev/i2c-11", O_RDWR);
  if (fd < 0) {
    syslog(LOG_WARNING, "Failed to open bus 11");
    goto error_exit;
  }
  
  get_adm1278_info(HSC_ID0, ADM1278_VOLTAGE, &addr, &m, &b, &r);

  tbuf[0] = PMBUS_READ_VIN;
  tlen = 1;
  rlen = 2;

  while ( ret < 0 && retry-- > 0 ) {
    ret = i2c_rdwr_msg_transfer(fd, addr, tbuf, tlen, rbuf, rlen);
  }

  if ( ret < 0 ) {
    ret = READING_NA;
    goto error_exit;
  }

  *value = ((float)(rbuf[1] << 8 | rbuf[0]) * r - b) / m;
error_exit:
  if ( fd > 0 ) close(fd);

  return ret;
}

int
pal_sensor_read_raw(uint8_t fru, uint8_t sensor_num, void *value) {
  char key[MAX_KEY_LEN] = {0};
  char str[MAX_VALUE_LEN] = {0};
  char fru_name[32];
  int ret=0;

  pal_get_fru_name(fru, fru_name);
  sprintf(key, "%s_sensor%d", fru_name, sensor_num);

  switch(fru) {
    case FRU_BMC:
      switch (sensor_num) {
        case BMC_SENSOR_OUTLET_TEMP:
          ret = read_sysfs_temp("lm75-i2c-12-4f", "BMC_OUTLET_TEMP", (float*)value);
          break;
        case BMC_SENSOR_INLET_TEMP:
          ret = read_sysfs_temp("lm75-i2c-12-4e", "BMC_INLET_TEMP", (float*)value);
          break; 
        case BMC_SENSOR_P5V:
          ret = read_sysfs_volt("BMC_SENSOR_P5V", (float*)value);
          break;
        case BMC_SENSOR_P12V:
          ret = read_sysfs_volt("BMC_SENSOR_P12V", (float*)value);
          break;
        case BMC_SENSOR_P3V3_STBY:
          ret = read_sysfs_volt("BMC_SENSOR_P3V3_STBY", (float*)value);
          break;
        case BMC_SENSOR_P1V15_BMC_STBY:
          ret = read_sysfs_volt("BMC_SENSOR_P1V15_STBY", (float*)value);
          break;
        case BMC_SENSOR_P1V2_BMC_STBY:
          ret = read_sysfs_volt("BMC_SENSOR_P1V2_STBY", (float*)value);
          break;
        case BMC_SENSOR_P2V5_BMC_STBY:
          ret = read_sysfs_volt("BMC_SENSOR_P2V5_STBY", (float*)value);
          break;
        case BMC_SENSOR_HSC_TEMP:
          ret = read_hsc_temp((float*)value);
          break;
        case BMC_SENSOR_HSC_IN_VOLT:
          ret = read_hsc_vin((float*)value);
          break;
      }
    case FRU_NIC:
      switch (sensor_num) {
        case NIC_SENSOR_MEZZ_TEMP:
          read_sysfs_temp("tmp421-i2c-8-1f", "NIC_SENSOR_MEZZ_TEMP", (float*) value);
          break;
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
  if(kv_set(key, str, 0, 0) < 0) {
    syslog(LOG_WARNING, "pal_sensor_read_raw: cache_set key = %s, str = %s failed.", key, str);
    return -1;
  } else {
    return ret;
  }

  return 0;
}

int
pal_get_sensor_name(uint8_t fru, uint8_t sensor_num, char *name) {
  switch(fru) {
  case FRU_BMC:
    switch (sensor_num) {
      case BMC_SENSOR_INLET_TEMP:
        sprintf(name, "BMC_INLET_TEMP");
        break;
      case BMC_SENSOR_OUTLET_TEMP:
        sprintf(name, "BMC_OUTLET_TEMP");
        break;
      case BMC_SENSOR_P5V:
        sprintf(name, "BMC_SENSOR_P5V");
        break;
      case BMC_SENSOR_P12V:
        sprintf(name, "BMC_SENSOR_P12V");
        break;
      case BMC_SENSOR_P3V3_STBY:
        sprintf(name, "BMC_SENSOR_P3V3_STBY");
        break;
      case BMC_SENSOR_P1V15_BMC_STBY:
        sprintf(name, "BMC_SENSOR_P1V15_STBY");
        break;
      case BMC_SENSOR_P1V2_BMC_STBY:
        sprintf(name, "BMC_SENSOR_P1V2_STBY");
        break;
      case BMC_SENSOR_P2V5_BMC_STBY:
        sprintf(name, "BMC_SENSOR_P2V5_STBY");
        break;
      case BMC_SENSOR_FAN0_TACH:
        sprintf(name, "BMC_SENSOR_FAN0_TACH0");
        break;
      case BMC_SENSOR_FAN1_TACH:
        sprintf(name, "BMC_SENSOR_FAN0_TACH1");
        break;
      case BMC_SENSOR_FAN2_TACH:
        sprintf(name, "BMC_SENSOR_FAN0_TACH2");
        break;
      case BMC_SENSOR_FAN3_TACH:
        sprintf(name, "BMC_SENSOR_FAN0_TACH3");
        break;
      case BMC_SENSOR_FAN4_TACH:
        sprintf(name, "BMC_SENSOR_FAN0_TACH4");
        break;
      case BMC_SENSOR_FAN5_TACH:
        sprintf(name, "BMC_SENSOR_FAN0_TACH5");
        break;
      case BMC_SENSOR_FAN6_TACH:
        sprintf(name, "BMC_SENSOR_FAN0_TACH6");
        break;
      case BMC_SENSOR_FAN7_TACH:
        sprintf(name, "BMC_SENSOR_FAN0_TACH7");
        break;
      case BMC_SENSOR_HSC_TEMP:
        sprintf(name, "BMC_SENSOR_HSC_TEMP");
        break;
      case BMC_SENSOR_HSC_IN_VOLT:
        sprintf(name, "BMC_SENSOR_HSC_IN_VOLT");
        break;
    }
  case FRU_NIC:
    switch (sensor_num) {
      case NIC_SENSOR_MEZZ_TEMP:
        sprintf(name, "NIC_SENSOR_MEZZ_TEMP");
        break;
    }
    break;
    
  default:
    return -1;
  }
  return 0;
}

int
pal_get_sensor_threshold(uint8_t fru, uint8_t sensor_num, uint8_t thresh, void *value) {
  float *val = (float*) value;
  *val = 0;
  return 0;
}

int
pal_get_sensor_units(uint8_t fru, uint8_t sensor_num, char *units) {

  switch(sensor_num) {
    case BMC_SENSOR_OUTLET_TEMP:
    case BMC_SENSOR_INLET_TEMP:
    case NIC_SENSOR_MEZZ_TEMP:
    case BMC_SENSOR_HSC_TEMP:
      sprintf(units, "C");
      break;
    case BMC_SENSOR_P5V:
    case BMC_SENSOR_P12V:
    case BMC_SENSOR_P3V3_STBY:
    case BMC_SENSOR_P1V15_BMC_STBY:
    case BMC_SENSOR_P1V2_BMC_STBY:
    case BMC_SENSOR_P2V5_BMC_STBY:
    case BMC_SENSOR_HSC_IN_VOLT:
      sprintf(units, "Volts");
      break;
    case BMC_SENSOR_FAN0_TACH:
    case BMC_SENSOR_FAN1_TACH:
    case BMC_SENSOR_FAN2_TACH:
    case BMC_SENSOR_FAN3_TACH:
    case BMC_SENSOR_FAN4_TACH:
    case BMC_SENSOR_FAN5_TACH:
    case BMC_SENSOR_FAN6_TACH:
    case BMC_SENSOR_FAN7_TACH:
      sprintf(units, "RPM");
      break;
    default:
      syslog(LOG_WARNING, "%s() unknown sensor number %x", __func__, sensor_num);
    break;
  }
  return 0;
}

