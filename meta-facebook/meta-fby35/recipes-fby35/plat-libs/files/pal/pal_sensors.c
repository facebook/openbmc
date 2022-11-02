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
#define MAX_SNR_NAME_LEN 32
#define SDR_PATH "/tmp/sdr_%s.bin"

#define PWM_PLAT_SET 0x80
#define PWM_MASK     0x0f

#define DEVICE_KEY "sys_config/fru%d_B_drive0_model_name"

#define DUAL_FAN_UCR 13500
#define DUAL_FAN_UNC 10200

#define FAN_15K_LCR  1200
#define FAN_15K_UNC  13000
#define FAN_15K_UCR  17000

#define BB_CPU_VDELTA_48V_UCR 41.8

#define BB_HSC_SENSOR_CNT 7

enum {
  /* Fan Type */
  DUAL_TYPE    = 0x00,
  SINGLE_TYPE  = 0x03,
  UNKNOWN_TYPE = 0xff,
};

enum {
  /* Fan Cnt*/
  DUAL_FAN_CNT = 0x08,
  SINGLE_FAN_CNT = 0x04,
  UNKNOWN_FAN_CNT = 0x00,
};

enum {
  /* Solution */
  MAXIM_SOLUTION = 0x00,
  MPS_SOLUTION = 0x01,
  UNKNOWN_SOLUTION = 0xff,
};

static int read_adc_val(uint8_t adc_id, float *value);
static int read_temp(uint8_t snr_id, float *value);
static int read_hsc_vin(uint8_t hsc_id, float *value);
static int read_hsc_temp(uint8_t hsc_id, float *value);
static int read_hsc_pin(uint8_t hsc_id, float *value);
static int read_hsc_iout(uint8_t hsc_id, float *value);
static int read_hsc_peak_iout(uint8_t hsc_id, float *value);
static int read_hsc_peak_pin(uint8_t hsc_id, float *value);
static int read_medusa_val(uint8_t snr_number, float *value);
static int read_cached_val(uint8_t snr_number, float *value);
static int read_fan_speed(uint8_t snr_number, float *value);
static int read_fan_pwm(uint8_t pwm_id, float *value);
static int read_curr_leakage(uint8_t snr_number, float *value);
static int read_pdb_vin(uint8_t pdb_id, float *value);
static int read_pdb_vout(uint8_t pdb_id, float *value);
static int read_pdb_iout(uint8_t pdb_id, float *value);
static int read_pdb_pout(uint8_t pdb_id, float *value);
static int read_pdb_temp(uint8_t pdb_id, float *value);
static int read_pdb_cl_vdelta(uint8_t snr_number, float *value);
static int read_dpv2_efuse(uint8_t info, float *value);
static int pal_sdr_init(uint8_t fru);
static sensor_info_t g_sinfo[MAX_NUM_FRUS][MAX_SENSOR_NUM + 1] = {0};
static bool sdr_init_done[MAX_NUM_FRUS] = {false};
static uint8_t bic_dynamic_sensor_list[4][MAX_SENSOR_NUM + 1] = {0};
static uint8_t bic_dynamic_skip_sensor_list[4][MAX_SENSOR_NUM + 1] = {0};
static uint8_t rev_id = UNKNOWN_REV;
static uint8_t bmc_dynamic_sensor_list[MAX_SENSOR_NUM + 1] = {0};

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
  BMC_SENSOR_P1V8_STBY,
  BMC_SENSOR_P1V2_BMC_STBY,
  BMC_SENSOR_P2V5_BMC_STBY,
  BMC_SENSOR_P1V0_STBY,
  BMC_SENSOR_P0V6_STBY,
  BMC_SENSOR_P3V3_RGM_STBY,
  BMC_SENSOR_P5V_USB,
  BMC_SENSOR_P3V3_NIC,
  BMC_SENSOR_HSC_TEMP,
  BMC_SENSOR_HSC_VIN,
  BMC_SENSOR_HSC_PIN,
  BMC_SENSOR_HSC_IOUT,
  BMC_SENSOR_HSC_PEAK_IOUT,
  BMC_SENSOR_HSC_PEAK_PIN,
  BMC_SENSOR_MEDUSA_VOUT,
  BMC_SENSOR_MEDUSA_VIN,
  BMC_SENSOR_MEDUSA_CURR,
  BMC_SENSOR_MEDUSA_PWR,
  BMC_SENSOR_MEDUSA_VDELTA,
  BMC_SENSOR_PDB_CL_VDELTA,
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
  BMC_SENSOR_P12V,
  BMC_SENSOR_P3V3_STBY,
  BMC_SENSOR_P1V8_STBY,
  BMC_SENSOR_P1V2_BMC_STBY,
  BMC_SENSOR_P2V5_BMC_STBY,
  BMC_SENSOR_P1V0_STBY,
  BMC_SENSOR_P0V6_STBY,
  BMC_SENSOR_P3V3_RGM_STBY,
  BMC_SENSOR_P3V3_NIC,
  BMC_SENSOR_NIC_P12V,
  BMC_SENSOR_NIC_IOUT,
  BMC_SENSOR_NIC_PWR,
  NIC_SENSOR_TEMP,
  BMC_SENSOR_NICEXP_TEMP
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
  BIC_SENSOR_DIMMC0_TEMP,
  BIC_SENSOR_DIMMD0_TEMP,
  BIC_SENSOR_DIMME0_TEMP,
  BIC_SENSOR_DIMMG0_TEMP,
  BIC_SENSOR_DIMMH0_TEMP,
  BIC_SENSOR_M2A_TEMP,
  BIC_SENSOR_HSC_TEMP,
  BIC_SENSOR_VCCIN_VR_TEMP,
  BIC_SENSOR_FIVRA_VR_TEMP,
  BIC_SENSOR_EHV_VR_TEMP,
  BIC_SENSOR_VCCD_VR_TEMP,
  BIC_SENSOR_FAON_VR_TEMP,

  //BIC - voltage sensors
  BIC_SENSOR_P12V_STBY_VOL,
  BIC_SENSOR_P3V_BAT_VOL,
  BIC_SENSOR_P3V3_STBY_VOL,
  BIC_SENSOR_P1V8_STBY_VOL,
  BIC_SENSOR_P1V05_PCH_STBY_VOL,
  BIC_SENSOR_P5V_STBY_VOL,
  BIC_SENSOR_P12V_DIMM_VOL,
  BIC_SENSOR_P1V2_STBY_VOL,
  BIC_SENSOR_P3V3_M2_VOL,
  BIC_SENSOR_HSC_INPUT_VOL,
  BIC_SENSOR_VCCIN_VR_VOL,
  BIC_SENSOR_FIVRA_VR_VOL,
  BIC_SENSOR_EHV_VR_VOL,
  BIC_SENSOR_VCCD_VR_VOL,
  BIC_SENSOR_FAON_VR_VOL,

  //BIC - current sensors
  BIC_SENSOR_HSC_OUTPUT_CUR,
  BIC_SENSOR_VCCIN_VR_CUR,
  BIC_SENSOR_FIVRA_VR_CUR,
  BIC_SENSOR_EHV_VR_CUR,
  BIC_SENSOR_VCCD_VR_CUR,
  BIC_SENSOR_FAON_VR_CUR,

  //BIC - power sensors
  BIC_SENSOR_CPU_PWR,
  BIC_SENSOR_HSC_INPUT_PWR,
  BIC_SENSOR_VCCIN_VR_POUT,
  BIC_SENSOR_FIVRA_VR_POUT,
  BIC_SENSOR_EHV_VR_POUT,
  BIC_SENSOR_VCCD_VR_POUT,
  BIC_SENSOR_FAON_VR_POUT,
  BIC_SENSOR_DIMMA_PMIC_Pout,
  BIC_SENSOR_DIMMC_PMIC_Pout,
  BIC_SENSOR_DIMMD_PMIC_Pout,
  BIC_SENSOR_DIMME_PMIC_Pout,
  BIC_SENSOR_DIMMG_PMIC_Pout,
  BIC_SENSOR_DIMMH_PMIC_Pout,
};

