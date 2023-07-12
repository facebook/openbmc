#include <stdio.h>
#include <syslog.h>
#include <openbmc/libgpio.h>
#include <openbmc/obmc-i2c.h>
#include <openbmc/obmc-sensors.h>
#include <openbmc/kv.h>
#include <openbmc/sensor-correction.h>
#include <math.h>
#include "pal.h"
#include "pal_common.h"

//#define DEBUG
#define FAN_PRESNT_TEMPLATE       "FAN%d_PRESENT"

size_t pal_pwm_cnt = FAN_PWM_CNT;
size_t pal_tach_cnt = FAN_TACH_CNT;
const char pal_pwm_list[] = "0,1,2,3,4,5,6,7,8,9,10,11,12,13,14,15";
const char pal_tach_list[] = "0,1,2,3,4,5,6,7,8,9,10,11,12,13,14,15,16,17,18,19,20,21,22,23,24,25,26,27,28,29,30,31";
//static float InletCalibration = 0;

//Fan CTRL
static int get_max31790_rpm(uint8_t fru, uint8_t sensor_num, float *value);
static int get_max31790_duty(uint8_t fan_id, uint8_t* pwm);
static int set_max31790_duty(uint8_t fan_id, uint8_t pwm);
static int get_nct7363y_rpm(uint8_t fru, uint8_t sensor_num, float *value);
static int get_nct7363y_duty(uint8_t fan_id, uint8_t* pwm);
static int set_nct7363y_duty(uint8_t fan_id, uint8_t pwm);
//Sensor Read

static int read_fan_speed(uint8_t fru, uint8_t sensor_num, float *value);
static int read_nic0_power(uint8_t fru, uint8_t sensor_num, float *value);
static int read_nic1_power(uint8_t fru, uint8_t sensor_num, float *value);
static int read_bb_sensor(uint8_t fru, uint8_t sensor_num, float *value);
static int read_hsc_pin(uint8_t fru, uint8_t sensor_num, float *value);
FAN_CTRL_DEV fan_ctrl_map[] = {
  {FAN_CTRL_ID0, get_max31790_rpm, get_max31790_duty, set_max31790_duty},
  {FAN_CTRL_ID1, get_nct7363y_rpm, get_nct7363y_duty, set_nct7363y_duty},
};

#define VPDB_HSC0_ADDR              (0x20)
#define HPDB_HSC1_ADDR              (0x38)
#define HPDB_HSC2_ADDR              (0x26)
#define EIN_ROLLOVER_CNT            (0x10000)
#define EIN_SAMPLE_CNT              (0x1000000)
#define EIN_ENERGY_CNT              (0x800000)
#define ADM1272_REG_READ_EIN_EXT    (0xDC)


PAL_I2C_BUS_INFO adm1272_info_list[] = {
  {VPDB_HSC_ID0, I2C_BUS_38, VPDB_HSC0_ADDR},
  {HPDB_HSC_ID1, I2C_BUS_39, HPDB_HSC1_ADDR},
  {HPDB_HSC_ID2, I2C_BUS_39, HPDB_HSC2_ADDR},
};

const uint8_t nic0_sensor_list[] = {
  NIC0_SNR_MEZZ0_TEMP,
  NIC0_SNR_MEZZ0_P3V3_VOUT,
  NIC0_SNR_MEZZ0_P12V_IOUT,
  NIC0_SNR_MEZZ0_P12V_POUT,
};

const uint8_t nic1_sensor_list[] = {
  NIC1_SNR_MEZZ1_TEMP,
  NIC1_SNR_MEZZ1_P3V3_VOUT,
  NIC1_SNR_MEZZ1_P12V_IOUT,
  NIC1_SNR_MEZZ1_P12V_POUT,
};

const uint8_t vpdb_sensor_list[] = {
  PDBV_SNR_HSC0_VIN,
  PDBV_SNR_HSC0_VOUT,
  PDBV_SNR_HSC0_IOUT,
  PDBV_SNR_HSC0_PIN,
  PDBV_SNR_HSC0_TEMP,
  PDBV_SNR_ADC128_P3V3_AUX,
};

const uint8_t vpdb_adc_sensor_list[] = {
  PDBV_SNR_CABLE0_PLUS,
  PDBV_SNR_CABLE0_MINUS,
};


const uint8_t vpdb_3brick_sensor_list[] = {
  PDBV_SNR_BRICK0_VOUT,
  PDBV_SNR_BRICK0_IOUT,
  PDBV_SNR_BRICK0_TEMP,
  PDBV_SNR_BRICK1_VOUT,
  PDBV_SNR_BRICK1_IOUT,
  PDBV_SNR_BRICK1_TEMP,
  PDBV_SNR_BRICK2_VOUT,
  PDBV_SNR_BRICK2_IOUT,
  PDBV_SNR_BRICK2_TEMP,
};

const uint8_t vpdb_1brick_sensor_list[] = {
  PDBV_SNR_BRICK0_VOUT,
  PDBV_SNR_BRICK0_IOUT,
  PDBV_SNR_BRICK0_TEMP,
};


const uint8_t hpdb_sensor_list[] = {
  PDBH_SNR_HSC1_VIN,
  PDBH_SNR_HSC1_VOUT,
  PDBH_SNR_HSC1_IOUT,
  PDBH_SNR_HSC1_PIN,
  PDBH_SNR_HSC1_TEMP,
  PDBH_SNR_HSC2_VIN,
  PDBH_SNR_HSC2_VOUT,
  PDBH_SNR_HSC2_IOUT,
  PDBH_SNR_HSC2_PIN,
  PDBH_SNR_HSC2_TEMP,
};

const uint8_t hpdb_adc_sensor_list[] = {
  PDBH_SNR_CABLE1_PLUS,
  PDBH_SNR_CABLE1_MINUS,
  PDBH_SNR_CABLE2_PLUS,
  PDBH_SNR_CABLE2_MINUS,
};

const uint8_t fan_bp1_sensor_list[] = {
  FAN_BP1_SNR_FAN0_INLET_SPEED,
  FAN_BP1_SNR_FAN0_OUTLET_SPEED,
  FAN_BP1_SNR_FAN1_INLET_SPEED,
  FAN_BP1_SNR_FAN1_OUTLET_SPEED,
  FAN_BP1_SNR_FAN4_INLET_SPEED,
  FAN_BP1_SNR_FAN4_OUTLET_SPEED,
  FAN_BP1_SNR_FAN5_INLET_SPEED,
  FAN_BP1_SNR_FAN5_OUTLET_SPEED,
  FAN_BP1_SNR_FAN8_INLET_SPEED,
  FAN_BP1_SNR_FAN8_OUTLET_SPEED,
  FAN_BP1_SNR_FAN9_INLET_SPEED,
  FAN_BP1_SNR_FAN9_OUTLET_SPEED,
  FAN_BP1_SNR_FAN12_INLET_SPEED,
  FAN_BP1_SNR_FAN12_OUTLET_SPEED,
  FAN_BP1_SNR_FAN13_INLET_SPEED,
  FAN_BP1_SNR_FAN13_OUTLET_SPEED,
};

const uint8_t fan_bp2_sensor_list[] = {
  FAN_BP2_SNR_FAN2_INLET_SPEED,
  FAN_BP2_SNR_FAN2_OUTLET_SPEED,
  FAN_BP2_SNR_FAN3_INLET_SPEED,
  FAN_BP2_SNR_FAN3_OUTLET_SPEED,
  FAN_BP2_SNR_FAN6_INLET_SPEED,
  FAN_BP2_SNR_FAN6_OUTLET_SPEED,
  FAN_BP2_SNR_FAN7_INLET_SPEED,
  FAN_BP2_SNR_FAN7_OUTLET_SPEED,
  FAN_BP2_SNR_FAN10_INLET_SPEED,
  FAN_BP2_SNR_FAN10_OUTLET_SPEED,
  FAN_BP2_SNR_FAN11_INLET_SPEED,
  FAN_BP2_SNR_FAN11_OUTLET_SPEED,
  FAN_BP2_SNR_FAN14_INLET_SPEED,
  FAN_BP2_SNR_FAN14_OUTLET_SPEED,
  FAN_BP2_SNR_FAN15_INLET_SPEED,
  FAN_BP2_SNR_FAN15_OUTLET_SPEED,
};

const uint8_t scm_sensor_list[] = {
  SCM_SNR_P12V,
  SCM_SNR_P5V,
  SCM_SNR_P3V3,
  SCM_SNR_P2V5,
  SCM_SNR_P1V8,
  SCM_SNR_PGPPA,
  SCM_SNR_P1V2,
  SCM_SNR_P1V0,
  SCM_SNR_BMC_TEMP,
};


