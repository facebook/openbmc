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
#include <openbmc/obmc-i2c.h>
#include <openbmc/ipmb.h>
#include <openbmc/obmc-sensors.h>
#include <openbmc/sensor-correction.h>
#include "pal.h"

//#define DEBUG

#define MAX_RETRY 3
#define FAN_DIR "/sys/bus/platform/devices/1e786000.pwm-tacho-controller/hwmon/hwmon0"
#define MAX_SDR_LEN 64
#define SDR_PATH "/tmp/sdr_%s.bin"

enum {
  /* Fan Type */
  DUAL_TYPE    = 0x00,
  SINGLE_TYPE  = 0x03,
  UNKNOWN_TYPE = 0xff,

  /* Fan Cnt*/
  DUAL_FAN_CNT = 0x08,
  SINGLE_FAN_CNT = 0x04,
  UNKNOWN_FAN_CNT = 0x00,
};

static int read_adc_val(uint8_t adc_id, float *value);
static int read_temp(uint8_t snr_id, float *value);
static int read_hsc_vin(uint8_t hsc_id, float *value);
static int read_hsc_temp(uint8_t hsc_id, float *value);
static int read_hsc_pin(uint8_t hsc_id, float *value);
static int read_hsc_iout(uint8_t hsc_id, float *value);
static int read_medusa_val(uint8_t snr_number, float *value);
static int read_cached_val(uint8_t snr_number, float *value);
static int pal_bic_sensor_read_raw(uint8_t fru, uint8_t sensor_num, float *value);

static sensor_info_t g_sinfo[MAX_NUM_FRUS][MAX_SENSOR_NUM] = {0};
static bool sdr_init_done[MAX_NUM_FRUS] = {false};
static uint8_t bic_dynamic_sensor_list[4][MAX_SENSOR_NUM] = {0};
static uint8_t bic_dynamic_skip_sensor_list[4][MAX_SENSOR_NUM] = {0};

size_t pal_pwm_cnt = 4;
size_t pal_tach_cnt = 8;
const char pal_pwm_list[] = "0, 1, 2, 3";
const char pal_fan_opt_list[] = "enable, disable, status";

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
  BMC_SENSOR_HSC_VIN,
  BMC_SENSOR_HSC_PIN,
  BMC_SENSOR_HSC_IOUT,
  BMC_SENSOR_MEDUSA_VOUT,
  BMC_SENSOR_MEDUSA_VIN,
  BMC_SENSOR_MEDUSA_CURR,
  BMC_SENSOR_MEDUSA_PWR,
  BMC_SENSOR_MEDUSA_VDELTA,
  BMC_SENSOR_PDB_VDELTA,
  BMC_SENSOR_FAN_IOUT,
  BMC_SENSOR_NIC_P12V,
  BMC_SENSOR_NIC_IOUT,
  BMC_SENSOR_NIC_PWR,
};

const uint8_t nicexp_sensor_list[] = {
  BMC_SENSOR_OUTLET_TEMP,
  BMC_SENSOR_P12V,
  BMC_SENSOR_P3V3_STBY,
  BMC_SENSOR_P1V15_BMC_STBY,
  BMC_SENSOR_P1V2_BMC_STBY,
  BMC_SENSOR_P2V5_BMC_STBY,
};

const uint8_t bic_sensor_list[] = {
  //BIC - threshold sensors
  BIC_SENSOR_INLET_TEMP,
  BIC_SENSOR_OUTLET_TEMP,
  BIC_SENSOR_FIO_TEMP,
  BIC_SENSOR_PCH_TEMP,
  BIC_SENSOR_CPU_TEMP,
  BIC_SENSOR_CPU_THERM_MARGIN,
  BIC_SENSOR_CPU_TJMAX,
  BIC_SENSOR_DIMMA0_TEMP,
  BIC_SENSOR_DIMMB0_TEMP,
  BIC_SENSOR_DIMMC0_TEMP,
  BIC_SENSOR_DIMMD0_TEMP,
  BIC_SENSOR_DIMME0_TEMP,
  BIC_SENSOR_DIMMF0_TEMP,
  BIC_SENSOR_M2B_TEMP,
  BIC_SENSOR_HSC_TEMP,
  BIC_SENSOR_VCCIN_VR_TEMP,
  BIC_SENSOR_VCCSA_VR_TEMP,
  BIC_SENSOR_VCCIO_VR_Temp,
  BIC_SENSOR_P3V3_STBY_VR_TEMP,
  BIC_SENSOR_PVDDQ_ABC_VR_TEMP,
  BIC_SENSOR_PVDDQ_DEF_VR_TEMP,

  //BIC - voltage sensors
  BIC_SENSOR_P12V_STBY_VOL,
  BIC_SENSOR_P3V_BAT_VOL,
  BIC_SENSOR_P3V3_STBY_VOL,
  BIC_SENSOR_P1V05_PCH_STBY_VOL,
  BIC_SENSOR_PVNN_PCH_STBY_VOL,
  BIC_SENSOR_HSC_INPUT_VOL,
  BIC_SENSOR_VCCIN_VR_VOL,
  BIC_SENSOR_VCCSA_VR_VOL,
  BIC_SENSOR_VCCIO_VR_VOL,
  BIC_SENSOR_P3V3_STBY_VR_VOL,
  BIC_PVDDQ_ABC_VR_VOL,
  BIC_PVDDQ_DEF_VR_VOL,

  //BIC - current sensors
  BIC_SENSOR_HSC_OUTPUT_CUR,
  BIC_SENSOR_VCCIN_VR_CUR,
  BIC_SENSOR_VCCSA_VR_CUR,
  BIC_SENSOR_VCCIO_VR_CUR,
  BIC_SENSOR_P3V3_STBY_VR_CUR,
  BIC_SENSOR_PVDDQ_ABC_VR_CUR,
  BIC_SENSOR_PVDDQ_DEF_VR_CUR,

  //BIC - power sensors
  BIC_SENSOR_HSC_INPUT_PWR,
  BIC_SENSOR_VCCIN_VR_POUT,
  BIC_SENSOR_VCCSA_VR_POUT,
  BIC_SENSOR_VCCIO_VR_POUT,
  BIC_SENSOR_P3V3_STBY_VR_POUT,
  BIC_SENSOR_PVDDQ_ABC_VR_POUT,
  BIC_SENSOR_PVDDQ_DEF_VR_POUT,
};
//BIC 1OU EXP Sensors
const uint8_t bic_1ou_sensor_list[] = {
  BIC_1OU_EXP_SENSOR_OUTLET_TEMP,
  BIC_1OU_EXP_SENSOR_P12_VOL,
  BIC_1OU_EXP_SENSOR_P1V8_VOL,
  BIC_1OU_EXP_SENSOR_P3V3_VOL,
  BIC_1OU_EXP_SENSOR_P3V3_STBY_VR_VOL,
  BIC_1OU_EXP_SENSOR_P3V3_STBY2_VR_VOL,
  BIC_1OU_EXP_SENSOR_P3V3_M2A_PWR,
  BIC_1OU_EXP_SENSOR_P3V3_M2A_VOL,
  BIC_1OU_EXP_SENSOR_P3V3_M2A_TMP,
  BIC_1OU_EXP_SENSOR_P3V3_M2B_PWR,
  BIC_1OU_EXP_SENSOR_P3V3_M2B_VOL,
  BIC_1OU_EXP_SENSOR_P3V3_M2B_TMP,
  BIC_1OU_EXP_SENSOR_P3V3_M2C_PWR,
  BIC_1OU_EXP_SENSOR_P3V3_M2C_VOL,
  BIC_1OU_EXP_SENSOR_P3V3_M2C_TMP,
  BIC_1OU_EXP_SENSOR_P3V3_M2D_PWR,
  BIC_1OU_EXP_SENSOR_P3V3_M2D_VOL,
  BIC_1OU_EXP_SENSOR_P3V3_M2D_TMP,
};
//BIC 2OU EXP Sensors
const uint8_t bic_2ou_sensor_list[] = {
  BIC_2OU_EXP_SENSOR_OUTLET_TEMP,
  BIC_2OU_EXP_SENSOR_P12_VOL,
  BIC_2OU_EXP_SENSOR_P1V8_VOL,
  BIC_2OU_EXP_SENSOR_P3V3_VOL,
  BIC_2OU_EXP_SENSOR_P3V3_STBY_VR_VOL,
  BIC_2OU_EXP_SENSOR_P3V3_STBY2_VR_VOL,
  BIC_2OU_EXP_SENSOR_P3V3_M2A_PWR,
  BIC_2OU_EXP_SENSOR_P3V3_M2A_VOL,
  BIC_2OU_EXP_SENSOR_P3V3_M2A_TMP,
  BIC_2OU_EXP_SENSOR_P3V3_M2B_PWR,
  BIC_2OU_EXP_SENSOR_P3V3_M2B_VOL,
  BIC_2OU_EXP_SENSOR_P3V3_M2B_TMP,
  BIC_2OU_EXP_SENSOR_P3V3_M2C_PWR,
  BIC_2OU_EXP_SENSOR_P3V3_M2C_VOL,
  BIC_2OU_EXP_SENSOR_P3V3_M2C_TMP,
  BIC_2OU_EXP_SENSOR_P3V3_M2D_PWR,
  BIC_2OU_EXP_SENSOR_P3V3_M2D_VOL,
  BIC_2OU_EXP_SENSOR_P3V3_M2D_TMP,
  BIC_2OU_EXP_SENSOR_P3V3_M2E_PWR,
  BIC_2OU_EXP_SENSOR_P3V3_M2E_VOL,
  BIC_2OU_EXP_SENSOR_P3V3_M2E_TMP,
  BIC_2OU_EXP_SENSOR_P3V3_M2F_PWR,
  BIC_2OU_EXP_SENSOR_P3V3_M2F_VOL,
  BIC_2OU_EXP_SENSOR_P3V3_M2F_TMP,
};

