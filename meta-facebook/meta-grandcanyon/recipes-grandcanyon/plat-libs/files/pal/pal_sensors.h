#ifndef __PAL_SENSORS_H__
#define __PAL_SENSORS_H__

#include <openbmc/obmc_pal_sensors.h>
#include <facebook/exp.h>

#define MAX_SENSOR_NUM               (0xFF)
#define MAX_DEVICE_NAME_SIZE         (128)
#define MAX_SENSOR_NAME_SIZE         (32)

#define NIC_INFO_SLAVE_ADDR          (0x3E)
#define NIC_INFO_TEMP_CMD            (0x1)

#define MAX_EXP_IPMB_SENSOR_COUNT    40

#define MAX_RETRY                    3
#define MAX_SDR_PATH                 32
#define SDR_PATH                     "/tmp/sdr_%s.bin"
#define SDR_TYPE_FULL_SENSOR_RECORD  0x1

// BIC read error
#define BIC_SNR_FLAG_READ_NA         (1 << 5)

#define READING_SKIP                 1

// MAX15090 INFO
#define MAX15090_IGAIN               (220)  // unit: uA/A
#define MAX15090_RSENSE              (4220) // unit: ohm

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
  char* snr_name;
  uint8_t id;
  int (*read_sensor) (uint8_t id, float *value);
  uint8_t stby_read;
  PAL_SENSOR_THRESHOLD snr_thresh;
  uint8_t units;
} PAL_SENSOR_MAP;

enum {
  UNSET_UNIT = 0,
  TEMP = 1,
  CURR,
  VOLT,
  FAN,
  PERCENT,
  POWER,
};

// Sensors under Barton Springs (BS)
enum {
  BS_INLET_TEMP = 0x01,
  BS_P12V_STBY = 0x02,
  BS_P3V3_STBY = 0x03,
  BS_P1V05_STBY = 0x04,
  BS_P3V_BAT = 0x05,
  BS_PVNN_PCH_STBY = 0x06,
  BS_HSC_TEMP = 0x07,
  BS_HSC_IN_VOLT = 0x08,
  BS_HSC_IN_PWR = 0x09,
  BS_HSC_OUT_CUR = 0x0A,
  BS_BOOT_DRV_TEMP = 0x0B,
  BS_PCH_TEMP = 0x0C,
  BS_DIMM1_TEMP = 0x0D,
  BS_DIMM2_TEMP = 0x0E,
  BS_DIMM3_TEMP = 0x0F,
  BS_DIMM4_TEMP = 0x10,
  BS_VR_VCCIO_TEMP = 0x11,
  BS_CPU_TEMP = 0x12,
};

// Sensors under User Interface Card (UIC)
enum {
  UIC_ADC_P12V_DPB = 0x10,
  UIC_ADC_P12V_STBY = 0x11,
  UIC_ADC_P5V_STBY = 0x12,
  UIC_ADC_P3V3_STBY = 0x13,
  UIC_ADC_P3V3_RGM = 0x14,
  UIC_ADC_P2V5_STBY = 0x15,
  UIC_ADC_P1V8_STBY = 0x16,
  UIC_ADC_P1V2_STBY = 0x17,
  UIC_ADC_P1V0_STBY = 0x18,
  UIC_P12V_ISENSE_CUR = 0x19,
  UIC_INLET_TEMP = 0x1A,
};

// Sensors under Storage Controller Card (SCC)
enum {
  SCC_EXP_TEMP = 0x40,
  SCC_IOC_TEMP = 0x41,
  SCC_TEMP_1 = 0x42,
  SCC_TEMP_2 = 0x43,
  SCC_P3V3_E_SENSE = 0x44,
  SCC_P1V8_E_SENSE = 0x45,
  SCC_P1V5_E_SENSE = 0x46,
  SCC_P0V92_E_SENSE = 0x47,
  SCC_P1V8_C_SENSE = 0x48,
  SCC_P1V5_C_SENSE = 0x49,
  SCC_P0V865_C_SENSE = 0x4A,
  SCC_HSC_P12V = 0x4B,
  SCC_HSC_CUR = 0x4C,
  SCC_HSC_PWR = 0x4D,
};