//FAN
//NCT7363
PAL_I2C_BUS_INFO nct7363y_dev_list[] = {
  {FAN_CHIP_ID0, I2C_BUS_40, 0x04},
  {FAN_CHIP_ID1, I2C_BUS_40, 0x02},
  {FAN_CHIP_ID2, I2C_BUS_41, 0x04},
  {FAN_CHIP_ID3, I2C_BUS_41, 0x02},
};

NCT7363Y_CTRL nct7363y_ctrl_list[] = {
  {NCT7363Y_PWM0_INLET,   0x90, 0x5A, 0x5B},
  {NCT7363Y_PWM0_OUTLET,  0x90, 0x5E, 0x5F},
  {NCT7363Y_PWM4_INLET,   0x98, 0x5C, 0x5D},
  {NCT7363Y_PWM4_OUTLET,  0x98, 0x62, 0x63},
  {NCT7363Y_PWM6_INLET,   0x9C, 0x66, 0x67},
  {NCT7363Y_PWM6_OUTLET,  0x9C, 0x4A, 0x4B},
  {NCT7363Y_PWM10_INLET,  0xA4, 0x48, 0x49},
  {NCT7363Y_PWM10_OUTLET, 0xA4, 0x4E, 0x4F},
};

//FAN CHIP
char *max31790_chips[] = {
  "max31790-i2c-40-2f",  // FAN_BP1
  "max31790-i2c-40-20",  // FAN_BP1
  "max31790-i2c-41-2f",  // FAN_BP2
  "max31790-i2c-41-20",  // FAN_BP2
};
char **fan_chips = max31790_chips;