const uint8_t bic_hd_sensor_list[] = {
  BIC_HD_SENSOR_MB_INLET_TEMP,
  BIC_HD_SENSOR_MB_OUTLET_TEMP,
  BIC_HD_SENSOR_FIO_TEMP,
  BIC_HD_SENSOR_CPU_TEMP,
  BIC_HD_SENSOR_DIMMA_TEMP,
  BIC_HD_SENSOR_DIMMB_TEMP,
  BIC_HD_SENSOR_DIMMC_TEMP,
  BIC_HD_SENSOR_DIMME_TEMP,
  BIC_HD_SENSOR_DIMMG_TEMP,
  BIC_HD_SENSOR_DIMMH_TEMP,
  BIC_HD_SENSOR_DIMMI_TEMP,
  BIC_HD_SENSOR_DIMMK_TEMP,
  BIC_HD_SENSOR_SSD_TEMP,
  BIC_HD_SENSOR_HSC_TEMP,
  BIC_HD_SENSOR_CPU0_VR_TEMP,
  BIC_HD_SENSOR_SOC_VR_TEMP,
  BIC_HD_SENSOR_CPU1_VR_TEMP,
  BIC_HD_SENSOR_PVDDIO_VR_TEMP,
  BIC_HD_SENSOR_PVDD11_VR_TEMP,
  BIC_HD_SENSOR_P12V_STBY_VOL,
  BIC_HD_SENSOR_PVDD18_S5_VOL,
  BIC_HD_SENSOR_P3V3_STBY_VOL,
  BIC_HD_SENSOR_PVDD11_S3_VOL,
  BIC_HD_SENSOR_P3V_BAT_VOL,
  BIC_HD_SENSOR_PVDD33_S5_VOL,
  BIC_HD_SENSOR_P5V_STBY_VOL,
  BIC_HD_SENSOR_P12V_MEM_1_VOL,
  BIC_HD_SENSOR_P12V_MEM_0_VOL,
  BIC_HD_SENSOR_P1V2_STBY_VOL,
  BIC_HD_SENSOR_P3V3_M2_VOL,
  BIC_HD_SENSOR_P1V8_STBY_VOL,
  BIC_HD_SENSOR_HSC_INPUT_VOL,
  BIC_HD_SENSOR_CPU0_VR_VOL,
  BIC_HD_SENSOR_SOC_VR_VOL,
  BIC_HD_SENSOR_CPU1_VR_VOL,
  BIC_HD_SENSOR_PVDDIO_VR_VOL,
  BIC_HD_SENSOR_PVDD11_VR_VOL,
  BIC_HD_SENSOR_HSC_OUTPUT_CUR,
  BIC_HD_SENSOR_CPU0_VR_CUR,
  BIC_HD_SENSOR_SOC_VR_CUR,
  BIC_HD_SENSOR_CPU1_VR_CUR,
  BIC_HD_SENSOR_PVDDIO_VR_CUR,
  BIC_HD_SENSOR_PVDD11_VR_CUR,
  BIC_HD_SENSOR_HSC_INPUT_PWR,
  BIC_HD_SENSOR_CPU0_VR_PWR,
  BIC_HD_SENSOR_SOC_VR_PWR,
  BIC_HD_SENSOR_CPU1_VR_PWR,
  BIC_HD_SENSOR_PVDDIO_VR_PWR,
  BIC_HD_SENSOR_PVDD11_VR_PWR,
  BIC_HD_SENSOR_CPU_PWR,
  BIC_HD_SENSOR_DIMMA_PWR,
  BIC_HD_SENSOR_DIMMB_PWR,
  BIC_HD_SENSOR_DIMMC_PWR,
  BIC_HD_SENSOR_DIMME_PWR,
  BIC_HD_SENSOR_DIMMG_PWR,
  BIC_HD_SENSOR_DIMMH_PWR,
  BIC_HD_SENSOR_DIMMI_PWR,
  BIC_HD_SENSOR_DIMMK_PWR,
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

const uint8_t bic_1ou_vf_sensor_list[] = {
  BIC_1OU_VF_SENSOR_NUM_T_MB_OUTLET_TEMP_T,
  BIC_1OU_VF_SENSOR_NUM_V_12_AUX,
  BIC_1OU_VF_SENSOR_NUM_V_12_EDGE,
  BIC_1OU_VF_SENSOR_NUM_V_3_3_AUX,
  BIC_1OU_VF_SENSOR_NUM_INA231_PWR_M2A,
  BIC_1OU_VF_SENSOR_NUM_INA231_VOL_M2A,
  BIC_1OU_VF_SENSOR_NUM_NVME_TEMP_M2A,
  BIC_1OU_VF_SENSOR_NUM_ADC_3V3_VOL_M2A,
  BIC_1OU_VF_SENSOR_NUM_ADC_12V_VOL_M2A,
  BIC_1OU_VF_SENSOR_NUM_INA231_PWR_M2B,
  BIC_1OU_VF_SENSOR_NUM_INA231_VOL_M2B,
  BIC_1OU_VF_SENSOR_NUM_NVME_TEMP_M2B,
  BIC_1OU_VF_SENSOR_NUM_ADC_3V3_VOL_M2B,
  BIC_1OU_VF_SENSOR_NUM_ADC_12V_VOL_M2B,
  BIC_1OU_VF_SENSOR_NUM_INA231_PWR_M2C,
  BIC_1OU_VF_SENSOR_NUM_INA231_VOL_M2C,
  BIC_1OU_VF_SENSOR_NUM_NVME_TEMP_M2C,
  BIC_1OU_VF_SENSOR_NUM_ADC_3V3_VOL_M2C,
  BIC_1OU_VF_SENSOR_NUM_ADC_12V_VOL_M2C,
  BIC_1OU_VF_SENSOR_NUM_INA231_PWR_M2D,
  BIC_1OU_VF_SENSOR_NUM_INA231_VOL_M2D,
  BIC_1OU_VF_SENSOR_NUM_NVME_TEMP_M2D,
  BIC_1OU_VF_SENSOR_NUM_ADC_3V3_VOL_M2D,
  BIC_1OU_VF_SENSOR_NUM_ADC_12V_VOL_M2D,
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
  BB_INLET_TEMP,
  BB_OUTLET_TEMP,
  BB_HSC_TEMP,
  BB_P5V,
  BB_P12V,
  BB_P3V3_STBY,
  BB_P5V_USB,
  BB_P1V2_STBY,
  BB_P1V0_STBY,
  BB_MEDUSA_VIN,
  BB_MEDUSA_VOUT,
  BB_HSC_VIN,
  BB_MEDUSA_CUR,
  BB_HSC_IOUT,
  BB_FAN_IOUT,
  BB_MEDUSA_PWR,
  BB_HSC_PIN,
  BB_HSC_EIN,
  BB_HSC_PEAK_IOUT,
  BB_HSC_PEAK_PIN,
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

const uint8_t bmc_dpv2_x8_sensor_list[] = {
  BMC_DPV2_SENSOR_DPV2_1_12V_VIN,
  BMC_DPV2_SENSOR_DPV2_1_12V_VOUT,
  BMC_DPV2_SENSOR_DPV2_1_12V_IOUT,
  BMC_DPV2_SENSOR_DPV2_1_EFUSE_TEMP,
  BMC_DPV2_SENSOR_DPV2_1_EFUSE_PWR,
};

const uint8_t bic_dpv2_x16_sensor_list[] = {
  BIC_DPV2_SENSOR_DPV2_2_12V_VIN,
  BIC_DPV2_SENSOR_DPV2_2_12V_VOUT,
  BIC_DPV2_SENSOR_DPV2_2_12V_IOUT,
  BIC_DPV2_SENSOR_DPV2_2_EFUSE_TEMP,
  BIC_DPV2_SENSOR_DPV2_2_EFUSE_PWR,
};

// BIC Rainbow Falls Sensors
const uint8_t bic_1ou_rf_sensor_list[] = {
  BIC_1OU_RF_INLET_TEMP,
  BIC_1OU_RF_CXL_CNTR_TEMP,
  BIC_1OU_RF_P0V9A_SPS_TEMP,
  BIC_1OU_RF_P0V8A_SPS_TEMP,
  BIC_1OU_RF_P0V8D_SPS_TEMP,
  BIC_1OU_RF_PVDDQ_AB_SPS_TEMP,
  BIC_1OU_RF_PVDDQ_CD_SPS_TEMP,
  BIC_1OU_RF_P12V_STBY_VOL,
  BIC_1OU_RF_P3V3_STBY_VOL,
  BIC_1OU_RF_P5V_STBY_VOL,
  BIC_1OU_RF_P1V2_STBY_VOL,
  BIC_1OU_RF_P1V8_ASIC_VOL,
  BIC_1OU_RF_P0V9_ASIC_A_VOL,
  BIC_1OU_RF_P0V8_ASIC_A_VOL,
  BIC_1OU_RF_P0V8_ASIC_D_VOL,
  BIC_1OU_RF_PVDDQ_AB_VOL,
  BIC_1OU_RF_PVDDQ_CD_VOL,
  BIC_1OU_RF_PVPP_AB_VOL,
  BIC_1OU_RF_PVPP_CD_VOL,
  BIC_1OU_RF_PVTT_AB_VOL,
  BIC_1OU_RF_PVTT_CD_VOL,
  BIC_1OU_RF_P12V_STBY_CUR,
  BIC_1OU_RF_P3V3_STBY_CUR,
  BIC_1OU_RF_P0V9A_VR_CUR,
  BIC_1OU_RF_P0V8A_VR_CUR,
  BIC_1OU_RF_P0V8D_VR_CUR,
  BIC_1OU_RF_PVDDQ_VR_CUR,
  BIC_1OU_RF_PVDDQV_VR_CUR,
  BIC_1OU_RF_P12V_STBY_PWR,
  BIC_1OU_RF_P3V3_STBY_PWR,
  BIC_1OU_RF_P0V9A_SPS_PWR,
  BIC_1OU_RF_P0V8A_SPS_PWR,
  BIC_1OU_RF_P0V8D_SPS_PWR,
  BIC_1OU_RF_PVDDQ_AB_SPS_PWR,
  BIC_1OU_RF_PVDDQ_CD_SPS_PWR,
};

const uint8_t bic_skip_sensor_list[] = {
  BIC_SENSOR_PCH_TEMP,
  BIC_SENSOR_CPU_TEMP,
  BIC_SENSOR_CPU_THERM_MARGIN,
  BIC_SENSOR_CPU_TJMAX,
  BIC_SENSOR_DIMMA0_TEMP,
  BIC_SENSOR_DIMMC0_TEMP,
  BIC_SENSOR_DIMMD0_TEMP,
  BIC_SENSOR_DIMME0_TEMP,
  BIC_SENSOR_DIMMG0_TEMP,
  BIC_SENSOR_DIMMH0_TEMP,
  BIC_SENSOR_M2A_TEMP,
  BIC_SENSOR_VCCIN_VR_TEMP,
  BIC_SENSOR_FIVRA_VR_TEMP,
  BIC_SENSOR_EHV_VR_TEMP,
  BIC_SENSOR_VCCD_VR_TEMP,
  BIC_SENSOR_FAON_VR_TEMP,
  //BIC - voltage sensors
  BIC_SENSOR_P12V_DIMM_VOL,
  BIC_SENSOR_P3V3_M2_VOL,
  BIC_SENSOR_VCCIN_VR_VOL,
  BIC_SENSOR_FIVRA_VR_VOL,
  BIC_SENSOR_EHV_VR_VOL,
  BIC_SENSOR_VCCD_VR_VOL,
  BIC_SENSOR_FAON_VR_VOL,
  //BIC - current sensors
  BIC_SENSOR_VCCIN_VR_CUR,
  BIC_SENSOR_FIVRA_VR_CUR,
  BIC_SENSOR_EHV_VR_CUR,
  BIC_SENSOR_VCCD_VR_CUR,
  BIC_SENSOR_FAON_VR_CUR,
  //BIC - power sensors
  BIC_SENSOR_CPU_PWR,
  BIC_SENSOR_VCCIN_VR_POUT,
  BIC_SENSOR_FIVRA_VR_POUT,
  BIC_SENSOR_EHV_VR_POUT,
  BIC_SENSOR_VCCD_VR_POUT,
  BIC_SENSOR_FAON_VR_POUT,
  BIC_SENSOR_DIMMA_PMIC_Pout,
  BIC_SENSOR_DIMMC_PMIC_Pout,
  BIC_SENSOR_DIMMD_PMIC_Pout,
  BIC_SENSOR_DIMME_PMIC_Pout,
  BIC_SENSOR_DIMMG_PMIC_Pout,
  BIC_SENSOR_DIMMH_PMIC_Pout,
  //BIC - DPv2 x16 sensors
  BIC_DPV2_SENSOR_DPV2_2_12V_VIN,
  BIC_DPV2_SENSOR_DPV2_2_12V_VOUT,
  BIC_DPV2_SENSOR_DPV2_2_12V_IOUT,
  BIC_DPV2_SENSOR_DPV2_2_EFUSE_TEMP,
  BIC_DPV2_SENSOR_DPV2_2_EFUSE_PWR,
};


const uint8_t bic_hd_skip_sensor_list[] = {
  BIC_HD_SENSOR_CPU_TEMP,
  BIC_HD_SENSOR_DIMMA_TEMP,
  BIC_HD_SENSOR_DIMMB_TEMP,
  BIC_HD_SENSOR_DIMMC_TEMP,
  BIC_HD_SENSOR_DIMME_TEMP,
  BIC_HD_SENSOR_DIMMG_TEMP,
  BIC_HD_SENSOR_DIMMH_TEMP,
  BIC_HD_SENSOR_DIMMI_TEMP,
  BIC_HD_SENSOR_DIMMK_TEMP,
  BIC_HD_SENSOR_SSD_TEMP,
  BIC_HD_SENSOR_CPU0_VR_TEMP,
  BIC_HD_SENSOR_SOC_VR_TEMP,
  BIC_HD_SENSOR_CPU1_VR_TEMP,
  BIC_HD_SENSOR_PVDDIO_VR_TEMP,
  BIC_HD_SENSOR_PVDD11_VR_TEMP,
  //BIC - Halfdome voltage sensors
  BIC_HD_SENSOR_PVDD18_S5_VOL,
  BIC_HD_SENSOR_PVDD11_S3_VOL,
  BIC_HD_SENSOR_PVDD33_S5_VOL,
  BIC_HD_SENSOR_P12V_MEM_1_VOL,
  BIC_HD_SENSOR_P12V_MEM_0_VOL,
  BIC_HD_SENSOR_P3V3_M2_VOL,
  BIC_HD_SENSOR_CPU0_VR_VOL,
  BIC_HD_SENSOR_SOC_VR_VOL,
  BIC_HD_SENSOR_CPU1_VR_VOL,
  BIC_HD_SENSOR_PVDDIO_VR_VOL,
  BIC_HD_SENSOR_PVDD11_VR_VOL,
  //BIC - Halfdome current sensors
  BIC_HD_SENSOR_CPU0_VR_CUR,
  BIC_HD_SENSOR_SOC_VR_CUR,
  BIC_HD_SENSOR_CPU1_VR_CUR,
  BIC_HD_SENSOR_PVDDIO_VR_CUR,
  BIC_HD_SENSOR_PVDD11_VR_CUR,
  //BIC - Halfdome power sensors
  BIC_HD_SENSOR_CPU0_VR_PWR,
  BIC_HD_SENSOR_SOC_VR_PWR,
  BIC_HD_SENSOR_CPU1_VR_PWR,
  BIC_HD_SENSOR_PVDDIO_VR_PWR,
  BIC_HD_SENSOR_PVDD11_VR_PWR,
  BIC_HD_SENSOR_CPU_PWR,
  BIC_HD_SENSOR_DIMMA_PWR,
  BIC_HD_SENSOR_DIMMB_PWR,
  BIC_HD_SENSOR_DIMMC_PWR,
  BIC_HD_SENSOR_DIMME_PWR,
  BIC_HD_SENSOR_DIMMG_PWR,
  BIC_HD_SENSOR_DIMMH_PWR,
  BIC_HD_SENSOR_DIMMI_PWR,
  BIC_HD_SENSOR_DIMMK_PWR,
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

const uint8_t bic_1ou_vf_skip_sensor_list[] = {
  BIC_1OU_VF_SENSOR_NUM_INA231_PWR_M2A,
  BIC_1OU_VF_SENSOR_NUM_INA231_VOL_M2A,
  BIC_1OU_VF_SENSOR_NUM_NVME_TEMP_M2A,
  BIC_1OU_VF_SENSOR_NUM_ADC_3V3_VOL_M2A,
  BIC_1OU_VF_SENSOR_NUM_ADC_12V_VOL_M2A,
  BIC_1OU_VF_SENSOR_NUM_INA231_PWR_M2B,
  BIC_1OU_VF_SENSOR_NUM_INA231_VOL_M2B,
  BIC_1OU_VF_SENSOR_NUM_NVME_TEMP_M2B,
  BIC_1OU_VF_SENSOR_NUM_ADC_3V3_VOL_M2B,
  BIC_1OU_VF_SENSOR_NUM_ADC_12V_VOL_M2B,
  BIC_1OU_VF_SENSOR_NUM_INA231_PWR_M2C,
  BIC_1OU_VF_SENSOR_NUM_INA231_VOL_M2C,
  BIC_1OU_VF_SENSOR_NUM_NVME_TEMP_M2C,
  BIC_1OU_VF_SENSOR_NUM_ADC_3V3_VOL_M2C,
  BIC_1OU_VF_SENSOR_NUM_ADC_12V_VOL_M2C,
  BIC_1OU_VF_SENSOR_NUM_INA231_PWR_M2D,
  BIC_1OU_VF_SENSOR_NUM_INA231_VOL_M2D,
  BIC_1OU_VF_SENSOR_NUM_NVME_TEMP_M2D,
  BIC_1OU_VF_SENSOR_NUM_ADC_3V3_VOL_M2D,
  BIC_1OU_VF_SENSOR_NUM_ADC_12V_VOL_M2D,
};

const uint8_t nic_sensor_list[] = {
  NIC_SENSOR_TEMP,
};

// List of MB discrete sensors to be monitored
const uint8_t bmc_discrete_sensor_list[] = {
};

const uint8_t delta_pdb_sensor_list[] = {
  BMC_SENSOR_VPDB_DELTA_1_TEMP,
  BMC_SENSOR_VPDB_DELTA_2_TEMP,
  BMC_SENSOR_PDB_48V_DELTA_1_VIN,
  BMC_SENSOR_PDB_48V_DELTA_2_VIN,
  BMC_SENSOR_PDB_12V_DELTA_1_VOUT,
  BMC_SENSOR_PDB_12V_DELTA_2_VOUT,
  BMC_SENSOR_PDB_DELTA_1_IOUT,
  BMC_SENSOR_PDB_DELTA_2_IOUT,
};

const uint8_t rns_pdb_sensor_list[] = {
  BMC_SENSOR_VPDB_WW_TEMP,
  BMC_SENSOR_PDB_48V_WW_VIN,
  BMC_SENSOR_PDB_12V_WW_VOUT,
  BMC_SENSOR_PDB_12V_WW_IOUT,
  BMC_SENSOR_PDB_12V_WW_PWR,
};

/*  PAL_ATTR_INFO:
 *  For the PMBus direct format conversion
 *  X = 1/m × (Y × 10^-R − b)
 *
 *  {m, b, R} coefficient
 */
//LTC4286
PAL_ATTR_INFO ltc4286_info_list[] = {
  [HSC_VOLTAGE] = {320, 0, 1},
  [HSC_CURRENT] = {1024, 0, 1},
  [HSC_POWER] = {100, 0, 10},
  [HSC_TEMP] = {1, 273, 1},
};

//ADM1278
PAL_ATTR_INFO adm1278_info_list[] = {
  [HSC_VOLTAGE] = {19599, 0, 100},
  [HSC_CURRENT] = {800 * ADM1278_RSENSE, 20475, 10},
  [HSC_POWER] = {6123 * ADM1278_RSENSE, 0, 100},
  [HSC_TEMP] = {42, 31880, 10},
};

//MP5990
PAL_ATTR_INFO mp5990_info_list[] = {
  [HSC_VOLTAGE] = {32, 0, 1},
  [HSC_CURRENT] = {16, 0, 1},
  [HSC_POWER] = {1, 0, 1},
  [HSC_TEMP] = {1, 0, 1},
};

//Delta
PAL_PDB_COEFFICIENT delta_pdb_info_list[] = {
  [PDB_VOLTAGE_OUT] = {0.00024414},
  [PDB_VOLTAGE_IN] = {0.125},
  [PDB_CURRENT] = {0.25},
  [PDB_TEMP] = {0.25},
};

//RNS
PAL_PDB_COEFFICIENT rns_pdb_info_list[] = {
  [PDB_VOLTAGE_OUT] = {0.005},
  [PDB_VOLTAGE_IN] = {0.1},
  [PDB_CURRENT] = {0.1},
  [PDB_TEMP] = {1},
  [PDB_POWER] = {0.2},
};

PAL_PMBUS_INFO dpv2_efuse_info_list[] = {
  //(offset, m, b, r}
  [DPV2_EFUSE_VIN]  = {0x88,                     7578,     0, -2},
  [DPV2_EFUSE_VOUT] = {0x8B,                     7578,     0, -2},
  [DPV2_EFUSE_IOUT] = {0x8C, 3.824 * DPV2_EFUSE_RLOAD, -4300, -3},
  [DPV2_EFUSE_TEMP] = {0x8D,                      199,  7046, -2},
  [DPV2_EFUSE_PIN]  = {0x97, 0.895 * DPV2_EFUSE_RLOAD, -9100, -2},
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
  //{      SensorName,              ID, FUNCTION, PWR_STATUS, {LNR, LCR, LNC,   UNC,   UCR,   UNR, Pos, Neg}, Unit}
  {"BB_DPV2_1_12V_INPUT_VOLT_V" , DPV2_EFUSE_VIN , read_dpv2_efuse, 0, {10, 10.8, 11.04, 12.96,  13.2,    14, 0, 0}, VOLT}, //0x8C
  {"BB_DPV2_1_12V_OUTPUT_VOLT_V", DPV2_EFUSE_VOUT, read_dpv2_efuse, 0, {10, 10.8, 11.04, 12.96,  13.2,    14, 0, 0}, VOLT}, //0x8D
  {"BB_DPV2_1_12V_OUTPUT_CURR_A", DPV2_EFUSE_IOUT, read_dpv2_efuse, 0, { 0,    0,     0, 15.17, 15.47,  18.9, 0, 0}, CURR}, //0x8E
  {"BB_DPV2_1_EFUSE_TEMP_C"     , DPV2_EFUSE_TEMP, read_dpv2_efuse, 0, { 0,    0,     0,     0,   100,   150, 0, 0}, TEMP}, //0x8F

  {"BB_DPV2_1_EFUSE_PWR_W" , DPV2_EFUSE_PIN , read_dpv2_efuse, 0, { 0,    0,     0,     0,   186, 226.8, 0, 0}, POWER}, //0x90
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
//{                     SensorName,          ID, FUNCTION, PWR_STATUS, {   LNR,    LCR,   LNC,    UNC,    UCR,     UNR,  Pos, Neg}, Unit}
  {"PDB_VPDB_DELTA1_TEMP_C"       , PDB_DELTA_1, read_pdb_temp,  true, {     0,      0,     0,      0,    115,     130,    0,   0}, TEMP}, //0xB0
  {"PDB_VPDB_DELTA2_TEMP_C"       , PDB_DELTA_2, read_pdb_temp,  true, {     0,      0,     0,      0,    115,     130,    0,   0}, TEMP}, //0xB1
  {"PDB_VPDB_WW_TEMP_C"           , PDB_RNS    , read_pdb_temp,  true, {     0,      0,     0,      0,    100,     115,    0,   0}, TEMP}, //0XB2
  {"PDB_48V_DELTA1_INPUT_VOLT_V"  , PDB_DELTA_1, read_pdb_vin ,  true, {    36,  42.72,  43.2,   56.1,  56.61,      62,    0,   0}, VOLT}, //0xB3
  {"PDB_48V_DELTA2_INPUT_VOLT_V"  , PDB_DELTA_2, read_pdb_vin ,  true, {    36,  42.72,  43.2,   56.1,  56.61,      62,    0,   0}, VOLT}, //0xB4
  {"PDB_48V_WW_INPUT_VOLT_V"      , PDB_RNS    , read_pdb_vin ,  true, {  38.5,  42.72,  43.2,   56.1,  56.61,    61.5,    0,   0}, VOLT}, //0xB5
  {"PDB_12V_DELTA1_OUTPUT_VOLT_V" , PDB_DELTA_1, read_pdb_vout,  true, { 8.162, 11.125, 11.25,  13.75, 13.875,      15,    0,   0}, VOLT}, //0xB6
  {"PDB_12V_DELTA2_OUTPUT_VOLT_V" , PDB_DELTA_2, read_pdb_vout,  true, { 8.162, 11.125, 11.25,  13.75, 13.875,      15,    0,   0}, VOLT}, //0xB7
  {"PDB_12V_WW_OUTPUT_VOLT_V"     , PDB_RNS    , read_pdb_vout,  true, {    10, 11.125, 11.25,  13.75, 13.875,    13.6,    0,   0}, VOLT}, //0xB8
  {"PDB_DELTA1_OUTPUT_CURR_A"     , PDB_DELTA_1, read_pdb_iout,  true, {     0,      0,     0,      0,    100,       0,    0,   0}, CURR}, //0xB9
  {"PDB_DELTA2_OUTPUT_CURR_A"     , PDB_DELTA_2, read_pdb_iout,  true, {     0,      0,     0,      0,    136,       0,    0,   0}, CURR}, //0xBA
  {"PDB_12V_WW_OUTPUT_CURR_A"     , PDB_RNS    , read_pdb_iout,  true, {     0,      0,     0,      0,    144,       0,    0,   0}, CURR}, //0xBB
  {"PDB_12V_WW_PWR_W"             , PDB_RNS    , read_pdb_pout,  true, {     0,      0,     0,      0,   1800,       0,    0,   0}, POWER}, //0xBC
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
//{                      SensorName,      ID,     FUNCTION, PWR_STATUS, {   LNR,    LCR,   LNC,    UNC,    UCR,     UNR,  Pos, Neg}, Unit}
  {"BB_HSC_PEAK_OUTPUT_CURR_A"     , HSC_ID0, read_hsc_peak_iout,    0, {     0,      0,     0,      0,      0,       0,    0,   0}, CURR}, //0xC8
  {"BB_HSC_PEAK_INPUT_PWR_W"       , HSC_ID0, read_hsc_peak_pin ,    0, {     0,      0,     0,      0,      0,       0,    0,   0}, POWER}, //0xC9
  {"BB_FAN_PWR_W"                  ,    0xCA, read_cached_val   , true, {     0,      0,     0,      0,201.465,  544.88,    0,   0}, POWER}, //0xCA
  {NULL, 0, NULL, 0, {0, 0, 0, 0, 0, 0, 0, 0}, 0}, //0xCB
  {"BB_CPU_CL_VDELTA_VOLT_V"       ,    0xCC, read_pdb_cl_vdelta, true, {     0,      0,     0,      0,    0.9,       0,    0,   0}, VOLT}, //0xCC
  {"BB_PMON_CURRENT_LEAKAGE_CURR_A",    0xCD, read_curr_leakage , true, {     0,      0,     0,      0,      0,       0,    0,   0}, PERCENT}, //0xCD
  {"BB_CPU_PDB_VDELTA_VOLT_V"      ,    0xCE, read_cached_val   , true, {     0,      0,     0,      0,    0.8,       0,    0,   0}, VOLT}, //0xCE
  {"BB_MEDUSA_VDELTA_VOLT_V"       ,    0xCF, read_medusa_val   , true, {     0,      0,     0,      0,    0.5,       0,    0,   0}, VOLT}, //0xCF
  {"BB_MEDUSA_CURR_A"              ,    0xD0, read_medusa_val   ,    0, {     0,      0,     0,      0,    144,       0,    0,   0}, CURR}, //0xD0
  {"BB_MEDUSA_PWR_W"               ,    0xD1, read_medusa_val   ,    0, {     0,      0,     0,      0,   1800,       0,    0,   0}, POWER}, //0xD1
  {"BB_ADC_NIC_P12V_VOLT_V"        ,   ADC12, read_adc_val      , true, { 10.17,  10.68,  10.8,   13.2,  13.32,   14.91,    0,   0}, VOLT},//0xD2
  {"BB_NIC_PWR_W"                  ,    0xD3, read_cached_val   , true, {     0,      0,     0,      0,   82.5, 101.875,    0,   0}, POWER},//0xD3
  {"BB_ADC_P1V0_STBY_VOLT_V"       ,    ADC7, read_adc_val      , true, {  0.83,   0.92,  0.93,   1.07,   1.08,    1.13,    0,   0}, VOLT}, //0xD4
  {"BB_ADC_P0V6_STBY_VOLT_V"       ,    ADC8, read_adc_val      , true, {     0,  0.552, 0.558,  0.642,  0.648,       0,    0,   0}, VOLT}, //0xD5
  {"BB_ADC_P3V3_RGM_STBY_VOLT_V"   ,   ADC13, read_adc_val      , true, {     0,  3.036, 3.069,  3.531,  3.564,       0,    0,   0}, VOLT}, //0xD6
  {"BB_ADC_P5V_USB_VOLT_V"         ,    ADC4, read_adc_val      ,    0, {  4.15,    4.6,  4.65,   5.35,    5.4,    5.65,    0,   0}, VOLT}, //0xD7
  {"BB_ADC_P3V3_NIC_VOLT_V"        ,   ADC14, read_adc_val      ,    0, {  2.95,   2.97, 3.003,  3.597,  3.630,   3.729,    0,   0}, VOLT}, //0xD8
  {NULL, 0, NULL, 0, {0, 0, 0, 0, 0, 0, 0, 0}, 0}, //0xD9
  {NULL, 0, NULL, 0, {0, 0, 0, 0, 0, 0, 0, 0}, 0}, //0xDA
  {NULL, 0, NULL, 0, {0, 0, 0, 0, 0, 0, 0, 0}, 0}, //0xDB
  {NULL, 0, NULL, 0, {0, 0, 0, 0, 0, 0, 0, 0}, 0}, //0xDC
  {NULL, 0, NULL, 0, {0, 0, 0, 0, 0, 0, 0, 0}, 0}, //0xDD
  {NULL, 0, NULL, 0, {0, 0, 0, 0, 0, 0, 0, 0}, 0}, //0xDE
  {NULL, 0, NULL, 0, {0, 0, 0, 0, 0, 0, 0, 0}, 0}, //0xDF
//{              SensorName,          ID,  FUNCTION, PWR_STATUS, {    LNR,    LCR,    LNC,   UNC,   UCR,    UNR, Pos, Neg}, Unit}
  {"BB_FAN0_TACH_RPM",        0xE0, read_fan_speed , true, {      0,    500,      0,  8500, 11500,      0,   0,   0}, FAN}, //0xE0
  {"BB_FAN1_TACH_RPM",        0xE1, read_fan_speed , true, {      0,    500,      0,  8500, 11500,      0,   0,   0}, FAN}, //0xE1
  {"BB_FAN2_TACH_RPM",        0xE2, read_fan_speed , true, {      0,    500,      0,  8500, 11500,      0,   0,   0}, FAN}, //0xE2
  {"BB_FAN3_TACH_RPM",        0xE3, read_fan_speed , true, {      0,    500,      0,  8500, 11500,      0,   0,   0}, FAN}, //0xE3
  {"BB_FAN4_TACH_RPM",        0xE4, read_fan_speed , true, {      0,    500,      0,  8500, 11500,      0,   0,   0}, FAN}, //0xE4
  {"BB_FAN5_TACH_RPM",        0xE5, read_fan_speed , true, {      0,    500,      0,  8500, 11500,      0,   0,   0}, FAN}, //0xE5
  {"BB_FAN6_TACH_RPM",        0xE6, read_fan_speed , true, {      0,    500,      0,  8500, 11500,      0,   0,   0}, FAN}, //0xE6
  {"BB_FAN7_TACH_RPM",        0xE7, read_fan_speed , true, {      0,    500,      0,  8500, 11500,      0,   0,   0}, FAN}, //0xE7
  {"BB_PWM0_TACH_PCT",       PWM_0, read_fan_pwm   , true, {      0,      0,      0,     0,     0,      0,   0,   0}, PERCENT}, //0xE8
  {"BB_PWM1_TACH_PCT",       PWM_1, read_fan_pwm   , true, {      0,      0,      0,     0,     0,      0,   0,   0}, PERCENT}, //0xE9
  {"BB_PWM2_TACH_PCT",       PWM_2, read_fan_pwm   , true, {      0,      0,      0,     0,     0,      0,   0,   0}, PERCENT}, //0xEA
  {"BB_PWM3_TACH_PCT",       PWM_3, read_fan_pwm   , true, {      0,      0,      0,     0,     0,      0,   0,   0}, PERCENT}, //0xEB
  {NULL, 0, NULL, 0, {0, 0, 0, 0, 0, 0, 0, 0}, 0}, //0xEC
  {"BB_INLET_TEMP_C"         ,  TEMP_INLET, read_temp      , true, {      0,      0,      0,     0,    50,    150,   0,   0}, TEMP}, //0xED
  {"BB_OUTLET_TEMP_C"        , TEMP_OUTLET, read_temp      , true, {      0,      0,      0,     0,    55,    150,   0,   0}, TEMP}, //0xEE
  {"NIC_TEMP_C"              ,    TEMP_NIC, read_temp      , true, {      0,      0,      0,     0,   105,    120,   0,   0}, TEMP}, //0xEF
  {"BB_P5V_VOLT_V"           ,        ADC0, read_adc_val   , true, {   4.15,   4.45,    4.5,   5.5,  5.55,   5.65,   0,   0}, VOLT}, //0xF0
  {"BB_P12V_VOLT_V"          ,        ADC1, read_adc_val   , true, { 10.091,  10.68,   10.8,  13.2, 13.32, 14.333,   0,   0}, VOLT}, //0xF1
  {"BB_P3V3_STBY_VOLT_V"     ,        ADC2, read_adc_val   , true, {  2.739,  3.036,  3.069, 3.531, 3.564,  3.729,   0,   0}, VOLT}, //0xF2
  {"BB_P1V8_STBY_VOLT_V"     ,        ADC5, read_adc_val   , true, {      0,  1.656,  1.674, 1.926, 1.944,      0,   0,   0}, VOLT}, //0xF3
  {"BB_P1V2_STBY_VOLT_V"     ,        ADC6, read_adc_val   , true, {  0.996,  1.104,  1.116, 1.284, 1.296,  1.356,   0,   0}, VOLT}, //0xF4
  {"BB_P2V5_STBY_VOLT_V"     ,        ADC3, read_adc_val   , true, {      0,    2.3,  2.325, 2.675,   2.7,      0,   0,   0}, VOLT}, //0xF5
  {"BB_MEDUSA_OUTPUT_VOLT_V" ,        0xF6, read_medusa_val, true, {   9.25, 11.125,  11.25, 13.75,13.875,   13.9,   0,   0}, VOLT}, //0xF6
  {"BB_HSC_INPUT_VOLT_V"     ,     HSC_ID0, read_hsc_vin   , true, { 10.091,  10.68,   10.8, 13.20, 13.32, 14.333,   0,   0}, VOLT}, //0xF7
  {"BB_HSC_TEMP_C"           ,     HSC_ID0, read_hsc_temp  , true, {      0,      0,      0,     0,    55,    125,   0,   0}, TEMP}, //0xF8
  {"BB_HSC_INPUT_PWR_W"      ,     HSC_ID0, read_hsc_pin   , true, {      0,      0,      0,     0, 287.5, 398.75,   0,   0}, POWER}, //0xF9
  {"BB_HSC_OUTPUT_CURR_A"    ,     HSC_ID0, read_hsc_iout  , true, {      0,      0,      0,     0,    23,   31.9,   0,   0}, CURR}, //0xFA
  {"BB_ADC_FAN_OUTPUT_CURR_A",       ADC10, read_adc_val   ,    0, {      0,      0,      0,     0, 14.52,   39.2,   0,   0}, CURR}, //0xFB
  {"BB_ADC_NIC_OUTPUT_CURR_A",       ADC11, read_adc_val   ,    0, {      0,      0,      0,     0,   6.6,   8.15,   0,   0}, CURR}, //0xFC
  {"BB_MEDUSA_INPUT_VOLT_V"  ,        0xFD, read_medusa_val, true, {   9.25, 11.125,  11.25, 13.75,13.875,   13.9,   0,   0}, VOLT}, //0xFD
  {"NICEXP_TEMP_C"           ,TEMP_NICEXP , read_temp      , true, {      0,      0,      0,     0,    50,    150,   0,   0}, TEMP}, //0xFE
  {NULL, 0, NULL, 0, {0, 0, 0, 0, 0, 0, 0, 0}, 0}, //0xFF
};

/*PAL_HSC_INFO = {SLAVE_ADDR, PEAK_IOUT_command_code, PEAK_PIN_command_code, "M B R" coefficent info}
*/
PAL_HSC_INFO hsc_info_list[] = {
  [HSC_ADM1278] = {ADM1278_SLAVE_ADDR, ADM1278_PEAK_IOUT, ADM1278_PEAK_PIN, adm1278_info_list},
  [HSC_LTC4286] = {LTC4286_SLAVE_ADDR, LTC4286_MFR_IOUT_MAX, LTC4286_MFR_PIN_MAX, ltc4286_info_list},
  [HSC_MP5990] =  {MP5990_SLAVE_ADDR, MP5990_PEAK_IOUT, MP5990_PEAK_PIN, mp5990_info_list}
};

PAL_CHIP_INFO medusa_hsc_list[] = {
  {"ltc4282", "ltc4282", MEDUSA_HSC_LTC4282_ADDR},
  {"adm1272", "adm1272", MEDUSA_HSC_ADM1272_ADDR}
};

PAL_PDB_INFO pdb_info_list[] = {
  [PDB_DELTA_1] = {DELTA_1_PDB_ADDR, delta_pdb_info_list},
  [PDB_DELTA_2] = {DELTA_2_PDB_ADDR, delta_pdb_info_list},
  [PDB_RNS] = {RNS_PDB_ADDR, rns_pdb_info_list}
};

size_t bmc_sensor_cnt = sizeof(bmc_sensor_list)/sizeof(uint8_t);
size_t nicexp_sensor_cnt = sizeof(nicexp_sensor_list)/sizeof(uint8_t);
size_t nic_sensor_cnt = sizeof(nic_sensor_list)/sizeof(uint8_t);
size_t bic_sensor_cnt = sizeof(bic_sensor_list)/sizeof(uint8_t);
size_t bic_hd_sensor_cnt = sizeof(bic_hd_sensor_list)/sizeof(uint8_t);
size_t bic_2ou_sensor_cnt = sizeof(bic_2ou_sensor_list)/sizeof(uint8_t);
size_t bic_bb_sensor_cnt = sizeof(bic_bb_sensor_list)/sizeof(uint8_t);
size_t bic_1ou_vf_sensor_cnt = sizeof(bic_1ou_vf_sensor_list)/sizeof(uint8_t);
size_t bic_2ou_gpv3_sensor_cnt = sizeof(bic_2ou_gpv3_sensor_list)/sizeof(uint8_t);
size_t bic_spe_sensor_cnt = sizeof(bic_spe_sensor_list)/sizeof(uint8_t);
size_t bic_1ou_rf_sensor_cnt = sizeof(bic_1ou_rf_sensor_list)/sizeof(uint8_t);
size_t bic_skip_sensor_cnt = sizeof(bic_skip_sensor_list)/sizeof(uint8_t);
size_t bic_hd_skip_sensor_cnt = sizeof(bic_hd_skip_sensor_list)/sizeof(uint8_t);
size_t bic_2ou_skip_sensor_cnt = sizeof(bic_2ou_skip_sensor_list)/sizeof(uint8_t);
size_t bic_2ou_gpv3_skip_sensor_cnt = sizeof(bic_2ou_gpv3_skip_sensor_list)/sizeof(uint8_t);
size_t bic_1ou_vf_skip_sensor_cnt = sizeof(bic_1ou_vf_skip_sensor_list)/sizeof(uint8_t);
size_t bmc_dpv2_x8_sensor_cnt = sizeof(bmc_dpv2_x8_sensor_list)/sizeof(uint8_t);
size_t bic_dpv2_x16_sensor_cnt = sizeof(bic_dpv2_x16_sensor_list)/sizeof(uint8_t);
size_t rns_pdb_sensor_cnt = sizeof(rns_pdb_sensor_list)/sizeof(uint8_t);
size_t delta_pdb_sensor_cnt = sizeof(delta_pdb_sensor_list)/sizeof(uint8_t);

static int compare(const void *arg1, const void *arg2) {
  return(*(int *)arg2 - *(int *)arg1);
}

int
read_device(const char *device, float *value) {
  FILE *fp = NULL;

  if (device == NULL || value == NULL) {
    syslog(LOG_ERR, "%s: Invalid parameter", __func__);
    return -1;
  }

  fp = fopen(device, "r");
  if (fp == NULL) {
#ifdef DEBUG
    syslog(LOG_INFO, "%s failed to open device %s error: %s", __func__, device, strerror(errno));
#endif
    return -1;
  }

  if (fscanf(fp, "%f", value) != 1) {
    syslog(LOG_INFO, "%s failed to read device %s error: %s", __func__, device, strerror(errno));
    fclose(fp);
    return -1;
  }
  fclose(fp);

  return 0;
}

int
get_skip_sensor_list(uint8_t fru, uint8_t **skip_sensor_list, int *cnt, const uint8_t bmc_location, const uint8_t config_status) {
  uint8_t type = TYPE_1OU_UNKNOWN;
  uint8_t type_2ou = UNKNOWN_BOARD;
  uint8_t count;
  static uint8_t skip_count[MAX_NODES] = {0};

  if (bic_dynamic_skip_sensor_list[fru-1][0] == 0) {

    if (fby35_common_get_slot_type(fru) == SERVER_TYPE_HD) {
      memcpy(bic_dynamic_skip_sensor_list[fru-1], bic_hd_skip_sensor_list, bic_hd_skip_sensor_cnt);
      count = bic_hd_skip_sensor_cnt;
    } else {
      memcpy(bic_dynamic_skip_sensor_list[fru-1], bic_skip_sensor_list, bic_skip_sensor_cnt);
      count = bic_skip_sensor_cnt;
    }

    //if 1OU board doesn't exist, use the default 1ou skip list.
    if ((bmc_location != NIC_BMC) && ((config_status & PRESENT_1OU) == PRESENT_1OU) &&
        (bic_get_1ou_type(fru, &type) == 0)) {
      if (type == TYPE_1OU_VERNAL_FALLS_WITH_AST) {
        memcpy(&bic_dynamic_skip_sensor_list[fru-1][count], bic_1ou_vf_skip_sensor_list, bic_1ou_vf_skip_sensor_cnt);
        count += bic_1ou_vf_skip_sensor_cnt;
      }
    }

    if (((config_status & PRESENT_2OU) == PRESENT_2OU) &&
        (fby35_common_get_2ou_board_type(fru, &type_2ou) == 0)) {
      if (type_2ou == GPV3_MCHP_BOARD || type_2ou == GPV3_BRCM_BOARD) {
        memcpy(&bic_dynamic_skip_sensor_list[fru-1][count], bic_2ou_gpv3_skip_sensor_list, bic_2ou_gpv3_skip_sensor_cnt);
        count += bic_2ou_gpv3_skip_sensor_cnt;
      } else {
        memcpy(&bic_dynamic_skip_sensor_list[fru-1][count], bic_2ou_skip_sensor_list, bic_2ou_skip_sensor_cnt);
        count += bic_2ou_skip_sensor_cnt;
      }
    }

    skip_count[fru-1] = count;
  }

  *skip_sensor_list = (uint8_t *)bic_dynamic_skip_sensor_list[fru-1];
  *cnt = skip_count[fru-1];

  return 0;
}

int
pal_get_pdb_sensor_list(size_t *current_bmc_cnt) {
  const uint8_t pdb_bus = 11;

  if (current_bmc_cnt == NULL) {
    return -1;
  }

  if (i2c_detect_device(pdb_bus, RNS_PDB_ADDR >> 1) == 0) { // RNS PDB
    if ((*current_bmc_cnt + rns_pdb_sensor_cnt) > MAX_SENSOR_NUM) {
      syslog(LOG_ERR, "%s() current_bmc_cnt exceeds the limit of max sensor number", __func__);
      return -1;
    }
    memcpy(&bmc_dynamic_sensor_list[*current_bmc_cnt], rns_pdb_sensor_list, rns_pdb_sensor_cnt);
    *current_bmc_cnt += rns_pdb_sensor_cnt;
    return 0;
  } else if ((i2c_detect_device(pdb_bus, DELTA_1_PDB_ADDR >> 1) == 0) || (i2c_detect_device(pdb_bus, DELTA_2_PDB_ADDR >> 1) == 0)) {
    if ((*current_bmc_cnt + delta_pdb_sensor_cnt) > MAX_SENSOR_NUM) {
      syslog(LOG_ERR, "%s() current_bmc_cnt exceeds the limit of max sensor number", __func__);
      return -1;
    }
    memcpy(&bmc_dynamic_sensor_list[*current_bmc_cnt], delta_pdb_sensor_list, delta_pdb_sensor_cnt);
    *current_bmc_cnt += delta_pdb_sensor_cnt;
    return 0;
  } else {
    syslog(LOG_ERR, "%s() Fail to identify the module of PDB", __func__);
    return -1;
  }
}

int
pal_get_fru_sensor_list(uint8_t fru, uint8_t **sensor_list, int *cnt) {
  int ret = 0, config_status = 0;
  uint8_t bmc_location = 0, type = TYPE_1OU_UNKNOWN;
  uint8_t board_type = 0;
  uint8_t current_cnt = 0;
  size_t current_bmc_cnt = 0;
  char key[MAX_KEY_LEN] = {0};
  char hsc_type[MAX_VALUE_LEN] = {0};
  bool is_48v_medusa = false;

  ret = fby35_common_get_bmc_location(&bmc_location);
  if (ret < 0) {
    syslog(LOG_ERR, "%s() Cannot get the location of BMC", __func__);
  }

  switch(fru) {
  case FRU_BMC:
    if (bmc_location == NIC_BMC) {
      *sensor_list = (uint8_t *) nicexp_sensor_list;
      *cnt = nicexp_sensor_cnt;
    } else {
      memcpy(bmc_dynamic_sensor_list, bmc_sensor_list, bmc_sensor_cnt);
      current_bmc_cnt = bmc_sensor_cnt;
      snprintf(key, sizeof(key), "bb_hsc_conf");
      if (kv_get(key, hsc_type, NULL, KV_FPERSIST) < 0) {
        syslog(LOG_WARNING, "%s() Cannot get the key bb_hsc_conf", __func__);
      } else {
        is_48v_medusa = strncmp(hsc_type, "ltc4282", sizeof(hsc_type)) ? true : false;
      }
      if (is_48v_medusa) { // if is 48V medusa, need to add PDB sensor list
        ret = pal_get_pdb_sensor_list(&current_bmc_cnt);
        if (ret < 0) {
          syslog(LOG_ERR, "%s() Cannot get the PDB sensor list", __func__);
        }
      }

      *sensor_list = (uint8_t *) bmc_dynamic_sensor_list;
      *cnt = current_bmc_cnt;
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
    if (fby35_common_get_slot_type(fru) == SERVER_TYPE_HD) {
      memcpy(bic_dynamic_sensor_list[fru-1], bic_hd_sensor_list, bic_hd_sensor_cnt);
      current_cnt = bic_hd_sensor_cnt;
    } else {
      memcpy(bic_dynamic_sensor_list[fru-1], bic_sensor_list, bic_sensor_cnt);
      current_cnt = bic_sensor_cnt;
    }
    config_status = bic_is_exp_prsnt(fru);
    if (config_status < 0) config_status = 0;

    // 1OU
    if ( (bmc_location == BB_BMC) && ((config_status & PRESENT_1OU) == PRESENT_1OU) ) {
      if (bic_get_1ou_type(fru, &type) == 0) {
        switch (type) {
          case TYPE_1OU_VERNAL_FALLS_WITH_AST:
            memcpy(&bic_dynamic_sensor_list[fru-1][current_cnt], bic_1ou_vf_sensor_list, bic_1ou_vf_sensor_cnt);
            current_cnt += bic_1ou_vf_sensor_cnt;
            break;
          case TYPE_1OU_RAINBOW_FALLS:
            memcpy(&bic_dynamic_sensor_list[fru-1][current_cnt], bic_1ou_rf_sensor_list, bic_1ou_rf_sensor_cnt);
            current_cnt += bic_1ou_rf_sensor_cnt;
            break;
          default:
            syslog(LOG_ERR, "%s() Cannot identify the 1OU board type or 1OU board is not present", __func__);
            break;
        }
      } else {
        syslog(LOG_ERR, "%s() Cannot get board_type and card_type", __func__);
      }
    }

    // 2OU
    if ( (config_status & PRESENT_2OU) == PRESENT_2OU ) {
      ret = fby35_common_get_2ou_board_type(fru, &board_type);
      if (ret < 0) {
        syslog(LOG_ERR, "%s() Cannot get board_type", __func__);
      }
      if (board_type == E1S_BOARD) { // Sierra point expansion
        memcpy(&bic_dynamic_sensor_list[fru-1][current_cnt], bic_spe_sensor_list, bic_spe_sensor_cnt);
        current_cnt += bic_spe_sensor_cnt;
      } else if (board_type == GPV3_MCHP_BOARD || board_type == GPV3_BRCM_BOARD) {
        memcpy(&bic_dynamic_sensor_list[fru-1][current_cnt], bic_2ou_gpv3_sensor_list, bic_2ou_gpv3_sensor_cnt);
        current_cnt += bic_2ou_gpv3_sensor_cnt;
      } else if ((board_type & DPV2_X8_BOARD) == DPV2_X8_BOARD || (board_type & DPV2_X16_BOARD) == DPV2_X16_BOARD) {
        if ((board_type & DPV2_X8_BOARD) == DPV2_X8_BOARD) {
          memcpy(&bic_dynamic_sensor_list[fru-1][current_cnt], bmc_dpv2_x8_sensor_list, bmc_dpv2_x8_sensor_cnt);
          current_cnt += bmc_dpv2_x8_sensor_cnt;
        }
        if ((board_type & DPV2_X16_BOARD) == DPV2_X16_BOARD) {
          memcpy(&bic_dynamic_sensor_list[fru-1][current_cnt], bic_dpv2_x16_sensor_list, bic_dpv2_x16_sensor_cnt);
          current_cnt += bic_dpv2_x16_sensor_cnt;
        }
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

  ret = fby35_common_get_bmc_location(bmc_location);
  if ( ret < 0 ) {
    syslog(LOG_WARNING, "%s() Cannot get the location of BMC", __func__);
  }

  if ( BB_BMC == *bmc_location ) {
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

  ret = fby35_common_get_bmc_location(&bmc_location);
  if (ret < 0) {
    syslog(LOG_ERR, "%s() Cannot get the location of BMC", __func__);
    return ret;
  }

  if ( bmc_location == BB_BMC ) {
    if (pwm_num > pal_pwm_cnt ||
      snprintf(label, sizeof(label), "pwm%d", pwm_num) > sizeof(label)) {
      return -1;
    }
    return sensors_write_pwmfan(pwm_num, (float)pwm);
  } else if (bmc_location == NIC_BMC) {
    ret = bic_set_fan_auto_mode(GET_FAN_MODE, &status);
    if (ret < 0) {
      return -1;
    }
    snprintf(cmd, sizeof(cmd), "sv status fscd | grep run | wc -l");
    if ((fp = popen(cmd, "r"))) {
      if (fgets(buf, sizeof(buf), fp)) {
        res = atoi(buf);
        if (res == 0) {
          is_fscd_run = false;
        }
      }
      pclose(fp);
    } else {
      is_fscd_run = false;
    }

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

  if ( bmc_location == BB_BMC ) {
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
    ret = bic_get_fan_speed(fan, &value);
  } else {
    syslog(LOG_WARNING, "%s() Cannot get the location of BMC", __func__);
    return -1;
  }

  if (value <= 0) {
    return -1;
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
    return bic_get_fan_pwm(pwm, value);
  }
  return sensors_read_pwmfan(pwm, value);
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

  //try to get the system type. The default config is CONFIG_A.
  if ( is_inited == false ) {
    char sys_conf[16] = {0};
    if ( kv_get("sled_system_conf", sys_conf, NULL, KV_FPERSIST) < 0 ) {
      syslog(LOG_WARNING, "%s() Failed to read sled_system_conf", __func__);
      return READING_NA;
    }

    if ( strcmp(sys_conf, "Type_1") == 0 ) config = CONFIG_A;
    else if ( strcmp(sys_conf, "Type_DPV2") == 0 ) config = CONFIG_B;
    else if ( strcmp(sys_conf, "Type_HD") == 0 ) config = CONFIG_B;
    else if ( strcmp(sys_conf, "Type_17") == 0 ) config = CONFIG_D;
    else syslog(LOG_WARNING, "%s() Couldn't identiy the system type: %s", __func__, sys_conf);

    syslog(LOG_INFO, "%s() Get the system type: %s", __func__, sys_conf);
    is_inited = true;
  }

  int i = 0;
  float temp_val = 0;
  for ( i = FRU_SLOT1; i <= FRU_SLOT4; i++ ) {
    //Only two slots are present on Config B. Skip slot2 and slot4.
    if ( (config == CONFIG_B) && (i % 2 == 0) ) continue;

    //If one of slots is failed to read, return READING_NA.
    if ( sensor_cache_read(i, target_snr_num, &temp_val) < 0) return READING_NA;

    if ( action == GET_MAX_VAL ) {
      if ( temp_val > *val ) *val = temp_val;
    } else if ( action == GET_MIN_VAL ) {
      if ( ((int)(*val) == 0) || (temp_val < *val) ) *val = temp_val;
    } else if ( action == GET_TOTAL_VAL ) {
      *val += temp_val;
    }
  }

  return PAL_EOK;
}

static int
get_pdb_reading(uint8_t pdb_id, uint8_t type, uint16_t cmd, float *value) {
  const uint8_t pdb_bus = 11;
  uint8_t addr = pdb_info_list[pdb_id].target_addr;
  static int fd = -1;
  uint8_t rbuf[PMBUS_RESP_LEN_MAX] = {0x00}, tbuf[PMBUS_CMD_LEN_MAX] = {cmd};
  uint8_t rlen = 2;
  uint8_t tlen = 1;
  int retry = MAX_RETRY;
  int ret = ERR_NOT_READY;

  if (value == NULL) {
    return READING_NA;
  }

  if ( fd < 0 ) {
    fd = i2c_cdev_slave_open(pdb_bus, addr >> 1, I2C_SLAVE_FORCE_CLAIM);
    if ( fd < 0 ) {
      syslog(LOG_WARNING, "Failed to open bus %d", pdb_bus);
      return READING_NA;
    }
  }

  for (retry = MAX_RETRY; retry > 0; retry--) {
    ret = i2c_rdwr_msg_transfer(fd, addr, tbuf, tlen, rbuf, rlen);
    if (ret == 0) {
      break;
    }
  }
  if ( ret < 0 ) {
    ret = READING_NA;
    goto exit;
  }
  // Renesas's response data is direct format
  if ((pdb_id == PDB_RNS) || (cmd == PMBUS_READ_VOUT)) {
    *value = (float)(rbuf[1] << 8 | rbuf[0]);
  // Delta's reponse data need to ignore bits 11~15, except PMBUS_READ_VOUT
  } else if ((pdb_id == PDB_DELTA_1) || (pdb_id == PDB_DELTA_2)) {
    *value = (float)(((rbuf[1] & 0x07) << 8) | rbuf[0]);
  } else {
    ret = READING_NA;
    goto exit;
  }

  return ret;
exit:
  if ( fd >= 0 ) {
    close(fd);
    fd = -1;
  }
  return ret;
}

static int
read_pdb_vin(uint8_t pdb_id, float *value) {
  if (value == NULL) {
    return READING_NA;
  }

  if (get_pdb_reading(pdb_id, PDB_VOLTAGE_IN, PMBUS_READ_VIN, value) < 0 ) {
    return READING_NA;
  }
  *value *= pdb_info_list[pdb_id].coefficient[PDB_VOLTAGE_IN].data;
  return PAL_EOK;
}

static int
read_pdb_vout(uint8_t pdb_id, float *value) {
  if (value == NULL) {
    return READING_NA;
  }

  if (get_pdb_reading(pdb_id, PDB_VOLTAGE_OUT, PMBUS_READ_VOUT, value) < 0 ) {
    return READING_NA;
  }
  *value *= pdb_info_list[pdb_id].coefficient[PDB_VOLTAGE_OUT].data;
  return PAL_EOK;
}

static int
read_pdb_iout(uint8_t pdb_id, float *value) {
  if (value == NULL) {
    return READING_NA;
  }

  if (get_pdb_reading(pdb_id, PDB_CURRENT, PMBUS_READ_IOUT, value) < 0 ) {
    return READING_NA;
  }
  *value *= pdb_info_list[pdb_id].coefficient[PDB_CURRENT].data;
  return PAL_EOK;
}

static int
read_pdb_pout(uint8_t pdb_id, float *value) {
  if (value == NULL) {
    return READING_NA;
  }

  if (get_pdb_reading(pdb_id, PDB_POWER, PMBUS_READ_POUT, value) < 0 ) {
    return READING_NA;
  }
  *value *= pdb_info_list[pdb_id].coefficient[PDB_POWER].data;
  return PAL_EOK;
}

static int
read_pdb_temp(uint8_t pdb_id, float *value) {
  if (value == NULL) {
    return READING_NA;
  }

  if (get_pdb_reading(pdb_id, PDB_TEMP, PMBUS_READ_TEMP1, value) < 0 ) {
    return READING_NA;
  }
  *value *= pdb_info_list[pdb_id].coefficient[PDB_TEMP].data;
  return PAL_EOK;
}

// Calculate Crater lake vdelta
static int
read_pdb_cl_vdelta(uint8_t snr_number, float *value) {
  float medusa_vout = 0;
  float min_hsc_vin = 0;
  uint8_t hsc_input_vol_num = BIC_SENSOR_HSC_INPUT_VOL; // Default Crater Lake sensor

  if (fby35_common_get_slot_type(FRU_SLOT1) == SERVER_TYPE_HD || fby35_common_get_slot_type(FRU_SLOT3) == SERVER_TYPE_HD) {
    hsc_input_vol_num = BIC_HD_SENSOR_HSC_INPUT_VOL;
  }

  if ( sensor_cache_read(FRU_BMC, BMC_SENSOR_MEDUSA_VOUT, &medusa_vout) < 0) return READING_NA;
  if ( read_snr_from_all_slots(hsc_input_vol_num, GET_MIN_VAL, &min_hsc_vin) < 0) return READING_NA;

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
  uint8_t hsc_output_cur_num = BIC_SENSOR_HSC_OUTPUT_CUR; // Default Crater Lake sensor

  if (fby35_common_get_slot_type(FRU_SLOT1) == SERVER_TYPE_HD || fby35_common_get_slot_type(FRU_SLOT3) == SERVER_TYPE_HD) {
    hsc_output_cur_num = BIC_HD_SENSOR_HSC_OUTPUT_CUR;
  }

  if ( sensor_cache_read(FRU_BMC, BMC_SENSOR_MEDUSA_CURR, &medusa_curr) < 0) return READING_NA;
  if ( sensor_cache_read(FRU_BMC, BMC_SENSOR_HSC_IOUT, &bb_hsc_curr) < 0) return READING_NA;
  if ( read_snr_from_all_slots(hsc_output_cur_num, GET_TOTAL_VAL, &total_hsc_iout) < 0) return READING_NA;

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
    bmc_location = ( (pwm_id & PWM_PLAT_SET) == PWM_PLAT_SET ) ? NIC_BMC : BB_BMC;
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
  if ( ret < 0 || rpm <= 0) {
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
        snr1_num = BMC_SENSOR_P12V;
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
  static float medusa_vin = 0;
  static float medusa_vout = 0;
  static char chip[32] = {0};
  static char hsc_conf[16] = {0};
  int ret = READING_NA;
  int i = 0;

  if ( is_cached == false) {
    if ( kv_get("bb_hsc_conf", hsc_conf, NULL, KV_FPERSIST) < 0 ) {
      syslog(LOG_WARNING, "%s() Failed to read bb_hsc_conf", __func__);
      return ret;
    }

    is_cached = true;
    for (i = 0; i < sizeof(medusa_hsc_list)/sizeof(medusa_hsc_list[0]); i++) {
      if (strncmp(hsc_conf, medusa_hsc_list[i].chip_name, sizeof(hsc_conf)) == 0) {
        snprintf(chip, sizeof(chip), "%s-i2c-11-%x",
          medusa_hsc_list[i].driver, medusa_hsc_list[i].target_addr);
        break;
      }
    }
    syslog(LOG_INFO, "%s() Use '%s'", __func__, chip);
  }

  switch(snr_number) {
    case BMC_SENSOR_MEDUSA_VIN:
      ret = sensors_read(chip, "BMC_SENSOR_MEDUSA_VIN", value);
      medusa_vin = *value;
      break;
    case BMC_SENSOR_MEDUSA_VOUT:
      ret = sensors_read(chip, "BMC_SENSOR_MEDUSA_VOUT", value);
      medusa_vout = *value;
      break;
    case BMC_SENSOR_MEDUSA_CURR:
      ret = sensors_read(chip, "BMC_SENSOR_MEDUSA_CURR", value);
      break;
    case BMC_SENSOR_MEDUSA_PWR:
      ret = sensors_read(chip, "BMC_SENSOR_MEDUSA_PWR", value);
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
read_temp(uint8_t snr_id, float *value) {
  struct {                                                          //******************************
    const char *chip;                                               //Corresponds to GENERIC I2C Sensors define in pal_sensor.h
    const char *label;                                              //Because of Class1 & Class2, we have to define two duplicate devs for TEMP_OUTLET and TEMP_NICEXP
  } devs[] = {                                                      //enum {
    {"lm75-i2c-12-4e",  "BMC_INLET_TEMP"},                          //  TEMP_INLET = 0,
    {"lm75-i2c-12-4f",  "BMC_OUTLET_TEMP/BMC_SENSOR_NICEXP_TEMP"},  //  TEMP_OUTLET,
    {"tmp421-i2c-8-1f", "NIC_SENSOR_TEMP"},                         //  TEMP_NIC,
    {"lm75-i2c-2-4f",  "BMC_OUTLET_TEMP"},                          //  TEMP_NICEXP_OUTLET,
    {"lm75-i2c-12-4f",  "BMC_OUTLET_TEMP/BMC_SENSOR_NICEXP_TEMP"}   //  TEMP_NICEXP
  };                                                                //};
  if (snr_id >= ARRAY_SIZE(devs)) {                                 //******************************
    return -1;
  }

  return sensors_read(devs[snr_id].chip, devs[snr_id].label, value);
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
  static uint8_t gval = UNKNOWN_SOLUTION;

  const char *adc_label[] = {
    "BMC_SENSOR_P5V",
    "BMC_SENSOR_P12V",
    "BMC_SENSOR_P3V3_STBY",
    "BMC_SENSOR_P2V5_STBY",
    "BMC_SENSOR_P5V_USB",
    "BMC_SENSOR_P1V8_STBY",
    "BMC_SENSOR_P1V2_STBY",
    "BMC_SENSOR_P1V0_STBY",
    "BMC_SENSOR_P0V6_STBY",
    "Dummy sensor",
    "BMC_SENSOR_FAN_IOUT",
    "BMC_SENSOR_NIC_IOUT",
    "BMC_SENSOR_NIC_P12V",
    "BMC_SENSOR_P3V3_RGM_STBY",
    "BMC_SENSOR_P3V3_NIC",
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

  if ( ADC10 == adc_id ) {
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

  if ( rev_id == UNKNOWN_REV ) {
    if ( get_board_rev(FRU_BMC, BOARD_ID_BB, &rev_id) < 0 ) {
      syslog(LOG_WARNING, "%s() Failed to get board revision", __func__);
      return -1;
    }
  }

  if ( ret == PAL_EOK ) {

    if ( rev_id >= BB_REV_DVT ) {

      if ( gval == UNKNOWN_SOLUTION ) {
        gval = gpio_get_value_by_shadow("P12V_EFUSE_DETECT_N");
      }

      if ( gval == GPIO_VALUE_INVALID ) {
        syslog(LOG_WARNING, "%s() Failed to read P12V_EFUSE_DETECT_N", __func__);
        return -1;
      }

      if ( ADC10 == adc_id ) { // 0xFB, BMC_SENSOR_FAN_IOUT
        if ( gval == MAXIM_SOLUTION ) {
          *value = (((*value/0.157/0.33) - 2.5) * 0.96) + 0.15;
        } else if ( gval == MPS_SOLUTION ) {
          *value = *value/0.01/5.11;
        } else {
          syslog(LOG_WARNING, "%s() Fan current solution not support, gval = %d", __func__, gval);
          return -1;
        }
      }
      else if ( ADC11 == adc_id ) { // 0xFC, BMC_SENSOR_NIC_IOUT
        if ( rev_id >= BB_REV_DVT_1C ) {
          *value = *value/0.01/30;
        }
        else { // DVT
          if ( gval == MAXIM_SOLUTION ) {
            *value = *value/0.22/1.2;
          } else if ( gval == MPS_SOLUTION ) {
            *value = *value/0.01/30;
          } else {
            syslog(LOG_WARNING, "%s() Fan current solution not support, gval = %d", __func__, gval);
            return -1;
          }
        }
      }
      // Other ADC channels do not need any value correction, just keep original value
    }
    else { // EVT
      if ( ADC10 == adc_id ) {
        *value = *value/0.22/0.665; // EVT: /0.22/0.237/4
        //when pwm is kept low, the current is very small or close to 0
        //BMC will show 0.00 amps. make it show 0.01 at least.
      } else if ( ADC11 == adc_id ) {
        *value = (*value/0.22/1.2) * 1.005 + 0.04;
        //it's not support to show the negative value, make it show 0.01 at least.
      }
      // Other ADC channels do not need any value correction, just keep original value
    }
    if ( *value <= 0 ) {
      *value = 0.01;
    }
  }
  return ret;
}

static int
get_hsc_reading(uint8_t hsc_id, uint8_t reading_type, uint8_t type, uint16_t cmd, float *value, uint8_t *raw_data) {
  const uint8_t hsc_bus = 11;
  uint8_t addr = hsc_info_list[hsc_id].slv_addr;
  static int fd = -1;
  uint8_t rbuf[PMBUS_RESP_LEN_MAX] = {0x00}, tbuf[8] = {cmd, 0x0};
  uint8_t rlen = 0;
  uint8_t tlen = 1;
  int retry = MAX_RETRY;
  int ret = ERR_NOT_READY;

  if (value == NULL) {
    return READING_NA;
  }

  if ( fd < 0 ) {
    fd = i2c_cdev_slave_open(hsc_bus, addr >> 1, I2C_SLAVE_FORCE_CLAIM);
    if ( fd < 0 ) {
      syslog(LOG_WARNING, "Failed to open bus %d", hsc_bus);
      return READING_NA;
    }
  }

  if (cmd == LTC4286_MFR_IOUT_MAX || cmd == LTC4286_MFR_PIN_MAX) {
    tbuf[0] = cmd >> 8;
    tbuf[1] = cmd;
    tlen = 2;
  }

  switch(reading_type) {
    case I2C_BYTE:
      rlen = 1;
      break;
    case I2C_WORD:
      rlen = 2;
      break;
    case I2C_BLOCK:
      for (retry = MAX_RETRY; retry > 0; retry--) {
        ret = i2c_rdwr_msg_transfer(fd, addr, tbuf, tlen, rbuf, 1);
        if (ret == 0) {
          break;
        }
      }
      if ( ret < 0 ) {
        ret = READING_NA;
        goto exit;
      }
      rlen = rbuf[0] + 1;
      break;
    default:
      rlen = 1;
  }

  if(rlen > PMBUS_RESP_LEN_MAX) {
    syslog(LOG_WARNING, "Failed to transfer message with i2c, because of the Block Read overflow!");
    ret = READING_NA;
    goto exit;
  }

  for (retry = MAX_RETRY; retry > 0; retry--) {
    ret = i2c_rdwr_msg_transfer(fd, addr, tbuf, tlen, rbuf, rlen);
    if (ret == 0) {
      break;
    }
  }
  if ( ret < 0 ) {
    ret = READING_NA;
    goto exit;
  }

  if (reading_type != I2C_BLOCK) {
    float m = hsc_info_list[hsc_id].info[type].m;
    float b = hsc_info_list[hsc_id].info[type].b;
    float r = hsc_info_list[hsc_id].info[type].r;

    *value = ((float)(rbuf[1] << 8 | rbuf[0]) * r - b) / m;
  }
  else {
    if (raw_data == NULL){
      ret = READING_NA;
      goto exit;
    }
    memcpy(raw_data, rbuf, rlen);
  }

  ret = PAL_EOK;

exit:
  if ( fd >= 0 ) {
    close(fd);
    fd = -1;
  }
  return ret;

}

static int
read_hsc_pin(uint8_t hsc_id, float *value) {
  if ( get_hsc_reading(hsc_id, I2C_WORD, HSC_POWER, PMBUS_READ_PIN, value, NULL) < 0 ) {
    return READING_NA;
  }

  if ( rev_id == UNKNOWN_REV ) {
    if ( get_board_rev(FRU_BMC, BOARD_ID_BB, &rev_id) < 0 ) {
      syslog(LOG_WARNING, "%s() Failed to get revision id", __func__);
      return -1;
    }
  }

  switch (hsc_id) {
    case HSC_ADM1278:
      if (rev_id < BB_REV_DVT) {
        *value *= 0.96; //improve the accuracy of PIN to +-2%
      }
      break;
    case HSC_MP5990:
      *value = ((*value + 0.05) * 1.02) + 0.2;
      break;
    case HSC_LTC4286:
      break;
    default:
      return READING_NA;
  }

  return PAL_EOK;
}

static int
read_hsc_iout(uint8_t hsc_id, float *value) {
  if ( get_hsc_reading(hsc_id, I2C_WORD, HSC_CURRENT, PMBUS_READ_IOUT, value, NULL) < 0 ) {
    return READING_NA;
  }

  if ( rev_id == UNKNOWN_REV ) {
    if ( get_board_rev(FRU_BMC, BOARD_ID_BB, &rev_id) < 0 ) {
      syslog(LOG_WARNING, "%s() Failed to get revision id", __func__);
      return -1;
    }
  }

  switch (hsc_id) {
    case HSC_ADM1278:
      if (rev_id < BB_REV_DVT) {
        *value *= 0.96; //improve the accuracy of PIN to +-2%
      }
      break;
    case HSC_MP5990:
      *value = ((*value - 0.05) * 0.99) + 0.16;
      break;
    case HSC_LTC4286:
      break;
    default:
      return READING_NA;
  }

  return PAL_EOK;
}

static int
read_hsc_temp(uint8_t hsc_id, float *value) {
  if ( get_hsc_reading(hsc_id, I2C_WORD, HSC_TEMP, PMBUS_READ_TEMP1, value, NULL) < 0 ) {
    return READING_NA;
  }
  return PAL_EOK;
}

static int
read_hsc_vin(uint8_t hsc_id, float *value) {
  if ( get_hsc_reading(hsc_id, I2C_WORD, HSC_VOLTAGE, PMBUS_READ_VIN, value, NULL) < 0 ) {
    return READING_NA;
  }
  return PAL_EOK;
}

static int
read_hsc_peak_iout(uint8_t hsc_id, float *value) {
  static float peak = 0;

  if ( get_hsc_reading(hsc_id, I2C_WORD, HSC_CURRENT, hsc_info_list[hsc_id].cmd_peak_iout, value, NULL) < 0 ) {
    return READING_NA;
  }

  // it's "read-clear" data, so need to be cached
  if (peak > *value) {
    *value = peak;
  } else {
    peak = *value;
  }

  return PAL_EOK;
}

static int
read_hsc_peak_pin(uint8_t hsc_id, float *value) {
  static float peak = 0;

  if ( get_hsc_reading(hsc_id, I2C_WORD, HSC_POWER, hsc_info_list[hsc_id].cmd_peak_pin, value, NULL) < 0 ) {
    return READING_NA;
  }

  // it's "read-clear" data, so need to be cached
  if (peak > *value) {
    *value = peak;
  } else {
    peak = *value;
  }

  return PAL_EOK;
}

static int
read_dpv2_efuse(uint8_t info, float *value) {
  uint8_t offset = 0x0;
  static int fd = -1;
  uint8_t rbuf[9] = {0x00};
  uint8_t rlen = 2;
  int retry = MAX_RETRY;
  int ret = ERR_NOT_READY;
  float m = 0, b = 0, r = 0;
  int raw_val = 0;
  int fru = 0, type = 0;
  PAL_SNR_INFO* snr_info = (PAL_SNR_INFO*)&info;

  if (value == NULL) {
    syslog(LOG_WARNING, "Invalid parameter");
    return READING_NA;
  }
  fru = snr_info->fru;
  type = snr_info->type;
  offset = dpv2_efuse_info_list[type].offset;
  if (fd < 0) { // open first time
    fd = i2c_cdev_slave_open(FRU_DEVICE_BUS(fru), DPV2_EFUSE_SLAVE_ADDR, I2C_SLAVE_FORCE_CLAIM);
    if (fd < 0) {
      syslog(LOG_WARNING, "Failed to open bus %d", FRU_DEVICE_BUS(fru));
      return READING_NA;
    }
  }

  while ( ret < 0 && retry-- > 0 ) {
    ret = i2c_rdwr_msg_transfer(fd, DPV2_EFUSE_SLAVE_ADDR << 1, &offset, 1, rbuf, rlen);
  }
  if ( ret < 0 ) {
    if ( fd >= 0 ) {
      close(fd);
      fd = -1;
    }
    return READING_NA;
  }
  raw_val = (rbuf[1] << 8) | rbuf[0];
  // 2's complement
  if ((rbuf[1] >> 7) == 1) {
    raw_val = (~raw_val + 1);
    raw_val = -raw_val;
  }
  m = dpv2_efuse_info_list[type].m;
  b = dpv2_efuse_info_list[type].b;
  r = dpv2_efuse_info_list[type].r;
  *value = ((float)raw_val * pow(10, -r) - b) / m;

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

  if ( fby35_common_get_bmc_location(&bmc_location) < 0 ) {
    syslog(LOG_WARNING, "Failed to get the location of BMC");
  }

  if ( is_fan_fail_otp_asserted == false && bmc_location != NIC_BMC) {
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

  switch(fru) {
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
pal_bic_sensor_read_raw(uint8_t fru, uint8_t sensor_num, float *value, uint8_t bmc_location, const uint8_t config_status) {
#define BIC_SENSOR_READ_NA 0x20
  int ret = 0;
  uint8_t power_status = 0;
  ipmi_extend_sensor_reading_t sensor = {0};
  sdr_full_t *sdr = NULL;
  char path[128];
  sprintf(path, SLOT_SENSOR_LOCK, fru);
  uint8_t *bic_skip_list;
  int skip_sensor_cnt = 0;
  uint8_t reading_msb = 0;
  static uint8_t board_type[MAX_NODES] = {UNKNOWN_BOARD, UNKNOWN_BOARD, UNKNOWN_BOARD, UNKNOWN_BOARD};
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
    if ((skip_bic_sensor_list(fru, sensor_num, bmc_location, config_status) < 0) && (temp_cnt < skip_sensor_cnt)) {
      temp_cnt ++;
      return READING_NA;
    }

    if (temp_cnt == skip_sensor_cnt) {
      pwr_off_flag[fru-1] = 0;
      temp_cnt = 0;
    }
  }

  ret = access(path, F_OK);
  if (ret == 0) {
    return READING_SKIP;
  }
  if (((config_status & PRESENT_2OU) == PRESENT_2OU) && (board_type[fru-1] == UNKNOWN_BOARD)) {
    ret = fby35_common_get_2ou_board_type(fru, &board_type[fru-1]);
    if (ret < 0) {
      syslog(LOG_ERR, "%s() Cannot get board_type", __func__);
    }
  }

  //check snr number first. If it not holds, it will move on
  if (sensor_num <= 0x48 || (((board_type[fru-1] & DPV2_X16_BOARD) == DPV2_X16_BOARD) && (board_type[fru-1] != UNKNOWN_BOARD) &&
      (sensor_num >= BIC_DPV2_SENSOR_DPV2_2_12V_VIN && sensor_num <= BIC_DPV2_SENSOR_DPV2_2_EFUSE_PWR))) { //server board
    ret = bic_get_sensor_reading(fru, sensor_num, &sensor, NONE_INTF);
  } else if ( (sensor_num >= 0x50 && sensor_num <= 0x7F) && (bmc_location != NIC_BMC) && //1OU
       ((config_status & PRESENT_1OU) == PRESENT_1OU) ) {
    ret = bic_get_sensor_reading(fru, sensor_num, &sensor, FEXP_BIC_INTF);
  } else if ( ((sensor_num >= 0x80 && sensor_num <= 0xCE) ||     //2OU
               (sensor_num >= 0x49 && sensor_num <= 0x4D)) &&    //Many sensors are defined in GPv3.
              ((config_status & PRESENT_2OU) == PRESENT_2OU) ) { //The range from 0x80 to 0xCE is not enough for adding new sensors.
                                                                 //So, we take 0x49 ~ 0x4D here
    ret = bic_get_sensor_reading(fru, sensor_num, &sensor, REXP_BIC_INTF);
  } else if ( (sensor_num >= 0xD1 && sensor_num <= 0xF1) ) { //BB
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

  if(sensor.read_type == ACCURATE_CMD_4BYTE) {
    *value = ((int16_t)((sensor.value & 0xFFFF0000) >> 16)) * 0.001 + ((int16_t)(sensor.value & 0x0000FFFF)) ;

    if (fby35_common_get_slot_type(fru) == SERVER_TYPE_HD) {
      //Apply value correction for Half Dome
      switch (sensor_num) {
        case BIC_HD_SENSOR_FIO_TEMP:
          apply_frontIO_correction(fru, sensor_num, value, bmc_location);
          break;

        default:
          break;
      }
    } else if (fby35_common_get_slot_type(fru) == SERVER_TYPE_CL) {
      //Apply value correction for Crater Lake
      switch (sensor_num) {
        case BIC_SENSOR_FIO_TEMP:
          apply_frontIO_correction(fru, sensor_num, value, bmc_location);
          break;

        case BIC_SENSOR_CPU_THERM_MARGIN:
          if ( *value > 0 ) *value = -(*value);
          break;

        default:
          break;
      }
    } else {
      syslog(LOG_WARNING, "%s()  fru%x, unknown slot type 0x%x", __func__, fru, fby35_common_get_slot_type(fru));
    }
    return 0;
  } else if ( sensor.read_type == ACCURATE_CMD) {
    uint32_t val = ((sensor.value & 0xff) << 8) + ((sensor.value & 0xff00) >> 8);
    sensor.value = val;
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
    reading_msb = (sensor.read_type == ACCURATE_CMD) ? 15 : 7;
    x = (sensor.value & (1 << reading_msb)) ? (0-(~sensor.value)) : sensor.value;
  } else if ((sdr->sensor_units1 & 0xC0) == 0x80) {  // 2's complements
    x = (sensor.read_type == ACCURATE_CMD) ? (int16_t)sensor.value : (int8_t)sensor.value;
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
  if (sensor.read_type == ACCURATE_CMD) {
    *value /= 256;
  }
  //  Apply value correction for Crater Lake
  if (fby35_common_get_slot_type(fru) == SERVER_TYPE_CL) {
    switch (sensor_num) {
      case BIC_SENSOR_FIO_TEMP:
        apply_frontIO_correction(fru, sensor_num, value, bmc_location);
        break;
      case BIC_SENSOR_CPU_THERM_MARGIN:
        if ( *value > 0 ) *value = -(*value);
        break;
    }
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
  int ret = 0, i = 0;
  uint8_t id=0;
  uint8_t hsc_sesnor_num = 0x0;
  uint8_t info = 0;
  PAL_SNR_INFO* dpv2_snr_info = (PAL_SNR_INFO*) &info;
  static uint8_t hsc_type = HSC_ADM1278; // set main source as default type
  static uint8_t bmc_location = 0;
  static uint8_t config_status[MAX_NODES] = {CONFIG_UNKNOWN, CONFIG_UNKNOWN, CONFIG_UNKNOWN, CONFIG_UNKNOWN};
  static bool is_hsc_init = false;
  static uint8_t hsc_sesnors[BB_HSC_SENSOR_CNT] = {
    BMC_SENSOR_HSC_PEAK_IOUT,
    BMC_SENSOR_HSC_PEAK_PIN,
    BMC_SENSOR_HSC_VIN,
    BMC_SENSOR_HSC_TEMP,
    BMC_SENSOR_HSC_PIN,
    BMC_SENSOR_HSC_IOUT
  };
  static uint8_t board_type[MAX_NODES] = {UNKNOWN_BOARD, UNKNOWN_BOARD, UNKNOWN_BOARD, UNKNOWN_BOARD};

  if ( bmc_location == 0 ) {
    if ( fby35_common_get_bmc_location(&bmc_location) < 0 ) {
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
        // unnecessary to init for 2nd source HSC
        is_hsc_init = true;
      }
    }
  }

  // initialization for 2nd source HSC
  if ((is_hsc_init == true) || (fru != FRU_BMC)) {
    goto skip_hsc_init;
  }

  if (fby35_common_get_bb_hsc_type(&hsc_type) != 0) {
    goto skip_hsc_init;
  }

  for (i = 0; i < BB_HSC_SENSOR_CNT; i++) {
    hsc_sesnor_num = hsc_sesnors[i];
    sensor_map[hsc_sesnor_num].id = hsc_type;
  }

  if (hsc_type == HSC_LTC4286) {
    float rsense = 0.3;

    if ( rev_id == UNKNOWN_REV ) {
      if ( get_board_rev(FRU_BMC, BOARD_ID_BB, &rev_id) < 0 ) {
        syslog(LOG_WARNING, "%s() Failed to get board revision ID", __func__);
        goto skip_hsc_init;
      }
    }

    if (rev_id >= BB_REV_DVT ) {
      rsense = 0.3;
    } else { // EVT3 and previous revision
      rsense = 0.5;
    }

    hsc_info_list[hsc_type].info[HSC_CURRENT].m *= rsense;
    hsc_info_list[hsc_type].info[HSC_POWER].m *= rsense;
  }
  is_hsc_init = true;
skip_hsc_init:
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
        ret = bic_is_exp_prsnt(fru);
        if ( ret < 0 ) {
          syslog(LOG_WARNING, "%s() Failed to run bic_is_exp_prsnt", __func__);
        } else {
          config_status[fru-1] = (uint8_t)ret;
        }
        syslog(LOG_INFO, "%s() fru: %02x. config:%02x", __func__, fru, config_status[fru-1]);
      }
      // DPv2 X8 sensors
      if (((config_status[fru-1] & PRESENT_2OU) == PRESENT_2OU)) {
        if (board_type[fru-1] == UNKNOWN_BOARD) {
          ret = fby35_common_get_2ou_board_type(fru, &board_type[fru-1]);
          if (ret < 0) {
            syslog(LOG_WARNING, "%s() Failed to get 2OU board type", __func__);
          }
        }
        if (((board_type[fru-1] & DPV2_X8_BOARD) == DPV2_X8_BOARD) && (board_type[fru-1] != UNKNOWN_BOARD) && \
          (sensor_num >= BMC_DPV2_SENSOR_DPV2_1_12V_VIN && sensor_num <= BMC_DPV2_SENSOR_DPV2_1_EFUSE_PWR)) {
          dpv2_snr_info->fru = fru;
          dpv2_snr_info->type = id;
          ret = sensor_map[sensor_num].read_sensor(info, (float*) value);
          break;
        }
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
  if (kv_set(key, str, 0, 0) < 0) {
    syslog(LOG_WARNING, "pal_sensor_read_raw: cache_set key = %s, str = %s failed.", key, str);
    return -1;
  } else {
    return ret;
  }

  return 0;
}

int
pal_get_sensor_name(uint8_t fru, uint8_t sensor_num, char *name) {
  uint8_t config_status = 0, board_type = 0;
  int ret = 0;

  switch(fru) {
    case FRU_BMC:
    case FRU_NIC:
      sprintf(name, "%s", sensor_map[sensor_num].snr_name);
      break;
    case FRU_SLOT1:
    case FRU_SLOT3:
      if ((sensor_num >= BMC_DPV2_SENSOR_DPV2_1_12V_VIN) && (sensor_num <= BMC_DPV2_SENSOR_DPV2_1_EFUSE_PWR)) {
        config_status = bic_is_exp_prsnt(fru);
        if ((config_status & PRESENT_2OU) == PRESENT_2OU ) {
          ret = fby35_common_get_2ou_board_type(fru, &board_type);
          if (ret < 0) {
            return -1;
          }
          if ((board_type & DPV2_X8_BOARD) == DPV2_X8_BOARD) {
              snprintf(name, MAX_SNR_NAME_LEN, "%s", sensor_map[sensor_num].snr_name);
          }
        }
      }
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

  if (fby35_common_get_bmc_location(&bmc_location) < 0) {
    syslog(LOG_ERR, "%s() Cannot get the location of BMC", __func__);
    return -1;
  } else if (pal_get_fan_type(&bmc_location, &fan_type) != PAL_EOK) {
    syslog(LOG_ERR, "%s() Cannot get the type of fan, fvaule=%d", __func__, fan_type);
    return -1;
  } else {

    if ( rev_id == UNKNOWN_REV ) {
      if ( get_board_rev(FRU_BMC, BOARD_ID_BB, &rev_id) < 0 ) {
        syslog(LOG_WARNING, "%s() Failed to get revision id", __func__);
        return -1;
      }
    }

    if (rev_id >= BB_REV_DVT && fan_type == SINGLE_TYPE) {
        /*
          BMC_SENSOR_FAN0_TACH,
          BMC_SENSOR_FAN1_TACH,
          BMC_SENSOR_FAN2_TACH,
          BMC_SENSOR_FAN3_TACH,
          sensor threshold change for 15k fan
        */
        for (int fan_num = BMC_SENSOR_FAN0_TACH; fan_num <= BMC_SENSOR_FAN3_TACH; fan_num++) {
          sensor_map[fan_num].snr_thresh.lcr_thresh = FAN_15K_LCR;
          sensor_map[fan_num].snr_thresh.unc_thresh = FAN_15K_UNC;
          sensor_map[fan_num].snr_thresh.ucr_thresh = FAN_15K_UCR;
        }
    }

    if (fan_type != SINGLE_TYPE) {
      // set fan tach UCR to 13800 if it's not single type
      sensor_map[BMC_SENSOR_FAN0_TACH].snr_thresh.ucr_thresh = DUAL_FAN_UCR;
      sensor_map[BMC_SENSOR_FAN1_TACH].snr_thresh.ucr_thresh = DUAL_FAN_UCR;
      sensor_map[BMC_SENSOR_FAN2_TACH].snr_thresh.ucr_thresh = DUAL_FAN_UCR;
      sensor_map[BMC_SENSOR_FAN3_TACH].snr_thresh.ucr_thresh = DUAL_FAN_UCR;
      sensor_map[BMC_SENSOR_FAN4_TACH].snr_thresh.ucr_thresh = DUAL_FAN_UCR;
      sensor_map[BMC_SENSOR_FAN5_TACH].snr_thresh.ucr_thresh = DUAL_FAN_UCR;
      sensor_map[BMC_SENSOR_FAN6_TACH].snr_thresh.ucr_thresh = DUAL_FAN_UCR;
      sensor_map[BMC_SENSOR_FAN7_TACH].snr_thresh.ucr_thresh = DUAL_FAN_UCR;

      sensor_map[BMC_SENSOR_FAN0_TACH].snr_thresh.unc_thresh = DUAL_FAN_UNC;
      sensor_map[BMC_SENSOR_FAN1_TACH].snr_thresh.unc_thresh = DUAL_FAN_UNC;
      sensor_map[BMC_SENSOR_FAN2_TACH].snr_thresh.unc_thresh = DUAL_FAN_UNC;
      sensor_map[BMC_SENSOR_FAN3_TACH].snr_thresh.unc_thresh = DUAL_FAN_UNC;
      sensor_map[BMC_SENSOR_FAN4_TACH].snr_thresh.unc_thresh = DUAL_FAN_UNC;
      sensor_map[BMC_SENSOR_FAN5_TACH].snr_thresh.unc_thresh = DUAL_FAN_UNC;
      sensor_map[BMC_SENSOR_FAN6_TACH].snr_thresh.unc_thresh = DUAL_FAN_UNC;
      sensor_map[BMC_SENSOR_FAN7_TACH].snr_thresh.unc_thresh = DUAL_FAN_UNC;

    }
#ifdef DEBUG
    syslog(LOG_INFO, "%s() fan tach threshold initialized, fan_type: %u", __func__, fan_type);
#endif
  }
  return 0;
}

static int
pal_medusa_hsc_threshold_init() {
  static bool is_inited = false;
  bool is_48v_medusa = false;
  char hsc_type[MAX_VALUE_LEN] = {0};
  int i = 0, index = 0;
  struct {
    uint8_t sensor_num;
    PAL_SENSOR_THRESHOLD thres;
  } medusa_sensor[] = {
    //           sensor_num, ( LNR,   LCR,  LNC,  UNC,  UCR, UNR)
    {BMC_SENSOR_MEDUSA_VIN,  {41.1, 42.72, 43.2, 56.1, 56.61, 0}},
    {BMC_SENSOR_MEDUSA_VOUT, {41.1, 42.72, 43.2, 56.1, 56.61, 0}},
    {BMC_SENSOR_MEDUSA_CURR, {   0,     0,    0,   62,     0, 0}},
  };
  uint8_t bmc_location = 0;

  if (fby35_common_get_bmc_location(&bmc_location) < 0) {
    syslog(LOG_ERR, "%s() Cannot get the location of BMC", __func__);
  } else {
    // Since NICEXP does not have medusa HSC sensor.
    // Config D system don't need to initialize medusa HSC.
    if (bmc_location == NIC_BMC) {
      return 0;
    }
  }
  if (is_inited == true) {
    return 0;
  }
  if (kv_get("bb_hsc_conf", hsc_type, NULL, KV_FPERSIST) < 0) {
    return -1;
  } else {
    // 12V medusa: LTC4282
    // 48V medusa: ADM1272, LTC4287
    is_48v_medusa = strncmp(hsc_type, "ltc4282", sizeof(hsc_type)) ? true : false;
  }
  if (is_48v_medusa == true) {
    for(i = 0; i < sizeof(medusa_sensor) / sizeof(medusa_sensor[1]); i++) {
      index = medusa_sensor[i].sensor_num;
      memcpy(&sensor_map[index].snr_thresh, &medusa_sensor[i].thres, sizeof(PAL_SENSOR_THRESHOLD));
    }
  }
  is_inited = true;
  return 0;
}

static int
pal_vpdb_threshold_init() {
  static bool is_inited = false;
  bool is_48v_medusa = false;
  char hsc_type[MAX_VALUE_LEN] = {0};

  if (is_inited == true) {
    return 0;
  }
  if (kv_get("bb_hsc_conf", hsc_type, NULL, KV_FPERSIST) < 0) {
    return -1;
  } else {
    // 12V medusa: LTC4282
    // 48V medusa: ADM1272, LTC4287
    is_48v_medusa = strncmp(hsc_type, "ltc4282", sizeof(hsc_type)) ? true : false;
  }
  if (is_48v_medusa == true) {
    sensor_map[BMC_SENSOR_PDB_CL_VDELTA].snr_thresh.ucr_thresh = BB_CPU_VDELTA_48V_UCR;
    sensor_map[BMC_SENSOR_PDB_BB_VDELTA].snr_thresh.ucr_thresh = BB_CPU_VDELTA_48V_UCR;
  }
  is_inited = true;
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
  if (fru == FRU_BMC) {
    if ((pal_medusa_hsc_threshold_init() < 0) || (pal_vpdb_threshold_init() < 0)) {
      return -1;
    }
  }
  if ( rev_id == UNKNOWN_REV ) {
    if ( get_board_rev(FRU_BMC, BOARD_ID_BB, &rev_id) < 0 ) {
      syslog(LOG_WARNING, "%s() Failed to get board revision", __func__);
      return -1;
    }
  }

  switch (fru) {
    case FRU_BMC:
    case FRU_NIC:
    case FRU_SLOT1:
    case FRU_SLOT3:
      // Only DPv2 X8 sensor threshold is got from sensor_map in slot1 & 3
      if (((fru == FRU_SLOT1) || (fru == FRU_SLOT3)) &&
        (sensor_num < BMC_DPV2_SENSOR_DPV2_1_12V_VIN || sensor_num > BMC_DPV2_SENSOR_DPV2_1_EFUSE_PWR)) {
        return -1;
      }

      switch(thresh) {
        case UCR_THRESH:
          if ( fru == FRU_BMC && rev_id >= BB_REV_DVT ) {
            if ( sensor_num == BMC_SENSOR_HSC_IOUT ) {
              *val = 36;
              break;
            }
            if (sensor_num == BMC_SENSOR_FAN_IOUT ) {
              *val = 28.6;
              break;
            }
            if ( sensor_num == BMC_SENSOR_FAN_PWR ) {
              *val = 396.825;
              break;
            }
            if ( sensor_num == BMC_SENSOR_HSC_PIN ) {
              *val = 450;
              break;
            }
          }
          *val = sensor_map[sensor_num].snr_thresh.ucr_thresh;
          break;
        case UNC_THRESH:
          *val = sensor_map[sensor_num].snr_thresh.unc_thresh;
          break;
        case UNR_THRESH:
          if ( fru == FRU_BMC && rev_id >= BB_REV_DVT ) {
            if ( sensor_num == BMC_SENSOR_HSC_IOUT ) {
              *val = 45;
              break;
            }
            if ( sensor_num == BMC_SENSOR_HSC_PIN ) {
              *val = 562.5;
              break;
            }
          }
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

bool
pal_is_sdr_from_file(uint8_t fru, uint8_t snr_num) {
  uint8_t config_status = 0, board_type = 0;
  int ret = 0;

  switch(fru) {
    case FRU_SLOT1:
    case FRU_SLOT3:
      if ((snr_num >= BMC_DPV2_SENSOR_DPV2_1_12V_VIN) && (snr_num <= BMC_DPV2_SENSOR_DPV2_1_EFUSE_PWR)) {
        config_status = bic_is_exp_prsnt(fru);
        if ((config_status & PRESENT_2OU) == PRESENT_2OU ) {
          ret = fby35_common_get_2ou_board_type(fru, &board_type);
          if (ret < 0) {
            return true;
          }
          if ((board_type & DPV2_X8_BOARD) == DPV2_X8_BOARD) {
              return false;
          }
        }
      }
      break;
    default:
      return true;
  }
  return true;
}

static int
_sdr_init(char *path, sensor_info_t *sinfo, uint8_t bmc_location, \
          const uint8_t config_status, const uint8_t board_type) {
  int fd;
  int ret = 0;
  uint8_t buf[sizeof(sdr_full_t)] = {0};
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
  ret = fby35_common_get_bmc_location(&bmc_location);
  if (ret < 0) {
    syslog(LOG_ERR, "%s() Cannot get the location of BMC", __func__);
    goto error_exit;
  }

  while ( prsnt_retry-- > 0 ) {
    // get the status of m2 board
    ret = bic_is_exp_prsnt(fru);
    if ( ret < 0 ) {
      sleep(3);
      continue;
    } else config_status = (uint8_t) ret;

    if ( (config_status & PRESENT_2OU) == PRESENT_2OU ) {
      //if it's present, get its type
      fby35_common_get_2ou_board_type(fru, &board_type);
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
    {"1344", "5410"}, // Micron
    {"15b7", "5002"}, // WD
    {"1179", "011a"}, // Toshiba
  };
  char key[100] = {0};
  char drive_id[16] = {0};
  char read_vid[16] = {0};
  char read_did[16] = {0};
  int i = 0;

  if (snr_num == BIC_SENSOR_M2A_TEMP) {
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
