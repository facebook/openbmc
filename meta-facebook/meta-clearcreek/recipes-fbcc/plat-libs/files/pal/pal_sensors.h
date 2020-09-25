#ifndef __PAL_SENSORS_H__
#define __PAL_SENSORS_H__

#include <openbmc/obmc_pal_sensors.h>

#ifdef __cplusplus
extern "C" {
#endif

#define MAX_SNR_READ_RETRY 3
#define MAX_DEVICE_NAME_SIZE   (128)

//Sensor Table
enum {
  MB_P12V_AUX  = 0X54,
  MB_P3V3_STBY = 0X55,
  MB_P5V_STBY  = 0X56,
  MB_P3V3      = 0X57,
  MB_P3V3_PAX  = 0X58,
  MB_P3V_BAT   = 0X59,
  MB_P2V5_AUX  = 0X5A,
  MB_P1V2_AUX  = 0X5B,
  MB_P1V15_AUX = 0X5C,

  //ADC128 VOLT/CURR/POWER
  PDB_FAN0_VOLT = 0x84,
  PDB_FAN0_CURR = 0x85,
  PDB_FAN0_PWR  = 0x86,
  PDB_FAN1_VOLT = 0x87,
  PDB_FAN1_CURR = 0x88,
  PDB_FAN1_PWR  = 0x89,
  PDB_FAN2_VOLT = 0x8A,
  PDB_FAN2_CURR = 0x8B,
  PDB_FAN2_PWR  = 0x8C,
  PDB_FAN3_VOLT = 0x8D,
  PDB_FAN3_CURR = 0x8E,
  PDB_FAN3_PWR  = 0x8F,

  PDB_INLET_TEMP_L = 0x96,
  PDB_INLET_REMOTE_TEMP_L = 0x97,
  PDB_INLET_TEMP_R = 0x98,
  PDB_INLET_REMOTE_TEMP_R = 0x99,
};

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
};

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

typedef struct {
  uint8_t id;
  uint8_t bus;
  uint8_t slv_addr;
} PAL_I2C_BUS_INFO;

typedef struct {
  char* snr_name;
  uint8_t id;
  int (*read_sensor) (uint8_t id, float *value);
  uint8_t stby_read;
  PAL_SENSOR_THRESHOLD snr_thresh;
  uint8_t units;
} PAL_SENSOR_MAP;

enum {
  TEMP = 1,
  CURR,
  VOLT,
  FAN,
  POWER,
  STATE,
};

enum {
  INLET_TEMP_L = 0,
  INLET_REMOTE_TEMP_L,
  INLET_TEMP_R,
  INLET_REMOTE_TEMP_R,
};

enum {
  FAN_ID0 = 0,
  FAN_ID1,
  FAN_ID2,
  FAN_ID3,
};

int pal_set_fan_speed(uint8_t fan, uint8_t pwm);
int pal_get_fan_speed(uint8_t fan, int *rpm);
int pal_get_fan_name(uint8_t num, char *name);
int pal_get_pwm_value(uint8_t fan_num, uint8_t *value);

#ifdef __cplusplus
} // extern "C"
#endif

#endif /* __PAL_SENSORS_H__ */