//{SensorName, ID, FUNCTION, PWR_STATUS, {UCR, UNC, UNR, LCR, LNC, LNR, Pos, Neg}
PAL_SENSOR_MAP bb_sensor_map[] = {
  {NULL, 0, NULL, 0, {0, 0, 0, 0, 0, 0, 0, 0}, 0}, //0x00
  {NULL, 0, NULL, 0, {0, 0, 0, 0, 0, 0, 0, 0}, 0}, //0x01
  {NULL, 0, NULL, 0, {0, 0, 0, 0, 0, 0, 0, 0}, 0}, //0x02
  {NULL, 0, NULL, 0, {0, 0, 0, 0, 0, 0, 0, 0}, 0}, //0x03
  {NULL, 0, NULL, 0, {0, 0, 0, 0, 0, 0, 0, 0}, 0}, //0x04
  {NULL, 0, NULL, 0, {0, 0, 0, 0, 0, 0, 0, 0}, 0}, //0x05
  {NULL, 0, NULL, 0, {0, 0, 0, 0, 0, 0, 0, 0}, 0}, //0x06
  {NULL, 0, NULL, 0, {0, 0, 0, 0, 0, 0, 0, 0}, 0}, //0x07
  {NULL, 0, NULL, 0, {0, 0, 0, 0, 0, 0, 0, 0}, 0}, //0x08
  {NULL, 0, NULL, 0, {0, 0, 0, 0, 0, 0, 0, 0}, 0}, //0x09
  {NULL, 0, NULL, 0, {0, 0, 0, 0, 0, 0, 0, 0}, 0}, //0x0A
  {NULL, 0, NULL, 0, {0, 0, 0, 0, 0, 0, 0, 0}, 0}, //0x0B
  {NULL, 0, NULL, 0, {0, 0, 0, 0, 0, 0, 0, 0}, 0}, //0x0C
  {NULL, 0, NULL, 0, {0, 0, 0, 0, 0, 0, 0, 0}, 0}, //0x0D
  {NULL, 0, NULL, 0, {0, 0, 0, 0, 0, 0, 0, 0}, 0}, //0x0E
  {NULL, 0, NULL, 0, {0, 0, 0, 0, 0, 0, 0, 0}, 0}, //0x0F

  {NULL, 0, NULL, 0, {0, 0, 0, 0, 0, 0, 0, 0}, 0}, //0x10
  {NULL, 0, NULL, 0, {0, 0, 0, 0, 0, 0, 0, 0}, 0}, //0x11
  {NULL, 0, NULL, 0, {0, 0, 0, 0, 0, 0, 0, 0}, 0}, //0x12
  {NULL, 0, NULL, 0, {0, 0, 0, 0, 0, 0, 0, 0}, 0}, //0x13
  {NULL, 0, NULL, 0, {0, 0, 0, 0, 0, 0, 0, 0}, 0}, //0x14
  {NULL, 0, NULL, 0, {0, 0, 0, 0, 0, 0, 0, 0}, 0}, //0x15
  {NULL, 0, NULL, 0, {0, 0, 0, 0, 0, 0, 0, 0}, 0}, //0x16
  {NULL, 0, NULL, 0, {0, 0, 0, 0, 0, 0, 0, 0}, 0}, //0x17
  {NULL, 0, NULL, 0, {0, 0, 0, 0, 0, 0, 0, 0}, 0}, //0x18
  {NULL, 0, NULL, 0, {0, 0, 0, 0, 0, 0, 0, 0}, 0}, //0x19
  {NULL, 0, NULL, 0, {0, 0, 0, 0, 0, 0, 0, 0}, 0}, //0x1A
  {NULL, 0, NULL, 0, {0, 0, 0, 0, 0, 0, 0, 0}, 0}, //0x1B
  {NULL, 0, NULL, 0, {0, 0, 0, 0, 0, 0, 0, 0}, 0}, //0x1C
  {NULL, 0, NULL, 0, {0, 0, 0, 0, 0, 0, 0, 0}, 0}, //0x1D
  {NULL, 0, NULL, 0, {0, 0, 0, 0, 0, 0, 0, 0}, 0}, //0x1E
  {NULL, 0, NULL, 0, {0, 0, 0, 0, 0, 0, 0, 0}, 0}, //0x1F

  {NULL, 0, NULL, 0, {0, 0, 0, 0, 0, 0, 0, 0}, 0}, //0x20
  {NULL, 0, NULL, 0, {0, 0, 0, 0, 0, 0, 0, 0}, 0}, //0x21
  {NULL, 0, NULL, 0, {0, 0, 0, 0, 0, 0, 0, 0}, 0}, //0x22
  {NULL, 0, NULL, 0, {0, 0, 0, 0, 0, 0, 0, 0}, 0}, //0x23
  {NULL, 0, NULL, 0, {0, 0, 0, 0, 0, 0, 0, 0}, 0}, //0x24
  {NULL, 0, NULL, 0, {0, 0, 0, 0, 0, 0, 0, 0}, 0}, //0x25
  {NULL, 0, NULL, 0, {0, 0, 0, 0, 0, 0, 0, 0}, 0}, //0x26
  {NULL, 0, NULL, 0, {0, 0, 0, 0, 0, 0, 0, 0}, 0}, //0x27
  {NULL, 0, NULL, 0, {0, 0, 0, 0, 0, 0, 0, 0}, 0}, //0x28
  {NULL, 0, NULL, 0, {0, 0, 0, 0, 0, 0, 0, 0}, 0}, //0x29
  {NULL, 0, NULL, 0, {0, 0, 0, 0, 0, 0, 0, 0}, 0}, //0x2A
  {NULL, 0, NULL, 0, {0, 0, 0, 0, 0, 0, 0, 0}, 0}, //0x2B
  {NULL, 0, NULL, 0, {0, 0, 0, 0, 0, 0, 0, 0}, 0}, //0x2C
  {NULL, 0, NULL, 0, {0, 0, 0, 0, 0, 0, 0, 0}, 0}, //0x2D
  {NULL, 0, NULL, 0, {0, 0, 0, 0, 0, 0, 0, 0}, 0}, //0x2E
  {NULL, 0, NULL, 0, {0, 0, 0, 0, 0, 0, 0, 0}, 0}, //0x2F

  {NULL, 0, NULL, 0, {0, 0, 0, 0, 0, 0, 0, 0}, 0}, //0x30
  {NULL, 0, NULL, 0, {0, 0, 0, 0, 0, 0, 0, 0}, 0}, //0x31
  {NULL, 0, NULL, 0, {0, 0, 0, 0, 0, 0, 0, 0}, 0}, //0X32
  {NULL, 0, NULL, 0, {0, 0, 0, 0, 0, 0, 0, 0}, 0}, //0x33
  {NULL, 0, NULL, 0, {0, 0, 0, 0, 0, 0, 0, 0}, 0}, //0x34
  {NULL, 0, NULL, 0, {0, 0, 0, 0, 0, 0, 0, 0}, 0}, //0x35
  {NULL, 0, NULL, 0, {0, 0, 0, 0, 0, 0, 0, 0}, 0}, //0X36
  {NULL, 0, NULL, 0, {0, 0, 0, 0, 0, 0, 0, 0}, 0}, //0x37
  {NULL, 0, NULL, 0, {0, 0, 0, 0, 0, 0, 0, 0}, 0}, //0x38
  {NULL, 0, NULL, 0, {0, 0, 0, 0, 0, 0, 0, 0}, 0}, //0x39
  {NULL, 0, NULL, 0, {0, 0, 0, 0, 0, 0, 0, 0}, 0}, //0X3A
  {NULL, 0, NULL, 0, {0, 0, 0, 0, 0, 0, 0, 0}, 0}, //0x3B
  {NULL, 0, NULL, 0, {0, 0, 0, 0, 0, 0, 0, 0}, 0}, //0x3C
  {NULL, 0, NULL, 0, {0, 0, 0, 0, 0, 0, 0, 0}, 0}, //0x3D
  {NULL, 0, NULL, 0, {0, 0, 0, 0, 0, 0, 0, 0}, 0}, //0X3E
  {NULL, 0, NULL, 0, {0, 0, 0, 0, 0, 0, 0, 0}, 0}, //0x3F
  {NULL, 0, NULL, 0, {0, 0, 0, 0, 0, 0, 0, 0}, 0}, //0x40
  {NULL, 0, NULL, 0, {0, 0, 0, 0, 0, 0, 0, 0}, 0}, //0x41
  {NULL, 0, NULL, 0, {0, 0, 0, 0, 0, 0, 0, 0}, 0}, //0X42
  {NULL, 0, NULL, 0, {0, 0, 0, 0, 0, 0, 0, 0}, 0}, //0x43
  {NULL, 0, NULL, 0, {0, 0, 0, 0, 0, 0, 0, 0}, 0}, //0x44
  {NULL, 0, NULL, 0, {0, 0, 0, 0, 0, 0, 0, 0}, 0}, //0x45
  {NULL, 0, NULL, 0, {0, 0, 0, 0, 0, 0, 0, 0}, 0}, //0x46
  {NULL, 0, NULL, 0, {0, 0, 0, 0, 0, 0, 0, 0}, 0}, //0x47

  {NULL, 0, NULL, 0, {0, 0, 0, 0, 0, 0, 0, 0}, 0}, //0x48
  {NULL, 0, NULL, 0, {0, 0, 0, 0, 0, 0, 0, 0}, 0}, //0x49
  {NULL, 0, NULL, 0, {0, 0, 0, 0, 0, 0, 0, 0}, 0}, //0X4A
  {NULL, 0, NULL, 0, {0, 0, 0, 0, 0, 0, 0, 0}, 0}, //0x4B
  {NULL, 0, NULL, 0, {0, 0, 0, 0, 0, 0, 0, 0}, 0}, //0x4C
  {NULL, 0, NULL, 0, {0, 0, 0, 0, 0, 0, 0, 0}, 0}, //0x4D
  {NULL, 0, NULL, 0, {0, 0, 0, 0, 0, 0, 0, 0}, 0}, //0X4E
  {NULL, 0, NULL, 0, {0, 0, 0, 0, 0, 0, 0, 0}, 0}, //0x4F
  {NULL, 0, NULL, 0, {0, 0, 0, 0, 0, 0, 0, 0}, 0}, //0x50
  {NULL, 0, NULL, 0, {0, 0, 0, 0, 0, 0, 0, 0}, 0}, //0x51
  {NULL, 0, NULL, 0, {0, 0, 0, 0, 0, 0, 0, 0}, 0}, //0X52
  {NULL, 0, NULL, 0, {0, 0, 0, 0, 0, 0, 0, 0}, 0}, //0x53
  {NULL, 0, NULL, 0, {0, 0, 0, 0, 0, 0, 0, 0}, 0}, //0x54
  {NULL, 0, NULL, 0, {0, 0, 0, 0, 0, 0, 0, 0}, 0}, //0x55
  {NULL, 0, NULL, 0, {0, 0, 0, 0, 0, 0, 0, 0}, 0}, //0X56
  {NULL, 0, NULL, 0, {0, 0, 0, 0, 0, 0, 0, 0}, 0}, //0x57
  {NULL, 0, NULL, 0, {0, 0, 0, 0, 0, 0, 0, 0}, 0}, //0x58
  {NULL, 0, NULL, 0, {0, 0, 0, 0, 0, 0, 0, 0}, 0}, //0x59
  {NULL, 0, NULL, 0, {0, 0, 0, 0, 0, 0, 0, 0}, 0}, //0X5A
  {NULL, 0, NULL, 0, {0, 0, 0, 0, 0, 0, 0, 0}, 0}, //0x5B
  {NULL, 0, NULL, 0, {0, 0, 0, 0, 0, 0, 0, 0}, 0}, //0x5C
  {NULL, 0, NULL, 0, {0, 0, 0, 0, 0, 0, 0, 0}, 0}, //0x5D
  {NULL, 0, NULL, 0, {0, 0, 0, 0, 0, 0, 0, 0}, 0}, //0x5E
  {NULL, 0, NULL, 0, {0, 0, 0, 0, 0, 0, 0, 0}, 0}, //0x5F

  {NULL, 0, NULL, 0, {0, 0, 0, 0, 0, 0, 0, 0}, 0}, //0x60
  {NULL, 0, NULL, 0, {0, 0, 0, 0, 0, 0, 0, 0}, 0}, //0x61
  {NULL, 0, NULL, 0, {0, 0, 0, 0, 0, 0, 0, 0}, 0}, //0x62
  {NULL, 0, NULL, 0, {0, 0, 0, 0, 0, 0, 0, 0}, 0}, //0x63
  {NULL, 0, NULL, 0, {0, 0, 0, 0, 0, 0, 0, 0}, 0}, //0x64
  {NULL, 0, NULL, 0, {0, 0, 0, 0, 0, 0, 0, 0}, 0}, //0x65
  {NULL, 0, NULL, 0, {0, 0, 0, 0, 0, 0, 0, 0}, 0}, //0x66
  {NULL, 0, NULL, 0, {0, 0, 0, 0, 0, 0, 0, 0}, 0}, //0x67
  {NULL, 0, NULL, 0, {0, 0, 0, 0, 0, 0, 0, 0}, 0}, //0x68
  {NULL, 0, NULL, 0, {0, 0, 0, 0, 0, 0, 0, 0}, 0}, //0x69
  {NULL, 0, NULL, 0, {0, 0, 0, 0, 0, 0, 0, 0}, 0}, //0x6A
  {NULL, 0, NULL, 0, {0, 0, 0, 0, 0, 0, 0, 0}, 0}, //0x6B
  {NULL, 0, NULL, 0, {0, 0, 0, 0, 0, 0, 0, 0}, 0}, //0x6C
  {NULL, 0, NULL, 0, {0, 0, 0, 0, 0, 0, 0, 0}, 0}, //0x6D
  {NULL, 0, NULL, 0, {0, 0, 0, 0, 0, 0, 0, 0}, 0}, //0x6E
  {NULL, 0, NULL, 0, {0, 0, 0, 0, 0, 0, 0, 0}, 0}, //0x6F

  {NULL, 0, NULL, 0, {0, 0, 0, 0, 0, 0, 0, 0}, 0}, //0x70
  {NULL, 0, NULL, 0, {0, 0, 0, 0, 0, 0, 0, 0}, 0}, //0x71
  {NULL, 0, NULL, 0, {0, 0, 0, 0, 0, 0, 0, 0}, 0}, //0x72
  {NULL, 0, NULL, 0, {0, 0, 0, 0, 0, 0, 0, 0}, 0}, //0x73
  {NULL, 0, NULL, 0, {0, 0, 0, 0, 0, 0, 0, 0}, 0}, //0x74
  {NULL, 0, NULL, 0, {0, 0, 0, 0, 0, 0, 0, 0}, 0}, //0x75
  {NULL, 0, NULL, 0, {0, 0, 0, 0, 0, 0, 0, 0}, 0}, //0x76
  {NULL, 0, NULL, 0, {0, 0, 0, 0, 0, 0, 0, 0}, 0}, //0x77
  {NULL, 0, NULL, 0, {0, 0, 0, 0, 0, 0, 0, 0}, 0}, //0x78
  {NULL, 0, NULL, 0, {0, 0, 0, 0, 0, 0, 0, 0}, 0}, //0x79
  {NULL, 0, NULL, 0, {0, 0, 0, 0, 0, 0, 0, 0}, 0}, //0x7A
  {NULL, 0, NULL, 0, {0, 0, 0, 0, 0, 0, 0, 0}, 0}, //0x7B
  {NULL, 0, NULL, 0, {0, 0, 0, 0, 0, 0, 0, 0}, 0}, //0x7C
  {NULL, 0, NULL, 0, {0, 0, 0, 0, 0, 0, 0, 0}, 0}, //0x7D
  {NULL, 0, NULL, 0, {0, 0, 0, 0, 0, 0, 0, 0}, 0}, //0x7E
  {NULL, 0, NULL, 0, {0, 0, 0, 0, 0, 0, 0, 0}, 0}, //0x7F

  {"MEZZ0_P3V3_VOLT", DPM_0, read_dpm_vout, true, {3.345, 0, 0, 3.135, 0, 0, 0, 0}, VOLT}, //0x80
  {NULL, 0, NULL, 0, {0, 0, 0, 0, 0, 0, 0, 0}, 0}, //0x81
  {"MEZZ0_P12V_CURR", ADC_CH1, read_iic_adc_val, true, {0, 0, 0, 0, 0, 0, 0, 0}, CURR}, //0x82
  {"MEZZ0_P12V_PWR", MEZZ0, read_nic0_power, true, {0, 0, 0, 0, 0, 0, 0, 0}, POWER}, //0x83
  {"MEZZ0_TEMP", TEMP_MEZZ0, read_bb_sensor, true, {105.0, 0, 0, 5.0, 0, 0, 0, 0}, TEMP}, //0x84
  {NULL, 0, NULL, 0, {0, 0, 0, 0, 0, 0, 0, 0}, 0}, //0x85
  {NULL, 0, NULL, 0, {0, 0, 0, 0, 0, 0, 0, 0}, 0}, //0x86
  {NULL, 0, NULL, 0, {0, 0, 0, 0, 0, 0, 0, 0}, 0}, //0x87

  {"MEZZ1_P3V3_VOLT", DPM_2, read_dpm_vout, true, {3.345, 0, 0, 3.135, 0, 0, 0, 0}, VOLT}, //0x88
  {NULL, 0, NULL, 0, {0, 0, 0, 0, 0, 0, 0, 0}, 0}, //0x89
  {"MEZZ1_P12V_CURR", ADC_CH2, read_iic_adc_val, true, {0, 0, 0, 0, 0, 0, 0, 0}, CURR}, //0x8A
  {"MEZZ1_P12V_PWR", MEZZ1, read_nic1_power, true, {0, 0, 0, 0, 0, 0, 0, 0}, POWER}, //0x8B
  {"MEZZ1_TEMP", TEMP_MEZZ1, read_bb_sensor, true, {105.0, 0, 0, 5.0, 0, 0, 0, 0}, TEMP}, //0x8C
  {NULL, 0, NULL, 0, {0, 0, 0, 0, 0, 0, 0, 0}, 0}, //0x8D
  {NULL, 0, NULL, 0, {0, 0, 0, 0, 0, 0, 0, 0}, 0}, //0x8E

  {"HSC0_VOUT_VOLT", VPDB_HSC_ID0, read_bb_sensor, true, {57.0, 0, 0, 45.0, 0, 0, 0, 0}, VOLT}, //0x8F
  {"HSC0_VIN_VOLT",  VPDB_HSC_ID0, read_bb_sensor, true, {57.0, 0, 0, 45.0, 0, 0, 0, 0}, VOLT}, //0x90
  {"HSC0_CURR",      VPDB_HSC_ID0, read_bb_sensor, true, {0, 0, 0, 0, 0, 0, 0, 0}, CURR}, //0x91
  {"HSC0_PWR",       VPDB_HSC_ID0, read_hsc_pin,   true, {0, 0, 0, 0, 0, 0, 0, 0}, POWER}, //0x92
  {"HSC0_TEMP",      VPDB_HSC_ID0, read_bb_sensor, true, {85.0, 0, 0, 5.0, 0, 0, 0, 0}, TEMP}, //0x93
  {"HSC0_PEAK_PIN",  VPDB_HSC_ID0, read_bb_sensor, true, {6160, 0, 0, 0, 0, 0, 0, 0}, POWER}, //0x94

  {"BRICK0_VOLT", BRICK_ID0, read_bb_sensor, true, {13.2, 0, 0, 10.8, 0, 0, 0, 0}, VOLT}, //0x95
  {"BRICK0_CURR", BRICK_ID0, read_bb_sensor, true, {0, 0, 0, 0, 0, 0, 0, 0}, CURR}, //0x96
  {"BRICK0_TEMP", BRICK_ID0, read_bb_sensor, true, {115.0, 0, 0, 5.0, 0, 0, 0, 0}, TEMP}, //0x97
  {NULL, 0, NULL, 0, {0, 0, 0, 0, 0, 0, 0, 0}, 0}, //0x98
  {"BRICK1_VOLT", BRICK_ID1, read_bb_sensor, true, {13.2, 0, 0, 10.8, 0, 0, 0, 0}, VOLT}, //0x99
  {"BRICK1_CURR", BRICK_ID1, read_bb_sensor, true, {0, 0, 0, 0, 0, 0, 0, 0}, CURR}, //0x9A
  {"BRICK1_TEMP", BRICK_ID1, read_bb_sensor, true, {115.0, 0, 0, 5.0, 0, 0, 0, 0}, TEMP}, //0x9B
  {NULL, 0, NULL, 0, {0, 0, 0, 0, 0, 0, 0, 0}, 0}, //0x9C
  {"BRICK2_VOLT", BRICK_ID2, read_bb_sensor, true, {13.2, 0, 0, 10.8, 0, 0, 0, 0}, VOLT}, //0x9D
  {"BRICK2_CURR", BRICK_ID2, read_bb_sensor, true, {0, 0, 0, 0, 0, 0, 0, 0}, CURR}, //0x9E
  {"BRICK2_TEMP", BRICK_ID2, read_bb_sensor, true, {115.0, 0, 0, 5.0, 0, 0, 0, 0}, TEMP}, //0x9F

  {NULL, 0, NULL, 0, {0, 0, 0, 0, 0, 0, 0, 0}, 0}, //0xA0
  {"P3V3_AUX_IN6_VOLT", ADC_CH6, read_iic_adc_val, true, {3.6, 0, 0, 3.1, 0, 0, 0, 0}, VOLT}, //0xA1
  {"MEDUSA0_POSITIVE_VDROP",  VPDB_ADC_ID0, read_bb_sensor, true, {0, 0, 0, 0, 0, 0, 0, 0}, mVOLT}, //0xA2
  {"MEDUSA0_RETURN_VDROP",    VPDB_ADC_ID1, read_bb_sensor, true, {0, 0, 0, 0, 0, 0, 0, 0}, mVOLT}, //0xA3

  {NULL, 0, NULL, 0, {0, 0, 0, 0, 0, 0, 0, 0}, 0}, //0xA4
  {NULL, 0, NULL, 0, {0, 0, 0, 0, 0, 0, 0, 0}, 0}, //0xA5
  {NULL, 0, NULL, 0, {0, 0, 0, 0, 0, 0, 0, 0}, 0}, //0xA6
  {NULL, 0, NULL, 0, {0, 0, 0, 0, 0, 0, 0, 0}, 0}, //0xA7
  {NULL, 0, NULL, 0, {0, 0, 0, 0, 0, 0, 0, 0}, 0}, //0xA8
  {NULL, 0, NULL, 0, {0, 0, 0, 0, 0, 0, 0, 0}, 0}, //0xA9
  {NULL, 0, NULL, 0, {0, 0, 0, 0, 0, 0, 0, 0}, 0}, //0xAA
  {NULL, 0, NULL, 0, {0, 0, 0, 0, 0, 0, 0, 0}, 0}, //0xAB
  {NULL, 0, NULL, 0, {0, 0, 0, 0, 0, 0, 0, 0}, 0}, //0xAC
  {NULL, 0, NULL, 0, {0, 0, 0, 0, 0, 0, 0, 0}, 0}, //0xAD
  {NULL, 0, NULL, 0, {0, 0, 0, 0, 0, 0, 0, 0}, 0}, //0xAE
  {NULL, 0, NULL, 0, {0, 0, 0, 0, 0, 0, 0, 0}, 0}, //0xAF

  {"HSC1_VIN_VOLT", HPDB_HSC_ID1, read_bb_sensor, true, {57.0, 0, 0, 45.0, 0, 0, 0, 0}, VOLT}, //0xB0
  {"HSC1_CURR",     HPDB_HSC_ID1, read_bb_sensor, true, {0, 0, 0, 0, 0, 0, 0, 0}, CURR}, //0xB1
  {"HSC1_PWR",      HPDB_HSC_ID1, read_hsc_pin,   true, {0, 0, 0, 0, 0, 0, 0, 0}, POWER}, //0xB2
  {"HSC1_TEMP",     HPDB_HSC_ID1, read_bb_sensor, true, {85.0, 0, 0, 5.0, 0, 0, 0, 0}, TEMP}, //0xB3
  {"HSC1_PEAK_PIN", HPDB_HSC_ID1, read_bb_sensor, true, {0, 0, 0, 0, 0, 0, 0, 0}, POWER}, //0xB4
  {"HSC2_VIN_VOLT", HPDB_HSC_ID2, read_bb_sensor, true, {57.0, 0, 0, 45.0, 0, 0, 0, 0}, VOLT}, //0xB5
  {"HSC2_CURR",     HPDB_HSC_ID2, read_bb_sensor, true, {0, 0, 0, 0, 0, 0, 0, 0}, CURR}, //0xB6
  {"HSC2_PWR",      HPDB_HSC_ID2, read_hsc_pin,   true, {0, 0, 0, 0, 0, 0, 0, 0}, POWER}, //0xB7
  {"HSC2_TEMP",     HPDB_HSC_ID2, read_bb_sensor, true, {85.0, 0, 0, 5.0, 0, 0, 0, 0}, TEMP}, //0xB8
  {"HSC2_PEAK_PIN", HPDB_HSC_ID2, read_bb_sensor, true, {0, 0, 0, 0, 0, 0, 0, 0}, POWER}, //0xB9

  {"HSC1_VOUT_VOLT", HPDB_HSC_ID1, read_bb_sensor, true, {57.0, 0, 0, 45.0, 0, 0, 0, 0}, VOLT}, //0xBA
  {"HSC2_VOUT_VOLT", HPDB_HSC_ID2, read_bb_sensor, true, {57.0, 0, 0, 45.0, 0, 0, 0, 0}, VOLT}, //0xBB
  {"MEDUSA1_POSITIVE_VDROP", HPDB_ADC_ID0, read_bb_sensor, true, {0, 0, 0, 0, 0, 0, 0, 0}, mVOLT}, //0xBC
  {"MEDUSA1_RETURN_VDROP",   HPDB_ADC_ID1, read_bb_sensor, true, {0, 0, 0, 0, 0, 0, 0, 0}, mVOLT}, //0xBD
  {"MEDUSA2_POSITIVE_VDROP", HPDB_ADC_ID2, read_bb_sensor, true, {0, 0, 0, 0, 0, 0, 0, 0}, mVOLT}, //0xBE
  {"MEDUSA2_RETURN_VDROP",   HPDB_ADC_ID3, read_bb_sensor, true, {0, 0, 0, 0, 0, 0, 0, 0}, mVOLT}, //0xBF

  {NULL, 0, NULL, 0, {0, 0, 0, 0, 0, 0, 0, 0}, 0}, //0xC0
  {NULL, 0, NULL, 0, {0, 0, 0, 0, 0, 0, 0, 0}, 0}, //0xC1
  {NULL, 0, NULL, 0, {0, 0, 0, 0, 0, 0, 0, 0}, 0}, //0xC2
  {NULL, 0, NULL, 0, {0, 0, 0, 0, 0, 0, 0, 0}, 0}, //0xC3
  {NULL, 0, NULL, 0, {0, 0, 0, 0, 0, 0, 0, 0}, 0}, //0xC4
  {NULL, 0, NULL, 0, {0, 0, 0, 0, 0, 0, 0, 0}, 0}, //0xC5
  {NULL, 0, NULL, 0, {0, 0, 0, 0, 0, 0, 0, 0}, 0}, //0xC6
  {NULL, 0, NULL, 0, {0, 0, 0, 0, 0, 0, 0, 0}, 0}, //0xC7
  {NULL, 0, NULL, 0, {0, 0, 0, 0, 0, 0, 0, 0}, 0}, //0xC8
  {NULL, 0, NULL, 0, {0, 0, 0, 0, 0, 0, 0, 0}, 0}, //0xC9
  {NULL, 0, NULL, 0, {0, 0, 0, 0, 0, 0, 0, 0}, 0}, //0xCA
  {NULL, 0, NULL, 0, {0, 0, 0, 0, 0, 0, 0, 0}, 0}, //0xCB
  {NULL, 0, NULL, 0, {0, 0, 0, 0, 0, 0, 0, 0}, 0}, //0xCC
  {NULL, 0, NULL, 0, {0, 0, 0, 0, 0, 0, 0, 0}, 0}, //0xCD
  {NULL, 0, NULL, 0, {0, 0, 0, 0, 0, 0, 0, 0}, 0}, //0xCE
  {NULL, 0, NULL, 0, {0, 0, 0, 0, 0, 0, 0, 0}, 0}, //0xCF

  {"FAN0_INLET_SPEED",   FAN_TACH_ID0,  read_fan_speed, true, {19000.0, 0, 0, 965.0, 0, 0, 0, 0}, FAN}, //0xD0
  {"FAN0_OUTLET_SPEED",  FAN_TACH_ID1,  read_fan_speed, true, {17000.0, 0, 0, 965.0, 0, 0, 0, 0}, FAN}, //0xD1
  {"FAN1_INLET_SPEED",   FAN_TACH_ID2,  read_fan_speed, true, {19000.0, 0, 0, 965.0, 0, 0, 0, 0}, FAN}, //0xD2
  {"FAN1_OUTLET_SPEED",  FAN_TACH_ID3,  read_fan_speed, true, {17000.0, 0, 0, 965.0, 0, 0, 0, 0}, FAN}, //0xD3
  {"FAN2_INLET_SPEED",   FAN_TACH_ID4,  read_fan_speed, true, {19000.0, 0, 0, 965.0, 0, 0, 0, 0}, FAN}, //0xD4
  {"FAN2_OUTLET_SPEED",  FAN_TACH_ID5,  read_fan_speed, true, {17000.0, 0, 0, 965.0, 0, 0, 0, 0}, FAN}, //0xD5
  {"FAN3_INLET_SPEED",   FAN_TACH_ID6,  read_fan_speed, true, {19000.0, 0, 0, 965.0, 0, 0, 0, 0}, FAN}, //0xD6
  {"FAN3_OUTLET_SPEED",  FAN_TACH_ID7,  read_fan_speed, true, {17000.0, 0, 0, 965.0, 0, 0, 0, 0}, FAN}, //0xD7
  {"FAN4_INLET_SPEED",   FAN_TACH_ID8,  read_fan_speed, true, {19000.0, 0, 0, 965.0, 0, 0, 0, 0}, FAN}, //0xD8
  {"FAN4_OUTLET_SPEED",  FAN_TACH_ID9,  read_fan_speed, true, {17000.0, 0, 0, 965.0, 0, 0, 0, 0}, FAN}, //0xD9
  {"FAN5_INLET_SPEED",   FAN_TACH_ID10, read_fan_speed, true, {19000.0, 0, 0, 965.0, 0, 0, 0, 0}, FAN}, //0xDA
  {"FAN5_OUTLET_SPEED",  FAN_TACH_ID11, read_fan_speed, true, {17000.0, 0, 0, 965.0, 0, 0, 0, 0}, FAN}, //0xDB
  {"FAN6_INLET_SPEED",   FAN_TACH_ID12, read_fan_speed, true, {19000.0, 0, 0, 965.0, 0, 0, 0, 0}, FAN}, //0xDC
  {"FAN6_OUTLET_SPEED",  FAN_TACH_ID13, read_fan_speed, true, {17000.0, 0, 0, 965.0, 0, 0, 0, 0}, FAN}, //0xDD
  {"FAN7_INLET_SPEED",   FAN_TACH_ID14, read_fan_speed, true, {19000.0, 0, 0, 965.0, 0, 0, 0, 0}, FAN}, //0xDE
  {"FAN7_OUTLET_SPEED",  FAN_TACH_ID15, read_fan_speed, true, {17000.0, 0, 0, 965.0, 0, 0, 0, 0}, FAN}, //0xDF

  {"FAN8_INLET_SPEED",   FAN_TACH_ID16, read_fan_speed, true, {19000.0, 0, 0, 965.0, 0, 0, 0, 0}, FAN}, //0xE0
  {"FAN8_OUTLET_SPEED",  FAN_TACH_ID17, read_fan_speed, true, {17000.0, 0, 0, 965.0, 0, 0, 0, 0}, FAN}, //0xE1
  {"FAN9_INLET_SPEED",   FAN_TACH_ID18, read_fan_speed, true, {19000.0, 0, 0, 965.0, 0, 0, 0, 0}, FAN}, //0xE2
  {"FAN9_OUTLET_SPEED",  FAN_TACH_ID19, read_fan_speed, true, {17000.0, 0, 0, 965.0, 0, 0, 0, 0}, FAN}, //0xE3
  {"FAN10_INLET_SPEED",  FAN_TACH_ID20, read_fan_speed, true, {19000.0, 0, 0, 965.0, 0, 0, 0, 0}, FAN}, //0xE4
  {"FAN10_OUTLET_SPEED", FAN_TACH_ID21, read_fan_speed, true, {17000.0, 0, 0, 965.0, 0, 0, 0, 0}, FAN}, //0xE5
  {"FAN11_INLET_SPEED",  FAN_TACH_ID22, read_fan_speed, true, {19000.0, 0, 0, 965.0, 0, 0, 0, 0}, FAN}, //0xE6
  {"FAN11_OUTLET_SPEED", FAN_TACH_ID23, read_fan_speed, true, {17000.0, 0, 0, 965.0, 0, 0, 0, 0}, FAN}, //0xE7
  {"FAN12_INLET_SPEED",  FAN_TACH_ID24, read_fan_speed, true, {19000.0, 0, 0, 965.0, 0, 0, 0, 0}, FAN}, //0xE8
  {"FAN12_OUTLET_SPEED", FAN_TACH_ID25, read_fan_speed, true, {17000.0, 0, 0, 965.0, 0, 0, 0, 0}, FAN}, //0xE9
  {"FAN13_INLET_SPEED",  FAN_TACH_ID26, read_fan_speed, true, {19000.0, 0, 0, 965.0, 0, 0, 0, 0}, FAN}, //0xEA
  {"FAN13_OUTLET_SPEED", FAN_TACH_ID27, read_fan_speed, true, {17000.0, 0, 0, 965.0, 0, 0, 0, 0}, FAN}, //0xEB
  {"FAN14_INLET_SPEED",  FAN_TACH_ID28, read_fan_speed, true, {19000.0, 0, 0, 965.0, 0, 0, 0, 0}, FAN}, //0xEC
  {"FAN14_OUTLET_SPEED", FAN_TACH_ID29, read_fan_speed, true, {17000.0, 0, 0, 965.0, 0, 0, 0, 0}, FAN}, //0xED
  {"FAN15_INLET_SPEED",  FAN_TACH_ID30, read_fan_speed, true, {19000.0, 0, 0, 965.0, 0, 0, 0, 0}, FAN}, //0xEE
  {"FAN15_OUTLET_SPEED", FAN_TACH_ID31, read_fan_speed, true, {17000.0, 0, 0, 965.0, 0, 0, 0, 0}, FAN}, //0xEF

  {"P12V_VOLT",  ADC0, read_adc_val, true, {13.2, 0, 0, 10.8, 0, 0, 0, 0}, VOLT}, //0xF0
  {"P5V_VOLT",   ADC1, read_adc_val, true, {5.5, 0, 0, 4.5, 0, 0, 0, 0}, VOLT}, //0xF1
  {"P3V3_VOLT",  ADC2, read_adc_val, true, {3.53, 0, 0, 3.07, 0, 0, 0, 0}, VOLT}, //0xF2
  {"P2V5_VOLT",  ADC3, read_adc_val, true, {2.625, 0, 0, 2.375, 0, 0, 0, 0}, VOLT}, //0xF3
  {"P1V8_VOLT",  ADC4, read_adc_val, true, {1.89, 0, 0, 1.71, 0, 0, 0, 0}, VOLT}, //0xF4
  {"PGPPA_VOLT", ADC5, read_adc_val, true, {1.89, 0, 0, 1.71, 0, 0, 0, 0}, VOLT}, //0xF5
  {"P1V2_VOLT",  ADC6, read_adc_val, true, {1.26, 0, 0, 1.14, 0, 0, 0, 0}, VOLT}, //0xF6
  {"P1V0_VOLT",  ADC8, read_adc_val, true, {1.08, 0, 0, 0.92, 0, 0, 0, 0}, VOLT}, //0xF7
  {"BMC_P12V_VOLT", DPM_4, read_dpm_vout, true, {13.2, 0, 0, 10.8, 0, 0, 0, 0}, VOLT}, //0xF8
  {NULL, 0, NULL, 0, {0, 0, 0, 0, 0, 0, 0, 0}, 0}, //0xF9
  {NULL, 0, NULL, 0, {0, 0, 0, 0, 0, 0, 0, 0}, 0}, //0xFA
  {"INLET_TEMP", TEMP_BMC, read_bb_sensor, true, {45.0, 0, 0, 5.0, 0, 0, 0, 0}, TEMP}, //0xFB
  {NULL, 0, NULL, 0, {0, 0, 0, 0, 0, 0, 0, 0}, 0}, //0xFC
  {NULL, 0, NULL, 0, {0, 0, 0, 0, 0, 0, 0, 0}, 0}, //0xFD
  {NULL, 0, NULL, 0, {0, 0, 0, 0, 0, 0, 0, 0}, 0}, //0xFE
  {NULL, 0, NULL, 0, {0, 0, 0, 0, 0, 0, 0, 0}, 0}, //0xFF
};