const uint8_t bic_1ou_edsff_sensor_list[] = {
  //BIC 1OU EDSFF Sensors 
  BIC_1OU_EDSFF_SENSOR_NUM_T_MB_OUTLET_TEMP_T,
  BIC_1OU_EDSFF_SENSOR_NUM_V_12_AUX,
  BIC_1OU_EDSFF_SENSOR_NUM_V_12_EDGE,
  BIC_1OU_EDSFF_SENSOR_NUM_V_3_3_AUX,
  BIC_1OU_EDSFF_SENSOR_NUM_V_HSC_IN,
  BIC_1OU_EDSFF_SENSOR_NUM_I_HSC_OUT,
  BIC_1OU_EDSFF_SENSOR_NUM_P_HSC_IN,
  BIC_1OU_EDSFF_SENSOR_NUM_T_HSC,
  BIC_1OU_EDSFF_SENSOR_NUM_INA231_PWR_M2A,
  BIC_1OU_EDSFF_SENSOR_NUM_INA231_VOL_M2A,
  BIC_1OU_EDSFF_SENSOR_NUM_NVME_TEMP_M2A,
  BIC_1OU_EDSFF_SENSOR_NUM_ADC_3V3_VOL_M2A,
  BIC_1OU_EDSFF_SENSOR_NUM_ADC_12V_VOL_M2A,
  BIC_1OU_EDSFF_SENSOR_NUM_INA231_PWR_M2B,
  BIC_1OU_EDSFF_SENSOR_NUM_INA231_VOL_M2B,
  BIC_1OU_EDSFF_SENSOR_NUM_NVME_TEMP_M2B,
  BIC_1OU_EDSFF_SENSOR_NUM_ADC_3V3_VOL_M2B,
  BIC_1OU_EDSFF_SENSOR_NUM_ADC_12V_VOL_M2B,
  BIC_1OU_EDSFF_SENSOR_NUM_INA231_PWR_M2C,
  BIC_1OU_EDSFF_SENSOR_NUM_INA231_VOL_M2C,
  BIC_1OU_EDSFF_SENSOR_NUM_NVME_TEMP_M2C,
  BIC_1OU_EDSFF_SENSOR_NUM_ADC_3V3_VOL_M2C,
  BIC_1OU_EDSFF_SENSOR_NUM_ADC_12V_VOL_M2C,
  BIC_1OU_EDSFF_SENSOR_NUM_INA231_PWR_M2D,
  BIC_1OU_EDSFF_SENSOR_NUM_INA231_VOL_M2D,
  BIC_1OU_EDSFF_SENSOR_NUM_NVME_TEMP_M2D,
  BIC_1OU_EDSFF_SENSOR_NUM_ADC_3V3_VOL_M2D,
  BIC_1OU_EDSFF_SENSOR_NUM_ADC_12V_VOL_M2D,
};

//BIC BaseBoard Sensors
const uint8_t bic_bb_sensor_list[] = {
  BIC_BB_SENSOR_INLET_TEMP,
  BIC_BB_SENSOR_OUTLET_TEMP,
  BIC_BB_SENSOR_HSC_TEMP,
  BIC_BB_SENSOR_HSC_VIN,
  BIC_BB_SENSOR_HSC_PIN,
  BIC_BB_SENSOR_HSC_IOUT,
  BIC_BB_SENSOR_P12V_MEDUSA_CUR,
  BIC_BB_SENSOR_P5V,
  BIC_BB_SENSOR_P12V,
  BIC_BB_SENSOR_P3V3_STBY,
  BIC_BB_SENSOR_P1V2_BMC_STBY,
  BIC_BB_SENSOR_P2V5_BMC_STBY,
  BIC_BB_SENSOR_MEDUSA_VOUT,
  BIC_BB_SENSOR_MEDUSA_VIN,
  BIC_BB_SENSOR_MEDUSA_PIN,
  BIC_BB_SENSOR_MEDUSA_IOUT
};

const uint8_t bic_skip_sensor_list[] = {
  BIC_SENSOR_CPU_TEMP,
  BIC_SENSOR_DIMMA0_TEMP,
  BIC_SENSOR_DIMMB0_TEMP,
  BIC_SENSOR_DIMMC0_TEMP,
  BIC_SENSOR_DIMMD0_TEMP,
  BIC_SENSOR_DIMME0_TEMP,
  BIC_SENSOR_DIMMF0_TEMP,
  BIC_SENSOR_M2B_TEMP,
  BIC_SENSOR_HSC_TEMP,
  BIC_SENSOR_VCCIN_VR_TEMP,
  BIC_SENSOR_VCCSA_VR_TEMP,
  BIC_SENSOR_VCCIO_VR_Temp,
  BIC_SENSOR_PVDDQ_ABC_VR_TEMP,
  BIC_SENSOR_PVDDQ_DEF_VR_TEMP,
  //BIC - voltage sensors
  BIC_SENSOR_VCCIN_VR_VOL,
  BIC_SENSOR_VCCSA_VR_VOL,
  BIC_SENSOR_VCCIO_VR_VOL,
  BIC_SENSOR_P3V3_STBY_VR_VOL,
  BIC_PVDDQ_ABC_VR_VOL,
  BIC_PVDDQ_DEF_VR_VOL,
  //BIC - current sensors
  BIC_SENSOR_VCCIN_VR_CUR,
  BIC_SENSOR_VCCSA_VR_CUR,
  BIC_SENSOR_VCCIO_VR_CUR,
  BIC_SENSOR_PVDDQ_ABC_VR_CUR,
  BIC_SENSOR_PVDDQ_DEF_VR_CUR,
  //BIC - power sensors
  BIC_SENSOR_VCCIN_VR_POUT,
  BIC_SENSOR_VCCSA_VR_POUT,
  BIC_SENSOR_VCCIO_VR_POUT,
  BIC_SENSOR_PVDDQ_ABC_VR_POUT,
  BIC_SENSOR_PVDDQ_DEF_VR_POUT,
};

const uint8_t bic_1ou_skip_sensor_list[] = {
  //BIC 1OU EXP Sensors
  BIC_1OU_EXP_SENSOR_P1V8_VOL,
  BIC_1OU_EXP_SENSOR_P3V3_M2A_PWR,
  BIC_1OU_EXP_SENSOR_P3V3_M2A_VOL,
  BIC_1OU_EXP_SENSOR_P3V3_M2A_TMP,
  BIC_1OU_EXP_SENSOR_P3V3_M2B_PWR,
  BIC_1OU_EXP_SENSOR_P3V3_M2B_VOL,
  BIC_1OU_EXP_SENSOR_P3V3_M2B_TMP,
  BIC_1OU_EXP_SENSOR_P3V3_M2C_PWR,
  BIC_1OU_EXP_SENSOR_P3V3_M2C_VOL,
  BIC_1OU_EXP_SENSOR_P3V3_M2C_TMP,
  BIC_1OU_EXP_SENSOR_P3V3_M2D_PWR,
  BIC_1OU_EXP_SENSOR_P3V3_M2D_VOL,
  BIC_1OU_EXP_SENSOR_P3V3_M2D_TMP,
};

const uint8_t bic_2ou_skip_sensor_list[] = {
  //BIC 2OU EXP Sensors
  BIC_2OU_EXP_SENSOR_P1V8_VOL,
  BIC_2OU_EXP_SENSOR_P3V3_M2A_PWR,
  BIC_2OU_EXP_SENSOR_P3V3_M2A_VOL,
  BIC_2OU_EXP_SENSOR_P3V3_M2A_TMP,
  BIC_2OU_EXP_SENSOR_P3V3_M2B_PWR,
  BIC_2OU_EXP_SENSOR_P3V3_M2B_VOL,
  BIC_2OU_EXP_SENSOR_P3V3_M2B_TMP,
  BIC_2OU_EXP_SENSOR_P3V3_M2C_PWR,
  BIC_2OU_EXP_SENSOR_P3V3_M2C_VOL,
  BIC_2OU_EXP_SENSOR_P3V3_M2C_TMP,
  BIC_2OU_EXP_SENSOR_P3V3_M2D_PWR,
  BIC_2OU_EXP_SENSOR_P3V3_M2D_VOL,
  BIC_2OU_EXP_SENSOR_P3V3_M2D_TMP,
  BIC_2OU_EXP_SENSOR_P3V3_M2E_PWR,
  BIC_2OU_EXP_SENSOR_P3V3_M2E_VOL,
  BIC_2OU_EXP_SENSOR_P3V3_M2E_TMP,
  BIC_2OU_EXP_SENSOR_P3V3_M2F_PWR,
  BIC_2OU_EXP_SENSOR_P3V3_M2F_VOL,
  BIC_2OU_EXP_SENSOR_P3V3_M2F_TMP,
};

