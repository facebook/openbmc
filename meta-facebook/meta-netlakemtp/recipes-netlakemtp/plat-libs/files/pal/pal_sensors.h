#ifndef __PAL_SENSORS_H__
#define __PAL_SENSORS_H__

#include <openbmc/obmc_pal_sensors.h>

#define MAX_SENSOR_NUM               (0xFF)
#define MAX_SENSOR_NAME_SIZE         (32)
#define MAX_PMBUS_SUP_CMD_CNT 16

#define SENSOR_RETRY_TIME                   3
#define SENSOR_RETRY_INTERVAL               100000

#define HSC_NO_SET_PAGE              0
#define ADM1278_RSENSE               0.5

#define MAX_PMBUS_SUP_PMBUS_TYPE_CNT  2
#define MAX_HSC_SUP_CMD_CNT 3

#define SENSOR_RETRY_TIME                   3
#define SENSOR_RETRY_INTERVAL_USEC          100000

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
  uint8_t reading_available;
  PAL_SENSOR_THRESHOLD snr_thresh;
  uint8_t units;
  uint32_t poll_invernal;
} PAL_SENSOR_MAP;

enum {
  STBY_READING = 0,
  POWER_ON_READING,
  POST_COMPLT_READING,
};

enum {
  UNSET_UNIT = 0,
  TEMP = 1,
  CURR,
  VOLT,
  FAN,
  PERCENT,
  POWER,
  FLOW,
};

// Sensors under Server Board (SERVER)
enum {
  SERVER_INLET_TEMP = 0x01,
  SERVER_OUTLET_TEMP = 0x02,
  SERVER_SOC_TEMP = 0x03,
  SERVER_DIMMA_TEMP = 0x04,
  SERVER_DIMMB_TEMP = 0x05,
  SERVER_A_P12V_STBY_NETLAKE_VOL = 0x07,
  SERVER_A_P3V3_STBY_NETLAKE_VOL = 0x08,
  SERVER_A_P1V8_STBY_NETLAKE_VOL = 0x09,
  SERVER_A_P5V_STBY_NETLAKE_VOL = 0x0A,
  SERVER_A_P1V05_STBY_NETLAKE_VOL = 0x0B,
  SERVER_A_PVNN_PCH_VOL = 0x0C,
  SERVER_A_PVCCIN_VOL = 0x0D,
  SERVER_A_PVCCANA_CPU_VOL = 0x0E,
  SERVER_A_PVDDQ_ABC_CPU_VOL = 0x0F,
  SERVER_A_PVPP_ABC_CPU_VOL = 0x10,
  SERVER_A_P3V3_NETLAKE_VOL = 0x11,
  SERVER_P1V8_STBY_VOL = 0x13,
  SERVER_P1V05_STBY_VOL = 0x14,
  SERVER_PVNN_PCH_VOL = 0x15,
  SERVER_PVCCIN_VOL = 0x16,
  SERVER_PVCCANCPU_VOL = 0x17,
  SERVER_PVDDQ_ABC_CPU_VOL = 0x18,
  SERVER_P1V8_STBY_CUR = 0x1A,
  SERVER_P1V05_STBY_CUR = 0x1B,
  SERVER_PVNN_PCH_CUR = 0x1C,
  SERVER_PVCCIN_CUR = 0x1D,
  SERVER_PVCCANCPU_CUR = 0x1E,
  SERVER_PVDDQ_ABC_CPU_CUR = 0x1F,
  SERVER_P1V8_STBY_PWR = 0x21,
  SERVER_P1V05_STBY_PWR = 0x22,
  SERVER_PVNN_PCH_PWR = 0x23,
  SERVER_PVCCIN_PWR = 0x24,
  SERVER_PVCCANCPU_PWR = 0x25,
  SERVER_PVDDQ_ABC_CPU_PWR = 0x26,
};

enum {
  BMC_HSC_OUTPUT_VOL = 0x30,
  BMC_P12V_STBY_MTP_VOL = 0x31,
  BMC_P12V_COME_VOL = 0x32,
  BMC_P3V_BAT_MTP_VOL = 0x33,
  BMC_P3V3_STBY_MTP_VOL = 0x34,
  BMC_P5V_STBY_VOL = 0x35,
  BMC_P12V_NIC_MTP_VOL = 0x36,
  BMC_P3V3_NIC_MTP_VOL = 0x37,
  BMC_HSC_OUTPUT_CUR = 0x38,
  BMC_P12V_NIC_MTP_CURR = 0x39,
  BMC_P3V3_NIC_MTP_CURR = 0x3A,
  BMC_HSC_INPUT_PWR = 0x3B,
};

// Sensors under Power Distribution Board (PDB)
enum {
  FAN0_TACH = 0x50,
  FAN1_TACH = 0x51,
  FAN2_TACH = 0x52,
  FAN3_TACH = 0x53,
};

// Sensors under Front IO Board (FIO)
enum {
  FIO_INLET_TEMP = 0x60,
};

// GENERIC I2C Sensors
enum {
  MB_INLET = 0,
  MB_OUTLET,
  FIO_INLET,
  SOC_TEMP,
  DIMMA_TEMP,
  DIMMB_TEMP,
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
};

// PMBUS_INFO
enum {
  VR_P1V8_STBY_VOL = 0,
  VR_P1V05_STBY_VOL,
  VR_PVNN_PCH_VOL,
  VR_PVCCIN_VOL,
  VR_PVCCANCPU_VOL,
  VR_PVDDQ_ABC_CPU_VOL,
  VR_P1V8_STBY_CUR,
  VR_P1V05_STBY_CUR,
  VR_PVNN_PCH_CUR,
  VR_PVCCIN_CUR,
  VR_PVCCANCPU_CUR,
  VR_PVDDQ_ABC_CPU_CUR,
  VR_P1V8_STBY_PWR,
  VR_P1V05_STBY_PWR,
  VR_PVNN_PCH_PWR,
  VR_PVCCIN_PWR,
  VR_PVCCANCPU_PWR,
  VR_PVDDQ_ABC_CPU_PWR,
};