extern struct snr_map sensor_map[];

size_t nic0_sensor_cnt = sizeof(nic0_sensor_list)/sizeof(uint8_t);
size_t nic1_sensor_cnt = sizeof(nic1_sensor_list)/sizeof(uint8_t);
size_t vpdb_sensor_cnt = sizeof(vpdb_sensor_list)/sizeof(uint8_t);
size_t vpdb_1brick_sensor_cnt = sizeof(vpdb_1brick_sensor_list)/sizeof(uint8_t);
size_t vpdb_3brick_sensor_cnt = sizeof(vpdb_3brick_sensor_list)/sizeof(uint8_t);
size_t vpdb_adc_sensor_cnt = sizeof(vpdb_adc_sensor_list)/sizeof(uint8_t);
size_t hpdb_sensor_cnt = sizeof(hpdb_sensor_list)/sizeof(uint8_t);
size_t hpdb_adc_sensor_cnt = sizeof(hpdb_adc_sensor_list)/sizeof(uint8_t);
size_t fan_bp1_sensor_cnt = sizeof(fan_bp1_sensor_list)/sizeof(uint8_t);
size_t fan_bp2_sensor_cnt = sizeof(fan_bp2_sensor_list)/sizeof(uint8_t);
size_t scm_sensor_cnt = sizeof(scm_sensor_list)/sizeof(uint8_t);