const uint8_t bic_1ou_edsff_skip_sensor_list[] = {
  BIC_1OU_EDSFF_SENSOR_NUM_V_HSC_IN,
  BIC_1OU_EDSFF_SENSOR_NUM_I_HSC_OUT,
  BIC_1OU_EDSFF_SENSOR_NUM_P_HSC_IN,
  BIC_1OU_EDSFF_SENSOR_NUM_T_HSC,
  BIC_1OU_EDSFF_SENSOR_NUM_INA231_PWR_M2A,
  BIC_1OU_EDSFF_SENSOR_NUM_INA231_VOL_M2A,
  BIC_1OU_EDSFF_SENSOR_NUM_NVME_TEMP_M2A,
  BIC_1OU_EDSFF_SENSOR_NUM_ADC_3V3_VOL_M2A,
  BIC_1OU_EDSFF_SENSOR_NUM_ADC_12V_VOL_M2A,
  BIC_1OU_EDSFF_SENSOR_NUM_INA231_PWR_M2B,
  BIC_1OU_EDSFF_SENSOR_NUM_INA231_VOL_M2B,
  BIC_1OU_EDSFF_SENSOR_NUM_NVME_TEMP_M2B,
  BIC_1OU_EDSFF_SENSOR_NUM_ADC_3V3_VOL_M2B,
  BIC_1OU_EDSFF_SENSOR_NUM_ADC_12V_VOL_M2B,
  BIC_1OU_EDSFF_SENSOR_NUM_INA231_PWR_M2C,
  BIC_1OU_EDSFF_SENSOR_NUM_INA231_VOL_M2C,
  BIC_1OU_EDSFF_SENSOR_NUM_NVME_TEMP_M2C,
  BIC_1OU_EDSFF_SENSOR_NUM_ADC_3V3_VOL_M2C,
  BIC_1OU_EDSFF_SENSOR_NUM_ADC_12V_VOL_M2C,
  BIC_1OU_EDSFF_SENSOR_NUM_INA231_PWR_M2D,
  BIC_1OU_EDSFF_SENSOR_NUM_INA231_VOL_M2D,
  BIC_1OU_EDSFF_SENSOR_NUM_NVME_TEMP_M2D,
  BIC_1OU_EDSFF_SENSOR_NUM_ADC_3V3_VOL_M2D,
  BIC_1OU_EDSFF_SENSOR_NUM_ADC_12V_VOL_M2D,
};

const uint8_t nic_sensor_list[] = {
  NIC_SENSOR_TEMP,
};

// List of MB discrete sensors to be monitored
const uint8_t bmc_discrete_sensor_list[] = {
};

//ADM1278
PAL_ATTR_INFO adm1278_info_list[] = {
  {HSC_VOLTAGE, 19599, 0, 100},
  {HSC_CURRENT, 800 * ADM1278_RSENSE, 20475, 10},
  {HSC_POWER, 6123 * ADM1278_RSENSE, 0, 100},
  {HSC_TEMP, 42, 31880, 10},
};

