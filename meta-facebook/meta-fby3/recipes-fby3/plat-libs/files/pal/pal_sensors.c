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
#define MAX_SDR_LEN 64
#define SDR_PATH "/tmp/sdr_%s.bin"

#define PWM_PLAT_SET 0x80
#define PWM_MASK     0x0f

#define DEVICE_KEY "sys_config/fru%d_B_drive0_model_name"
#define DP_INNER_PCIE_INFO_KEY "sys_config/fru%u_pcie_i04_s40_info"
#define DP_OUTER_PCIE_INFO_KEY "sys_config/fru%u_pcie_i04_s44_info"

#define DUAL_FAN_UCR  13500

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

enum {
  // Vendor ID
  PCIE_VENDOR_MARVELL_HSM = 0x8086,
  PCIE_VENDOR_NCIPHER_HSM = 0x1957,

  // Device ID
  PCIE_DEVICE_MARVELL_HSM = 0x10d8,
  PCIE_DEVICE_NCIPHER_HSM = 0x082c,
};

struct pcie_info {
  uint8_t   interface;
  uint8_t   slot_num;
  uint16_t  vendor_id;
  uint16_t  device_id;
  uint32_t  sub_vendor_id;
  uint32_t  sub_device_id;
  uint8_t   dev_type;
  uint8_t   rated_width;
  uint8_t   rated_speed;
}__attribute__ ((__packed__));

static int read_adc_val(uint8_t adc_id, float *value);
static int read_temp(uint8_t snr_id, float *value);
static int read_hsc_vin(uint8_t hsc_id, float *value);
static int read_hsc_temp(uint8_t hsc_id, float *value);
static int read_hsc_pin(uint8_t hsc_id, float *value);
static int read_hsc_ein(uint8_t hsc_id, float *value);
static int read_hsc_iout(uint8_t hsc_id, float *value);
static int read_hsc_peak_iout(uint8_t hsc_id, float *value);
static int read_hsc_peak_pin(uint8_t hsc_id, float *value);
static int read_medusa_val(uint8_t snr_number, float *value);
static int read_cached_val(uint8_t snr_number, float *value);
static int read_fan_speed(uint8_t snr_number, float *value);
static int read_fan_pwm(uint8_t pwm_id, float *value);
static int read_curr_leakage(uint8_t snr_number, float *value);
static int read_pdb_dl_vdelta(uint8_t snr_number, float *value);

static int pal_sdr_init(uint8_t fru);
static sensor_info_t g_sinfo[MAX_NUM_FRUS][MAX_SENSOR_NUM] = {0};
static bool sdr_init_done[MAX_NUM_FRUS] = {false};
static uint8_t bic_dynamic_sensor_list[4][MAX_SENSOR_NUM] = {0};
static uint8_t bic_dynamic_skip_sensor_list[4][MAX_SENSOR_NUM] = {0};

int pwr_off_flag[MAX_NODES] = {0};
int temp_cnt = 0;
size_t pal_pwm_cnt = 4;
size_t pal_tach_cnt = 8;
const char pal_pwm_list[] = "0, 1, 2, 3";
const char pal_fan_opt_list[] = "enable, disable, status";

const uint8_t bmc_sensor_list[] = {
  BMC_SENSOR_FAN0_TACH,
  BMC_SENSOR_FAN1_TACH,
  BMC_SENSOR_FAN2_TACH,
  BMC_SENSOR_FAN3_TACH,
  BMC_SENSOR_FAN4_TACH,
  BMC_SENSOR_FAN5_TACH,
  BMC_SENSOR_FAN6_TACH,
  BMC_SENSOR_FAN7_TACH,
  BMC_SENSOR_PWM0,
  BMC_SENSOR_PWM1,
  BMC_SENSOR_PWM2,
  BMC_SENSOR_PWM3,
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
  BMC_SENSOR_HSC_EIN,
  BMC_SENSOR_HSC_IOUT,
  BMC_SENSOR_HSC_PEAK_IOUT,
  BMC_SENSOR_HSC_PEAK_PIN,
  BMC_SENSOR_MEDUSA_VOUT,
  BMC_SENSOR_MEDUSA_VIN,
  BMC_SENSOR_MEDUSA_CURR,
  BMC_SENSOR_MEDUSA_PWR,
  BMC_SENSOR_MEDUSA_VDELTA,
  BMC_SENSOR_PDB_DL_VDELTA,
  BMC_SENSOR_PDB_BB_VDELTA,
  BMC_SENSOR_CURR_LEAKAGE,
  BMC_SENSOR_FAN_IOUT,
  BMC_SENSOR_FAN_PWR,
  BMC_SENSOR_NIC_P12V,
  BMC_SENSOR_NIC_IOUT,
  BMC_SENSOR_NIC_PWR,
};