static int get_fan_chip_dev_id(uint8_t fru, uint8_t* dev_id) {
  uint8_t id = MAIN_SOURCE;

  if(get_comp_source(fru, fru == FRU_FAN_BP1 ? FAN_BP1_FAN_SOURCE : FAN_BP2_FAN_SOURCE, &id))
    return -1;

  if (id == MAIN_SOURCE)
    *dev_id = FAN_CTRL_ID0;
  else if (id == SECOND_SOURCE)
    *dev_id = FAN_CTRL_ID1;
  else
    return -1;

  return 0;
}


static int
read_nic0_power(uint8_t fru, uint8_t sensor_num, float *value) {
  int ret;

  ret = oper_iic_adc_power(fru, MB_SNR_HSC_VIN, NIC0_SNR_MEZZ0_P12V_IOUT, value);
  return ret;
}

static int
read_nic1_power(uint8_t fru, uint8_t sensor_num, float *value) {
  int ret;

  ret = oper_iic_adc_power(fru, MB_SNR_HSC_VIN, NIC1_SNR_MEZZ1_P12V_IOUT, value);
  return ret;
}


static void
inlet_temp_calibration(uint8_t fru, uint8_t sensor_num, float *value)
{
  int i, cnt = 0;
  float pwm_avg;
  uint8_t pwm = 0;
  uint32_t pwm_all = 0;
  static bool is_inited = false;

  for (i = 0; i < FAN_PWM_CNT; ++i) {
    if (pal_get_pwm_value(i, &pwm) < 0) {}
    else {
      ++cnt;
      pwm_all += pwm;
    }
  }

  if (cnt > 0) {
    pwm_avg = (float)pwm_all/ (float)cnt;
    if (!is_inited) {
      is_inited = true;
      sensor_correction_init("/etc/sensor-mb-correction.json");
    }
    sensor_correction_apply(fru, sensor_num, pwm_avg, value);
  } else {
    syslog(LOG_WARNING, "Failed to apply frontIO correction");
  }
}

