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
#define MAX_SDR_LEN 64
#define SDR_PATH "/tmp/sdr_%s.bin"

static sensor_info_t g_sinfo[MAX_NUM_FRUS][MAX_SENSOR_NUM] = {0};
static bool sdr_init_done[MAX_NUM_FRUS] = {false};

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

const uint8_t bic_sensor_list[] = {
  //BIC - threshold sensors
  BIC_SENSOR_INLET_TEMP,
  BIC_SENSOR_OUTLET_TEMP,
  BIC_SENSOR_FIO_TEMP,
  BIC_SENSOR_PCH_TEMP,
  BIC_SENSOR_CPU_THERMAL_MARGIN,
  BIC_SENSOR_CPU_TEMP,
  BIC_SENSOR_DIMMA0_TEMP,
  BIC_SENSOR_DIMMB0_TEMP,
  BIC_SENSOR_DIMMC0_TEMP,
  BIC_SENSOR_DIMMD0_TEMP,
  BIC_SENSOR_DIMME0_TEMP,
  BIC_SENSOR_DIMMF0_TEMP,
//  BIC_SENSOR_M2A_TEMP,
//  BIC_SENSOR_M2B_TEMP,
  BIC_SENSOR_HSC_TEMP,
  BIC_SENSOR_VCCIN_VR_TEMP,
  BIC_SENSOR_VCCSA_VR_TEMP,
  BIC_SENSOR_VCCIO_VR_Temp,
  BIC_SENSOR_P3V3_STBY_VR_TEMP,
  BIC_SENSOR_PVDDQ_ABC_VR_TEMP,
  BIC_SEnSOR_PVDDQ_DEF_VR_TEMP,

  //BIC - voltage sensors
  BIC_SENSOR_P3V3_M2A_VOL,
  BIC_SENSOR_P3V3_M2B_VOL,
  BIC_SENSOR_P12V_STBY_VOL,
  BIC_SENSOR_P3V_BAT_VOL,
  BIC_SENSOR_P3V3_STBY_VOL,
  BIC_SENSOR_P1V05_PCH_STBY_VOL,
  BIC_SENSOR_PVNN_PCH_STBY_VOL,
  BIC_SENSOR_P3V3_VOL,
  BIC_SENSOR_HSC_INPUT_VOL,
  BIC_SENSOR_VCCIN_VR_VOL,
  BIC_SENSOR_VCCSA_VR_VOL,
  BIC_SENSOR_VCCIO_VR_VOL,
  BIC_SENSOR_P3V3_STBY_VR_VOL,
  BIC_PVDDQ_ABC_VR_VOL,
  BIC_PVDDQ_DEF_VR_VOL,

  //BIC - current sensors
  BIC_SENSOR_P3V3_M2A_CUR,
  BIC_SENSOR_P3V3_M2B_CUR,
  BIC_SENSOR_HSC_OUTPUT_CUR1,
  BIC_SENSOR_HSC_OUTPUT_CUR2,
  BIC_SENSOR_VCCIN_VR_CUR,
  BIC_SENSOR_VCCSA_VR_CUR,
  BIC_SENSOR_VCCIO_VR_CUR,
  BIC_SENSOR_P3V3_STBY_VR_CUR,
  BIC_SENSOR_PVDDQ_ABC_VR_CUR,
  BIC_SENSOR_PVDDQ_DEF_VR_CUR,

  //BIC - power sensors
  BIC_SENSOR_P3V3_M2A_PWR,
  BIC_SENSOR_P3V3_M2B_PWR,
  BIC_SENSOR_HSC_INPUT_PWR1,
  BIC_SENSOR_HSC_INPUT_PWR2,
  BIC_SENSOR_VCCIN_VR_POUT,
  BIC_SENSOR_VCCSA_VR_POUT,
  BIC_SENSOR_VCCIO_VR_POUT,
  BIC_SENSOR_P3V3_STBY_VR_POUT,
  BIC_SENSOR_PVDDQ_ABC_VR_POUT,
  BIC_SENSOR_PVDDQ_DEF_VR_POUT,  
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
size_t bic_sensor_cnt = sizeof(bic_sensor_list)/sizeof(uint8_t);

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
  case FRU_SLOT1:
  case FRU_SLOT2:
  case FRU_SLOT3:
  case FRU_SLOT4:
    *sensor_list = (uint8_t *) bic_sensor_list;
    *cnt = bic_sensor_cnt;
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

static int
pal_bic_sensor_read_raw(uint8_t fru, uint8_t sensor_num, float *value){
#define BIC_SENSOR_READ_NA 0x20
  int ret = 0;
  uint8_t power_status = 0;
  ipmi_sensor_reading_t sensor = {0};
  sdr_full_t *sdr = NULL;

  if ( bic_get_server_power_status(fru, &power_status) < 0 || power_status != SERVER_POWER_ON) {
    //syslog(LOG_WARNING, "%s() Failed to run bic_get_server_power_status(). fru%d, snr#0x%x, pwr_sts:%d", __func__, fru, sensor_num, power_status);
    return READING_NA;
  }

  ret = bic_get_sensor_reading(fru, sensor_num, &sensor, NONE_INTF);
  if ( ret < 0 ) {
    syslog(LOG_WARNING, "%s() Failed to run bic_get_sensor_reading(). snr#0x%x", __func__, sensor_num);
    return READING_NA;
  }

  if (sensor.flags & BIC_SENSOR_READ_NA) {
    //syslog(LOG_WARNING, "%s() sensor@0x%x flags is NA", __func__, sensor_num);
    return READING_NA;
  }

  sdr = &g_sinfo[fru-1][sensor_num].sdr;
  //syslog(LOG_WARNING, "%s() fru %x, sensor_num:0x%x, val:0x%x, type: %x", __func__, fru, sensor_num, sensor.value, sdr->type);
  if ( sdr->type != 1 ) {
    *value = sensor.value;
    return 0;
  } 

  // y = (mx + b * 10^b_exp) * 10^r_exp
  int x;
  uint8_t m_lsb, m_msb;
  uint16_t m = 0;
  uint8_t b_lsb, b_msb;
  uint16_t b = 0;
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

  //syslog(LOG_WARNING, "%s() snr#0x%x raw:%x m=%x b=%x b_exp=%x r_exp=%x", __func__, sensor_num, x, m, b, b_exp, r_exp);  
  *value = ((m * x) + (b * pow(10, b_exp))) * (pow(10, r_exp));

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
    case FRU_SLOT1:
    case FRU_SLOT2:
    case FRU_SLOT3:
    case FRU_SLOT4:
      ret = pal_bic_sensor_read_raw(fru, sensor_num, (float*)value);
      break;
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
pal_sensor_sdr_path(uint8_t fru, char *path) {
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
    case FRU_BMC:
      sprintf(fru_name, "%s", "bmc");
    break;
    case FRU_NIC:
      sprintf(fru_name, "%s", "nic");
    break;

    default:
      syslog(LOG_WARNING, "%s() Wrong fru id %d", __func__, fru);
    return -1;
  }

  sprintf(path, SDR_PATH, fru_name);
  if (access(path, F_OK) == -1) {
    return -1;
  }

  return 0;
}

static int
_sdr_init(char *path, sensor_info_t *sinfo) {
  int fd;
  int ret = 0;
  uint8_t buf[MAX_SDR_LEN] = {0};
  uint8_t bytes_rd = 0;
  uint8_t snr_num = 0;
  sdr_full_t *sdr;
  int retry = 3;

  while ( access(path, F_OK) == -1 && retry > 0 ) {
    syslog(LOG_WARNING, "%s() Failed to access %s, wait a second.\n", __func__, path);
    retry--;
    sleep(3);
  }

  fd = open(path, O_RDONLY);
  if (fd < 0) {
    syslog(LOG_WARNING, "%s() Failed to open %s after retry 3 times\n", __func__, path);
    goto error_exit;
  }

  ret = pal_flock_retry(fd);
  if (ret == -1) {
    syslog(LOG_WARNING, "%s() Failed to flock on %s", __func__, path);
    goto error_exit;
  }

  while ((bytes_rd = read(fd, buf, sizeof(sdr_full_t))) > 0) {
    if (bytes_rd != sizeof(sdr_full_t)) {
      syslog(LOG_WARNING, "%s() read returns %d bytes\n", __func__, bytes_rd);
      goto error_exit;
    }

    sdr = (sdr_full_t *) buf;
    snr_num = sdr->sensor_num;
    sinfo[snr_num].valid = true;
    memcpy(&sinfo[snr_num].sdr, sdr, sizeof(sdr_full_t));
    //syslog(LOG_WARNING, "%s() copy num: 0x%x:%s success", __func__, snr_num, sdr->str);
  }

error_exit:

  if ( fd > 0 ) {
    if ( pal_unflock_retry(fd) < 0 ) syslog(LOG_WARNING, "%s() Failed to unflock on %s", __func__, path);
    close(fd);
  }

  return ret;
}

static bool
pal_is_sdr_init(uint8_t fru) {
  return sdr_init_done[fru - 1];
}

static void
pal_set_sdr_init(uint8_t fru, bool set) {
  sdr_init_done[fru - 1] = set;
}

int
pal_sensor_sdr_init(uint8_t fru, sensor_info_t *sinfo) {
  char path[64] = {0};
  int retry = 3;
  int ret = 0;

  //syslog(LOG_WARNING, "%s() pal_is_sdr_init  bool %d, fru %d, snr_num: %x\n", __func__, pal_is_sdr_init(fru), fru, g_sinfo[fru-1][1].sdr.sensor_num);
  if ( true == pal_is_sdr_init(fru) ) {
    memcpy(sinfo, g_sinfo[fru-1], sizeof(sensor_info_t) * MAX_SENSOR_NUM); 
    goto error_exit;
  }

  ret = pal_sensor_sdr_path(fru, path);
  if ( ret < 0 ) {
    //syslog(LOG_WARNING, "%s() Failed to run pal_sensor_sdr_path\n", __func__);
    goto error_exit;
  }
  
  while ( retry-- > 0 ) {
    ret = _sdr_init(path, sinfo);
    if ( ret < 0 ) {
      syslog(LOG_WARNING, "%s() Failed to run _sdr_init, retry..%d\n", __func__, retry);
      sleep(1);
      continue;
    } else {
      memcpy(g_sinfo[fru-1], sinfo, sizeof(sensor_info_t) * MAX_SENSOR_NUM);
      pal_set_sdr_init(fru, true);
      break;
    }
  }

error_exit:
  return ret;
}

int
pal_sdr_init(uint8_t fru) {

  if ( false == pal_is_sdr_init(fru) ) {

    sensor_info_t *sinfo = g_sinfo[fru-1];

    if (pal_sensor_sdr_init(fru, sinfo) < 0) {
      //syslog(LOG_WARNING, "%s() Failed to run pal_sensor_sdr_init fru%d\n", __func__, fru);
      return ERR_NOT_READY;
    }

    pal_set_sdr_init(fru, true);
  }

  return 0;
}

int
pal_get_sensor_units(uint8_t fru, uint8_t sensor_num, char *units) {
  int ret = 0;

  if (fru == FRU_SLOT1 || fru == FRU_SLOT2 || \
      fru == FRU_SLOT3 || fru == FRU_SLOT4) {
    ret = pal_sdr_init(fru);
    strcpy(units, "");
    return ret;
  }

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