const uint8_t nicexp_sensor_list[] = {
  BMC_SENSOR_FAN0_TACH,
  BMC_SENSOR_FAN1_TACH,
  BMC_SENSOR_FAN2_TACH,
  BMC_SENSOR_FAN3_TACH,
  BMC_SENSOR_FAN4_TACH,
  BMC_SENSOR_FAN5_TACH,
  BMC_SENSOR_FAN6_TACH,
  BMC_SENSOR_FAN7_TACH,
  BMC_SENSOR_PWM0,
  BMC_SENSOR_PWM1,
  BMC_SENSOR_PWM2,
  BMC_SENSOR_PWM3,
  BMC_SENSOR_OUTLET_TEMP,
  BMC_SENSOR_P12V,
  BMC_SENSOR_P3V3_STBY,
  BMC_SENSOR_P1V15_BMC_STBY,
  BMC_SENSOR_P1V2_BMC_STBY,
  BMC_SENSOR_P2V5_BMC_STBY,
  BMC_SENSOR_NIC_P12V,
  BMC_SENSOR_NIC_IOUT,
  BMC_SENSOR_NIC_PWR,
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
  BIC_SENSOR_SOC_PKG_PWR,
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
  BIC_SENSOR_HSC_INPUT_AVGPWR,
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

const uint8_t bic_2ou_gpv3_sensor_list[] = {
  // temperature
  BIC_GPV3_INLET_TEMP,
  BIC_GPV3_PCIE_SW_TEMP,

  // adc voltage
  BIC_GPV3_ADC_P12V_STBY_VOL,
  BIC_GPV3_ADC_P3V3_STBY_AUX_VOL,
  BIC_GPV3_ADC_P1V8_VOL,

  // P3V3 STBY1~3 
  BIC_GPV3_P3V3_STBY1_POWER,
  BIC_GPV3_P3V3_STBY1_VOLTAGE,
  BIC_GPV3_P3V3_STBY1_CURRENT,
  BIC_GPV3_P3V3_STBY1_TEMP,
  BIC_GPV3_P3V3_STBY2_POWER,
  BIC_GPV3_P3V3_STBY2_VOLTAGE,
  BIC_GPV3_P3V3_STBY2_CURRENT,
  BIC_GPV3_P3V3_STBY2_TEMP,
  BIC_GPV3_P3V3_STBY3_POWER,
  BIC_GPV3_P3V3_STBY3_VOLTAGE,
  BIC_GPV3_P3V3_STBY3_CURRENT,
  BIC_GPV3_P3V3_STBY3_TEMP,

  //VR P0V84
  BIC_GPV3_VR_P0V84_POWER,
  BIC_GPV3_VR_P0V84_VOLTAGE,
  BIC_GPV3_VR_P0V84_CURRENT,
  BIC_GPV3_VR_P0V84_TEMP,

  //VR P1V8
  BIC_GPV3_VR_P1V8_POWER,
  BIC_GPV3_VR_P1V8_VOLTAGE,
  BIC_GPV3_VR_P1V8_CURRENT,
  BIC_GPV3_VR_P1V8_TEMP,

  //PESW PWR = P0V84 + P1V8
  BIC_GPV3_PESW_PWR,

  //E1S
  BIC_GPV3_E1S_1_12V_POWER,
  BIC_GPV3_E1S_1_12V_VOLTAGE,
  BIC_GPV3_E1S_1_12V_CURRENT,
  BIC_GPV3_E1S_1_TEMP,
  BIC_GPV3_E1S_2_12V_POWER,
  BIC_GPV3_E1S_2_12V_VOLTAGE,
  BIC_GPV3_E1S_2_12V_CURRENT,
  BIC_GPV3_E1S_2_TEMP,

  //INA233 DEV0~11
  BIC_GPV3_INA233_PWR_DEV0,
  BIC_GPV3_INA233_VOL_DEV0,
  BIC_GPV3_NVME_TEMP_DEV0,

  BIC_GPV3_INA233_PWR_DEV1,
  BIC_GPV3_INA233_VOL_DEV1,
  BIC_GPV3_NVME_TEMP_DEV1,

  BIC_GPV3_INA233_PWR_DEV2,
  BIC_GPV3_INA233_VOL_DEV2,
  BIC_GPV3_NVME_TEMP_DEV2,

  BIC_GPV3_INA233_PWR_DEV3,
  BIC_GPV3_INA233_VOL_DEV3,
  BIC_GPV3_NVME_TEMP_DEV3,

  BIC_GPV3_INA233_PWR_DEV4,
  BIC_GPV3_INA233_VOL_DEV4,
  BIC_GPV3_NVME_TEMP_DEV4,

  BIC_GPV3_INA233_PWR_DEV5,
  BIC_GPV3_INA233_VOL_DEV5,
  BIC_GPV3_NVME_TEMP_DEV5,

  BIC_GPV3_INA233_PWR_DEV6,
  BIC_GPV3_INA233_VOL_DEV6,
  BIC_GPV3_NVME_TEMP_DEV6,

  BIC_GPV3_INA233_PWR_DEV7,
  BIC_GPV3_INA233_VOL_DEV7,
  BIC_GPV3_NVME_TEMP_DEV7,

  BIC_GPV3_INA233_PWR_DEV8,
  BIC_GPV3_INA233_VOL_DEV8,
  BIC_GPV3_NVME_TEMP_DEV8,

  BIC_GPV3_INA233_PWR_DEV9,
  BIC_GPV3_INA233_VOL_DEV9,
  BIC_GPV3_NVME_TEMP_DEV9,

  BIC_GPV3_INA233_PWR_DEV10,
  BIC_GPV3_INA233_VOL_DEV10,
  BIC_GPV3_NVME_TEMP_DEV10,
  
  BIC_GPV3_INA233_PWR_DEV11,
  BIC_GPV3_INA233_VOL_DEV11,
  BIC_GPV3_NVME_TEMP_DEV11,
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

//BIC Sierra Point Expansion Board Sensors
const uint8_t bic_spe_sensor_list[] = {
  BIC_SPE_SENSOR_SSD0_TEMP,
  BIC_SPE_SENSOR_SSD1_TEMP,
  BIC_SPE_SENSOR_SSD2_TEMP,
  BIC_SPE_SENSOR_SSD3_TEMP,
  BIC_SPE_SENSOR_SSD4_TEMP,
  BIC_SPE_SENSOR_SSD5_TEMP,
  BIC_SPE_SENSOR_INLET_TEMP,
  BIC_SPE_SENSOR_12V_EDGE_VOL,
  BIC_SPE_SENSOR_12V_MAIN_VOL,
  BIC_SPE_SENSOR_3V3_EDGE_VOL,
  BIC_SPE_SENSOR_3V3_MAIN_VOL,
  BIC_SPE_SENSOR_3V3_STBY_VOL,
  BIC_SPE_SENSOR_SSD0_CUR,
  BIC_SPE_SENSOR_SSD1_CUR,
  BIC_SPE_SENSOR_SSD2_CUR,
  BIC_SPE_SENSOR_SSD3_CUR,
  BIC_SPE_SENSOR_SSD4_CUR,
  BIC_SPE_SENSOR_SSD5_CUR,
  BIC_SPE_SENSOR_12V_MAIN_CUR,
};

const uint8_t bic_skip_sensor_list[] = {
  BIC_SENSOR_PCH_TEMP,
  BIC_SENSOR_CPU_TEMP,
  BIC_SENSOR_DIMMA0_TEMP,
  BIC_SENSOR_DIMMB0_TEMP,
  BIC_SENSOR_DIMMC0_TEMP,
  BIC_SENSOR_DIMMD0_TEMP,
  BIC_SENSOR_DIMME0_TEMP,
  BIC_SENSOR_DIMMF0_TEMP,
  BIC_SENSOR_M2B_TEMP,
  BIC_SENSOR_VCCIN_VR_TEMP,
  BIC_SENSOR_VCCSA_VR_TEMP,
  BIC_SENSOR_VCCIO_VR_Temp,
  BIC_SENSOR_PVDDQ_ABC_VR_TEMP,
  BIC_SENSOR_PVDDQ_DEF_VR_TEMP,
  //BIC - voltage sensors
  BIC_SENSOR_VCCIN_VR_VOL,
  BIC_SENSOR_VCCSA_VR_VOL,
  BIC_SENSOR_VCCIO_VR_VOL,
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

const uint8_t bic_2ou_gpv3_skip_sensor_list[] = {
  //BIC 2OU GPv3 Sensors
  BIC_GPV3_PCIE_SW_TEMP, // 0x92

  // adc voltage
  BIC_GPV3_ADC_P1V8_VOL, //0x9F

  //VR P0V84
  BIC_GPV3_VR_P0V84_POWER,
  BIC_GPV3_VR_P0V84_VOLTAGE,
  BIC_GPV3_VR_P0V84_CURRENT,
  BIC_GPV3_VR_P0V84_TEMP,

  //VR P1V8
  BIC_GPV3_VR_P1V8_POWER,
  BIC_GPV3_VR_P1V8_VOLTAGE,
  BIC_GPV3_VR_P1V8_CURRENT,
  BIC_GPV3_VR_P1V8_TEMP,

  //E1S
  BIC_GPV3_E1S_1_12V_POWER,
  BIC_GPV3_E1S_1_12V_VOLTAGE,
  BIC_GPV3_E1S_1_12V_CURRENT,
  BIC_GPV3_E1S_1_TEMP,
  BIC_GPV3_E1S_2_12V_POWER,
  BIC_GPV3_E1S_2_12V_VOLTAGE,
  BIC_GPV3_E1S_2_12V_CURRENT,
  BIC_GPV3_E1S_2_TEMP,

  //M.2 devices
  BIC_GPV3_INA233_PWR_DEV0,
  BIC_GPV3_INA233_VOL_DEV0,
  BIC_GPV3_NVME_TEMP_DEV0,

  BIC_GPV3_INA233_PWR_DEV1,
  BIC_GPV3_INA233_VOL_DEV1,
  BIC_GPV3_NVME_TEMP_DEV1,

  BIC_GPV3_INA233_PWR_DEV2,
  BIC_GPV3_INA233_VOL_DEV2,
  BIC_GPV3_NVME_TEMP_DEV2,

  BIC_GPV3_INA233_PWR_DEV3,
  BIC_GPV3_INA233_VOL_DEV3,
  BIC_GPV3_NVME_TEMP_DEV3,

  BIC_GPV3_INA233_PWR_DEV4,
  BIC_GPV3_INA233_VOL_DEV4,
  BIC_GPV3_NVME_TEMP_DEV4,

  BIC_GPV3_INA233_PWR_DEV5,
  BIC_GPV3_INA233_VOL_DEV5,
  BIC_GPV3_NVME_TEMP_DEV5,

  BIC_GPV3_INA233_PWR_DEV6,
  BIC_GPV3_INA233_VOL_DEV6,
  BIC_GPV3_NVME_TEMP_DEV6,

  BIC_GPV3_INA233_PWR_DEV7,
  BIC_GPV3_INA233_VOL_DEV7,
  BIC_GPV3_NVME_TEMP_DEV7,

  BIC_GPV3_INA233_PWR_DEV8,
  BIC_GPV3_INA233_VOL_DEV8,
  BIC_GPV3_NVME_TEMP_DEV8,

  BIC_GPV3_INA233_PWR_DEV9,
  BIC_GPV3_INA233_VOL_DEV9,
  BIC_GPV3_NVME_TEMP_DEV9,

  BIC_GPV3_INA233_PWR_DEV10,
  BIC_GPV3_INA233_VOL_DEV10,
  BIC_GPV3_NVME_TEMP_DEV10,

  BIC_GPV3_INA233_PWR_DEV11,
  BIC_GPV3_INA233_VOL_DEV11,
  BIC_GPV3_NVME_TEMP_DEV11,
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

const uint8_t bic_dp_sensor_list[] = {
  BIC_SENSOR_DP_PCIE_TEMP,
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

//{SensorName, ID, FUNCTION, PWR_STATUS, {UCR, UNC, UNR, LCR, LNR, LNC, Pos, Neg}
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
  {"BMC_SENSOR_HSC_PEAK_IOUT", HSC_ID0, read_hsc_peak_iout, 0, {0, 0, 0, 0, 0, 0, 0, 0}, CURR}, //0xC8
  {"BMC_SENSOR_HSC_PEAK_PIN", HSC_ID0, read_hsc_peak_pin, 0, {0, 0, 0, 0, 0, 0, 0, 0}, POWER}, //0xC9
  {"BMC_SENSOR_FAN_PWR", 0xCA, read_cached_val, true, {0, 0, 0, 0, 0, 0, 0, 0}, POWER}, //0xCA
  {"BMC_SENSOR_HSC_EIN", HSC_ID0, read_hsc_ein, true, {362, 0, 0, 0, 0, 0, 0, 0}, POWER}, //0xCB
  {"BMC_SENSOR_PDB_DL_VDELTA", 0xCC, read_pdb_dl_vdelta, true, {0.9, 0, 0, 0, 0, 0, 0, 0}, VOLT}, //0xCC
  {"BMC_SENSOR_CURR_LEAKAGE", 0xCD, read_curr_leakage, true, {0, 0, 0, 0, 0, 0, 0, 0}, PERCENT}, //0xCD
  {"BMC_SENSOR_PDB_BB_VDELTA", 0xCE, read_cached_val, true, {0.8, 0, 0, 0, 0, 0, 0, 0}, VOLT}, //0xCE
  {"BMC_SENSOR_MEDUSA_VDELTA", 0xCF, read_medusa_val, true, {0.5, 0, 0, 0, 0, 0, 0, 0}, VOLT}, //0xCF
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

  {"BMC_SENSOR_FAN0_TACH", 0xE0, read_fan_speed, true, {11500, 8500, 0, 500, 0, 0, 0, 0}, FAN}, //0xE0
  {"BMC_SENSOR_FAN1_TACH", 0xE1, read_fan_speed, true, {11500, 8500, 0, 500, 0, 0, 0, 0}, FAN}, //0xE1
  {"BMC_SENSOR_FAN2_TACH", 0xE2, read_fan_speed, true, {11500, 8500, 0, 500, 0, 0, 0, 0}, FAN}, //0xE2
  {"BMC_SENSOR_FAN3_TACH", 0xE3, read_fan_speed, true, {11500, 8500, 0, 500, 0, 0, 0, 0}, FAN}, //0xE3
  {"BMC_SENSOR_FAN4_TACH", 0xE4, read_fan_speed, true, {11500, 8500, 0, 500, 0, 0, 0, 0}, FAN}, //0xE4
  {"BMC_SENSOR_FAN5_TACH", 0xE5, read_fan_speed, true, {11500, 8500, 0, 500, 0, 0, 0, 0}, FAN}, //0xE5
  {"BMC_SENSOR_FAN6_TACH", 0xE6, read_fan_speed, true, {11500, 8500, 0, 500, 0, 0, 0, 0}, FAN}, //0xE6
  {"BMC_SENSOR_FAN7_TACH", 0xE7, read_fan_speed, true, {11500, 8500, 0, 500, 0, 0, 0, 0}, FAN}, //0xE7
  {"BMC_SENSOR_PWM0", PWM_0, read_fan_pwm, true, {0, 0, 0, 0, 0, 0, 0, 0}, PERCENT}, //0xE8
  {"BMC_SENSOR_PWM1", PWM_1, read_fan_pwm, true, {0, 0, 0, 0, 0, 0, 0, 0}, PERCENT}, //0xE9
  {"BMC_SENSOR_PWM2", PWM_2, read_fan_pwm, true, {0, 0, 0, 0, 0, 0, 0, 0}, PERCENT}, //0xEA
  {"BMC_SENSOR_PWM3", PWM_3, read_fan_pwm, true, {0, 0, 0, 0, 0, 0, 0, 0}, PERCENT}, //0xEB
  {NULL, 0, NULL, 0, {0, 0, 0, 0, 0, 0, 0, 0}, 0}, //0xEC
  {"BMC_INLET_TEMP",  TEMP_INLET,  read_temp, true, {50, 0, 0, 0, 0, 0, 0, 0}, TEMP}, //0xED
  {"BMC_OUTLET_TEMP", TEMP_OUTLET, read_temp, true, {55, 0, 0, 0, 0, 0, 0, 0}, TEMP}, //0xEE
  {"NIC_SENSOR_TEMP", TEMP_NIC, read_temp, true, {105, 0, 120, 0, 0, 0, 0, 0}, TEMP}, //0xEF
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
size_t bic_2ou_gpv3_sensor_cnt = sizeof(bic_2ou_gpv3_sensor_list)/sizeof(uint8_t);
size_t bic_spe_sensor_cnt = sizeof(bic_spe_sensor_list)/sizeof(uint8_t);
size_t bic_skip_sensor_cnt = sizeof(bic_skip_sensor_list)/sizeof(uint8_t);
size_t bic_1ou_skip_sensor_cnt = sizeof(bic_1ou_skip_sensor_list)/sizeof(uint8_t);
size_t bic_2ou_skip_sensor_cnt = sizeof(bic_2ou_skip_sensor_list)/sizeof(uint8_t);
size_t bic_2ou_gpv3_skip_sensor_cnt = sizeof(bic_2ou_gpv3_skip_sensor_list)/sizeof(uint8_t);
size_t bic_1ou_edsff_skip_sensor_cnt = sizeof(bic_1ou_edsff_skip_sensor_list)/sizeof(uint8_t);
size_t bic_dp_sensor_cnt = sizeof(bic_dp_sensor_list)/sizeof(uint8_t);

static int compare(const void *arg1, const void *arg2) {
  return(*(int *)arg2 - *(int *)arg1);
}

int
get_skip_sensor_list(uint8_t fru, uint8_t **skip_sensor_list, int *cnt, const uint8_t bmc_location, const uint8_t config_status) {
  uint8_t type = 0;
  uint8_t type_2ou = UNKNOWN_BOARD;
  static uint8_t current_cnt = 0;

  if (bic_dynamic_skip_sensor_list[fru-1][0] == 0) {

    memcpy(bic_dynamic_skip_sensor_list[fru-1], bic_skip_sensor_list, bic_skip_sensor_cnt);
    current_cnt = bic_skip_sensor_cnt;

    //if 1OU board doesn't exist, use the default 1ou skip list.
    if ( (bmc_location != NIC_BMC) && (config_status & PRESENT_1OU) == PRESENT_1OU ) {
      bic_get_1ou_type(fru, &type);
    }

    if ((config_status & PRESENT_2OU) == PRESENT_2OU ) {
      fby3_common_get_2ou_board_type(FRU_SLOT1, &type_2ou);
    }

    if (type == EDSFF_1U) {
      memcpy(&bic_dynamic_skip_sensor_list[fru-1][current_cnt], bic_1ou_edsff_skip_sensor_list, bic_1ou_edsff_skip_sensor_cnt);
      current_cnt += bic_1ou_edsff_skip_sensor_cnt;
    } else {
      memcpy(&bic_dynamic_skip_sensor_list[fru-1][current_cnt], bic_1ou_skip_sensor_list, bic_1ou_skip_sensor_cnt);
      current_cnt += bic_1ou_skip_sensor_cnt;
    }

    if ( type_2ou == GPV3_MCHP_BOARD || type_2ou == GPV3_BRCM_BOARD ) {
      memcpy(&bic_dynamic_skip_sensor_list[fru-1][current_cnt], bic_2ou_gpv3_skip_sensor_list, bic_2ou_gpv3_skip_sensor_cnt);
      current_cnt += bic_2ou_gpv3_skip_sensor_cnt;
    } else {
      memcpy(&bic_dynamic_skip_sensor_list[fru-1][current_cnt], bic_2ou_skip_sensor_list, bic_2ou_skip_sensor_cnt);
      current_cnt += bic_2ou_skip_sensor_cnt;
    }
  }

  *skip_sensor_list = (uint8_t *) bic_dynamic_skip_sensor_list[fru-1];
  *cnt = current_cnt;

  return 0;
}

int
pal_get_fru_sensor_list(uint8_t fru, uint8_t **sensor_list, int *cnt) {
  uint8_t bmc_location = 0, type = 0;
  int ret = 0, config_status = 0;
  uint8_t board_type = 0;
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
    if (config_status < 0) config_status = 0;

    // 1OU
    if ( (bmc_location == BB_BMC || bmc_location == DVT_BB_BMC) && ((config_status & PRESENT_1OU) == PRESENT_1OU) ) {
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
    if ( (config_status & PRESENT_2OU) == PRESENT_2OU ) {
      ret = fby3_common_get_2ou_board_type(fru, &board_type);
      if (ret < 0) {
        syslog(LOG_ERR, "%s() Cannot get board_type", __func__);
      }
      if (board_type == E1S_BOARD) { // Sierra point expansion
        memcpy(&bic_dynamic_sensor_list[fru-1][current_cnt], bic_spe_sensor_list, bic_spe_sensor_cnt);
        current_cnt += bic_spe_sensor_cnt;
      } else if (board_type == GPV3_MCHP_BOARD || board_type == GPV3_BRCM_BOARD){
        memcpy(&bic_dynamic_sensor_list[fru-1][current_cnt], bic_2ou_gpv3_sensor_list, bic_2ou_gpv3_sensor_cnt);
        current_cnt += bic_2ou_gpv3_sensor_cnt;
      } else if (board_type == DP_RISER_BOARD) {
        memcpy(&bic_dynamic_sensor_list[fru-1][current_cnt], bic_dp_sensor_list, bic_dp_sensor_cnt);
        current_cnt += bic_dp_sensor_cnt;
      } else {
        memcpy(&bic_dynamic_sensor_list[fru-1][current_cnt], bic_2ou_sensor_list, bic_2ou_sensor_cnt);
        current_cnt += bic_2ou_sensor_cnt;
      }
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
pal_get_fan_type(uint8_t *bmc_location, uint8_t *type) {
  static bool is_cached = false;
  static unsigned int cached_id = 0;
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

      if ( gpio_get_value_by_shadow_list(shadows, ARRAY_SIZE(shadows), &cached_id) ) {
        return PAL_ENOTSUP;
      }

      is_cached = true;
    }

    *type = (uint8_t)cached_id;
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

// Provide the fan speed to fan-util and it also will be called by read_fan_speed
int pal_get_fan_speed(uint8_t fan, int *rpm)
{
  char label[32] = {0};
  float value = 0;
  int ret = PAL_ENOTSUP;
  uint8_t bmc_location = 0;
  uint8_t fan_type = UNKNOWN_TYPE;

  ret = pal_get_fan_type(&bmc_location, &fan_type);
  if ( ret < 0 ) {
    syslog(LOG_ERR, "%s() Cannot get the type of fan", __func__);
    fan_type = UNKNOWN_TYPE;
  }

  if ( (bmc_location == BB_BMC) || (bmc_location == DVT_BB_BMC) ) {
    //8 fans are included in bmc_sensor_list. sensord will monitor all fans anyway.
    //in order to avoid accessing invalid fans, we add a condition to filter them out.
    if ( fan_type == SINGLE_TYPE ) {
      //only supports FAN_0, FAN_1, FAN_2, and FAN_3
      if ( fan < FAN_4 ) fan *= 2;
      else return PAL_ENOTSUP;
    }

    if (fan > pal_tach_cnt ||
        snprintf(label, sizeof(label), "fan%d", fan + 1) > sizeof(label)) {
      syslog(LOG_WARNING, "%s: invalid fan#:%d", __func__, fan);
      return -1;
    }
    ret = sensors_read_fan(label, &value);
  } else if ( bmc_location == NIC_BMC ) {
    if ( pal_is_fw_update_ongoing(FRU_SLOT1) == true ) return PAL_ENOTSUP;
    else ret = bic_get_fan_speed(fan, &value);
  }

  if (ret == PAL_EOK) {
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

static int
_pal_get_pwm_value(uint8_t pwm, float *value, uint8_t bmc_location) {
  if ( bmc_location == NIC_BMC ) {
    if ( pal_is_fw_update_ongoing(FRU_SLOT1) == true ) return PAL_ENOTSUP;
    else return bic_get_fan_pwm(pwm, value);
  }

  char label[32] = {0};
  snprintf(label, sizeof(label), "pwm%d", pwm + 1);
  return sensors_read_fan(label, value);
}

// Provide the fan pwm to fan-util and it also will be called by read_fan_pwm
int pal_get_pwm_value(uint8_t fan, uint8_t *pwm) {
  float value = 0;
  int ret = 0;
  uint8_t fan_src = 0;
  uint8_t pwm_snr = 0;
  uint8_t fan_type = UNKNOWN_TYPE;
  uint8_t bmc_location = 0;

  ret = pal_get_fan_type(&bmc_location, &fan_type);
  if ( ret < 0 ) {
    syslog(LOG_ERR, "%s() Cannot get the type of fan, fvaule=%d", __func__, fan_type);
    fan_type = UNKNOWN_TYPE;
  }

  if ( fan >= pal_tach_cnt ) {
    syslog(LOG_WARNING, "%s() invalid fan#%d", __func__, fan);
    return PAL_ENOTSUP;
  }

  if ( NIC_BMC == bmc_location ) {
    fan_src = pal_get_fan_source(fan);
  } else {
    //Config A and B use a single type of fan.
    //Config D uses a dual type of fan.
    if ( fan_type == SINGLE_TYPE) fan_src = fan;
    else fan_src = pal_get_fan_source(fan);
  }

  //read cached value if it's available
  pwm_snr = BMC_SENSOR_PWM0 + fan_src;
  if ( sensor_cache_read(FRU_BMC, pwm_snr, &value) < 0 ) {
    ret = _pal_get_pwm_value(fan_src, &value, bmc_location);
    if ( ret < 0 ) {
      syslog(LOG_WARNING, "%s() Failed to get the PWM%d of fan%d", __func__, fan_src, fan);
    }
  }

  *pwm = (uint8_t)value;
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
apply_frontIO_correction(uint8_t fru, uint8_t snr_num, float *value, uint8_t bmc_location) {
  int pwm = 0;
  static bool inited = false;
  float pwm_val = 0;
  float avg_pwm = 0;
  uint8_t cnt = 0;

  // Get PWM value
  for (pwm = 0; pwm < pal_pwm_cnt; pwm++) {
    if ( _pal_get_pwm_value(pwm, &pwm_val, bmc_location) < 0 ) continue;
    else {
      avg_pwm += pwm_val;
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
  } else {
    syslog(LOG_WARNING, "Failed to apply frontIO correction");
  }
}

enum {
  GET_TOTAL_VAL = 0x00,
  GET_MAX_VAL,
  GET_MIN_VAL,
};

// Get the sum of slot sensor readings on a given sensor number
static int
read_snr_from_all_slots(uint8_t target_snr_num, uint8_t action, float *val) {
  static bool is_inited = false;
  static uint8_t config = CONFIG_A;
  uint8_t type_2ou = UNKNOWN_BOARD;

  //try to get the system type. The default config is CONFIG_A.
  if ( is_inited == false ) {
    char sys_conf[16] = {0};
    if ( kv_get("sled_system_conf", sys_conf, NULL, KV_FPERSIST) < 0 ) {
      syslog(LOG_WARNING, "%s() Failed to read sled_system_conf", __func__);
      return READING_NA;
    }

    if ( strcmp(sys_conf, "Type_1") == 0 ) config = CONFIG_A;
    else if ( strcmp(sys_conf, "Type_10") == 0 ) config = CONFIG_B;
    else if ( strcmp(sys_conf, "Type_15") == 0 ) config = CONFIG_D;
    else syslog(LOG_WARNING, "%s() Couldn't identiy the system type: %s", __func__, sys_conf);

    syslog(LOG_WARNING, "%s() Get the system type: %s", __func__, sys_conf);
    is_inited = true;
  }

  int i = 0;
  float temp_val = 0;
  for ( i = FRU_SLOT1; i <= FRU_SLOT4; i++ ) {
    //Only two slots are present on Config D. Skip slot2 and slot4.
    if ( (config == CONFIG_D) && (i % 2 == 0) ) continue;

    //If one of slots is failed to read, return READING_NA.
    if ( sensor_cache_read(i, target_snr_num, &temp_val) < 0) return READING_NA;

    if ( action == GET_MAX_VAL ) {
      if ( temp_val > *val ) *val = temp_val;
    } else if ( action == GET_MIN_VAL ) {
      if ( ((int)(*val) == 0) || (temp_val < *val) ) *val = temp_val;
    } else if ( action == GET_TOTAL_VAL ) {
      *val += temp_val;
    }

    if (config == CONFIG_D && i == FRU_SLOT1) {
      if (fby3_common_get_2ou_board_type(FRU_SLOT1, &type_2ou) < 0) {
        syslog(LOG_WARNING, "%s() Failed to get 2OU board type\n", __func__);
        return READING_NA;
      } else if (type_2ou == DP_RISER_BOARD) {
        //DP only has slot1
        return PAL_EOK;
      }
    }
  }

  return PAL_EOK;
}

// Calculate Deltalake vdelta
static int
read_pdb_dl_vdelta(uint8_t snr_number, float *value) {
  float medusa_vout = 0;
  float min_hsc_vin = 0;

  if ( sensor_cache_read(FRU_BMC, BMC_SENSOR_MEDUSA_VOUT, &medusa_vout) < 0) return READING_NA;
  if ( read_snr_from_all_slots(BIC_SENSOR_HSC_INPUT_VOL, GET_MIN_VAL, &min_hsc_vin) < 0) return READING_NA;

  //Calculate the Vdrop between BMC_SENSOR_MEDUSA_VOUT and each individual HSC_Input_Vol
  //And return the max Vdrop. The UCR of Vdrop is 0.9V.
  *value = medusa_vout - min_hsc_vin;

  return PAL_EOK;
}

// Calculate curr leakage
static int
read_curr_leakage(uint8_t snr_number, float *value) {
#define CURR_LEAKAGE_THRESH  8.00
#define MEDUSA_CURR_THRESH  10.00
  //static bool is_issued_sel = false;
  float medusa_curr = 0;
  float bb_hsc_curr = 0;
  float total_hsc_iout = 0;

  if ( sensor_cache_read(FRU_BMC, BMC_SENSOR_MEDUSA_CURR, &medusa_curr) < 0) return READING_NA;
  if ( sensor_cache_read(FRU_BMC, BMC_SENSOR_HSC_IOUT, &bb_hsc_curr) < 0) return READING_NA;
  if ( read_snr_from_all_slots(BIC_SENSOR_HSC_OUTPUT_CUR, GET_TOTAL_VAL, &total_hsc_iout) < 0) return READING_NA;

  *value = (medusa_curr - bb_hsc_curr - total_hsc_iout) / medusa_curr;
  *value *= 100;
  //syslog(LOG_WARNING, "%s() value: %.2f %%, medusa_curr: %.2f, bb_hsc_curr: %.2f, slot_hsc_iout: %.2f", __func__, *value, medusa_curr, bb_hsc_curr, slot_hsc_iout);

  //If curr leakage >= 8% AND medusa_curr >= 10A, issue a SEL.
  //sensord don't support mutiple conditions to issue the SEL. So, we do it here.

  /* Remove it to avoid issuing the false alarm event temporally.
   * It will be added back when the false alarm event are clarified. */
#if 0
  if ( is_issued_sel == false && ((*value >= CURR_LEAKAGE_THRESH) && (medusa_curr >= MEDUSA_CURR_THRESH)) ) {
    syslog(LOG_CRIT, "ASSERT: Upper Critical threshold - raised - FRU: 7, num: 0x%2X "
        "curr_val: %.2f %%, thresh_val: %.2f %%, snr: BMC_SENSOR_CURR_LEAKAGE", snr_number, *value, CURR_LEAKAGE_THRESH);
    is_issued_sel = true;
  } else if ( is_issued_sel == true && ((*value < CURR_LEAKAGE_THRESH) || (medusa_curr < MEDUSA_CURR_THRESH)) ) {
    syslog(LOG_CRIT, "DEASSERT: Upper Critical threshold - settled - FRU: 7, num: 0x%2X "
        "curr_val: %.2f %%, thresh_val: %.2f %%, snr: BMC_SENSOR_CURR_LEAKAGE", snr_number, *value, CURR_LEAKAGE_THRESH);
    is_issued_sel = false;
  }
#endif
  return PAL_EOK;
}

// Provide the fan pwm to sensor-util
static int
read_fan_pwm(uint8_t pwm_id, float *value) {
  static uint8_t bmc_location = 0;
  float pwm = 0;
  int ret = 0;

  if ( bmc_location == 0 ) {
    if ( (pwm_id & PWM_PLAT_SET) == PWM_PLAT_SET ) bmc_location = NIC_BMC;
    else bmc_location = DVT_BB_BMC;
  }

  ret = _pal_get_pwm_value((pwm_id&PWM_MASK), &pwm, bmc_location);
  if ( ret < 0 ) {
    ret = READING_NA;
  }
  *value = pwm;
  return ret;
}

// Provide the fan speed to sensor-util
static int
read_fan_speed(uint8_t snr_number, float *value) {
  int rpm = 0;
  int ret = 0;
  uint8_t fan = snr_number - BMC_SENSOR_FAN0_TACH;
  ret = pal_get_fan_speed(fan, &rpm);
  if ( ret < 0 ) {
    ret = READING_NA;
  }
  *value = (float)rpm;
  return ret;
}

static int
read_cached_val(uint8_t snr_number, float *value) {
  float temp1 = 0, temp2 = 0;
  uint8_t snr1_num = 0, snr2_num = 0;

  switch (snr_number) {
    case BMC_SENSOR_FAN_PWR:
        snr1_num = BMC_SENSOR_MEDUSA_VOUT;
        snr2_num = BMC_SENSOR_FAN_IOUT;
      break;
    case BMC_SENSOR_NIC_PWR:
        snr1_num = BMC_SENSOR_NIC_P12V;
        snr2_num = BMC_SENSOR_NIC_IOUT;
      break;
    case BMC_SENSOR_PDB_BB_VDELTA:
        snr1_num = BMC_SENSOR_MEDUSA_VOUT;
        snr2_num = BMC_SENSOR_HSC_VIN;
      break;
    default:
      return READING_NA;
      break;
  }

  if ( sensor_cache_read(FRU_BMC, snr1_num, &temp1) < 0) return READING_NA;
  if ( sensor_cache_read(FRU_BMC, snr2_num, &temp2) < 0) return READING_NA;

  switch (snr_number) {
    case BMC_SENSOR_FAN_PWR:
    case BMC_SENSOR_NIC_PWR:
      *value = temp1 * temp2;
      break;
    case BMC_SENSOR_PDB_BB_VDELTA:
      *value = temp1 - temp2;
      break;
  }

  return PAL_EOK;
}

static int
read_medusa_val(uint8_t snr_number, float *value) {
  static bool is_cached = false;
  static bool is_ltc4282 = true;
  static float medusa_vin = 0;
  static float medusa_vout = 0;
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
      medusa_vin = *value;
      break;
    case BMC_SENSOR_MEDUSA_VOUT:
      ret = sensors_read(chip, "BMC_SENSOR_MEDUSA_VOUT", value);
      if ( is_ltc4282 == false ) *value *= 0.99;
      medusa_vout = *value;
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
    case BMC_SENSOR_MEDUSA_VDELTA:
      *value = medusa_vin - medusa_vout;
      //prevent sensor-util to show -0.00 volts
      if ( *value < 0 ) *value = 0;
      ret = PAL_EOK;
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
  uint8_t fan_type = UNKNOWN_TYPE;
  uint8_t bmc_location = 0;
  float arr[120] = {0};
  int ignore_sample = 0;
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

  ret = pal_get_fan_type(&bmc_location, &fan_type);
  //Config A and B use a single type of fan.
  //Config D uses a dual type of fan.
  if ( fan_type == SINGLE_TYPE) {
    ignore_sample = 20;
  } else {
    ignore_sample = 0;
  }
  int sampling = 100 + ignore_sample;

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
        arr[i] = temp_val;
        available_sampling++;
      }
    }

    if ( available_sampling == 0 ) {
      ret = READING_NA;
    } else {
      if (fan_type == SINGLE_TYPE) {
        qsort((void *)arr, sampling, sizeof(int), compare);
      }
      // Drop the last 20 lower value
      for(i = 0; i < (sampling - ignore_sample); i++) {
        *value += arr[i];
      }
      *value = *value / (available_sampling - ignore_sample);
    }
  } else {
    ret = sensors_read_adc(adc_label[adc_id], value);
  }

  //TODO: if devices are not installed, maybe we need to show NA instead of 0.01
  if ( ret == PAL_EOK ) {
    if ( ADC8 == adc_id ) {
      *value = *value/0.22/0.665; // EVT: /0.22/0.237/4
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

static int
get_hsc_reading(uint8_t hsc_id, uint8_t type, uint8_t cmd, float *value, uint8_t *raw_data) {
  const uint8_t adm1278_bus = 11;
  uint8_t addr = hsc_info_list[hsc_id].slv_addr;
  static int fd = -1;

  if ( fd < 0 ) {
    fd = i2c_cdev_slave_open(adm1278_bus, addr >> 1, I2C_SLAVE_FORCE_CLAIM);
    if ( fd < 0 ) {
      syslog(LOG_WARNING, "Failed to open bus %d", adm1278_bus);
      return READING_NA;
    }
  }

  uint8_t rbuf[9] = {0x00};
  uint8_t rlen = ( cmd != ADM1278_EIN_EXT )?2:9;
  int retry = MAX_RETRY;
  int ret = ERR_NOT_READY;
  while ( ret < 0 && retry-- > 0 ) {
    ret = i2c_rdwr_msg_transfer(fd, addr, &cmd, 1, rbuf, rlen);
  }

  if ( ret < 0 ) {
    if ( fd >= 0 ) {
      close(fd);
      fd = -1;
    }
    return READING_NA;
  }

  if ( cmd == ADM1278_EIN_EXT ) {
    if ( raw_data != NULL ) memcpy(raw_data, rbuf, rlen);
  } else {
    float m = hsc_info_list[hsc_id].info[type].m;
    float b = hsc_info_list[hsc_id].info[type].b;
    float r = hsc_info_list[hsc_id].info[type].r;
    *value = ((float)(rbuf[1] << 8 | rbuf[0]) * r - b) / m;
  }
  return PAL_EOK;
}

static int
read_hsc_ein(uint8_t hsc_id, float *value) {
#define EIN_ROLLOVER_CNT 0x10000
#define EIN_SAMPLE_CNT 0x1000000
#define EIN_ENERGY_CNT 0x800000
#define PIN_COEF (0.0163318634656214)  // X = 1/m * (Y * 10^(-R) - b) = 1/6123 * (Y * 100)
  uint8_t raw_data[9] = {0x00};

  if ( get_hsc_reading(hsc_id, -1, ADM1278_EIN_EXT, value, raw_data) < 0 ) return READING_NA;
  if ( raw_data[0] != 8 ) return READING_NA; //first byte is the num. of bytes. It should be 8.

  uint32_t energy = 0, rollover = 0, sample = 0;
  uint32_t pre_energy = 0, pre_rollover = 0, pre_sample = 0;
  uint32_t sample_diff = 0;
  double energy_diff = 0;
  static uint32_t last_energy = 0, last_rollover = 0, last_sample = 0;
  static bool pre_ein = false;

  //record the previous data
  pre_energy   = last_energy;
  pre_rollover = last_rollover;
  pre_sample   = last_sample;

  //record the current data
  last_energy   = energy   = (raw_data[3]<<16) | (raw_data[2]<<8) | raw_data[1];
  last_rollover = rollover = (raw_data[5]<<8) | raw_data[4];
  last_sample   = sample   = (raw_data[8]<<16) | (raw_data[7]<<8) | raw_data[6];

  //return since data isn't enough
  if ( pre_ein == false ) {
    pre_ein = true;
    return READING_NA;
  }

  if ((pre_rollover > rollover) || ((pre_rollover == rollover) && (pre_energy > energy))) {
    rollover += EIN_ROLLOVER_CNT;
  }
  if (pre_sample > sample) {
    sample += EIN_SAMPLE_CNT;
  }

  energy_diff = (double)(rollover-pre_rollover)*EIN_ENERGY_CNT + (double)energy - (double)pre_energy;
  if (energy_diff < 0) {
    return READING_NA;
  }
  sample_diff = sample - pre_sample;
  if (sample_diff == 0) {
    return READING_NA;
  }
  *value = (float)((energy_diff/sample_diff/256) * PIN_COEF/ADM1278_RSENSE);
  return PAL_EOK;
}

static int
read_hsc_pin(uint8_t hsc_id, float *value) {

  if ( get_hsc_reading(hsc_id, HSC_POWER, PMBUS_READ_PIN, value, NULL) < 0 ) return READING_NA;
  *value *= 0.99; //improve the accuracy of PIN to +-2%
  return PAL_EOK;
}

static int
read_hsc_iout(uint8_t hsc_id, float *value) {

  if ( get_hsc_reading(hsc_id, HSC_CURRENT, PMBUS_READ_IOUT, value, NULL) < 0 ) return READING_NA;
  *value *= 0.99; //improve the accuracy of IOUT to +-2%
  return PAL_EOK;
}

static int
read_hsc_temp(uint8_t hsc_id, float *value) {

  if ( get_hsc_reading(hsc_id, HSC_TEMP, PMBUS_READ_TEMP1, value, NULL) < 0 ) return READING_NA;
  return PAL_EOK;
}

static int
read_hsc_vin(uint8_t hsc_id, float *value) {

  if ( get_hsc_reading(hsc_id, HSC_VOLTAGE, PMBUS_READ_VIN, value, NULL) < 0 ) return READING_NA;
  return PAL_EOK;
}

static int
read_hsc_peak_iout(uint8_t hsc_id, float *value) {
  if ( get_hsc_reading(hsc_id, HSC_CURRENT, ADM1278_PEAK_IOUT, value, NULL) < 0 ) return READING_NA;
  return PAL_EOK;
}

static int
read_hsc_peak_pin(uint8_t hsc_id, float *value) {
  if ( get_hsc_reading(hsc_id, HSC_POWER, ADM1278_PEAK_PIN, value, NULL) < 0 ) return READING_NA;
  return PAL_EOK;
}

static void
pal_all_slot_power_ctrl(uint8_t opt, char *sel_desc) {
  int i = 0;
  uint8_t status = 0;
  for ( i = FRU_SLOT1; i <= FRU_SLOT4; i++ ) {
    if ( pal_is_fru_prsnt(i, &status) < 0 || status == 0 ) {
      continue;
    }
    syslog(LOG_CRIT, "FRU: %d, Turned %s 12V power of slot%d due to %s", i, (opt == SERVER_12V_ON)?"on":"off", i, sel_desc);
    if ( pal_set_server_power(i, opt) < 0 ) {
      syslog(LOG_CRIT, "Failed to turn %s 12V power of slot%d", (opt == SERVER_12V_ON)?"on":"off", i);
    }
  }
}

static void
pal_nic_otp_check(float *value, float unr, float ucr) {
  static int retry = MAX_RETRY;
  static bool is_otp_asserted = false;
  char sel_str[128] = {0};
  if ( *value < ucr ) {
    if ( is_otp_asserted == false ) return; //it will move on when is_otp_asserted is true
    if ( retry != MAX_RETRY ) {
      retry++;
      return;
    }

    //recover the system
    is_otp_asserted = false;
    snprintf(sel_str, sizeof(sel_str), "NIC temp is under UCR. (val = %.2f)", *value);
    pal_all_slot_power_ctrl(SERVER_12V_ON, sel_str);
    return;
  }

  if ( retry != 0 ) {
    retry--;
    return;
  }

  //need to turn off all slots since retry is reached
  if ( is_otp_asserted == false && *value >= unr ) {
    is_otp_asserted = true;
    snprintf(sel_str, sizeof(sel_str), "NIC temp is over UNR. (val = %.2f)", *value);
    pal_all_slot_power_ctrl(SERVER_12V_OFF, sel_str);
  }

  return;
}

void
pal_fan_fail_otp_check(void) {
  uint8_t bmc_location = 0;
  char sel_str[128] = {0};
  static bool is_fan_fail_otp_asserted = false;

  if (is_fan_fail_otp_asserted == true) return;

  if ( fby3_common_get_bmc_location(&bmc_location) < 0 ) {
    syslog(LOG_WARNING, "Failed to get the location of BMC");
  }

  if ( is_fan_fail_otp_asserted == false && bmc_location != NIC_BMC){
    is_fan_fail_otp_asserted = true;
    snprintf(sel_str, sizeof(sel_str), "all fans failed");
    pal_all_slot_power_ctrl(SERVER_12V_OFF, sel_str);
  } else if (is_fan_fail_otp_asserted == false && bmc_location == NIC_BMC) {
    syslog(LOG_CRIT, "Turned off power of slot1 due to all fans failed");
    pal_set_server_power(FRU_SLOT1, SERVER_POWER_OFF);
  }

  return;
}

static int
skip_bic_sensor_list(uint8_t fru, uint8_t sensor_num, const uint8_t bmc_location, const uint8_t config_status) {
  uint8_t *bic_skip_list;
  int skip_sensor_cnt;
  int i = 0;

  get_skip_sensor_list(fru, &bic_skip_list, &skip_sensor_cnt, bmc_location, config_status);

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
pal_bic_sensor_read_raw(uint8_t fru, uint8_t sensor_num, float *value, uint8_t bmc_location, const uint8_t config_status){
#define BIC_SENSOR_READ_NA 0x20
  int ret = 0;
  uint8_t power_status = 0;
  ipmi_sensor_reading_t sensor = {0};
  sdr_full_t *sdr = NULL;
  char path[128];
  sprintf(path, SLOT_SENSOR_LOCK, fru);
  uint8_t *bic_skip_list;
  int skip_sensor_cnt = 0;
  get_skip_sensor_list(fru, &bic_skip_list, &skip_sensor_cnt, bmc_location, config_status);

  ret = bic_get_server_power_status(fru, &power_status);
  if (ret < 0) {
    return READING_NA;
  }

  if (power_status != SERVER_POWER_ON) {
    pwr_off_flag[fru-1] = 1;
    //syslog(LOG_WARNING, "%s() Failed to run bic_get_server_power_status(). fru%d, snr#0x%x, pwr_sts:%d", __func__, fru, sensor_num, power_status);
    if (skip_bic_sensor_list(fru, sensor_num, bmc_location, config_status) < 0) {
      return READING_NA;
    }
  } else if (power_status == SERVER_POWER_ON && pwr_off_flag[fru-1]) {
    if ((skip_bic_sensor_list(fru, sensor_num, bmc_location, config_status) < 0) && (temp_cnt < skip_sensor_cnt)){
      temp_cnt ++;
      return READING_NA;
    }

    if (temp_cnt == skip_sensor_cnt) {
      pwr_off_flag[fru-1] = 0;
      temp_cnt = 0;
    }
  }

  ret = access(path, F_OK);
  if(ret == 0) {
    return READING_SKIP;
  }

  //check snr number first. If it not holds, it will move on
  if ( (sensor_num >= 0x0) && (sensor_num <= 0x42) ) { //server board
    ret = bic_get_sensor_reading(fru, sensor_num, &sensor, NONE_INTF);
  } else if ( (sensor_num >= 0x50 && sensor_num <= 0x7F) && (bmc_location != NIC_BMC) && //1OU
       ((config_status & PRESENT_1OU) == PRESENT_1OU) ) {
    ret = bic_get_sensor_reading(fru, sensor_num, &sensor, FEXP_BIC_INTF);
  } else if ( ((sensor_num >= 0x80 && sensor_num <= 0xCE) ||     //2OU
               (sensor_num >= 0x49 && sensor_num <= 0x4D)) &&    //Many sensors are defined in GPv3.
              ((config_status & PRESENT_2OU) == PRESENT_2OU) ) { //The range from 0x80 to 0xCE is not enough for adding new sensors.
                                                                 //So, we take 0x49 ~ 0x4D here
    ret = bic_get_sensor_reading(fru, sensor_num, &sensor, REXP_BIC_INTF);
  } else if ( sensor_num == 0x43 && (config_status & PRESENT_2OU) == PRESENT_2OU ) { // DP Riser
    ret = bic_get_sensor_reading(fru, sensor_num, &sensor, NONE_INTF);
  } else if ( (sensor_num >= 0xD1 && sensor_num <= 0xEC) ) { //BB
    ret = bic_get_sensor_reading(fru, sensor_num, &sensor, BB_BIC_INTF);
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
      apply_frontIO_correction(fru, sensor_num, value, bmc_location);
      break;
    case BIC_SENSOR_CPU_THERM_MARGIN:
      if ( *value > 0 ) *value = -(*value);
      break;
  }

  if ( bic_get_server_power_status(fru, &power_status) < 0 || power_status != SERVER_POWER_ON) {
    pwr_off_flag[fru-1] = 1;
    //syslog(LOG_WARNING, "%s() Failed to run bic_get_server_power_status(). fru%d, snr#0x%x, pwr_sts:%d", __func__, fru, sensor_num, power_status);
    if (skip_bic_sensor_list(fru, sensor_num, bmc_location, config_status) < 0) {
      return READING_NA;
    }
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
  static uint8_t config_status[MAX_NODES] = {CONFIG_UNKNOWN, CONFIG_UNKNOWN, CONFIG_UNKNOWN, CONFIG_UNKNOWN};

  if ( bmc_location == 0 ) {
    if ( fby3_common_get_bmc_location(&bmc_location) < 0 ) {
      syslog(LOG_WARNING, "Failed to get the location of BMC");
    } else {
      if ( bmc_location == NIC_BMC ) {
        sensor_map[BMC_SENSOR_OUTLET_TEMP].id = TEMP_NICEXP_OUTLET;
        //Use to indicate that we need to handle PWM sensors especially.
        //Also, use bit7 to represent class 1 or 2.
        sensor_map[BMC_SENSOR_PWM0].id |= PWM_PLAT_SET;
        sensor_map[BMC_SENSOR_PWM1].id |= PWM_PLAT_SET;
        sensor_map[BMC_SENSOR_PWM2].id |= PWM_PLAT_SET;
        sensor_map[BMC_SENSOR_PWM3].id |= PWM_PLAT_SET;
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
      //check a config status of a blade
      if ( config_status[fru-1] == CONFIG_UNKNOWN ) {
        ret = bic_is_m2_exp_prsnt(fru);
        if ( ret < 0 ) {
          syslog(LOG_WARNING, "%s() Failed to run bic_is_m2_exp_prsnt", __func__);
        } else config_status[fru-1] = (uint8_t)ret;
        syslog(LOG_WARNING, "%s() fru: %02x. config:%02x", __func__, fru, config_status[fru-1]);
      }

      //if we can't get the config status of the blade, return READING_NA.
      if ( pal_is_fw_update_ongoing(fru) == false && \
           config_status[fru-1] != CONFIG_UNKNOWN ) {
        if ( pal_sdr_init(fru) == ERR_NOT_READY ) ret = READING_NA;
        else ret = pal_bic_sensor_read_raw(fru, sensor_num, (float*)value, bmc_location, config_status[fru-1]);
      } else ret = READING_NA;
      break;
    case FRU_BMC:
      ret = sensor_map[sensor_num].read_sensor(id, (float*) value);
      break;
    case FRU_NIC:
      ret = sensor_map[sensor_num].read_sensor(id, (float*) value);
      //Over temperature protection
      pal_nic_otp_check((float*)value, sensor_map[sensor_num].snr_thresh.unr_thresh, sensor_map[sensor_num].snr_thresh.ucr_thresh);
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

static int
pal_bmc_fan_threshold_init() {
  uint8_t fan_type = UNKNOWN_TYPE;
  uint8_t bmc_location = 0;

  if (fby3_common_get_bmc_location(&bmc_location) < 0) {
    syslog(LOG_ERR, "%s() Cannot get the location of BMC", __func__);
    return -1;
  } else if (pal_get_fan_type(&bmc_location, &fan_type) != PAL_EOK) {
    syslog(LOG_ERR, "%s() Cannot get the type of fan, fvaule=%d", __func__, fan_type);
    return -1;
  } else {
    if (fan_type != SINGLE_TYPE) {
      // set fan tach UCR to 13500 if it's not single type
      sensor_map[BMC_SENSOR_FAN0_TACH].snr_thresh.ucr_thresh = DUAL_FAN_UCR;
      sensor_map[BMC_SENSOR_FAN1_TACH].snr_thresh.ucr_thresh = DUAL_FAN_UCR;
      sensor_map[BMC_SENSOR_FAN2_TACH].snr_thresh.ucr_thresh = DUAL_FAN_UCR;
      sensor_map[BMC_SENSOR_FAN3_TACH].snr_thresh.ucr_thresh = DUAL_FAN_UCR;
      sensor_map[BMC_SENSOR_FAN4_TACH].snr_thresh.ucr_thresh = DUAL_FAN_UCR;
      sensor_map[BMC_SENSOR_FAN5_TACH].snr_thresh.ucr_thresh = DUAL_FAN_UCR;
      sensor_map[BMC_SENSOR_FAN6_TACH].snr_thresh.ucr_thresh = DUAL_FAN_UCR;
      sensor_map[BMC_SENSOR_FAN7_TACH].snr_thresh.ucr_thresh = DUAL_FAN_UCR;
    }
#ifdef DEBUG
    syslog(LOG_INFO, "%s() fan tach threshold initialized, fan_type: %u", __func__, fan_type);
#endif
  }
  return 0;
}

int
pal_get_sensor_threshold(uint8_t fru, uint8_t sensor_num, uint8_t thresh, void *value) {
  static bool is_fan_threshold_init = false;
  float *val = (float*) value;

  if (fru == FRU_BMC && is_fan_threshold_init == false) {
    if (pal_bmc_fan_threshold_init() < 0) {
      return -1;
    } else {
      is_fan_threshold_init = true;
    }
  }

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
    case FRU_NIC:
      //Both FRUs don't own SDRs.
      return PAL_ENOTSUP;
    break;

    default:
      syslog(LOG_WARNING, "%s() Wrong fru id %d", __func__, fru);
    return PAL_ENOTSUP;
  }

  sprintf(path, SDR_PATH, fru_name);
  if (access(path, F_OK) == -1) {
    return PAL_ENOTSUP;
  }

  return PAL_EOK;
}

static int
_sdr_init(char *path, sensor_info_t *sinfo, uint8_t bmc_location, \
          const uint8_t config_status, const uint8_t board_type) {
  int fd;
  int ret = 0;
  uint8_t buf[MAX_SDR_LEN] = {0};
  uint8_t bytes_rd = 0;
  uint8_t snr_num = 0;
  sdr_full_t *sdr;
  int retry = MAX_RETRY;

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
    // If it is a system of class 2, change m_val and UCR of HSC.
    if (snr_num == BIC_SENSOR_HSC_OUTPUT_CUR) {
      if (bmc_location == NIC_BMC) {
        sdr->uc_thresh = HSC_OUTPUT_CUR_UC_THRESHOLD;
        sdr->m_val = 0x04;
      }
    } else if (snr_num == BIC_SENSOR_HSC_INPUT_PWR || snr_num == BIC_SENSOR_HSC_INPUT_AVGPWR){
      if (bmc_location == NIC_BMC) {
        sdr->uc_thresh = HSC_INPUT_PWR_UC_THRESHOLD;
        sdr->m_val = 0x04;
      }
    } else if ( (config_status & PRESENT_2OU) == PRESENT_2OU && (board_type == GPV3_BRCM_BOARD) \
                                       && (snr_num == BIC_GPV3_VR_P1V8_CURRENT || \
                                           snr_num == BIC_GPV3_VR_P1V8_POWER) ) {
      sdr->uc_thresh = 0x00; //NA
    } else if ( (config_status & PRESENT_2OU) == PRESENT_2OU && (board_type == GPV3_BRCM_BOARD) \
                                       && (snr_num == BIC_GPV3_VR_P0V84_VOLTAGE) ) {
      sdr->uc_thresh = 0xB8;
      sdr->lc_thresh = 0xB0;
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
  int ret = 0, prsnt_retry = 1;
  uint8_t bmc_location = 0;
  uint8_t config_status = 0;
  uint8_t board_type = 0;

  //syslog(LOG_WARNING, "%s() pal_is_sdr_init  bool %d, fru %d, snr_num: %x\n", __func__, pal_is_sdr_init(fru), fru, g_sinfo[fru-1][1].sdr.sensor_num);
  if ( true == pal_is_sdr_init(fru) ) {
    memcpy(sinfo, g_sinfo[fru-1], sizeof(sensor_info_t) * MAX_SENSOR_NUM);
    goto error_exit;
  }

  sprintf(path, PWR_UTL_LOCK, fru);
  if (access(path, F_OK) == 0) {
    prsnt_retry = MAX_READ_RETRY;
  } else {
    prsnt_retry = 1;
  }

  ret = pal_sensor_sdr_path(fru, path);
  if ( ret < 0 ) {
    //syslog(LOG_WARNING, "%s() Failed to run pal_sensor_sdr_path\n", __func__);
    goto error_exit;
  }

  // get the location
  ret = fby3_common_get_bmc_location(&bmc_location);
  if (ret < 0) {
    syslog(LOG_ERR, "%s() Cannot get the location of BMC", __func__);
    goto error_exit;
  }

  while ( prsnt_retry-- > 0 ) {
    // get the status of m2 board
    ret = bic_is_m2_exp_prsnt(fru);
    if ( ret < 0 ) {
      sleep(3);
      continue;
    } else config_status = (uint8_t) ret;

    if ( (config_status & PRESENT_2OU) == PRESENT_2OU ) {
      //if it's present, get its type
      fby3_common_get_2ou_board_type(fru, &board_type);
    }
    break;
  }
  if ( ret < 0 ) {
    syslog(LOG_ERR, "%s() Couldn't get the status of 1OU/2OU\n", __func__);
    goto error_exit;
  }

  while ( retry-- > 0 ) {
    ret = _sdr_init(path, sinfo, bmc_location, config_status, board_type);
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

static int
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
  uint8_t scale = sensor_map[sensor_num].units;

  switch(scale) {
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
      syslog(LOG_WARNING, "%s() unknown sensor number %x", __func__, sensor_num);
    break;
  }
  return 0;
}

int
pal_is_sensor_valid(uint8_t fru, uint8_t snr_num) {
  struct {
    const char *vendor_id;
    const char *device_id;
  } drives[] = { // Unsupported sensor reading drives
    {"1344",  "5410"}, // Micron
    {"15b7",  "5002"}, // WD
    {"1179",  "011a"}, // Toshiba
  };
  char key[100] = {0};
  char drive_id[16] = {0};
  char read_vid[16] = {0};
  char read_did[16] = {0};
  int i = 0;

  if (snr_num == BIC_SENSOR_M2B_TEMP) {
    snprintf(key, sizeof(key), DEVICE_KEY, fru);
    if (kv_get(key, drive_id, NULL, KV_FPERSIST) < 0) {
      syslog(LOG_WARNING, "%s() Failed to read %s", __func__, key);
      return 0;
    }

    snprintf(read_vid, sizeof(read_vid), "%02x%02x", drive_id[1], drive_id[0]);
    snprintf(read_did, sizeof(read_did), "%02x%02x", drive_id[3], drive_id[2]);
    for (i = 0; i < ARRAY_SIZE(drives) ; i++) {
      if (0 == strcmp(read_vid,drives[i].vendor_id) && 0 == strcmp(read_did,drives[i].device_id)) {
        return 1; // The Drive unsupport sensor reading
      }
    }
    return 0;
  }
  return 0;
}

bool
pal_sensor_is_source_host(uint8_t fru, uint8_t sensor_id) {
  bool ret = false;
  switch(fru) {
    case FRU_SLOT1:
    case FRU_SLOT2:
    case FRU_SLOT3:
    case FRU_SLOT4:
      if (sensor_id == BIC_SENSOR_DP_PCIE_TEMP) {
        char key[MAX_KEY_LEN] = {0};
        char value[MAX_VALUE_LEN] = {0};
        struct pcie_info* pcie_info = (struct pcie_info *) value;

        sprintf(key, DP_INNER_PCIE_INFO_KEY, fru);
        if (kv_get(key, value, NULL, KV_FPERSIST) != 0) {
          // pcie_info file not exist
          ret = true;
        } else if (pcie_info->vendor_id == PCIE_VENDOR_MARVELL_HSM && pcie_info->device_id == PCIE_DEVICE_MARVELL_HSM) {
          // dp marvell hsm
          ret =  false;
        } else {
          // other pcie cards
          ret =  true;
        }
      }
      break;
    default:
      break;
  }
  return ret;
}

bool
pal_is_host_snr_available(uint8_t fru, uint8_t snr_num) {
  uint8_t pwr_status = 0;

  if (pal_get_server_power(fru, &pwr_status) < 0) {
    return false;
  }

  if (pwr_status == SERVER_POWER_ON || pwr_status == SERVER_12V_ON) {
    return true;
  }

  return false;
}