static int
read_hsc_reg(uint8_t bus, uint8_t addr, uint8_t reg, uint8_t *rbuf, uint8_t len) {
  int dev, ret, retry = 2;
  uint8_t wbuf[16] = {0};
  char i2c_dev[32];

  snprintf(i2c_dev, sizeof(i2c_dev), "/dev/i2c-%d", bus);
  dev = open(i2c_dev, O_RDWR);
  if (dev < 0) {
    return -1;
  }

  while ((--retry) >= 0) {
    wbuf[0] = reg;
    ret = i2c_rdwr_msg_transfer(dev, addr, wbuf, 1, rbuf, len);
    if (!ret)
      break;
    if (retry)
      msleep(10);
  }
  close(dev);
  if (ret) {
    return -1;
  }

  return 0;
}

static int
read_adm1272_hsc_ein(uint8_t fru, uint8_t sensor_num, float *value) {
  int ret;
  uint8_t rbuf[16] = {0};
  uint32_t energy, rollover, sample;
  uint32_t sample_diff;
  uint32_t energy_diff;
  uint8_t id = sensor_map[fru].map[sensor_num].id;
  uint8_t bus = adm1272_info_list[id].bus;
  uint8_t addr= adm1272_info_list[id].slv_addr;
  static uint32_t pre_energy[PDB_HSC_CNT] = {0};
  static uint32_t pre_rollover[PDB_HSC_CNT] = {0};
  static uint32_t pre_sample[PDB_HSC_CNT] = {0};
  static uint32_t pre_ein[PDB_HSC_CNT] = {0};


  // read READ_EIN_EXT
  ret = read_hsc_reg(bus, addr, ADM1272_REG_READ_EIN_EXT, rbuf, 9);
  if (ret ) {    // length = 8 bytes
      return -1;
  }

  energy   = (uint32_t)((rbuf[3]<<16) | (rbuf[2]<<8) | rbuf[1]) ;
  rollover = (uint32_t)((rbuf[5]<<8) | rbuf[4]);
  sample   = (uint32_t)((rbuf[8]<<16) | (rbuf[7]<<8) | rbuf[6]);

  if (!pre_ein[id]) {
      pre_ein[id] = true;
      return READING_SKIP;
  }

  if ((pre_rollover[id] > rollover) || ((pre_rollover[id] == rollover) && (pre_energy[id] > energy))) {
      rollover += EIN_ROLLOVER_CNT;
  }
  if (pre_sample[id] > sample) {
      sample += EIN_SAMPLE_CNT;
  }

  energy_diff = (rollover-pre_rollover[id])*EIN_ENERGY_CNT + energy - pre_energy[id];

  sample_diff = sample - pre_sample[id];
  if (sample_diff == 0) {
      return -1;
  }
  *value = energy_diff/sample_diff;
  //printf("energy_diff = %x, sample_diff = %x,energy_diff/sample_diff = %f\n\n",energy_diff,sample_diff, *value);

  pre_energy[id] = energy;
  pre_rollover[id] = rollover;
  pre_sample[id] = sample;

  return 0;
}

