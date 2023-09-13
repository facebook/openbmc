#ifndef __PAL_SENSORS_H__
#define __PAL_SENSORS_H__

#include <openbmc/obmc_pal_sensors.h>
#include "pal_mb_sensors.h"
#include "pal_swb_sensors.h"
#include "pal_hgx_sensors.h"
#include "pal_bb_sensors.h"
#include "pal_meb_sensors.h"
#include "pal.h"

//CPU INFO
#define PECI_CPU0_ADDR            (0x30)
#define PECI_CPU1_ADDR            (0x31)

#define VR_SNR_PER_CPU 5

#define KEY_ACB_CARD_CONFIG "acb_card_config"

#define SENSOR_NUM_BIC_ALERT 0x10

typedef struct {
  float ucr_thresh;
  float unc_thresh;
  float unr_thresh;
  float lcr_thresh;
  float lnc_thresh;
  float lnr_thresh;
  float pos_hyst;
  float neg_hyst;
} PAL_SENSOR_THRESHOLD;

enum {
  TEMP = 1,
  CURR,
  VOLT,
  mVOLT,
  FAN,
  POWER,
  ENRGY,
  PRESS,
  STATE,
};

typedef struct {
  char* snr_name;
  uint8_t id;
  int (*read_sensor) (uint8_t fru, uint8_t sensor_num, float *value);
  uint8_t stby_read;
  PAL_SENSOR_THRESHOLD snr_thresh;
  uint8_t units;
} PAL_SENSOR_MAP;

struct snr_map {
  uint8_t fru;
  PAL_SENSOR_MAP *map;
  bool polling;
};

//HSC INFO
enum {
  HSC_VOLTAGE = 0,
  HSC_CURRENT,
  HSC_POWER,
  HSC_TEMP,
};

typedef struct {
  uint8_t type;
  float m;
  float b;
  float r;
} PAL_ATTR_INFO;

typedef struct {
  uint8_t id;
  uint8_t bus;
  uint8_t addr;
  PAL_ATTR_INFO* info;
} PAL_HSC_INFO;

//12V HSC INFO
enum {
  HSC_ID0,
  HSC_12V_CNT,
};

//I2C type
enum {
  I2C_BYTE = 0,
  I2C_WORD,
};

typedef struct {
  uint8_t id;
  uint8_t bus;
  uint8_t slv_addr;
} PAL_I2C_BUS_INFO;

//ADC INFO
enum {
  ADC0 = 0,
  ADC1,
  ADC2,
  ADC3,
  ADC4,
  ADC5,
  ADC6,
  ADC7,
  ADC8,
  ADC9,
  ADC10,
  ADC11,
  ADC12,
  ADC13,
  ADC14,
  ADC_NUM_CNT,
};

enum {
  DPM_0,
  DPM_1,
  DPM_2,
  DPM_3,
  DPM_4,
  DPM_NUM_CNT,
};

enum {
  E1S_0,
};

typedef struct {
  uint8_t id;
  uint8_t bus;
  uint8_t slv_addr;
  float vbus_scale;
  float curr_scale;
  float pwr_scale;
} PAL_DPM_DEV_INFO;

//VR  INFO
enum {
  VR_ID0 = 0,
  VR_ID1,
  VR_ID2,
  VR_ID3,
  VR_ID4,
  VR_ID5,
  VR_ID6,
  VR_ID7,
  VR_ID8,
  VR_ID9,
  VR_ID10,
  VR_ID11,
  VR_NUM_CNT,
};

enum {
  FAN_CTRL_ID0,
  FAN_CTRL_ID1,
};

enum {
  SKIP_POWER_ON_SENSOR_READING_TIME = 10, // second
};

// Aggregate sensors
enum {
  AGGREGATE_SENSOR_SYSTEM_AIRFLOW = 0x0,
  AGGREGATE_SENSOR_SYSTEM_POWER = 0x1
};

typedef struct {
  uint8_t status;
  uint8_t integer_l;
  uint8_t integer_h;
  uint8_t fraction_l;
  uint8_t fraction_h;
} oem_1s_sensor_reading_resp;

int retry_skip_handle(uint8_t retry_curr, uint8_t retry_max);
int retry_err_handle(uint8_t retry_curr, uint8_t retry_max);
int get_pldm_sensor(uint8_t bus, uint8_t eid, uint8_t sensor_num, float *value);
int mb_vr_polling_ctrl(bool enable);
int read_vr_temp(uint8_t fru, uint8_t sensor_num, float *value);
int read_vr_vout(uint8_t fru, uint8_t sensor_num, float *value);
int read_vr_iout(uint8_t fru, uint8_t sensor_num, float *value);
int read_vr_pout(uint8_t fru, uint8_t sensor_num, float *value);

extern const char pal_server_list[];
#endif