//{SensorName, ID, FUNCTION, PWR_STATUS, {UCR, UNR, UNC, LCR, LNR, LNC, Pos, Neg}
PAL_SENSOR_MAP sensor_map[] = {
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
  {NULL, 0, NULL, 0, {0, 0, 0, 0, 0, 0, 0, 0}, 0}, //0x32
  {NULL, 0, NULL, 0, {0, 0, 0, 0, 0, 0, 0, 0}, 0}, //0x33
  {NULL, 0, NULL, 0, {0, 0, 0, 0, 0, 0, 0, 0}, 0}, //0x34
  {NULL, 0, NULL, 0, {0, 0, 0, 0, 0, 0, 0, 0}, 0}, //0x35
  {NULL, 0, NULL, 0, {0, 0, 0, 0, 0, 0, 0, 0}, 0}, //0x36
  {NULL, 0, NULL, 0, {0, 0, 0, 0, 0, 0, 0, 0}, 0}, //0x37
  {NULL, 0, NULL, 0, {0, 0, 0, 0, 0, 0, 0, 0}, 0}, //0x38
  {NULL, 0, NULL, 0, {0, 0, 0, 0, 0, 0, 0, 0}, 0}, //0x39
  {NULL, 0, NULL, 0, {0, 0, 0, 0, 0, 0, 0, 0}, 0}, //0x3A
  {NULL, 0, NULL, 0, {0, 0, 0, 0, 0, 0, 0, 0}, 0}, //0x3B
  {NULL, 0, NULL, 0, {0, 0, 0, 0, 0, 0, 0, 0}, 0}, //0x3C
  {NULL, 0, NULL, 0, {0, 0, 0, 0, 0, 0, 0, 0}, 0}, //0x3D
  {NULL, 0, NULL, 0, {0, 0, 0, 0, 0, 0, 0, 0}, 0}, //0x3E
  {NULL, 0, NULL, 0, {0, 0, 0, 0, 0, 0, 0, 0}, 0}, //0x3F

  {NULL, 0, NULL, 0, {0, 0, 0, 0, 0, 0, 0, 0}, 0}, //0x40
  {NULL, 0, NULL, 0, {0, 0, 0, 0, 0, 0, 0, 0}, 0}, //0x41
  {NULL, 0, NULL, 0, {0, 0, 0, 0, 0, 0, 0, 0}, 0}, //0x42
  {NULL, 0, NULL, 0, {0, 0, 0, 0, 0, 0, 0, 0}, 0}, //0x43
  {NULL, 0, NULL, 0, {0, 0, 0, 0, 0, 0, 0, 0}, 0}, //0x44
  {NULL, 0, NULL, 0, {0, 0, 0, 0, 0, 0, 0, 0}, 0}, //0x45
  {NULL, 0, NULL, 0, {0, 0, 0, 0, 0, 0, 0, 0}, 0}, //0x46
  {NULL, 0, NULL, 0, {0, 0, 0, 0, 0, 0, 0, 0}, 0}, //0x47
  {NULL, 0, NULL, 0, {0, 0, 0, 0, 0, 0, 0, 0}, 0}, //0x48
  {NULL, 0, NULL, 0, {0, 0, 0, 0, 0, 0, 0, 0}, 0}, //0x49
  {NULL, 0, NULL, 0, {0, 0, 0, 0, 0, 0, 0, 0}, 0}, //0x4A
  {NULL, 0, NULL, 0, {0, 0, 0, 0, 0, 0, 0, 0}, 0}, //0x4B
  {NULL, 0, NULL, 0, {0, 0, 0, 0, 0, 0, 0, 0}, 0}, //0x4C
  {NULL, 0, NULL, 0, {0, 0, 0, 0, 0, 0, 0, 0}, 0}, //0x4D
  {NULL, 0, NULL, 0, {0, 0, 0, 0, 0, 0, 0, 0}, 0}, //0x4E
  {NULL, 0, NULL, 0, {0, 0, 0, 0, 0, 0, 0, 0}, 0}, //0x4F

  {NULL, 0, NULL, 0, {0, 0, 0, 0, 0, 0, 0, 0}, 0}, //0x50
  {NULL, 0, NULL, 0, {0, 0, 0, 0, 0, 0, 0, 0}, 0}, //0x51
  {NULL, 0, NULL, 0, {0, 0, 0, 0, 0, 0, 0, 0}, 0}, //0x52
  {NULL, 0, NULL, 0, {0, 0, 0, 0, 0, 0, 0, 0}, 0}, //0x53
  {NULL, 0, NULL, 0, {0, 0, 0, 0, 0, 0, 0, 0}, 0}, //0x54
  {NULL, 0, NULL, 0, {0, 0, 0, 0, 0, 0, 0, 0}, 0}, //0x55
  {NULL, 0, NULL, 0, {0, 0, 0, 0, 0, 0, 0, 0}, 0}, //0x56
  {NULL, 0, NULL, 0, {0, 0, 0, 0, 0, 0, 0, 0}, 0}, //0x57
  {NULL, 0, NULL, 0, {0, 0, 0, 0, 0, 0, 0, 0}, 0}, //0x58
  {NULL, 0, NULL, 0, {0, 0, 0, 0, 0, 0, 0, 0}, 0}, //0x59
  {NULL, 0, NULL, 0, {0, 0, 0, 0, 0, 0, 0, 0}, 0}, //0x5A
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

  {NULL, 0, NULL, 0, {0, 0, 0, 0, 0, 0, 0, 0}, 0}, //0x80
  {NULL, 0, NULL, 0, {0, 0, 0, 0, 0, 0, 0, 0}, 0}, //0x81
  {NULL, 0, NULL, 0, {0, 0, 0, 0, 0, 0, 0, 0}, 0}, //0x82
  {NULL, 0, NULL, 0, {0, 0, 0, 0, 0, 0, 0, 0}, 0}, //0x83
  {NULL, 0, NULL, 0, {0, 0, 0, 0, 0, 0, 0, 0}, 0}, //0x84
  {NULL, 0, NULL, 0, {0, 0, 0, 0, 0, 0, 0, 0}, 0}, //0x85
  {NULL, 0, NULL, 0, {0, 0, 0, 0, 0, 0, 0, 0}, 0}, //0x86
  {NULL, 0, NULL, 0, {0, 0, 0, 0, 0, 0, 0, 0}, 0}, //0x87
  {NULL, 0, NULL, 0, {0, 0, 0, 0, 0, 0, 0, 0}, 0}, //0x88
  {NULL, 0, NULL, 0, {0, 0, 0, 0, 0, 0, 0, 0}, 0}, //0x89
  {NULL, 0, NULL, 0, {0, 0, 0, 0, 0, 0, 0, 0}, 0}, //0x8A
  {NULL, 0, NULL, 0, {0, 0, 0, 0, 0, 0, 0, 0}, 0}, //0x8B
  {NULL, 0, NULL, 0, {0, 0, 0, 0, 0, 0, 0, 0}, 0}, //0x8C
  {NULL, 0, NULL, 0, {0, 0, 0, 0, 0, 0, 0, 0}, 0}, //0x8D
  {NULL, 0, NULL, 0, {0, 0, 0, 0, 0, 0, 0, 0}, 0}, //0x8E
  {NULL, 0, NULL, 0, {0, 0, 0, 0, 0, 0, 0, 0}, 0}, //0x8F

  {NULL, 0, NULL, 0, {0, 0, 0, 0, 0, 0, 0, 0}, 0}, //0x90
  {NULL, 0, NULL, 0, {0, 0, 0, 0, 0, 0, 0, 0}, 0}, //0x91
  {NULL, 0, NULL, 0, {0, 0, 0, 0, 0, 0, 0, 0}, 0}, //0x92
  {NULL, 0, NULL, 0, {0, 0, 0, 0, 0, 0, 0, 0}, 0}, //0x93
  {NULL, 0, NULL, 0, {0, 0, 0, 0, 0, 0, 0, 0}, 0}, //0x94
  {NULL, 0, NULL, 0, {0, 0, 0, 0, 0, 0, 0, 0}, 0}, //0x95
  {NULL, 0, NULL, 0, {0, 0, 0, 0, 0, 0, 0, 0}, 0}, //0x96
  {NULL, 0, NULL, 0, {0, 0, 0, 0, 0, 0, 0, 0}, 0}, //0x97
  {NULL, 0, NULL, 0, {0, 0, 0, 0, 0, 0, 0, 0}, 0}, //0x98
  {NULL, 0, NULL, 0, {0, 0, 0, 0, 0, 0, 0, 0}, 0}, //0x99
  {NULL, 0, NULL, 0, {0, 0, 0, 0, 0, 0, 0, 0}, 0}, //0x9A
  {NULL, 0, NULL, 0, {0, 0, 0, 0, 0, 0, 0, 0}, 0}, //0x9B
  {NULL, 0, NULL, 0, {0, 0, 0, 0, 0, 0, 0, 0}, 0}, //0x9C
  {NULL, 0, NULL, 0, {0, 0, 0, 0, 0, 0, 0, 0}, 0}, //0x9D
  {NULL, 0, NULL, 0, {0, 0, 0, 0, 0, 0, 0, 0}, 0}, //0x9E
  {NULL, 0, NULL, 0, {0, 0, 0, 0, 0, 0, 0, 0}, 0}, //0x9F

  {NULL, 0, NULL, 0, {0, 0, 0, 0, 0, 0, 0, 0}, 0}, //0xA0
  {NULL, 0, NULL, 0, {0, 0, 0, 0, 0, 0, 0, 0}, 0}, //0xA1
  {NULL, 0, NULL, 0, {0, 0, 0, 0, 0, 0, 0, 0}, 0}, //0xA2
  {NULL, 0, NULL, 0, {0, 0, 0, 0, 0, 0, 0, 0}, 0}, //0xA3
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

  {NULL, 0, NULL, 0, {0, 0, 0, 0, 0, 0, 0, 0}, 0}, //0xB0
  {NULL, 0, NULL, 0, {0, 0, 0, 0, 0, 0, 0, 0}, 0}, //0xB1
  {NULL, 0, NULL, 0, {0, 0, 0, 0, 0, 0, 0, 0}, 0}, //0XB2
  {NULL, 0, NULL, 0, {0, 0, 0, 0, 0, 0, 0, 0}, 0}, //0xB3
  {NULL, 0, NULL, 0, {0, 0, 0, 0, 0, 0, 0, 0}, 0}, //0xB4
  {NULL, 0, NULL, 0, {0, 0, 0, 0, 0, 0, 0, 0}, 0}, //0xB5
  {NULL, 0, NULL, 0, {0, 0, 0, 0, 0, 0, 0, 0}, 0}, //0xB6
  {NULL, 0, NULL, 0, {0, 0, 0, 0, 0, 0, 0, 0}, 0}, //0xB7
  {NULL, 0, NULL, 0, {0, 0, 0, 0, 0, 0, 0, 0}, 0}, //0xB8
  {NULL, 0, NULL, 0, {0, 0, 0, 0, 0, 0, 0, 0}, 0}, //0xB9
  {NULL, 0, NULL, 0, {0, 0, 0, 0, 0, 0, 0, 0}, 0}, //0xBA
  {NULL, 0, NULL, 0, {0, 0, 0, 0, 0, 0, 0, 0}, 0}, //0xBB
  {NULL, 0, NULL, 0, {0, 0, 0, 0, 0, 0, 0, 0}, 0}, //0xBC
  {NULL, 0, NULL, 0, {0, 0, 0, 0, 0, 0, 0, 0}, 0}, //0xBD
  {NULL, 0, NULL, 0, {0, 0, 0, 0, 0, 0, 0, 0}, 0}, //0xBE
  {NULL, 0, NULL, 0, {0, 0, 0, 0, 0, 0, 0, 0}, 0}, //0xBF

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
  {"BMC_SENSOR_PDB_VDELTA", 0xCE, read_cached_val, true, {0, 0, 0, 0, 0, 0, 0, 0}, VOLT}, //0xCE
  {"BMC_SENSOR_MEDUSA_VDELTA", 0xCF, read_cached_val, true, {0.5, 0, 0, 0, 0, 0, 0, 0}, VOLT}, //0xCF

  {"BMC_SENSOR_MEDUSA_CURR", 0xD0, read_medusa_val, 0, {144, 0, 0, 0, 0, 0, 0, 0}, CURR}, //0xD0
  {"BMC_SENSOR_MEDUSA_PWR", 0xD1, read_medusa_val, 0, {1800, 0, 0, 0, 0, 0, 0, 0}, POWER}, //0xD1
  {"BMC_SENSOR_NIC_P12V", ADC10, read_adc_val, true, {13.23, 0, 0, 11.277, 0, 0, 0, 0}, VOLT},//0xD2
  {"BMC_SENSOR_NIC_PWR" , 0xD3, read_cached_val, true, {82.5, 0, 0, 0, 0, 0, 0, 0}, POWER},//0xD3
  {NULL, 0, NULL, 0, {0, 0, 0, 0, 0, 0, 0, 0}, 0}, //0xD4
  {NULL, 0, NULL, 0, {0, 0, 0, 0, 0, 0, 0, 0}, 0}, //0xD5
  {NULL, 0, NULL, 0, {0, 0, 0, 0, 0, 0, 0, 0}, 0}, //0xD6
  {NULL, 0, NULL, 0, {0, 0, 0, 0, 0, 0, 0, 0}, 0}, //0xD7
  {NULL, 0, NULL, 0, {0, 0, 0, 0, 0, 0, 0, 0}, 0}, //0xD8
  {NULL, 0, NULL, 0, {0, 0, 0, 0, 0, 0, 0, 0}, 0}, //0xD9
  {NULL, 0, NULL, 0, {0, 0, 0, 0, 0, 0, 0, 0}, 0}, //0xDA
  {NULL, 0, NULL, 0, {0, 0, 0, 0, 0, 0, 0, 0}, 0}, //0xDB
  {NULL, 0, NULL, 0, {0, 0, 0, 0, 0, 0, 0, 0}, 0}, //0xDC
  {NULL, 0, NULL, 0, {0, 0, 0, 0, 0, 0, 0, 0}, 0}, //0xDD
  {NULL, 0, NULL, 0, {0, 0, 0, 0, 0, 0, 0, 0}, 0}, //0xDE
  {NULL, 0, NULL, 0, {0, 0, 0, 0, 0, 0, 0, 0}, 0}, //0xDF

  {NULL, 0, NULL, 0, {0, 0, 0, 0, 0, 0, 0, 0}, 0}, //0xE0
  {NULL, 0, NULL, 0, {0, 0, 0, 0, 0, 0, 0, 0}, 0}, //0xE1
  {NULL, 0, NULL, 0, {0, 0, 0, 0, 0, 0, 0, 0}, 0}, //0xE2
  {NULL, 0, NULL, 0, {0, 0, 0, 0, 0, 0, 0, 0}, 0}, //0xE3
  {NULL, 0, NULL, 0, {0, 0, 0, 0, 0, 0, 0, 0}, 0}, //0xE4
  {NULL, 0, NULL, 0, {0, 0, 0, 0, 0, 0, 0, 0}, 0}, //0xE5
  {NULL, 0, NULL, 0, {0, 0, 0, 0, 0, 0, 0, 0}, 0}, //0xE6
  {NULL, 0, NULL, 0, {0, 0, 0, 0, 0, 0, 0, 0}, 0}, //0xE7
  {NULL, 0, NULL, 0, {0, 0, 0, 0, 0, 0, 0, 0}, 0}, //0xE8
  {NULL, 0, NULL, 0, {0, 0, 0, 0, 0, 0, 0, 0}, 0}, //0xE9
  {NULL, 0, NULL, 0, {0, 0, 0, 0, 0, 0, 0, 0}, 0}, //0xEA
  {NULL, 0, NULL, 0, {0, 0, 0, 0, 0, 0, 0, 0}, 0}, //0xEB
  {NULL, 0, NULL, 0, {0, 0, 0, 0, 0, 0, 0, 0}, 0}, //0xEC
  {"BMC_INLET_TEMP",  TEMP_INLET,  read_temp, true, {50, 0, 0, 0, 0, 0, 0, 0}, TEMP}, //0xED
  {"BMC_OUTLET_TEMP", TEMP_OUTLET, read_temp, true, {55, 0, 0, 0, 0, 0, 0, 0}, TEMP}, //0xEE
  {"NIC_SENSOR_TEMP", TEMP_NIC, read_temp, true, {95, 0, 0, 0, 0, 0, 0, 0}, TEMP}, //0xEF

  {"BMC_SENSOR_P5V", ADC0, read_adc_val, true, {5.486, 0, 0, 4.524, 0, 0, 0, 0}, VOLT}, //0xF0
  {"BMC_SENSOR_P12V", ADC1, read_adc_val, true, {13.23, 0, 0, 11.277, 0, 0, 0, 0}, VOLT}, //0xF1
  {"BMC_SENSOR_P3V3_STBY", ADC2, read_adc_val, true, {3.629, 0, 0, 2.976, 0, 0, 0, 0}, VOLT}, //0xF2
  {"BMC_SENSOR_P1V15_STBY", ADC3, read_adc_val, true, {1.264, 0, 0, 1.037, 0, 0, 0, 0}, VOLT}, //0xF3
  {"BMC_SENSOR_P1V2_STBY", ADC4, read_adc_val, true, {1.314, 0, 0, 1.086, 0, 0, 0, 0}, VOLT}, //0xF4
  {"BMC_SENSOR_P2V5_STBY", ADC5, read_adc_val, true, {2.743, 0, 0, 2.262, 0, 0, 0, 0}, VOLT}, //0xF5
  {"BMC_SENSOR_MEDUSA_VOUT", 0xF6, read_medusa_val, true, {13.23, 0, 0, 11.277, 0, 0, 0, 0}, VOLT}, //0xF6
  {"BMC_SENSOR_HSC_VIN", HSC_ID0, read_hsc_vin, true, {13.2, 0, 0, 10.8, 0, 0, 0, 0}, VOLT}, //0xF7
  {"BMC_SENSOR_HSC_TEMP", HSC_ID0, read_hsc_temp, true, {55, 0, 0, 0, 0, 0, 0, 0}, TEMP}, //0xF8
  {"BMC_SENSOR_HSC_PIN" , HSC_ID0, read_hsc_pin , true, {362, 0, 0, 0, 0, 0, 0, 0}, POWER}, //0xF9
  {"BMC_SENSOR_HSC_IOUT", HSC_ID0, read_hsc_iout, true, {27.4, 0, 0, 0, 0, 0, 0, 0}, CURR}, //0xFA
  {"BMC_SENSOR_FAN_IOUT", ADC8, read_adc_val, 0, {25.6, 0, 0, 0, 0, 0, 0, 0}, CURR}, //0xFB
  {"BMC_SENSOR_NIC_IOUT", ADC9, read_adc_val, 0, {6.6, 0, 0, 0, 0, 0, 0, 0}, CURR}, //0xFC
  {"BMC_SENSOR_MEDUSA_VIN", 0xFD, read_medusa_val, true, {13.23, 0, 0, 11.277, 0, 0, 0, 0}, VOLT}, //0xFD
  {NULL, 0, NULL, 0, {0, 0, 0, 0, 0, 0, 0, 0}, 0}, //0xFE
  {NULL, 0, NULL, 0, {0, 0, 0, 0, 0, 0, 0, 0}, 0}, //0xFF
};