static int
read_hsc_pin(uint8_t fru, uint8_t sensor_num, float *value) {
  uint8_t id;
  static int retry[PDB_HSC_CNT];
  int ret = -1;
  uint8_t hsc_id=sensor_map[fru].map[sensor_num].id;

  get_comp_source(fru, fru == FRU_VPDB ? VPDB_HSC_SOURCE : HPDB_HSC_SOURCE, &id);
  if(id == SECOND_SOURCE) {
    ret = read_adm1272_hsc_ein(fru, sensor_num, value);
    if (!ret) {
      *value = (*value/256/0.15)*56.94/1000;
      get_comp_source(fru, fru == FRU_VPDB ? VPDB_RSENSE_SOURCE : HPDB_RSENSE_SOURCE, &id);
      if (id == MAIN_SOURCE) {
        if (hsc_id == VPDB_HSC_ID0)
          *value = (*value * 0.97) - 17.04;
        else if (hsc_id == HPDB_HSC_ID1)
          *value = (*value * 0.96) + 4.15;
        else if (hsc_id == HPDB_HSC_ID2)
          *value = (*value * 0.96) - 0.26;
        else
          return -1;
      }
    }
  } else {
    ret = read_bb_sensor(fru, sensor_num, value);
  }

  if (ret) {
    retry[id]++;
    return retry_err_handle(retry[id], 2);
  } else {
    retry[id] = 0;
  }

  return ret;
}

static int
read_bb_sensor(uint8_t fru, uint8_t sensor_num, float *value) {
  int ret;
  static int retry[255];

  ret = sensors_read(NULL, sensor_map[fru].map[sensor_num].snr_name, value);

  if (ret) {
    retry[sensor_num]++;
    return retry_err_handle(retry[sensor_num], 2);
  } else if (*value == 0) {
    retry[sensor_num]++;
    return retry_skip_handle(retry[sensor_num], 2);
  } else {
    retry[sensor_num] = 0;
  }

  if(sensor_num == SCM_SNR_BMC_TEMP)
    inlet_temp_calibration(fru, sensor_num, value);
  return ret;
}

static bool
is_fan_present(int fan_num) {
  int ret = 0;
  gpio_desc_t *gdesc;
  gpio_value_t val;
  char shadow[20] = {0};

  sprintf(shadow, FAN_PRESNT_TEMPLATE, fan_num);

  gdesc = gpio_open_by_shadow(shadow);
  if (gdesc) {
    if (gpio_get_value(gdesc, &val) < 0) {
      syslog(LOG_WARNING, "Get GPIO %s failed", shadow);
      val = GPIO_VALUE_INVALID;
    }
    ret |= (val == GPIO_VALUE_LOW)? 1: 0;
    gpio_close(gdesc);
  }

  return ret? true: false;
}


//NCT7363
static int
set_nct7363y_duty(uint8_t fan_id, uint8_t pwm) {
  int fd = 0, ret = -1;
  char fn[32];
  uint8_t addr, bus;
  uint8_t tbuf[16] = {0};
  uint8_t rbuf[16] = {0};
  uint8_t location = fan_id % 2;
  uint8_t reg_map = 2*(fan_id/4); //fan_id 0-15
  static uint8_t retry=0;

  bus = nct7363y_dev_list[fan_id%4].bus;
  addr = nct7363y_dev_list[fan_id%4].slv_addr;

  snprintf(fn, sizeof(fn), "/dev/i2c-%d", bus);
  fd = open(fn, O_RDWR);
  if (fd < 0) {
    goto err_exit;
  }

  //NCT7363Y Tach High Byte
  reg_map = location ? reg_map : (NCT7363Y_PWM_CNT - reg_map - 2);
  tbuf[0] = nct7363y_ctrl_list[reg_map].pwm_reg;
  tbuf[1] = round((float)pwm*127/100);

  ret = i2c_rdwr_msg_transfer(fd, addr, tbuf, 2, rbuf, 0);
  if( ret < 0 ) {
    retry++;
    ret = retry_err_handle(retry, 2);
    goto err_exit;
  }
#ifdef DEBUG
  syslog(LOG_DEBUG, "%s fan%d pwm_reg[%x]=%x bus=%x addr=%x\n",
         __func__, fan_id, nct7363y_ctrl_list[reg_map].pwm_reg, pwm, bus, addr);
#endif


  retry=0;
err_exit:
  if (fd > 0) {
    close(fd);
  }
  return ret;
}

static int
get_nct7363y_duty(uint8_t fan_id, uint8_t* pwm) {
  int fd = 0, ret = -1;
  char fn[32];
  uint8_t addr, bus;
  uint8_t tbuf[16] = {0};
  uint8_t rbuf[16] = {0};
  uint8_t location = fan_id % 2;
  uint8_t reg_map = 2*(fan_id/4); //fan_id 0-15
  static uint8_t retry=0;

  bus = nct7363y_dev_list[fan_id%4].bus;
  addr = nct7363y_dev_list[fan_id%4].slv_addr;

  snprintf(fn, sizeof(fn), "/dev/i2c-%d", bus);
  fd = open(fn, O_RDWR);
  if (fd < 0) {
    goto err_exit;
  }

  //NCT7363Y PWM REG
  reg_map = location ? reg_map : (NCT7363Y_PWM_CNT - reg_map - 2);
  tbuf[0] = nct7363y_ctrl_list[reg_map].pwm_reg;

  ret = i2c_rdwr_msg_transfer(fd, addr, tbuf, 1, rbuf, 1);
  if( ret < 0 ) {
    retry++;
    ret = retry_err_handle(retry, 2);
    goto err_exit;
  }

  *pwm = round((float)rbuf[0]*100/127);
  retry=0;
err_exit:
  if (fd > 0) {
    close(fd);
  }
#ifdef DEBUG
  syslog(LOG_DEBUG, "%s fan%d pwm_reg[%x]=%x bus=%x addr=%x\n",
         __func__, fan_id, nct7363y_ctrl_list[reg_map].pwm_reg, rbuf[0], bus, addr);
#endif

  return ret;
}


