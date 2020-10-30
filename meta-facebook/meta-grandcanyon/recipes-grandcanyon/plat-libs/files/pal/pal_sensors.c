#include <stdio.h>
#include <math.h>
#include <stdint.h>
#include <stdbool.h>
#include <fcntl.h>
#include <errno.h>
#include <syslog.h>
#include <sys/mman.h>
#include <sys/file.h>
#include <string.h>
#include <ctype.h>
#include <openbmc/kv.h>
#include <openbmc/libgpio.h>
#include <openbmc/ipmb.h>
#include <openbmc/obmc-sensors.h>
#include <openbmc/obmc-i2c.h>
#include "pal.h"

static int read_adc_val(uint8_t adc_id, float *value);
static int read_temp(uint8_t id, float *value);
static int read_nic_temp(uint8_t nic_id, float *value);

//{SensorName, ID, FUNCTION, PWR_STATUS, {UCR, UNC, UNR, LCR, LNR, LNC, Pos, Neg}, unit}
PAL_SENSOR_MAP uic_sensor_map[] = {
  [UIC_ADC_P12V_DPB] =
  {"UIC_ADC_P12V_DPB", ADC0, read_adc_val, true, {0, 0, 0, 0, 0, 0, 0, 0}, VOLT},
  [UIC_ADC_P12V_STBY] =
  {"UIC_ADC_P12V_STBY", ADC1, read_adc_val, true, {0, 0, 0, 0, 0, 0, 0, 0}, VOLT},
  [UIC_ADC_P5V_STBY] =
  {"UIC_ADC_P5V_STBY", ADC2, read_adc_val, true, {0, 0, 0, 0, 0, 0, 0, 0}, VOLT},
  [UIC_ADC_P3V3_STBY] =
  {"UIC_ADC_P3V3_STBY", ADC3, read_adc_val, true, {0, 0, 0, 0, 0, 0, 0, 0}, VOLT},
  [UIC_ADC_P3V3_RGM] =
  {"UIC_ADC_P3V3_RGM", ADC4, read_adc_val, true, {0, 0, 0, 0, 0, 0, 0, 0}, VOLT},
  [UIC_ADC_P2V5_STBY] =
  {"UIC_ADC_P2V5_STBY", ADC5, read_adc_val, true, {0, 0, 0, 0, 0, 0, 0, 0}, VOLT},
  [UIC_ADC_P1V8_STBY] =
  {"UIC_ADC_P1V8_STBY", ADC6, read_adc_val, true, {0, 0, 0, 0, 0, 0, 0, 0}, VOLT},
  [UIC_ADC_P1V2_STBY] =
  {"UIC_ADC_P1V2_STBY", ADC7, read_adc_val, true, {0, 0, 0, 0, 0, 0, 0, 0}, VOLT},
  [UIC_ADC_P1V0_STBY] =
  {"UIC_ADC_P1V0_STBY", ADC8, read_adc_val, true, {0, 0, 0, 0, 0, 0, 0, 0}, VOLT},
  [UIC_P12V_UIC_ISENSE] =
  {"UIC_P12V_UIC_ISENSE", ADC9, read_adc_val, true, {0, 0, 0, 0, 0, 0, 0, 0}, VOLT},
  [UIC_INLET_TEMP] =
  {"UIC_INLET_TEMP", TEMP_INLET, read_temp, true, {0, 0, 0, 0, 0, 0, 0, 0}, TEMP},
};

PAL_SENSOR_MAP nic_sensor_map[] = {
  [MEZZ_SENSOR_TEMP] =
  {"MEZZ_SENSOR_TEMP", MEZZ, read_nic_temp, true, {0, 0, 0, 0, 0, 0, 0, 0}, TEMP},
};

const uint8_t uic_sensor_list[] = {
  UIC_ADC_P12V_DPB ,
  UIC_ADC_P12V_STBY,
  UIC_ADC_P5V_STBY,
  UIC_ADC_P3V3_STBY,
  UIC_ADC_P3V3_RGM,
  UIC_ADC_P2V5_STBY,
  UIC_ADC_P1V8_STBY,
  UIC_ADC_P1V2_STBY,
  UIC_ADC_P1V0_STBY,
  UIC_P12V_UIC_ISENSE,
  UIC_INLET_TEMP,
};