// HSC_INFO
enum {
  HSC_VOUT = 0,
  HSC_IOUT,
  HSC_PIN,
};

// CPLD ADC
enum {
  CPLD_ADC_P12V_STBY = 0,
  CPLD_ADC_P3V3_STBY,
  CPLD_ADC_P1V8_STBY,
  CPLD_ADC_P5V_STBY,
  CPLD_ADC_P1V05_STBY,
  CPLD_ADC_PVNN_PCH,
  CPLD_ADC_PVCCIN,
  CPLD_ADC_PVCCANA,
  CPLD_ADC_PVDDQ,
  CPLD_ADC_PVPP_CPU,
  CPLD_ADC_P3V3,
};

enum {
  NORMAL_POLL_INTERVAL = 1000,
  ONE_HOUR_INVERVAL = 3600000,
};

enum {
  FAN0 = 0,
  FAN1,
  FAN2,
  FAN3,
};

enum pmbus_dev_type{
  TDA38640 = 0,
  XDPE15284D,
  MP9941GMJT,
  MP2993GU,
};

enum pmbus_page{
  PAGE0 = 0,
  PAGE1,
};

enum pmbus_read_byte{
  READ_BYTE = 0x1,
  READ_WORD = 0x2,
};

enum pmbus_read_type{
  LINEAR11 = 0,
  VOUT_MODE,
};

// PMBUS_CMD_INFO
enum cmdlist{
  CMD_PAGE = 0x00,
  CMD_VIN = 0x88,
  CMD_IIN = 0x89,
  CMD_VOUT = 0x8b,
  CMD_IOUT = 0x8c,
  CMD_POUT = 0x96,
  CMD_PIN = 0x97,
};

typedef struct {
  uint8_t read_cmd;
  uint8_t read_byte;
  uint8_t read_type;
} pmbus_cmd_info;

typedef struct {
  pmbus_cmd_info pmbus_cmd_list[MAX_PMBUS_SUP_CMD_CNT];
} pmbus_dev_info;

static pmbus_dev_info pmbus_dev_list[] = {
  [TDA38640] = {
  {{CMD_VOUT, READ_WORD, VOUT_MODE},
  {CMD_VIN, READ_WORD, LINEAR11},
  {CMD_IOUT, READ_WORD, LINEAR11},
  {CMD_IIN, READ_WORD, LINEAR11},
  {CMD_POUT, READ_WORD, LINEAR11},
  {CMD_PIN, READ_WORD, LINEAR11},}},
  [XDPE15284D] = {
  {{CMD_VOUT, READ_WORD, VOUT_MODE},
  {CMD_VIN, READ_WORD, LINEAR11},
  {CMD_IOUT, READ_WORD, LINEAR11},
  {CMD_IIN, READ_WORD, LINEAR11},
  {CMD_POUT, READ_WORD, LINEAR11},
  {CMD_PIN, READ_WORD, LINEAR11},}},
  [MP9941GMJT] = {
  {{CMD_VOUT, READ_WORD, LINEAR11},
  {CMD_VIN, READ_WORD, LINEAR11},
  {CMD_IOUT, READ_WORD, LINEAR11},
  {CMD_IIN, READ_WORD, LINEAR11},
  {CMD_POUT, READ_WORD, LINEAR11},
  {CMD_PIN, READ_WORD, LINEAR11},}},
  [MP2993GU] = {
  {{CMD_VOUT, READ_WORD, LINEAR11},
  {CMD_VIN, READ_WORD, LINEAR11},
  {CMD_IOUT, READ_WORD, LINEAR11},
  {CMD_IIN, READ_WORD, LINEAR11},
  {CMD_POUT, READ_WORD, LINEAR11},
  {CMD_PIN, READ_WORD, LINEAR11},}},
};

enum {
  SKU1 = 0,
  SKU2,
};

typedef struct {
  char *chip;
  char *label;
} PAL_DEV_INFO;

typedef struct {
  uint8_t type;
  uint8_t page;
} PAL_PMBUS_TYPE;

typedef struct {
  uint8_t bus;
  uint8_t slv_addr;
  uint8_t offset;
  PAL_PMBUS_TYPE sku_pmbus_type[MAX_PMBUS_SUP_PMBUS_TYPE_CNT];
} PAL_PMBUS_INFO;

enum hsc_dev_type{
  ADM1278 = 0,
};

typedef struct {
  uint8_t read_cmd;
  uint8_t read_byte;
  float m;
  float b;
  float r;
} hsc_cmd_info;

typedef struct {
  hsc_cmd_info hsc_cmd_list[MAX_HSC_SUP_CMD_CNT];
} hsc_dev_info;

static hsc_dev_info hsc_dev_list[] = {
  [ADM1278] = {
  {{CMD_VOUT, READ_WORD, 19599, 0, 100},
  {CMD_IOUT, READ_WORD, 800 * ADM1278_RSENSE, 20475, 10},
  {CMD_PIN, READ_WORD, 6123 * ADM1278_RSENSE, 0, 100},}},
};

typedef struct {
  uint8_t page;
  uint8_t read_byte;
  uint8_t read_type;
} pmbus_i2c_info;

typedef struct {
  uint8_t hsc_type_cnt;
  uint8_t hsc_cmd_cnt;
} hsc_i2c_info;

typedef struct {
  uint8_t lowbyte_cmd;
  float resistor_ratio;
} PAL_CPLD_ADC_INFO;

#endif