static int
get_nct7363y_rpm(uint8_t fru, uint8_t sensor_num, float *value) {
  int fd = 0, ret = -1;
  char fn[32];
  uint8_t addr, bus;
  uint8_t tbuf[16] = {0};
  uint8_t rbuf[16] = {0};
  uint8_t tach_hb=0;
  uint8_t tach_lb=0;
  uint8_t tach_id = sensor_map[fru].map[sensor_num].id;
  uint8_t location = (tach_id / 2 ) % 2;
  uint8_t reg_map = tach_id%2 + 2*(tach_id/8); //tach_id: 0 - 31
  static uint8_t retry=0;

  bus = nct7363y_dev_list[tach_id%8/2].bus;
  addr = nct7363y_dev_list[tach_id%8/2].slv_addr;

  snprintf(fn, sizeof(fn), "/dev/i2c-%d", bus);
  fd = open(fn, O_RDWR);
  if (fd < 0) {
    goto err_exit;
  }

  //NCT7363Y Tach High Byte
  reg_map = location ? reg_map : (NCT7363Y_PWM_CNT - reg_map - 2 + (reg_map%2)*2);
  tbuf[0] = nct7363y_ctrl_list[reg_map].tach_high;

  ret = i2c_rdwr_msg_transfer(fd, addr, tbuf, 1, rbuf, 1);
  if( ret < 0 ) {
    syslog(LOG_DEBUG, "%s Tach%d hreg=%x bus=%x slavaddr=%x\n",
         __func__, tach_id, nct7363y_ctrl_list[reg_map].tach_high, bus, addr);
    retry++;
    ret = retry_err_handle(retry, 2);
    goto err_exit;
  }
  tach_hb = rbuf[0];


  //NCT7363Y Tach Low Byte
  tbuf[0] = nct7363y_ctrl_list[reg_map].tach_low;

  ret = i2c_rdwr_msg_transfer(fd, addr, tbuf, 1, rbuf, 1);
  if( ret < 0 ) {
    syslog(LOG_DEBUG, "%s Tach%d lreg=%x bus=%x slavaddr=%x\n",
         __func__, tach_id, nct7363y_ctrl_list[reg_map].tach_low, bus, addr);

    retry++;
    ret = retry_err_handle(retry, 2);
    goto err_exit;
  }
  tach_lb = rbuf[0] & 0x1F;


#ifdef DEBUG
  syslog(LOG_DEBUG, "%s Tach%d HB=%x LB=%x bus=%x slavaddr=%x\n",
         __func__, tach_id, tach_hb, tach_lb, bus, addr);
#endif


  *value = 1350000 / (tach_hb << 5 | tach_lb);
  retry=0;
err_exit:
  if (fd > 0) {
    close(fd);
  }
  return ret;
}

//Max3170 Ctrl Fan Function
static int
set_max31790_duty(uint8_t fan_id, uint8_t pwm) {
  uint8_t location = fan_id % 2;
  uint8_t reg_map = fan_id/4;
  int pwm_map[4] = {1, 3, 4, 6};
  char label[32] = {0};

  reg_map = location ? reg_map : (FAN_CHIP_CNT - reg_map - 1);
  snprintf(label, sizeof(label), "pwm%d", pwm_map[reg_map]);
  return sensors_write(fan_chips[fan_id%4], label, (float)pwm);
}

static int
get_max31790_duty(uint8_t fan_id, uint8_t* pwm) {
  int ret;
  uint8_t location = fan_id % 2;
  uint8_t reg_map = fan_id/4;
  int pwm_map[4] = {1, 3, 4, 6};
  float val;
  char label[32] = {0};

  reg_map = location ? reg_map : (FAN_CHIP_CNT - reg_map - 1);
  snprintf(label, sizeof(label), "pwm%d", pwm_map[reg_map]);
  ret = sensors_read(fan_chips[fan_id%4], label, &val);

  if (ret == 0)
    *pwm = (uint8_t)val;

  return ret;
}

static int
get_max31790_rpm(uint8_t fru, uint8_t sensor_num, float *value) {
  uint8_t tach_id = sensor_map[fru].map[sensor_num].id;

  return sensors_read(fan_chips[tach_id%8/2], sensor_map[fru].map[sensor_num].snr_name, value);
}

static int
read_fan_speed(uint8_t fru, uint8_t sensor_num, float *value) {
  int ret = 0;
  uint8_t tach_id = sensor_map[fru].map[sensor_num].id;
  uint8_t dev_id=0;
  static uint8_t retry[FAN_TACH_CNT] = {0};

  if (tach_id >= FAN_TACH_CNT || !is_fan_present(tach_id/2))
    return -1;

  if (get_fan_chip_dev_id(fru, &dev_id))
    return -1;

  ret = fan_ctrl_map[dev_id].get_rpm(fru, sensor_num, value);
  if (*value == 0) {
    retry[tach_id]++;
    return retry_err_handle(retry[tach_id], 2);
  }

  retry[tach_id] = 0;
  return ret;
}

int
pal_get_fan_name(uint8_t num, char *name) {
  if (num >= pal_tach_cnt) {
    syslog(LOG_WARNING, "%s: invalid fan#:%d", __func__, num);
    return -1;
  }

  sprintf(name, "Fan %d %s", num/2, num%2==0? "In":"Out");
  return 0;
}

//Fan-Util Callback function
int
pal_set_fan_speed(uint8_t fan, uint8_t pwm) {
  uint8_t dev_id=0;
  uint8_t fru;
  uint8_t retry=0;

  if (fan >= pal_pwm_cnt) {
    syslog(LOG_INFO, "%s: fan number is invalid - %d", __func__, fan);
    return -1;
  }

  if((fan/2)%2 == 0)
    fru = FRU_FAN_BP1;
  else
    fru = FRU_FAN_BP2;

  if (get_fan_chip_dev_id(fru, &dev_id))
    return -1;

  while(1) {
    if (fan_ctrl_map[dev_id].set_duty(fan, pwm)) {
       retry++;
       if (retry_err_handle(retry, 3) == READING_NA)
         return READING_NA;

       usleep(100000);
    } else {
      break;
    }
  }
  return 0;
}

int
pal_get_fan_speed(uint8_t tach, int *rpm) {
  uint8_t sensor_num = FAN_SNR_START_INDEX + tach;
  float speed = 0;
  uint8_t fru;
  uint8_t retry = 0;

  if (tach >= pal_tach_cnt || !is_fan_present(tach/2)) {
    syslog(LOG_INFO, "%s: tach number is invalid - %d", __func__, tach);
    return -1;
  }

  if((tach/4)%2 == 0)
    fru = FRU_FAN_BP1;
  else
    fru = FRU_FAN_BP2;

  while(1) {
    if (read_fan_speed(fru, sensor_num, &speed)) {
       retry++;
       if (retry_err_handle(retry, 3) == READING_NA)
         return READING_NA;

       usleep(100000);
    } else {
      break;
    }
  }

  *rpm = (int)speed;
  return 0;
}

int
pal_get_pwm_value(uint8_t tach, uint8_t *value) {
  uint8_t fan = tach/2;
  uint8_t dev_id = 0;
  uint8_t fru;
  uint8_t retry = 0;

  if (fan >= pal_pwm_cnt || !is_fan_present(fan)) {
//    syslog(LOG_INFO, "%s: fan number is invalid - %d", __func__, fan);
    return -1;
  }

  if((tach/4)%2 == 0)
    fru = FRU_FAN_BP1;
  else
    fru = FRU_FAN_BP2;

  if (get_fan_chip_dev_id(fru, &dev_id))
    return -1;

  while(1) {
    if (fan_ctrl_map[dev_id].get_duty(fan, value)) {
       retry++;
       if (retry_err_handle(retry, 3) == READING_NA)
         return READING_NA;

       usleep(100000);
    } else {
      break;
    }
  }

  return 0;
}