//HSC
PAL_HSC_INFO hsc_info_list[] = {
  {HSC_ID0, ADM1278_SLAVE_ADDR, adm1278_info_list},
};

size_t bmc_sensor_cnt = sizeof(bmc_sensor_list)/sizeof(uint8_t);
size_t nicexp_sensor_cnt = sizeof(nicexp_sensor_list)/sizeof(uint8_t);
size_t nic_sensor_cnt = sizeof(nic_sensor_list)/sizeof(uint8_t);
size_t bic_sensor_cnt = sizeof(bic_sensor_list)/sizeof(uint8_t);
size_t bic_1ou_sensor_cnt = sizeof(bic_1ou_sensor_list)/sizeof(uint8_t);
size_t bic_2ou_sensor_cnt = sizeof(bic_2ou_sensor_list)/sizeof(uint8_t);
size_t bic_bb_sensor_cnt = sizeof(bic_bb_sensor_list)/sizeof(uint8_t);
size_t bic_1ou_edsff_sensor_cnt = sizeof(bic_1ou_edsff_sensor_list)/sizeof(uint8_t);
size_t bic_skip_sensor_cnt = sizeof(bic_skip_sensor_list)/sizeof(uint8_t);
size_t bic_1ou_skip_sensor_cnt = sizeof(bic_1ou_skip_sensor_list)/sizeof(uint8_t);
size_t bic_2ou_skip_sensor_cnt = sizeof(bic_2ou_skip_sensor_list)/sizeof(uint8_t);
size_t bic_1ou_edsff_skip_sensor_cnt = sizeof(bic_1ou_edsff_skip_sensor_list)/sizeof(uint8_t);


int
get_skip_sensor_list(uint8_t fru, uint8_t **skip_sensor_list, int *cnt) {
  uint8_t type = 0;
  static uint8_t current_cnt = 0;

  if (bic_dynamic_skip_sensor_list[fru-1][0] == 0) {

    memcpy(bic_dynamic_skip_sensor_list[fru-1], bic_skip_sensor_list, bic_skip_sensor_cnt);
    current_cnt = bic_skip_sensor_cnt;
        
    bic_get_1ou_type(fru, &type); 
    if (type == EDSFF_1U) {
      memcpy(&bic_dynamic_skip_sensor_list[fru-1][current_cnt], bic_1ou_edsff_skip_sensor_list, bic_1ou_edsff_skip_sensor_cnt);
      current_cnt += bic_1ou_edsff_skip_sensor_cnt;
    } else {
      memcpy(&bic_dynamic_skip_sensor_list[fru-1][current_cnt], bic_1ou_skip_sensor_list, bic_1ou_skip_sensor_cnt);
      current_cnt += bic_1ou_skip_sensor_cnt;
    }
    
    memcpy(&bic_dynamic_skip_sensor_list[fru-1][current_cnt], bic_2ou_skip_sensor_list, bic_2ou_skip_sensor_cnt);
    current_cnt += bic_2ou_skip_sensor_cnt;
  }
  
  *skip_sensor_list = (uint8_t *) bic_dynamic_skip_sensor_list[fru-1];
  *cnt = current_cnt;
  
  return 0;
}