const uint8_t nic_sensor_list[] = {
  MEZZ_SENSOR_TEMP,
};

PAL_I2C_BUS_INFO nic_info_list[] = {
  {MEZZ, I2C_BUS_8, NIC_INFO_SLAVE_ADDR},
};

const char *adc_label[] = {
  "ADC_P12V_DPB",
  "ADC_P12V_STBY",
  "ADC_P5V_STBY",
  "ADC_P3V3_STBY",
  "ADC_P3V3_RGM",
  "ADC_P2V5_STBY",
  "ADC_P1V8_STBY",
  "ADC_P1V2_STBY",
  "ADC_P1V0_STBY",
  "P12V_UIC_ISENSE",
};

PAL_TEMP_DEV_INFO temp_dev_list[] = {
  {"tmp75-i2c-4-4a",  "UIC_INLET_TEMP"},
};

size_t uic_sensor_cnt = sizeof(uic_sensor_list)/sizeof(uint8_t);
size_t nic_sensor_cnt = sizeof(nic_sensor_list)/sizeof(uint8_t);

int
pal_get_fru_sensor_list(uint8_t fru, uint8_t **sensor_list, int *cnt) {
  switch(fru) {
  case FRU_UIC:
    *sensor_list = (uint8_t *) uic_sensor_list;
    *cnt = uic_sensor_cnt;
    break;
  case FRU_NIC:
    *sensor_list = (uint8_t *) nic_sensor_list;
    *cnt = nic_sensor_cnt;
    break;
  default:
    if (fru > MAX_NUM_FRUS) {
      return ERR_UNKNOWN_FRU;
    }
    // Nothing to read yet.
    *sensor_list = NULL;
    *cnt = 0;
    break;
  }

  return 0;
}

static int
read_adc_val(uint8_t adc_id, float *value) {
  if (adc_id >= ARRAY_SIZE(adc_label)) {
    return ERR_SENSOR_NA;
  }
  return sensors_read_adc(adc_label[adc_id], value);
}

static int
read_temp(uint8_t id, float *value) {
  if (id >= ARRAY_SIZE(temp_dev_list)) {
    return ERR_SENSOR_NA;
  }
  return sensors_read(temp_dev_list[id].chip, temp_dev_list[id].label, value);
}

static int
read_nic_temp(uint8_t nic_id, float *value) {
  int fd = 0, ret = -1;
  uint8_t retry = 3, tlen = 0, rlen = 0, addr = 0, bus = 0;
  uint8_t tbuf[16] = {0};
  uint8_t rbuf[16] = {0};

  if (nic_id >= ARRAY_SIZE(nic_sensor_list)) {
    return ERR_SENSOR_NA;
  }

  bus = nic_info_list[nic_id].bus;
  addr = nic_info_list[nic_id].slv_addr;

  fd = i2c_cdev_slave_open(bus, addr >> 1, I2C_SLAVE_FORCE_CLAIM);
  if (fd < 0) {
    syslog(LOG_WARNING, "%s() Failed to open I2C bus %d\n", __func__, bus);
    return ERR_SENSOR_NA;
  }

  //Temp Register
  tbuf[0] = NIC_INFO_TEMP_CMD;
  tlen = 1;
  rlen = 1;

  while (ret < 0 && retry-- > 0 ) {
    ret = i2c_rdwr_msg_transfer(fd, addr, tbuf, tlen, rbuf, rlen);
  }

#ifdef DEBUG
  syslog(LOG_DEBUG, "%s() Temp[%d]=%x bus=%x slavaddr=%x\n", __func__, nic_id, rbuf[0], bus, addr);
#endif

  if (ret >= 0) {
    *value = (float)(rbuf[0]);
  }
  close(fd);

  return ret;
}