// Sensors under Drive Plane Board (DPB)
enum {
  HDD_SMART_TEMP_00 = 0x60,
  HDD_SMART_TEMP_01 = 0x61,
  HDD_SMART_TEMP_02 = 0x62,
  HDD_SMART_TEMP_03 = 0x63,
  HDD_SMART_TEMP_04 = 0x64,
  HDD_SMART_TEMP_05 = 0x65,
  HDD_SMART_TEMP_06 = 0x66,
  HDD_SMART_TEMP_07 = 0x67,
  HDD_SMART_TEMP_08 = 0x68,
  HDD_SMART_TEMP_09 = 0x69,
  HDD_SMART_TEMP_10 = 0x6A,
  HDD_SMART_TEMP_11 = 0x6B,
  HDD_SMART_TEMP_12 = 0x6C,
  HDD_SMART_TEMP_13 = 0x6D,
  HDD_SMART_TEMP_14 = 0x6E,
  HDD_SMART_TEMP_15 = 0x6F,
  HDD_SMART_TEMP_16 = 0x70,
  HDD_SMART_TEMP_17 = 0x71,
  HDD_SMART_TEMP_18 = 0x72,
  HDD_SMART_TEMP_19 = 0x73,
  HDD_SMART_TEMP_20 = 0x74,
  HDD_SMART_TEMP_21 = 0x75,
  HDD_SMART_TEMP_22 = 0x76,
  HDD_SMART_TEMP_23 = 0x77,
  HDD_SMART_TEMP_24 = 0x78,
  HDD_SMART_TEMP_25 = 0x79,
  HDD_SMART_TEMP_26 = 0x7A,
  HDD_SMART_TEMP_27 = 0x7B,
  HDD_SMART_TEMP_28 = 0x7C,
  HDD_SMART_TEMP_29 = 0x7D,
  HDD_SMART_TEMP_30 = 0x7E,
  HDD_SMART_TEMP_31 = 0x7F,
  HDD_SMART_TEMP_32 = 0x80,
  HDD_SMART_TEMP_33 = 0x81,
  HDD_SMART_TEMP_34 = 0x82,
  HDD_SMART_TEMP_35 = 0x83,
  HDD_P5V_SENSE_0   = 0x84,
  HDD_P12V_SENSE_0  = 0x85,
  HDD_P5V_SENSE_1   = 0x86,
  HDD_P12V_SENSE_1  = 0x87,
  HDD_P5V_SENSE_2   = 0x88,
  HDD_P12V_SENSE_2  = 0x89,
  HDD_P5V_SENSE_3   = 0x8A,
  HDD_P12V_SENSE_3  = 0x8B,
  HDD_P5V_SENSE_4   = 0x8C,
  HDD_P12V_SENSE_4  = 0x8D,
  HDD_P5V_SENSE_5   = 0x8E,
  HDD_P12V_SENSE_5  = 0x8F,
  HDD_P5V_SENSE_6   = 0x90,
  HDD_P12V_SENSE_6  = 0x91,
  HDD_P5V_SENSE_7   = 0x92,
  HDD_P12V_SENSE_7  = 0x93,
  HDD_P5V_SENSE_8   = 0x94,
  HDD_P12V_SENSE_8  = 0x95,
  HDD_P5V_SENSE_9   = 0x96,
  HDD_P12V_SENSE_9  = 0x97,
  HDD_P5V_SENSE_10  = 0x98,
  HDD_P12V_SENSE_10 = 0x99,
  HDD_P5V_SENSE_11  = 0x9A,
  HDD_P12V_SENSE_11 = 0x9B,
  HDD_P5V_SENSE_12  = 0x9C,
  HDD_P12V_SENSE_12 = 0x9D,
  HDD_P5V_SENSE_13  = 0x9E,
  HDD_P12V_SENSE_13 = 0x9F,
  HDD_P5V_SENSE_14  = 0xA0,
  HDD_P12V_SENSE_14 = 0xA1,
  HDD_P5V_SENSE_15  = 0xA2,
  HDD_P12V_SENSE_15 = 0xA3,
  HDD_P5V_SENSE_16  = 0xA4,
  HDD_P12V_SENSE_16 = 0xA5,
  HDD_P5V_SENSE_17  = 0xA6,
  HDD_P12V_SENSE_17 = 0xA7,
  HDD_P5V_SENSE_18  = 0xA8,
  HDD_P12V_SENSE_18 = 0xA9,
  HDD_P5V_SENSE_19  = 0xAA,
  HDD_P12V_SENSE_19 = 0xAB,
  HDD_P5V_SENSE_20  = 0xAC,
  HDD_P12V_SENSE_20 = 0xAD,
  HDD_P5V_SENSE_21  = 0xAE,
  HDD_P12V_SENSE_21 = 0xAF,
  HDD_P5V_SENSE_22  = 0xB0,
  HDD_P12V_SENSE_22 = 0xB1,
  HDD_P5V_SENSE_23  = 0xB2,
  HDD_P12V_SENSE_23 = 0xB3,
  HDD_P5V_SENSE_24  = 0xB4,
  HDD_P12V_SENSE_24 = 0xB5,
  HDD_P5V_SENSE_25  = 0xB6,
  HDD_P12V_SENSE_25 = 0xB7,
  HDD_P5V_SENSE_26  = 0xB8,
  HDD_P12V_SENSE_26 = 0xB9,
  HDD_P5V_SENSE_27  = 0xBA,
  HDD_P12V_SENSE_27 = 0xBB,
  HDD_P5V_SENSE_28  = 0xBC,
  HDD_P12V_SENSE_28 = 0xBD,
  HDD_P5V_SENSE_29  = 0xBE,
  HDD_P12V_SENSE_29 = 0xBF,
  HDD_P5V_SENSE_30  = 0xC0,
  HDD_P12V_SENSE_30 = 0xC1,
  HDD_P5V_SENSE_31  = 0xC2,
  HDD_P12V_SENSE_31 = 0xC3,
  HDD_P5V_SENSE_32  = 0xC4,
  HDD_P12V_SENSE_32 = 0xC5,
  HDD_P5V_SENSE_33  = 0xC6,
  HDD_P12V_SENSE_33 = 0xC7,
  HDD_P5V_SENSE_34  = 0xC8,
  HDD_P12V_SENSE_34 = 0xC9,
  HDD_P5V_SENSE_35  = 0xCA,
  HDD_P12V_SENSE_35 = 0xCB,
  DPB_INLET_TEMP_1 = 0xD0,
  DPB_INLET_TEMP_2 = 0xD1,
  DPB_OUTLET_TEMP = 0xD2,
  DPB_HSC_P12V = 0xD3,
  DPB_P3V3_STBY_SENSE = 0xD4,
  DPB_P5V_1_SYS_SENSE = 0xD5,
  DPB_P5V_2_SYS_SENSE = 0xD6,
  DPB_P5V_3_SYS_SENSE = 0xD7,
  DPB_P5V_4_SYS_SENSE = 0xD8,
  DPB_HSC_P12V_CLIP = 0xD9,
  DPB_HSC_CUR = 0xDA,
  DPB_HSC_CUR_CLIP = 0xDB,
  DPB_HSC_PWR = 0xDC,
  DPB_HSC_PWR_CLIP = 0xDD,
  FAN_1_FRONT = 0xE0,
  FAN_1_REAR = 0xE1,
  FAN_2_FRONT = 0xE2,
  FAN_2_REAR = 0xE3,
  FAN_3_FRONT = 0xE4,
  FAN_3_REAR = 0xE5,
  FAN_4_FRONT = 0xE6,
  FAN_4_REAR = 0xE7,
};

// NIC sensors
enum {
  MEZZ_SENSOR_TEMP = 0x82,

  // PLDM numeric sensors
  NIC_SOC_TEMP = PLDM_NUMERIC_SENSOR_START,
  PORT_0_TEMP,
  PORT_0_LINK_SPEED,

  // PLDM state sensors
  NIC_HEALTH_STATE = PLDM_STATE_SENSOR_START,
  PORT_0_LINK_STATE,
};

// ADC_INFO
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
};

// GENERIC I2C Sensors
enum {
  TEMP_INLET = 0,
};

// NIC INFO
enum {
  MEZZ = 0,
};

// EXPANDER
enum {
  EXPANDER = 0,
};

typedef struct {
  uint8_t id;
  uint8_t bus;
  uint8_t slv_addr;
} PAL_I2C_BUS_INFO;

typedef struct {
  char *chip;
  char *label;
} PAL_TEMP_DEV_INFO;

typedef struct {
  uint8_t sensor_num;
  uint8_t raw_data_1;
  uint8_t raw_data_2;
  uint8_t sensor_status;
  uint8_t reserved;
} EXPANDER_SENSOR_DATA;

#endif
