#ifndef __PAL_SENSORS_H__
#define __PAL_SENSORS_H__

#include <openbmc/obmc_pal_sensors.h>

#ifdef __cplusplus
extern "C" {
#endif

#define MAX_SNR_READ_RETRY 3
#define MAX_DEVICE_NAME_SIZE   (128)
#define MAX_READ_RETRY 10

//AMD1278 CMD INFO
#define ADM1278_SLAVE_ADDR  (0x22)
#define ADM1278_RSENSE      (0.15)

//PMBus
#define PMBUS_READ_VIN     (0x88)
#define PMBUS_READ_IOUT    (0x8C)
#define PMBUS_READ_TEMP1   (0x8D)
#define PMBUS_READ_TEMP2   (0x8E)
#define PMBUS_READ_PIN     (0x97)

//NVMe INFO
#define NVMe_SMBUS_ADDR           (0xD4)
#define NVMe_GET_STATUS_CMD       (0x00)
#define NVMe_GET_STATUS_LEN       (8)
#define NVMe_TEMP_REG             (0x03)

//ADM1278 INFO
enum {
  ADM1278_VOLTAGE = 0,
  ADM1278_CURRENT,
  ADM1278_POWER,
  ADM1278_TEMP,
};

enum {
  HSC_VOLTAGE = 0,
  HSC_CURRENT,
  HSC_POWER,
  HSC_TEMP,
};

//NIC INFO
enum {
  MEZZ0 = 0,
  MEZZ1,
  MEZZ2,
  MEZZ3,
  MEZZ4,
  MEZZ5,
  MEZZ6,
  MEZZ7,
};

//NVME INFO
enum {
  BAY0_FTEMP_ID = 0,
  BAY0_RTEMP_ID,
  BAY1_FTEMP_ID,
  BAY1_RTEMP_ID,
};

//NVME INFO
enum {
  PAX_ID0 = 0,
  PAX_ID1,
  PAX_ID2,
  PAX_ID3,
};

enum {
  VR_ID0 = 0,
  VR_ID1,
  VR_ID2,
  VR_ID3,
  vr_NUM,
};

enum {
  BAY_ID0 = 0,
  BAY_ID1,
};

//Sensor Table
enum {
  MB_NIC_0_TEMP  = 0x10,
  MB_NIC_0_VOLT  = 0x11,
  MB_NIC_0_CURR  = 0x12,
  MB_NIC_0_POWER = 0x13,
  MB_NIC_1_TEMP  = 0x14,
  MB_NIC_1_VOLT  = 0x15,
  MB_NIC_1_CURR  = 0x16,
  MB_NIC_1_POWER = 0x17,
  MB_NIC_2_TEMP  = 0x18,
  MB_NIC_2_VOLT  = 0x19,
  MB_NIC_2_CURR  = 0x1A,
  MB_NIC_2_POWER = 0x1B,
  MB_NIC_3_TEMP  = 0x1C,
  MB_NIC_3_VOLT  = 0x1D,
  MB_NIC_3_CURR  = 0x1E,
  MB_NIC_3_POWER = 0x1F,

  MB_NIC_4_TEMP  = 0x20,
  MB_NIC_4_VOLT  = 0x21,
  MB_NIC_4_CURR  = 0x22,
  MB_NIC_4_POWER = 0x23,
  MB_NIC_5_TEMP  = 0x24,
  MB_NIC_5_VOLT  = 0x25,
  MB_NIC_5_CURR  = 0x26,
  MB_NIC_5_POWER = 0x27,
  MB_NIC_6_TEMP  = 0x28,
  MB_NIC_6_VOLT  = 0x29,
  MB_NIC_6_CURR  = 0x2A,
  MB_NIC_6_POWER = 0x2B,
  MB_NIC_7_TEMP  = 0x2C,
  MB_NIC_7_VOLT  = 0x2D,
  MB_NIC_7_CURR  = 0x2E,
  MB_NIC_7_POWER = 0x2F,

  BAY_0_FTEMP = 0x30,
  BAY_0_RTEMP = 0x31,
  BAY_0_NVME_CTEMP = 0x32,
  BAY_0_0_NVME_CTEMP = 0x33,
  BAY_0_1_NVME_CTEMP = 0x34,
  BAY_0_2_NVME_CTEMP = 0x35,
  BAY_0_3_NVME_CTEMP = 0x36,
  BAY_0_4_NVME_CTEMP = 0x37,
  BAY_0_5_NVME_CTEMP = 0x38,
  BAY_0_6_NVME_CTEMP = 0x39,
  BAY_0_7_NVME_CTEMP = 0x3A,
  BAY_0_VOL  = 0x3B,
  BAY_0_IOUT = 0x3C,
  BAY_0_POUT = 0x3D,

  BAY_1_FTEMP = 0x40,
  BAY_1_RTEMP = 0x41,
  BAY_1_NVME_CTEMP = 0x42,
  BAY_1_0_NVME_CTEMP = 0x43,
  BAY_1_1_NVME_CTEMP = 0x44,
  BAY_1_2_NVME_CTEMP = 0x45,
  BAY_1_3_NVME_CTEMP = 0x46,
  BAY_1_4_NVME_CTEMP = 0x47,
  BAY_1_5_NVME_CTEMP = 0x48,
  BAY_1_6_NVME_CTEMP = 0x49,
  BAY_1_7_NVME_CTEMP = 0x4A,
  BAY_1_VOL  = 0x4B,
  BAY_1_IOUT = 0x4C,
  BAY_1_POUT = 0x4D,

  PDB_HSC_TEMP = 0x50,
  PDB_HSC_VIN = 0x51,
  PDB_HSC_IOUT = 0x52,
  PDB_HSC_PIN = 0x53,
  MB_P12V_AUX  = 0X54,
  MB_P3V3_STBY = 0X55,
  MB_P5V_STBY  = 0X56,
  MB_P3V3      = 0X57,
  MB_P3V3_PAX  = 0X58,
  MB_P3V_BAT   = 0X59,
  MB_P2V5_AUX  = 0X5A,
  MB_P1V2_AUX  = 0X5B,
  MB_P1V15_AUX = 0X5C,

  MB_VR_P0V8_VDD0_TEMP = 0x60,
  MB_VR_P0V8_VDD0_VOUT,
  MB_VR_P0V8_VDD0_CURR,
  MB_VR_P0V8_VDD0_POWER,

  MB_VR_P1V0_AVD0_TEMP,
  MB_VR_P1V0_AVD0_VOUT,
  MB_VR_P1V0_AVD0_CURR,
  MB_VR_P1V0_AVD0_POWER,

  MB_VR_P0V8_VDD1_TEMP,
  MB_VR_P0V8_VDD1_VOUT,
  MB_VR_P0V8_VDD1_CURR,
  MB_VR_P0V8_VDD1_POWER,

  MB_VR_P1V0_AVD1_TEMP,
  MB_VR_P1V0_AVD1_VOUT,
  MB_VR_P1V0_AVD1_CURR,
  MB_VR_P1V0_AVD1_POWER,

  MB_VR_P0V8_VDD2_TEMP,
  MB_VR_P0V8_VDD2_VOUT,
  MB_VR_P0V8_VDD2_CURR,
  MB_VR_P0V8_VDD2_POWER,

  MB_VR_P1V0_AVD2_TEMP,
  MB_VR_P1V0_AVD2_VOUT,
  MB_VR_P1V0_AVD2_CURR,
  MB_VR_P1V0_AVD2_POWER,

  MB_VR_P0V8_VDD3_TEMP,
  MB_VR_P0V8_VDD3_VOUT,
  MB_VR_P0V8_VDD3_CURR,
  MB_VR_P0V8_VDD3_POWER,

  MB_VR_P1V0_AVD3_TEMP,
  MB_VR_P1V0_AVD3_VOUT,
  MB_VR_P1V0_AVD3_CURR,
  MB_VR_P1V0_AVD3_POWER,

  MB_PAX_0_TEMP = 0x80,
  MB_PAX_1_TEMP = 0x81,
  MB_PAX_2_TEMP = 0x82,
  MB_PAX_3_TEMP = 0x83,
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

  SYSTEM_INLET_TEMP = 0x90,
  SYSTEM_INLET_REMOTE_TEMP = 0x91,
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

//NIC INFO
enum {
  NIC0 = 0,
  NIC1,
  NIC2,
  NIC3,
  NIC4,
  NIC5,
  NIC6,
  NIC7,
};

enum {
  HSC_ID0,
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

typedef struct {
  uint8_t type;
  float m;
  float b;
  float r;
} PAL_ADM1278_INFO;

typedef struct {
  uint8_t id;
  uint8_t slv_addr;
  PAL_ADM1278_INFO* info;
} PAL_HSC_INFO;


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
  SYS_TEMP,
  SYS_REMOTE_TEMP,
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