int
pal_get_fru_sensor_list(uint8_t fru, uint8_t **sensor_list, int *cnt) {
  uint8_t bmc_location = 0, type = 0;
  int ret = 0;
  uint8_t config_status = 0;
  uint8_t current_cnt = 0;

  ret = fby3_common_get_bmc_location(&bmc_location);
  if (ret < 0) {
    syslog(LOG_ERR, "%s() Cannot get the location of BMC", __func__);
  }

  switch(fru) {
  case FRU_BMC:
    if (bmc_location == NIC_BMC) {
      *sensor_list = (uint8_t *) nicexp_sensor_list;
      *cnt = nicexp_sensor_cnt;
    } else {
      *sensor_list = (uint8_t *) bmc_sensor_list;
      *cnt = bmc_sensor_cnt;
    }

    break;
  case FRU_NIC:
    *sensor_list = (uint8_t *) nic_sensor_list;
    *cnt = nic_sensor_cnt;
    break;
  case FRU_SLOT1:
  case FRU_SLOT2:
  case FRU_SLOT3:
  case FRU_SLOT4:
    memcpy(bic_dynamic_sensor_list[fru-1], bic_sensor_list, bic_sensor_cnt);
    current_cnt = bic_sensor_cnt;
    config_status = (pal_is_fw_update_ongoing(fru) == false) ? bic_is_m2_exp_prsnt(fru):bic_is_m2_exp_prsnt_cache(fru);

    // 1OU
    if ( (bmc_location == BB_BMC || bmc_location == DVT_BB_BMC) && ( (config_status == PRESENT_1OU) || (config_status == (PRESENT_1OU + PRESENT_2OU))) ) {
      ret = (pal_is_fw_update_ongoing(fru) == false) ? bic_get_1ou_type(fru, &type):bic_get_1ou_type_cache(fru, &type);
      if (type == EDSFF_1U) {
        memcpy(&bic_dynamic_sensor_list[fru-1][current_cnt], bic_1ou_edsff_sensor_list, bic_1ou_edsff_sensor_cnt);
        current_cnt += bic_1ou_edsff_sensor_cnt;
      } else {
        memcpy(&bic_dynamic_sensor_list[fru-1][current_cnt], bic_1ou_sensor_list, bic_1ou_sensor_cnt);
        current_cnt += bic_1ou_sensor_cnt;
      }
    }

    // 2OU
    if ( (config_status == PRESENT_2OU) || (config_status == (PRESENT_1OU + PRESENT_2OU)) ) {
      memcpy(&bic_dynamic_sensor_list[fru-1][current_cnt], bic_2ou_sensor_list, bic_2ou_sensor_cnt);
      current_cnt += bic_2ou_sensor_cnt;
    }

    // BB (for NIC_BMC only)
    if ( bmc_location == NIC_BMC ) {
      memcpy(&bic_dynamic_sensor_list[fru-1][current_cnt], bic_bb_sensor_list, bic_bb_sensor_cnt);
      current_cnt += bic_bb_sensor_cnt;
    }

    *sensor_list = (uint8_t *) bic_dynamic_sensor_list[fru-1];
    *cnt = current_cnt;
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

static int
get_gpio_shadow_array(const char **shadows, int num, uint8_t *mask) {
  int i;
  *mask = 0;
  for (i = 0; i < num; i++) {
    int ret;
    gpio_value_t value;
    gpio_desc_t *gpio = gpio_open_by_shadow(shadows[i]);
    if (!gpio) {
      return -1;
    }
    ret = gpio_get_value(gpio, &value);
    gpio_close(gpio);
    if (ret != 0) {
      return -1;
    }
    *mask |= (value == GPIO_VALUE_HIGH ? 1 : 0) << i;
  }
  return 0;
}

static int
pal_get_fan_type(uint8_t *bmc_location, uint8_t *type) {
  static bool is_cached = false;
  static uint8_t cached_id = 0;
  int ret = 0;

  ret = fby3_common_get_bmc_location(bmc_location);
  if ( ret < 0 ) {
    syslog(LOG_WARNING, "%s() Cannot get the location of BMC", __func__);
  }

  if ( (BB_BMC == *bmc_location) || (DVT_BB_BMC == *bmc_location) ) {
    if ( is_cached == false ) {
      const char *shadows[] = {
        "DUAL_FAN0_DETECT_BMC_N_R",
        "DUAL_FAN1_DETECT_BMC_N_R",
      };

      if ( get_gpio_shadow_array(shadows, ARRAY_SIZE(shadows), &cached_id) ) {
        return PAL_ENOTSUP;
      }

      is_cached = true;
    }

    *type = cached_id;
  } else {
    *type = DUAL_TYPE;
  }

  return PAL_EOK;
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
  FILE* fp;
  char label[32] = {0};
  uint8_t pwm_num = fan;
  uint8_t bmc_location = 0;
  uint8_t status;
  int ret = 0;
  char cmd[64] = {0};
  char buf[32];
  int res;
  bool is_fscd_run = true;

  ret = fby3_common_get_bmc_location(&bmc_location);
  if (ret < 0) {
    syslog(LOG_ERR, "%s() Cannot get the location of BMC", __func__);
    return ret;
  }

  if ( (bmc_location == BB_BMC) || (bmc_location == DVT_BB_BMC) ) {
    if (pwm_num > pal_pwm_cnt ||
      snprintf(label, sizeof(label), "pwm%d", pwm_num + 1) > sizeof(label)) {
      return -1;
    }
    return sensors_write_fan(label, (float)pwm);
  } else if (bmc_location == NIC_BMC) {
    ret = bic_set_fan_auto_mode(GET_FAN_MODE, &status);
    if (ret < 0) {
      return -1;
    }
    snprintf(cmd, sizeof(cmd), "sv status fscd | grep run | wc -l");
    if((fp = popen(cmd, "r")) == NULL) {
      is_fscd_run = false;
    }

    if(fgets(buf, sizeof(buf), fp) != NULL) {
      res = atoi(buf);
      if(res == 0) {
        is_fscd_run = false;
      }
    }
    pclose(fp);

    if ( (status == MANUAL_MODE) && (!is_fscd_run) ) {
      return bic_manual_set_fan_speed(fan, pwm);
    } else {
      return bic_set_fan_speed(fan, pwm);
    }
  }

  return -1;
}

int pal_get_fan_speed(uint8_t fan, int *rpm)
{
  char label[32] = {0};
  float value;
  int ret;
  uint8_t bmc_location = 0;
  uint8_t fan_type = UNKNOWN_TYPE;

  ret = pal_get_fan_type(&bmc_location, &fan_type);
  if ( ret < 0 ) {
    syslog(LOG_ERR, "%s() Cannot get the type of fan", __func__);
    fan_type = UNKNOWN_TYPE;
  }

  if ( (bmc_location == BB_BMC) || (bmc_location == DVT_BB_BMC) ) {
    if ( fan_type == SINGLE_TYPE ) fan *= 2;

    if (fan > pal_tach_cnt ||
        snprintf(label, sizeof(label), "fan%d", fan + 1) > sizeof(label)) {
      syslog(LOG_WARNING, "%s: invalid fan#:%d", __func__, fan);
      return -1;
    }
    ret = sensors_read_fan(label, &value);
  } else if (bmc_location == NIC_BMC) {
    ret = bic_get_fan_speed(fan, &value);
  }

  if (ret == 0) {
    *rpm = (int)value;
  }

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
  uint8_t fan_type = UNKNOWN_TYPE;
  uint8_t bmc_location = 0;

  ret = pal_get_fan_type(&bmc_location, &fan_type);
  if ( ret < 0 ) {
    syslog(LOG_ERR, "%s() Cannot get the type of fan, fvaule=%d", __func__, fan_type);
    fan_type = UNKNOWN_TYPE;
  }

  if ( fan_type == SINGLE_TYPE ) fan *= 2;

  fan_src = pal_get_fan_source(fan);
  if (fan >= pal_tach_cnt || fan_src == 0xff ) {
    syslog(LOG_WARNING, "%s: invalid fan#:%d", __func__, fan);
    return -1;
  }

  if ( (bmc_location == BB_BMC) || (bmc_location == DVT_BB_BMC) ) {
    snprintf(label, sizeof(label), "pwm%d", fan_src + 1);
    ret = sensors_read_fan(label, &value);
  } else if (bmc_location == NIC_BMC) {
    ret = bic_get_fan_pwm(fan_src, &value);
  }

  if (ret == 0) {
    *pwm = (int)value;
  }

  return ret;
}

char *
pal_get_tach_list(void) {
  uint8_t fan_type = UNKNOWN_TYPE;
  uint8_t bmc_location = 0;
  int ret = 0;

  ret = pal_get_fan_type(&bmc_location, &fan_type);
  if ( ret < 0 ) {
    syslog(LOG_ERR, "%s() Cannot get the type of fan, fvaule=%d", __func__, fan_type);
    fan_type = UNKNOWN_TYPE;
  }

  switch (fan_type) {
    case DUAL_TYPE:
      return "0..7";
      break;
    case SINGLE_TYPE:
      return "0..3";
      break;
    default:
      syslog(LOG_ERR, "%s() Cannot identify the type of fan", __func__);
      return "0..3";
      break;
  }

  syslog(LOG_ERR, "%s() it should not reach here...", __func__);
  return "NULL string";
}

int
pal_get_tach_cnt(void) {
  uint8_t fan_type = UNKNOWN_TYPE;
  uint8_t bmc_location = 0;
  int ret = 0;

  ret = pal_get_fan_type(&bmc_location, &fan_type);
  if ( ret < 0 ) {
    syslog(LOG_ERR, "%s() Cannot get the type of fan", __func__);
    fan_type = UNKNOWN_TYPE;
  }

  switch (fan_type) {
    case DUAL_TYPE:
      return DUAL_FAN_CNT;
      break;
    case SINGLE_TYPE:
      return SINGLE_FAN_CNT;
      break;
    default:
      syslog(LOG_ERR, "%s() Cannot identify the type of fan", __func__);
      return SINGLE_FAN_CNT;
      break;
  }

  syslog(LOG_ERR, "%s() it should not reach here...", __func__);
  return UNKNOWN_FAN_CNT;
}

static void
apply_frontIO_correction(uint8_t fru, uint8_t snr_num, float *value) {
  int i = 0;
  const uint8_t fan_num[4] = {FAN_0, FAN_1, FAN_2, FAN_3}; //Use fan# to get the PWM.
  static uint8_t pwm[4] = {0};
  static bool pwm_valid[4] = {false, false, false, false};
  static bool inited = false;
  float avg_pwm = 0;
  uint8_t cnt = 0;

  // Get PWM value
  for (i = 0; i < pal_pwm_cnt; i ++) {
    if (pal_get_pwm_value(fan_num[i], &pwm[i]) == 0 || pwm_valid[i] == true) {
      pwm_valid[i] = true;
      avg_pwm += (float)pwm[i];
      cnt++;
    }
  }

  if ( cnt > 0 ) {
    avg_pwm = avg_pwm / (float)cnt;
    if ( inited == false ) {
      inited = true;
      sensor_correction_init("/etc/sensor-frontIO-correction.json");
    }
    sensor_correction_apply(fru, snr_num, avg_pwm, value);
  }
}

static int
read_cached_val(uint8_t snr_number, float *value) {
  float temp1 = 0, temp2 = 0;
  uint8_t snr1_num = 0, snr2_num = 0;

  switch (snr_number) {
    case BMC_SENSOR_NIC_PWR:
        snr1_num = BMC_SENSOR_NIC_P12V;
        snr2_num = BMC_SENSOR_NIC_IOUT;
      break;
    case BMC_SENSOR_PDB_VDELTA:
        snr1_num = BMC_SENSOR_MEDUSA_VOUT;
        snr2_num = BMC_SENSOR_HSC_VIN;
      break;
    case BMC_SENSOR_MEDUSA_VDELTA:
        snr1_num = BMC_SENSOR_MEDUSA_VIN;
        snr2_num = BMC_SENSOR_MEDUSA_VOUT;
      break;
    default:
      return READING_NA;
      break;
  }

  if ( sensor_cache_read(FRU_BMC, snr1_num, &temp1) < 0) return READING_NA;
  if ( sensor_cache_read(FRU_BMC, snr2_num, &temp2) < 0) return READING_NA;

  switch (snr_number) {
    case BMC_SENSOR_NIC_PWR:
      *value = temp1 * temp2;
      break;
    case BMC_SENSOR_PDB_VDELTA:
    case BMC_SENSOR_MEDUSA_VDELTA:
      *value = temp1 - temp2;
      break;
  }

  return PAL_EOK;
}

static int
read_medusa_val(uint8_t snr_number, float *value) {
  static bool is_cached = false;
  static bool is_ltc4282 = true;
  static char chip[20] = {0};
  int ret = READING_NA;

  if ( is_cached == false) {
    if ( kv_get("bb_hsc_conf", chip, NULL, KV_FPERSIST) < 0 ) {
      syslog(LOG_WARNING, "%s() Failed to read bb_hsc_conf", __func__);
      return ret;
    }

    is_cached = true;
    strcat(chip, "-i2c-11-44");
    //MP5920 is 12-bit ADC. Use the flag to do the calibration of sensors of mp5920.
    //Make the readings more reliable
    if( strstr(chip, "mp5920")  != NULL ) is_ltc4282 = false;
    syslog(LOG_WARNING, "%s() Use '%s', flag:%d", __func__, chip, is_ltc4282);
  }

  switch(snr_number) {
    case BMC_SENSOR_MEDUSA_VIN:
      ret = sensors_read(chip, "BMC_SENSOR_MEDUSA_VIN", value);
      if ( is_ltc4282 == false ) *value *= 0.99;
      break;
    case BMC_SENSOR_MEDUSA_VOUT:
      ret = sensors_read(chip, "BMC_SENSOR_MEDUSA_VOUT", value);
      if ( is_ltc4282 == false ) *value *= 0.99;
      break;
    case BMC_SENSOR_MEDUSA_CURR:
      ret = sensors_read(chip, "BMC_SENSOR_MEDUSA_CURR", value);
      if ( is_ltc4282 == false ) {
        //The current in light load cannot be sensed. The value should be 0.
        if ( (int)(*value) != 0 ) {
          *value = (*value + 3) * 0.97;
        }
      }
      break;
    case BMC_SENSOR_MEDUSA_PWR:
      ret = sensors_read(chip, "BMC_SENSOR_MEDUSA_PWR", value);
      if ( is_ltc4282 == false ) {
        //The current is 0 amps when the system is in light load.
        //So, the power is 0, too.
        if ( (int)(*value) != 0 ) {
          *value = (*value * 0.98) + 25;
        }
      }
      break;
  }

  return ret;
}

static int
read_temp(uint8_t id, float *value) {
  struct {
    const char *chip;
    const char *label;
  } devs[] = {
    {"lm75-i2c-12-4e",  "BMC_INLET_TEMP"},
    {"lm75-i2c-12-4f",  "BMC_OUTLET_TEMP"},
    {"tmp421-i2c-8-1f", "NIC_SENSOR_TEMP"},
    {"lm75-i2c-2-4f",  "BMC_OUTLET_TEMP"},
  };
  if (id >= ARRAY_SIZE(devs)) {
    return -1;
  }

  return sensors_read(devs[id].chip, devs[id].label, value);
}

static int
read_adc_val(uint8_t adc_id, float *value) {
  int ret = PAL_EOK;
  int i = 0;
  int available_sampling = 0;
  float temp_val = 0;
  const int sampling = 100;
  const char *adc_label[] = {
    "BMC_SENSOR_P5V",
    "BMC_SENSOR_P12V",
    "BMC_SENSOR_P3V3_STBY",
    "BMC_SENSOR_P1V15_STBY",
    "BMC_SENSOR_P1V2_STBY",
    "BMC_SENSOR_P2V5_STBY",
    "Dummy sensor",
    "Dummy sensor",
    "BMC_SENSOR_FAN_IOUT",
    "BMC_SENSOR_NIC_IOUT",
    "BMC_SENSOR_NIC_P12V",
  };

  if (adc_id >= ARRAY_SIZE(adc_label)) {
    return -1;
  }

  if ( ADC8 == adc_id ) {
    *value = 0;
    for ( i = 0; i < sampling; i++ ) {
      ret = sensors_read_adc(adc_label[adc_id], &temp_val);
      if ( ret < 0 ) {
        syslog(LOG_WARNING,"%s() Failed to get val. i=%d", __func__, i);
      } else {
        *value += temp_val;
        available_sampling++;
      }
    }

    if ( available_sampling == 0 ) {
      ret = READING_NA;
    } else {
      *value = *value / available_sampling;
    }
  } else {
    ret = sensors_read_adc(adc_label[adc_id], value);
  }

  //TODO: if devices are not installed, maybe we need to show NA instead of 0.01
  if ( ret == PAL_EOK ) {
    if ( ADC8 == adc_id ) {
      *value = *value/0.22/0.237; // EVT: /0.22/0.237/4
      //when pwm is kept low, the current is very small or close to 0
      //BMC will show 0.00 amps. make it show 0.01 at least.
      if ( *value < 0.01 ) *value = 0.01;
    } else if ( ADC9 == adc_id ) {
      *value = *value/0.16/1.27;  // EVT: /0.16/0.649

      //adjust the reading to make it more accurate
      *value = (*value - 0.5) * 0.98;

      //it's not support to show the negative value, make it show 0.01 at least.
      if ( *value <= 0 ) *value = 0.01;
    }
  }

  return ret;
}

static void
get_hsc_info(uint8_t hsc_id, uint8_t type, uint8_t *addr, float* m, float* b, float* r) {
  *addr = hsc_info_list[hsc_id].slv_addr;
  *m = hsc_info_list[hsc_id].info[type].m;
  *b = hsc_info_list[hsc_id].info[type].b;
  *r = hsc_info_list[hsc_id].info[type].r;

  return;
}

static int
read_hsc_pin(uint8_t hsc_id, float *value) {
  uint8_t tbuf[1] = {0x00};
  uint8_t rbuf[2] = {0x00};
  uint8_t tlen = 0;
  uint8_t rlen = 0;
  uint8_t addr = 0;
  float m = 0, b = 0, r = 0;
  int retry = MAX_RETRY;
  int ret = ERR_NOT_READY;
  int fd = 0;

  fd = open("/dev/i2c-11", O_RDWR);
  if (fd < 0) {
    syslog(LOG_WARNING, "Failed to open bus 11");
    goto error_exit;
  }

  get_hsc_info(hsc_id, HSC_POWER, &addr, &m, &b, &r);

  tbuf[0] = PMBUS_READ_PIN;
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
read_hsc_iout(uint8_t hsc_id, float *value) {
  uint8_t tbuf[1] = {0x00};
  uint8_t rbuf[2] = {0x00};
  uint8_t tlen = 0;
  uint8_t rlen = 0;
  uint8_t addr = 0;
  float m = 0, b = 0, r = 0;
  int retry = MAX_RETRY;
  int ret = ERR_NOT_READY;
  int fd = 0;

  fd = open("/dev/i2c-11", O_RDWR);
  if (fd < 0) {
    syslog(LOG_WARNING, "Failed to open bus 11");
    goto error_exit;
  }

  get_hsc_info(hsc_id, HSC_CURRENT, &addr, &m, &b, &r);

  tbuf[0] = PMBUS_READ_IOUT;
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
  //improve the accuracy of IOUT to +-2%
  *value = *value * 0.98;
error_exit:
  if ( fd > 0 ) close(fd);

  return ret;
}

static int
read_hsc_temp(uint8_t hsc_id, float *value) {
  uint8_t tbuf[1] = {0x00};
  uint8_t rbuf[2] = {0x00};
  uint8_t tlen = 0;
  uint8_t rlen = 0;
  uint8_t addr = 0;
  float m = 0, b = 0, r = 0;
  int retry = MAX_RETRY;
  int ret = ERR_NOT_READY;
  int fd = 0;

  fd = open("/dev/i2c-11", O_RDWR);
  if (fd < 0) {
    syslog(LOG_WARNING, "Failed to open bus 11");
    goto error_exit;
  }

  get_hsc_info(hsc_id, HSC_TEMP, &addr, &m, &b, &r);

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
read_hsc_vin(uint8_t hsc_id, float *value) {
  uint8_t tbuf[1] = {0x00};
  uint8_t rbuf[2] = {0x00};
  uint8_t tlen = 0;
  uint8_t rlen = 0;
  uint8_t addr = 0;
  float m = 0, b = 0, r = 0;
  int retry = MAX_RETRY;
  int ret = ERR_NOT_READY;
  int fd = 0;

  fd = open("/dev/i2c-11", O_RDWR);
  if (fd < 0) {
    syslog(LOG_WARNING, "Failed to open bus 11");
    goto error_exit;
  }

  get_hsc_info(hsc_id, HSC_VOLTAGE, &addr, &m, &b, &r);

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
skip_bic_sensor_list(uint8_t fru, uint8_t sensor_num) {
  uint8_t *bic_skip_list;
  int skip_sensor_cnt;
  int i = 0;
  
  get_skip_sensor_list(fru, &bic_skip_list, &skip_sensor_cnt);

  switch(fru){
    case FRU_SLOT1:
    case FRU_SLOT2:
    case FRU_SLOT3:
    case FRU_SLOT4:
      
      for (i = 0; i < skip_sensor_cnt; i++) {
        if ( sensor_num == bic_skip_list[i] ) {
          return PAL_ENOTSUP;
        }
      }
      break;
  }

  return PAL_EOK;
}

static int
pal_bic_sensor_read_raw(uint8_t fru, uint8_t sensor_num, float *value){
#define BIC_SENSOR_READ_NA 0x20
#define SLOT_SENSOR_LOCK "/var/run/slot%d_sensor.lock"
  int ret = 0;
  uint8_t power_status = 0, config_status = 0;
  ipmi_sensor_reading_t sensor = {0};
  sdr_full_t *sdr = NULL;
  uint8_t bmc_location = 0;
  char path[128];
  sprintf(path, SLOT_SENSOR_LOCK, fru);

  if ( bic_get_server_power_status(fru, &power_status) < 0 || power_status != SERVER_POWER_ON) {
    //syslog(LOG_WARNING, "%s() Failed to run bic_get_server_power_status(). fru%d, snr#0x%x, pwr_sts:%d", __func__, fru, sensor_num, power_status);
    if (skip_bic_sensor_list(fru, sensor_num) < 0) {
      return READING_NA;
    }
  }

  ret = bic_is_m2_exp_prsnt(fru);

  if (ret < 0) {
    return READING_NA;
  }
  config_status = (uint8_t) ret;

  ret = fby3_common_get_bmc_location(&bmc_location);
  if (ret < 0) {
    syslog(LOG_ERR, "%s() Cannot get the location of BMC", __func__);
  }
  
  ret = access(path, F_OK);
  if(ret == 0) {
    return READING_SKIP;
  }

  if ( (bmc_location == BB_BMC) || (bmc_location == DVT_BB_BMC) ) {
    if ( (config_status == PRESENT_1OU || config_status == (PRESENT_1OU + PRESENT_2OU)) && (sensor_num >= 0x50 && sensor_num <= 0x7F)) { // 1OU Exp
      ret = bic_get_sensor_reading(fru, sensor_num, &sensor, FEXP_BIC_INTF);
    } else if ( (config_status == PRESENT_2OU || config_status == (PRESENT_1OU + PRESENT_2OU)) && (sensor_num >= 0x80 && sensor_num <= 0xCA)) { // 2OU Exp
      ret = bic_get_sensor_reading(fru, sensor_num, &sensor, REXP_BIC_INTF);
    } else if (sensor_num >= 0x0 && sensor_num <= 0x42) {
      ret = bic_get_sensor_reading(fru, sensor_num, &sensor, NONE_INTF);
    } else {
      return READING_NA;
    }
  } else if (bmc_location == NIC_BMC) {
    if (sensor_num >= 0xD1 && sensor_num <= 0xEC) { // BB
      ret = bic_get_sensor_reading(fru, sensor_num, &sensor, BB_BIC_INTF);
    } else if ( (config_status == PRESENT_2OU || config_status == (PRESENT_1OU + PRESENT_2OU)) && (sensor_num >= 0x50 && sensor_num <= 0xCA)) { // 2OU Exp
      ret = bic_get_sensor_reading(fru, sensor_num, &sensor, REXP_BIC_INTF);
    } else if (sensor_num >= 0x0 && sensor_num <= 0x42){
      ret = bic_get_sensor_reading(fru, sensor_num, &sensor, NONE_INTF);
    } else {
      return READING_NA;
    }
  } else {
    return READING_NA;
  }
  if ( ret < 0 ) {
    syslog(LOG_WARNING, "%s() Failed to run bic_get_sensor_reading(). fru: %x, snr#0x%x", __func__, fru, sensor_num);
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

  if ((sdr->sensor_units1 & 0xC0) == 0x00) {  // unsigned
    x = sensor.value;
  } else if ((sdr->sensor_units1 & 0xC0) == 0x40) {  // 1's complements
    x = (sensor.value & 0x80) ? (0-(~sensor.value)) : sensor.value;
  } else if ((sdr->sensor_units1 & 0xC0) == 0x80) {  // 2's complements
    x = (int8_t)sensor.value;
  } else { // Does not return reading
    return READING_NA;
  }

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

  //syslog(LOG_WARNING, "%s() snr#0x%x raw:%x m=%x b=%x b_exp=%x r_exp=%x s_units1=%x", __func__, sensor_num, x, m, b, b_exp, r_exp, sdr->sensor_units1);
  *value = ((m * x) + (b * pow(10, b_exp))) * (pow(10, r_exp));

  //correct the value
  switch (sensor_num) {
    case BIC_SENSOR_FIO_TEMP:
      apply_frontIO_correction(fru, sensor_num, value);
      break;
    case BIC_SENSOR_CPU_THERM_MARGIN:
      if ( *value > 0 ) *value = -(*value);
      break;
  }

  return ret;
}

int
pal_sensor_read_raw(uint8_t fru, uint8_t sensor_num, void *value) {
  char key[MAX_KEY_LEN] = {0};
  char str[MAX_VALUE_LEN] = {0};
  char fru_name[32];
  int ret=0;
  uint8_t id=0;
  static uint8_t bmc_location = 0;

  if ( bmc_location == 0 ) {
    if ( fby3_common_get_bmc_location(&bmc_location) < 0 ) {
      syslog(LOG_WARNING, "Failed to get the location of BMC");
    } else {
      if ( bmc_location == NIC_BMC ) {
        sensor_map[BMC_SENSOR_OUTLET_TEMP].id = TEMP_NICEXP_OUTLET;
      }
    }
  }

  pal_get_fru_name(fru, fru_name);
  sprintf(key, "%s_sensor%d", fru_name, sensor_num);
  id = sensor_map[sensor_num].id;

  switch(fru) {
    case FRU_SLOT1:
    case FRU_SLOT2:
    case FRU_SLOT3:
    case FRU_SLOT4:
      if (pal_is_fw_update_ongoing(fru)) {
        return READING_SKIP;
      } else {
        ret = pal_bic_sensor_read_raw(fru, sensor_num, (float*)value);
      }
      break;
    case FRU_BMC:
    case FRU_NIC:
      ret = sensor_map[sensor_num].read_sensor(id, (float*) value);
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
    case FRU_NIC:
      sprintf(name, "%s", sensor_map[sensor_num].snr_name);
      break;
    default:
      return -1;
  }
  return 0;
}

int
pal_get_sensor_threshold(uint8_t fru, uint8_t sensor_num, uint8_t thresh, void *value) {
  float *val = (float*) value;
  switch (fru) {
    case FRU_BMC:
    case FRU_NIC:
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
          syslog(LOG_WARNING, "Threshold type error value=%d\n", thresh);
          return -1;
      }
      break;
    default:
      return -1;
      break;
  }
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
  int retry = MAX_RETRY;
  uint8_t bmc_location = 0;

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
    // If Class 2, threshold change
    if (snr_num == BIC_SENSOR_HSC_OUTPUT_CUR) {
      fby3_common_get_bmc_location(&bmc_location);
      if (bmc_location == NIC_BMC) {
          sdr->uc_thresh = HSC_OUTPUT_CUR_UC_THRESHOLD;
      }
    } else if (snr_num == BIC_SENSOR_HSC_INPUT_PWR){
      fby3_common_get_bmc_location(&bmc_location);
      if (bmc_location == NIC_BMC) {
        sdr->uc_thresh = HSC_INPUT_PWR_UC_THRESHOLD;
      }
    }
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
  int retry = MAX_RETRY;
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
  uint8_t scale = sensor_map[sensor_num].units;

  if (fru == FRU_SLOT1 || fru == FRU_SLOT2 || \
      fru == FRU_SLOT3 || fru == FRU_SLOT4) {
    ret = pal_sdr_init(fru);
    strcpy(units, "");
    return ret;
  }

  switch(scale) {
    case TEMP:
      sprintf(units, "C");
      break;
    case FAN:
      sprintf(units, "RPM");
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
      syslog(LOG_WARNING, "%s() unknown sensor number %x", __func__, sensor_num);
    break;
  }
  return 0;
}