int
pal_sensor_read_raw(uint8_t fru, uint8_t sensor_num, void *value) {
  char key[MAX_KEY_LEN] = {0};
  char str[MAX_VALUE_LEN] = {0};
  char fru_name[32] = {0};
  int ret = 0;
  uint8_t id = 0;

  if (pal_get_fru_name(fru, fru_name)) {
    syslog(LOG_WARNING, "%s() Fail to get fru%d name\n", __func__, fru);
    return ERR_SENSOR_NA;
  }

  switch(fru) {
  case FRU_UIC:
    id = uic_sensor_map[sensor_num].id;
    ret = uic_sensor_map[sensor_num].read_sensor(id, (float*) value);
    break;
  case FRU_NIC:
    id = nic_sensor_map[sensor_num].id;
    ret = nic_sensor_map[sensor_num].read_sensor(id, (float*) value);
    break;
  default:
    return ERR_SENSOR_NA;
  }

  if (ret != 0) {
    if (ret == ERR_SENSOR_NA || ret == -1) {
      strncpy(str, "NA", sizeof(str));
    } else {
      return ret;
    }
  } else {
    snprintf(str, sizeof(str), "%.2f",*((float*)value));
  }

  snprintf(key, sizeof(key), "%s_sensor%d", fru_name, sensor_num);

  if(kv_set(key, str, 0, 0) < 0) {
    syslog(LOG_WARNING, "%s() cache_set key = %s, str = %s failed.\n", __func__, key, str);
    return ERR_SENSOR_NA;
  }

  return ret;
}

int
pal_get_sensor_name(uint8_t fru, uint8_t sensor_num, char *name) {
  switch(fru) {
    case FRU_UIC:
      sprintf(name, "%s", uic_sensor_map[sensor_num].snr_name);
      break;
    case FRU_NIC:
      sprintf(name, "%s", nic_sensor_map[sensor_num].snr_name);
      break;
    default:
      return ERR_UNKNOWN_FRU;
  }

  return 0;
}

int
pal_get_sensor_threshold(uint8_t fru, uint8_t sensor_num, uint8_t thresh, void *value) {
  float *val = (float*) value;
  PAL_SENSOR_MAP * sensor_map = NULL;

  switch (fru) {
  case FRU_UIC:
    sensor_map = uic_sensor_map;
    break;
  case FRU_NIC:
    sensor_map = nic_sensor_map;
    break;
  default:
    return ERR_UNKNOWN_FRU;
  }

  switch(thresh) {
  case UCR_THRESH:
    *val = sensor_map[sensor_num].snr_thresh.ucr_thresh;
    break;
  case UNC_THRESH:
    *val = sensor_map[sensor_num].snr_thresh.unc_thresh;
    break;
  case UNR_THRESH:
    *val = sensor_map[sensor_num].snr_thresh.unr_thresh;
    break;
  case LCR_THRESH:
    *val = sensor_map[sensor_num].snr_thresh.lcr_thresh;
    break;
  case LNC_THRESH:
    *val = sensor_map[sensor_num].snr_thresh.lnc_thresh;
    break;
  case LNR_THRESH:
    *val = sensor_map[sensor_num].snr_thresh.lnr_thresh;
    break;
  case POS_HYST:
    *val = sensor_map[sensor_num].snr_thresh.pos_hyst;
    break;
  case NEG_HYST:
    *val = sensor_map[sensor_num].snr_thresh.neg_hyst;
    break;
  default:
    syslog(LOG_WARNING, "%s() Threshold type error value=%d\n", __func__, thresh);
    return -1;
  }

  return 0;
}

int
pal_get_sensor_units(uint8_t fru, uint8_t sensor_num, char *units) {
  uint8_t sensor_units = 0;

  switch (fru) {
  case FRU_UIC:
    sensor_units = uic_sensor_map[sensor_num].units;
    break;
  case FRU_NIC:
    sensor_units = nic_sensor_map[sensor_num].units;
    break;
  default:
    return ERR_UNKNOWN_FRU;
  }

  switch(sensor_units) {
    case UNSET_UNIT:
      strcpy(units, "");
      break;
    case TEMP:
      sprintf(units, "C");
      break;
    case FAN:
      sprintf(units, "RPM");
      break;
    case PERCENT:
      sprintf(units, "%%");
      break;
    case VOLT:
      sprintf(units, "Volts");
      break;
    case CURR:
      sprintf(units, "Amps");
      break;
    case POWER:
      sprintf(units, "Watts");
      break;
    default:
      syslog(LOG_WARNING, "%s() unit not found, sensor number: %x, unit: %u\n", __func__, sensor_num, sensor_units);
    break;
  }

  return 0;
}
