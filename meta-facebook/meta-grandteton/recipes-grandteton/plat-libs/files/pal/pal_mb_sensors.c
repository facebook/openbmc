#include <stdio.h>
#include <math.h>
#include <stdint.h>
#include <fcntl.h>
#include <errno.h>
#include <syslog.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <string.h>
#include <ctype.h>
#include <time.h>
#include <peci.h>
#include <linux/peci-ioctl.h>
#include <openbmc/kv.h>
#include <openbmc/libgpio.h>
#include <openbmc/obmc-i2c.h>
#include <openbmc/nm.h>
#include <openbmc/ipmb.h>
#include <openbmc/obmc-sensors.h>
#include <openbmc/peci_sensors.h>
#include <openbmc/pmbus.h>
#include <openbmc/sensor-correction.h>
#include <openbmc/dimm.h>
#include "pal.h"
#include "pal_common.h"
#include "pal_bb_sensors.h"
#include "pal_def.h"

//#define DEBUG
#define GPIO_P3V_BAT_SCALED_EN    "BATTERY_DETECT"
#define FRB3_MAX_READ_RETRY       (10)
#define HSC_12V_BUS               (2)

#define IIO_DEV_DIR(device, bus, addr, index) \
  "/sys/bus/i2c/drivers/"#device"/"#bus"-00"#addr"/iio:device"#index"/%s"
#define IIO_AIN_NAME       "in_voltage%d_raw"

#define MAX11617_DIR     IIO_DEV_DIR(max1363, 20, 35, 2)
static int read_bat_val(uint8_t fru, uint8_t sensor_num, float *value);
static int read_mb_temp(uint8_t fru, uint8_t sensor_num, float *value);
static int read_cpu_temp(uint8_t fru, uint8_t sensor_num, float *value);
static int read_cpu_pkg_pwr(uint8_t fru, uint8_t sensor_num, float *value);
static int read_cpu_tjmax(uint8_t fru, uint8_t sensor_num, float *value);
static int read_cpu_thermal_margin(uint8_t fru, uint8_t sensor_num, float *value);
static int read_hsc_iout(uint8_t fru, uint8_t sensor_num, float *value);
static int read_hsc_vin(uint8_t fru, uint8_t sensor_num, float *value);
static int read_hsc_pin(uint8_t fru, uint8_t sensor_num, float *value);
static int read_hsc_temp(uint8_t fru, uint8_t sensor_num, float *value);
static int read_hsc_peak_pin(uint8_t fru, uint8_t sensor_num, float *value);
static int read_cpu_dimm_temp(uint8_t fru, uint8_t sensor_num, float *value);
static int read_cpu_dimm_power(uint8_t fru, uint8_t sensor_num, float *value);
static int read_cpu_dimm_state(uint8_t fru, uint8_t sensor_num, float *value);
static int read_NM_pch_temp(uint8_t fru, uint8_t sensor_num, float *value);
static int read_vr_vout(uint8_t fru, uint8_t sensor_num, float *value);
static int read_vr_temp(uint8_t fru, uint8_t sensor_num, float  *value);
static int read_vr_iout(uint8_t fru, uint8_t sensor_num, float  *value);
static int read_vr_pout(uint8_t fru, uint8_t sensor_num, float  *value);
static int read_frb3(uint8_t fru, uint8_t sensor_num, float *value);
static int read_e1s_power(uint8_t fru, uint8_t sensor_num, float *value);
static int read_e1s_temp(uint8_t fru, uint8_t sensor_num, float *value);
static int get_nm_rw_info(uint8_t* nm_bus, uint8_t* nm_addr, uint16_t* bmc_addr);
bool pal_bios_completed(uint8_t fru);
static uint8_t postcodes_last[256] = {0};

const uint8_t mb_sensor_list[] = {
  MB_SNR_INLET_TEMP_R,
  MB_SNR_INLET_TEMP_L,
  MB_SNR_OUTLET_TEMP_R,
  MB_SNR_OUTLET_TEMP_L,
  MB_SNR_PCH_TEMP,
  MB_SNR_DIMM_CPU0_GRPA_TEMP,
  MB_SNR_DIMM_CPU0_GRPB_TEMP,
  MB_SNR_DIMM_CPU0_GRPC_TEMP,
  MB_SNR_DIMM_CPU0_GRPD_TEMP,
  MB_SNR_DIMM_CPU0_GRPE_TEMP,
  MB_SNR_DIMM_CPU0_GRPF_TEMP,
  MB_SNR_DIMM_CPU0_GRPG_TEMP,
  MB_SNR_DIMM_CPU0_GRPH_TEMP,
  MB_SNR_DIMM_CPU1_GRPA_TEMP,
  MB_SNR_DIMM_CPU1_GRPB_TEMP,
  MB_SNR_DIMM_CPU1_GRPC_TEMP,
  MB_SNR_DIMM_CPU1_GRPD_TEMP,
  MB_SNR_DIMM_CPU1_GRPE_TEMP,
  MB_SNR_DIMM_CPU1_GRPF_TEMP,
  MB_SNR_DIMM_CPU1_GRPG_TEMP,
  MB_SNR_DIMM_CPU1_GRPH_TEMP,
  MB_SNR_VR_CPU0_VCCIN_VOLT,
  MB_SNR_VR_CPU0_VCCIN_TEMP,
  MB_SNR_VR_CPU0_VCCIN_CURR,
  MB_SNR_VR_CPU0_VCCIN_POWER,
  MB_SNR_VR_CPU0_VCCFA_FIVRA_VOLT,
  MB_SNR_VR_CPU0_VCCFA_FIVRA_TEMP,
  MB_SNR_VR_CPU0_VCCFA_FIVRA_CURR,
  MB_SNR_VR_CPU0_VCCFA_FIVRA_POWER,
  MB_SNR_VR_CPU0_VCCIN_FAON_VOLT,
  MB_SNR_VR_CPU0_VCCIN_FAON_TEMP,
  MB_SNR_VR_CPU0_VCCIN_FAON_CURR,
  MB_SNR_VR_CPU0_VCCIN_FAON_POWER,
  MB_SNR_VR_CPU0_VCCFA_VOLT,
  MB_SNR_VR_CPU0_VCCFA_TEMP,
  MB_SNR_VR_CPU0_VCCFA_CURR,
  MB_SNR_VR_CPU0_VCCFA_POWER,
  MB_SNR_VR_CPU0_VCCD_HV_VOLT,
  MB_SNR_VR_CPU0_VCCD_HV_TEMP,
  MB_SNR_VR_CPU0_VCCD_HV_CURR,
  MB_SNR_VR_CPU0_VCCD_HV_POWER,
  MB_SNR_VR_CPU1_VCCIN_VOLT,
  MB_SNR_VR_CPU1_VCCIN_TEMP,
  MB_SNR_VR_CPU1_VCCIN_CURR,
  MB_SNR_VR_CPU1_VCCIN_POWER,
  MB_SNR_VR_CPU1_VCCFA_FIVRA_VOLT,
  MB_SNR_VR_CPU1_VCCFA_FIVRA_TEMP,
  MB_SNR_VR_CPU1_VCCFA_FIVRA_CURR,
  MB_SNR_VR_CPU1_VCCFA_FIVRA_POWER,
  MB_SNR_VR_CPU1_VCCIN_FAON_VOLT,
  MB_SNR_VR_CPU1_VCCIN_FAON_TEMP,
  MB_SNR_VR_CPU1_VCCIN_FAON_CURR,
  MB_SNR_VR_CPU1_VCCIN_FAON_POWER,
  MB_SNR_VR_CPU1_VCCFA_VOLT,
  MB_SNR_VR_CPU1_VCCFA_TEMP,
  MB_SNR_VR_CPU1_VCCFA_CURR,
  MB_SNR_VR_CPU1_VCCFA_POWER,
  MB_SNR_VR_CPU1_VCCD_HV_VOLT,
  MB_SNR_VR_CPU1_VCCD_HV_TEMP,
  MB_SNR_VR_CPU1_VCCD_HV_CURR,
  MB_SNR_VR_CPU1_VCCD_HV_POWER,
  MB_SNR_CPU0_TEMP,
  MB_SNR_CPU1_TEMP,
  MB_SNR_CPU0_PKG_POWER,
  MB_SNR_CPU1_PKG_POWER,
  MB_SNR_CPU0_TJMAX,
  MB_SNR_CPU1_TJMAX,
  MB_SNR_CPU0_THERM_MARGIN,
  MB_SNR_CPU1_THERM_MARGIN,
  MB_SNR_P3V_BAT,
  MB_SNR_E1S_P3V3_VOUT,
  MB_SNR_E1S_P12V_IOUT,
  MB_SNR_E1S_P12V_POUT,
  MB_SNR_E1S_TEMP,
  MB_SNR_ADC128_P12V_AUX,
  NB_SNR_ADC128_P5V,
  MB_SNR_ADC128_P3V3,
  MB_SNR_ADC128_P3V3_AUX,
  MB_SNR_DIMM_CPU0_A0_POWER,
  MB_SNR_DIMM_CPU0_C0_POWER,
  MB_SNR_DIMM_CPU0_A1_POWER,
  MB_SNR_DIMM_CPU0_C1_POWER,
  MB_SNR_DIMM_CPU0_A2_POWER,
  MB_SNR_DIMM_CPU0_C2_POWER,
  MB_SNR_DIMM_CPU0_A3_POWER,
  MB_SNR_DIMM_CPU0_C3_POWER,
  MB_SNR_DIMM_CPU0_A4_POWER,
  MB_SNR_DIMM_CPU0_C4_POWER,
  MB_SNR_DIMM_CPU0_A5_POWER,
  MB_SNR_DIMM_CPU0_C5_POWER,
  MB_SNR_DIMM_CPU0_A6_POWER,
  MB_SNR_DIMM_CPU0_C6_POWER,
  MB_SNR_DIMM_CPU0_A7_POWER,
  MB_SNR_DIMM_CPU0_C7_POWER,
  MB_SNR_DIMM_CPU1_B0_POWER,
  MB_SNR_DIMM_CPU1_D0_POWER,
  MB_SNR_DIMM_CPU1_B1_POWER,
  MB_SNR_DIMM_CPU1_D1_POWER,
  MB_SNR_DIMM_CPU1_B2_POWER,
  MB_SNR_DIMM_CPU1_D2_POWER,
  MB_SNR_DIMM_CPU1_B3_POWER,
  MB_SNR_DIMM_CPU1_D3_POWER,
  MB_SNR_DIMM_CPU1_B4_POWER,
  MB_SNR_DIMM_CPU1_D4_POWER,
  MB_SNR_DIMM_CPU1_B5_POWER,
  MB_SNR_DIMM_CPU1_D5_POWER,
  MB_SNR_DIMM_CPU1_B6_POWER,
  MB_SNR_DIMM_CPU1_D6_POWER,
  MB_SNR_DIMM_CPU1_B7_POWER,
  MB_SNR_DIMM_CPU1_D7_POWER,
};

const uint8_t hsc_sensor_list[] = {
  MB_SNR_HSC_VIN,
  MB_SNR_HSC_IOUT,
  MB_SNR_HSC_PIN,
  MB_SNR_HSC_TEMP,
};

// List of MB discrete sensors to be monitored
const uint8_t mb_discrete_sensor_list[] = {
  MB_SNR_PROCESSOR_FAIL,
  MB_SNR_CPLD_HEALTH,
  MB_SNR_DIMM_CPU0_A0_STATE,
  MB_SNR_DIMM_CPU0_C0_STATE,
  MB_SNR_DIMM_CPU0_A1_STATE,
  MB_SNR_DIMM_CPU0_C1_STATE,
  MB_SNR_DIMM_CPU0_A2_STATE,
  MB_SNR_DIMM_CPU0_C2_STATE,
  MB_SNR_DIMM_CPU0_A3_STATE,
  MB_SNR_DIMM_CPU0_C3_STATE,
  MB_SNR_DIMM_CPU0_A4_STATE,
  MB_SNR_DIMM_CPU0_C4_STATE,
  MB_SNR_DIMM_CPU0_A5_STATE,
  MB_SNR_DIMM_CPU0_C5_STATE,
  MB_SNR_DIMM_CPU0_A6_STATE,
  MB_SNR_DIMM_CPU0_C6_STATE,
  MB_SNR_DIMM_CPU0_A7_STATE,
  MB_SNR_DIMM_CPU0_C7_STATE,
  MB_SNR_DIMM_CPU1_B0_STATE,
  MB_SNR_DIMM_CPU1_D0_STATE,
  MB_SNR_DIMM_CPU1_B1_STATE,
  MB_SNR_DIMM_CPU1_D1_STATE,
  MB_SNR_DIMM_CPU1_B2_STATE,
  MB_SNR_DIMM_CPU1_D2_STATE,
  MB_SNR_DIMM_CPU1_B3_STATE,
  MB_SNR_DIMM_CPU1_D3_STATE,
  MB_SNR_DIMM_CPU1_B4_STATE,
  MB_SNR_DIMM_CPU1_D4_STATE,
  MB_SNR_DIMM_CPU1_B5_STATE,
  MB_SNR_DIMM_CPU1_D5_STATE,
  MB_SNR_DIMM_CPU1_B6_STATE,
  MB_SNR_DIMM_CPU1_D6_STATE,
  MB_SNR_DIMM_CPU1_B7_STATE,
  MB_SNR_DIMM_CPU1_D7_STATE,
};

//CPU
COMMON_CPU_INFO cpu_info_list[] = {
  {CPU_ID0, PECI_CPU0_ADDR},
  {CPU_ID1, PECI_CPU1_ADDR},
};

//12V HSC
//NM
PAL_I2C_BUS_INFO nm_info_list[] = {
  {NM_ID0, NM_IPMB_BUS_ID, NM_SLAVE_ADDR},
};


PAL_DPM_DEV_INFO dpm_info_list[] = {
  {DPM_0, I2C_BUS_34, 0x82, 0.004, 0, 0 },
  {DPM_1, I2C_BUS_34, 0x84, 0.004, 0, 0 },
  {DPM_2, I2C_BUS_34, 0x86, 0.004, 0, 0 },
  {DPM_3, I2C_BUS_34, 0x88, 0.004, 0, 0 },
  {DPM_4, I2C_BUS_34, 0x8A, 0.004, 0, 0 },
};

//E1S
PAL_I2C_BUS_INFO e1s_info_list[] = {
  {E1S_0, I2C_BUS_31, 0xD4},
};

PAL_DIMM_PMIC_INFO dimm_pmic_list[] = {
  {DIMM_ID0,  0x90, 0},
  {DIMM_ID1,  0x92, 0},
  {DIMM_ID2,  0x94, 0},
  {DIMM_ID3,  0x96, 0},
  {DIMM_ID4,  0x98, 0},
  {DIMM_ID5,  0x9A, 0},
  {DIMM_ID6,  0x9C, 0},
  {DIMM_ID7,  0x9E, 0},
  {DIMM_ID8,  0x90, 1},
  {DIMM_ID9,  0x92, 1},
  {DIMM_ID10, 0x94, 1},
  {DIMM_ID11, 0x96, 1},
  {DIMM_ID12, 0x98, 1},
  {DIMM_ID13, 0x9A, 1},
  {DIMM_ID14, 0x9C, 1},
  {DIMM_ID15, 0x9E, 1},
};

char *adc128_devs[] = {
  "adc128d818-i2c-20-1d",
};

char *max11617_devs[] = {
  MAX11617_DIR,
};

PAL_ADC_CH_INFO max11617_ch_info[] = {
  {ADC_CH0, 7870,  1210},
  {ADC_CH1, 12000, 2500},
  {ADC_CH2, 12000, 2500},
  {ADC_CH3, 1800,  909},
  {ADC_CH4, 511,   470},
  {ADC_CH5, 511,   470},
  {ADC_CH6, 511,   470},
  {ADC_CH7, 120,   25},
};

char **adc_chips = adc128_devs;


//{SensorName, ID, FUNCTION, PWR_STATUS, {UCR, UNC, UNR, LCR, LNC, LNR, Pos, Neg}
PAL_SENSOR_MAP mb_sensor_map[] = {
  {NULL, 0, NULL, 0, {0, 0, 0, 0, 0, 0, 0, 0}, 0}, //0x00
  {"E1S_P3V3_VOLT", DPM_1, read_dpm_vout, false, {3.465, 0, 0, 3.135, 0, 0, 0, 0}, VOLT}, //0x01
  {NULL, 0, NULL, 0, {0, 0, 0, 0, 0, 0, 0, 0}, 0}, //0x02
  {"E1S_P12V_CURR", ADC_CH7, read_iic_adc_val, false, {0, 0, 0, 0, 0, 0, 0, 0}, CURR}, //0x03
  {"E1S_P12V_PWR", E1S_0, read_e1s_power, false, {0, 0, 0, 0, 0, 0, 0, 0}, POWER}, //0x04
  {"E1S_TEMP", E1S_0, read_e1s_temp, false, {70.0, 0, 0, 10.0, 0, 0, 0, 0}, TEMP}, //0x05
  {NULL, 0, NULL, 0, {0, 0, 0, 0, 0, 0, 0, 0}, 0}, //0x06
  {NULL, 0, NULL, 0, {0, 0, 0, 0, 0, 0, 0, 0}, 0}, //0x07

  {NULL, 0, NULL, 0, {0, 0, 0, 0, 0, 0, 0, 0}, 0}, //0x08
  {NULL, 0, NULL, 0, {0, 0, 0, 0, 0, 0, 0, 0}, 0}, //0x09
  {NULL, 0, NULL, 0, {0, 0, 0, 0, 0, 0, 0, 0}, 0}, //0x0A
  {"HSC_VOLT",  HSC_ID0, read_hsc_vin, true, {14.333, 0, 0, 10.091, 0, 0, 0, 0}, VOLT}, //0x0B
  {"HSC_CURR",  HSC_ID0, read_hsc_iout, true, {0, 0, 0, 0, 0, 0, 0, 0}, CURR}, //0x0C
  {"HSC_PWR",   HSC_ID0, read_hsc_pin, true, {0, 0, 0, 0, 0, 0, 0, 0}, POWER}, //0x0D
  {"HSC_TEMP",  HSC_ID0, read_hsc_temp, true, {120.0, 0, 0, 10.0, 0, 0, 0, 0}, TEMP}, //0x0E
  {"HSC_PEAK_PIN",  HSC_ID0, read_hsc_peak_pin, true, {2832.0, 0, 0, 0, 0, 0, 0, 0}, POWER}, //0x0F

  {"OUTLET_L_TEMP", TEMP_OUTLET_L, read_mb_temp, true, {55.0, 0, 0, 10.0, 0, 0, 0, 0}, TEMP}, //0x10
  {"INLET_L_TEMP", TEMP_INLET_L, read_mb_temp, true, {55.0, 0, 0, 10.0, 0, 0, 0, 0}, TEMP}, //0x11
  {"OUTLET_R_TEMP", TEMP_OUTLET_R, read_mb_temp, true, {75.0, 0, 0, 10.0, 0, 0, 0, 0}, TEMP}, //0x12
  {"INLET_R_TEMP", TEMP_INLET_R, read_mb_temp, true, {75.0, 0, 0, 10.0, 0, 0, 0, 0}, TEMP}, //0x13
  {NULL, 0, NULL, 0, {0, 0, 0, 0, 0, 0, 0, 0}, 0}, //0x14
  {NULL, 0, NULL, 0, {0, 0, 0, 0, 0, 0, 0, 0}, 0}, //0x15
  {NULL, 0, NULL, 0, {0, 0, 0, 0, 0, 0, 0, 0}, 0}, //0x16
  {NULL, 0, NULL, 0, {0, 0, 0, 0, 0, 0, 0, 0}, 0}, //0x17

  {"CPU0_TEMP", CPU_ID0, read_cpu_temp, false, {88.0, 0, 0, 10.0, 0, 0, 0, 0}, TEMP}, //0x18
  {"CPU1_TEMP", CPU_ID1, read_cpu_temp, false, {88.0, 0, 0, 10.0, 0, 0, 0, 0}, TEMP}, //0x19
  {"CPU0_THERM_MARGIN", CPU_ID0, read_cpu_thermal_margin, false, {-5.0, 0, 0, -81.0, 0, 0, 0, 0}, TEMP}, //0x1A
  {"CPU1_THERM_MARGIN", CPU_ID1, read_cpu_thermal_margin, false, {-5.0, 0, 0, -81.0, 0, 0, 0, 0}, TEMP}, //0x1B
  {"CPU0_TJMAX", CPU_ID0, read_cpu_tjmax, false, {0, 0, 0, 0, 0, 0, 0, 0}, TEMP}, //0x1C
  {"CPU1_TJMAX", CPU_ID1, read_cpu_tjmax, false, {0, 0, 0, 0, 0, 0, 0, 0}, TEMP}, //0x1D
  {"CPU0_PKG_PWR", CPU_ID0, read_cpu_pkg_pwr, false, {0, 0, 0, 0.0, 0, 0, 0, 0}, POWER}, //0x1E
  {"CPU1_PKG_PWR", CPU_ID1, read_cpu_pkg_pwr, false, {0, 0, 0, 0.0, 0, 0, 0, 0}, POWER}, //0x1F

  {"CPU0_DIMM_A0_C0_TEMP", DIMM_CPU0_CRPA, read_cpu_dimm_temp, false, {85.0, 0, 0, 10.0, 0, 0, 0, 0}, TEMP}, //0x20
  {"CPU0_DIMM_A1_C1_TEMP", DIMM_CPU0_CRPB, read_cpu_dimm_temp, false, {85.0, 0, 0, 10.0, 0, 0, 0, 0}, TEMP}, //0x21
  {"CPU0_DIMM_A2_C2_TEMP", DIMM_CPU0_CRPC, read_cpu_dimm_temp, false, {85.0, 0, 0, 10.0, 0, 0, 0, 0}, TEMP}, //0x22
  {"CPU0_DIMM_A3_C3_TEMP", DIMM_CPU0_CRPD, read_cpu_dimm_temp, false, {85.0, 0, 0, 10.0, 0, 0, 0, 0}, TEMP}, //0x23
  {"CPU0_DIMM_A4_C4_TEMP", DIMM_CPU0_CRPE, read_cpu_dimm_temp, false, {85.0, 0, 0, 10.0, 0, 0, 0, 0}, TEMP}, //0x24
  {"CPU0_DIMM_A5_C5_TEMP", DIMM_CPU0_CRPF, read_cpu_dimm_temp, false, {85.0, 0, 0, 10.0, 0, 0, 0, 0}, TEMP}, //0x25
  {"CPU0_DIMM_A6_C6_TEMP", DIMM_CPU0_CRPG, read_cpu_dimm_temp, false, {85.0, 0, 0, 10.0, 0, 0, 0, 0}, TEMP}, //0x26
  {"CPU0_DIMM_A7_C7_TEMP", DIMM_CPU0_CRPH, read_cpu_dimm_temp, false, {85.0, 0, 0, 10.0, 0, 0, 0, 0}, TEMP}, //0x27

  {"CPU1_DIMM_B0_D0_TEMP", DIMM_CPU1_CRPA, read_cpu_dimm_temp, false, {85.0, 0, 0, 10.0, 0, 0, 0, 0}, TEMP}, //0x28
  {"CPU1_DIMM_B1_D1_TEMP", DIMM_CPU1_CRPB, read_cpu_dimm_temp, false, {85.0, 0, 0, 10.0, 0, 0, 0, 0}, TEMP}, //0x29
  {"CPU1_DIMM_B2_D2_TEMP", DIMM_CPU1_CRPC, read_cpu_dimm_temp, false, {85.0, 0, 0, 10.0, 0, 0, 0, 0}, TEMP}, //0x2A
  {"CPU1_DIMM_B3_D3_TEMP", DIMM_CPU1_CRPD, read_cpu_dimm_temp, false, {85.0, 0, 0, 10.0, 0, 0, 0, 0}, TEMP}, //0x2B
  {"CPU1_DIMM_B4_D4_TEMP", DIMM_CPU1_CRPE, read_cpu_dimm_temp, false, {85.0, 0, 0, 10.0, 0, 0, 0, 0}, TEMP}, //0x2C
  {"CPU1_DIMM_B5_D5_TEMP", DIMM_CPU1_CRPF, read_cpu_dimm_temp, false, {85.0, 0, 0, 10.0, 0, 0, 0, 0}, TEMP}, //0x2D
  {"CPU1_DIMM_B6_D6_TEMP", DIMM_CPU1_CRPG, read_cpu_dimm_temp, false, {85.0, 0, 0, 10.0, 0, 0, 0, 0}, TEMP}, //0x2E
  {"CPU1_DIMM_B7_D7_TEMP", DIMM_CPU1_CRPH, read_cpu_dimm_temp, false, {85.0, 0, 0, 10.0, 0, 0, 0, 0}, TEMP}, //0x2F

  {"VR_CPU0_VCCIN_VOLT", VR_ID0, read_vr_vout, false, {1.88, 0, 0, 1.6, 0, 0, 0, 0}, VOLT}, //0x30
  {"VR_CPU0_VCCIN_TEMP", VR_ID0, read_vr_temp, false, {105.0, 0, 0, 10.0, 0, 0, 0, 0}, TEMP}, //0x31
  {"VR_CPU0_VCCIN_CURR", VR_ID0, read_vr_iout, false, {0, 0, 0, 0, 0, 0, 0, 0}, CURR}, //0X32
  {"VR_CPU0_VCCIN_PWR",  VR_ID0, read_vr_pout, false, {0, 0, 0, 0, 0, 0, 0, 0}, POWER}, //0x33
  {"VR_CPU0_FIVRA_VOLT", VR_ID1, read_vr_vout, false, {1.88, 0, 0, 1.6, 0, 0, 0, 0}, VOLT}, //0x34
  {"VR_CPU0_FIVRA_TEMP", VR_ID1, read_vr_temp, false, {105.0, 0, 0, 10.0, 0, 0, 0, 0}, TEMP}, //0x35
  {"VR_CPU0_FIVRA_CURR", VR_ID1, read_vr_iout, false, {0, 0, 0, 0, 0, 0, 0, 0}, CURR}, //0X36
  {"VR_CPU0_FIVRA_PWR",  VR_ID1, read_vr_pout, false, {0, 0, 0, 0, 0, 0, 0, 0}, POWER}, //0x37
  {"VR_CPU0_FAON_VOLT",  VR_ID2, read_vr_vout, false, {1.05, 0, 0, 0.7, 0, 0, 0, 0}, VOLT}, //0x38
  {"VR_CPU0_FAON_TEMP",  VR_ID2, read_vr_temp, false, {105.0, 0, 0, 10.0, 0, 0, 0, 0}, TEMP}, //0x39
  {"VR_CPU0_FAON_CURR",  VR_ID2, read_vr_iout, false, {0, 0, 0, 0, 0, 0, 0, 0}, CURR}, //0X3A
  {"VR_CPU0_FAON_PWR",   VR_ID2, read_vr_pout, false, {0, 0, 0, 0, 0, 0, 0, 0}, POWER}, //0x3B
  {"VR_CPU0_VCCFA_VOLT", VR_ID3, read_vr_vout, false, {1.87, 0, 0, 1.6, 0, 0, 0, 0}, VOLT}, //0x3C
  {"VR_CPU0_VCCFA_TEMP", VR_ID3, read_vr_temp, false, {105.0, 0, 0, 10.0, 0, 0, 0, 0}, TEMP}, //0x3D
  {"VR_CPU0_VCCFA_CURR", VR_ID3, read_vr_iout, false, {0, 0, 0, 0, 0, 0, 0, 0}, CURR}, //0X3E
  {"VR_CPU0_VCCFA_PWR",  VR_ID3, read_vr_pout, false, {0, 0, 0, 0, 0, 0, 0, 0}, POWER}, //0x3F
  {"VR_CPU0_VCCD_HV_VOLT", VR_ID4, read_vr_vout, false, {1.189, 0, 0, 1.081, 0, 0, 0, 0}, VOLT}, //0x40
  {"VR_CPU0_VCCD_HV_TEMP", VR_ID4, read_vr_temp, false, {105.0, 0, 0, 10.0, 0, 0, 0, 0}, TEMP}, //0x41
  {"VR_CPU0_VCCD_HV_CURR", VR_ID4, read_vr_iout, false, {0, 0, 0, 0, 0, 0, 0, 0}, CURR}, //0X42
  {"VR_CPU0_VCCD_HV_PWR", VR_ID4, read_vr_pout, false, {0, 0, 0, 0, 0, 0, 0, 0}, POWER}, //0x43
  {"P12V_AUX_IN0_VOLT", ADC_CH0, read_iic_adc_val, true, {13.2, 0, 0, 10.8, 0, 0, 0, 0}, VOLT}, //0x44
  {"P5V_IN3_VOLT",      ADC_CH3, read_iic_adc_val, false, {5.25, 0, 0, 4.70, 0, 0, 0, 0}, VOLT}, //0x45
  {"P3V3_IN4_VOLT",     ADC_CH4, read_iic_adc_val, false, {3.465, 0, 0, 3.135, 0, 0, 0, 0}, VOLT}, //0x46
  {"P3V3_AUX_IN5_VOLT", ADC_CH5, read_iic_adc_val, true, {3.465, 0, 0, 3.135, 0, 0, 0, 0}, VOLT}, //0x47

  {"VR_CPU1_VCCIN_VOLT", VR_ID5, read_vr_vout, false, {1.88, 0, 0, 1.6, 0, 0, 0, 0}, VOLT}, //0x48
  {"VR_CPU1_VCCIN_TEMP", VR_ID5, read_vr_temp, false, {105.0, 0, 0, 10.0, 0, 0, 0, 0}, TEMP}, //0x49
  {"VR_CPU1_VCCIN_CURR", VR_ID5, read_vr_iout, false, {0, 0, 0, 0, 0, 0, 0, 0}, CURR}, //0X4A
  {"VR_CPU1_VCCIN_PWR", VR_ID5, read_vr_pout, false, {0, 0, 0, 0, 0, 0, 0, 0}, POWER}, //0x4B
  {"VR_CPU1_FIVRA_VOLT", VR_ID6, read_vr_vout, false, {1.88, 0, 0, 1.6, 0, 0, 0, 0}, VOLT}, //0x4C
  {"VR_CPU1_FIVRA_TEMP", VR_ID6, read_vr_temp, false, {105.0, 0, 0, 10.0, 0, 0, 0, 0}, TEMP}, //0x4D
  {"VR_CPU1_FIVRA_CURR", VR_ID6, read_vr_iout, false, {0, 0, 0, 0, 0, 0, 0, 0}, CURR}, //0X4E
  {"VR_CPU1_FIVRA_PWR", VR_ID6, read_vr_pout, false, {0, 0, 0, 0, 0, 0, 0, 0}, POWER}, //0x4F
  {"VR_CPU1_FAON_VOLT", VR_ID7, read_vr_vout, false, {1.05, 0, 0, 0.7, 0, 0, 0, 0}, VOLT}, //0x50
  {"VR_CPU1_FAON_TEMP", VR_ID7, read_vr_temp, false, {105.0, 0, 0, 10.0, 0, 0, 0, 0}, TEMP}, //0x51
  {"VR_CPU1_FAON_CURR", VR_ID7, read_vr_iout, false, {0, 0, 0, 0, 0, 0, 0, 0}, CURR}, //0X52
  {"VR_CPU1_FAON_PWR", VR_ID7, read_vr_pout, false, {0, 0, 0, 0, 0, 0, 0, 0}, POWER}, //0x53
  {"VR_CPU1_VCCFA_VOLT", VR_ID8, read_vr_vout, false, {1.87, 0, 0, 1.6, 0, 0, 0, 0}, VOLT}, //0x54
  {"VR_CPU1_VCCFA_TEMP", VR_ID8, read_vr_temp, false, {105.0, 0, 0, 10.0, 0, 0, 0, 0}, TEMP}, //0x55
  {"VR_CPU1_VCCFA_CURR", VR_ID8, read_vr_iout, false, {0, 0, 0, 0, 0, 0, 0, 0}, CURR}, //0X56
  {"VR_CPU1_VCCFA_PWR", VR_ID8, read_vr_pout, false, {0, 0, 0, 0, 0, 0, 0, 0}, POWER}, //0x57
  {"VR_CPU1_VCCD_HV_VOLT", VR_ID9, read_vr_vout, false, {1.189, 0, 0, 1.081, 0, 0, 0, 0}, VOLT}, //0x58
  {"VR_CPU1_VCCD_HV_TEMP", VR_ID9, read_vr_temp, false, {105.0, 0, 0, 10.0, 0, 0, 0, 0}, TEMP}, //0x59
  {"VR_CPU1_VCCD_HV_CURR", VR_ID9, read_vr_iout, false, {0, 0, 0, 0, 0, 0, 0, 0}, CURR}, //0X5A
  {"VR_CPU1_VCCD_HV_PWR", VR_ID9, read_vr_pout, false, {0, 0, 0, 0, 0, 0, 0, 0}, POWER}, //0x5B
  {NULL, 0, NULL, 0, {0, 0, 0, 0, 0, 0, 0, 0}, 0}, //0x5C
  {NULL, 0, NULL, 0, {0, 0, 0, 0, 0, 0, 0, 0}, 0}, //0x5D
  {"P3V_BAT_VOLT", ADC7, read_bat_val, true, {3.4, 0, 0, 2.6, 0, 0, 0, 0}, VOLT}, //0x5E
  {"PCH_TEMP", NM_ID0, read_NM_pch_temp, false, {85.0, 0, 0, 10.0, 0, 0, 0, 0}, TEMP}, //0x5F

  {"CPU0_DIMM_A0_PWR", DIMM_ID0,  read_cpu_dimm_power, false, {0, 0, 0, 0, 0, 0, 0, 0}, POWER}, //0x60
  {"CPU0_DIMM_C0_PWR", DIMM_ID1,  read_cpu_dimm_power, false, {0, 0, 0, 0, 0, 0, 0, 0}, POWER}, //0x61
  {"CPU0_DIMM_A1_PWR", DIMM_ID2,  read_cpu_dimm_power, false, {0, 0, 0, 0, 0, 0, 0, 0}, POWER}, //0x62
  {"CPU0_DIMM_C1_PWR", DIMM_ID3,  read_cpu_dimm_power, false, {0, 0, 0, 0, 0, 0, 0, 0}, POWER}, //0x63
  {"CPU0_DIMM_A2_PWR", DIMM_ID4,  read_cpu_dimm_power, false, {0, 0, 0, 0, 0, 0, 0, 0}, POWER}, //0x64
  {"CPU0_DIMM_C2_PWR", DIMM_ID5,  read_cpu_dimm_power, false, {0, 0, 0, 0, 0, 0, 0, 0}, POWER}, //0x65
  {"CPU0_DIMM_A3_PWR", DIMM_ID6,  read_cpu_dimm_power, false, {0, 0, 0, 0, 0, 0, 0, 0}, POWER}, //0x66
  {"CPU0_DIMM_C3_PWR", DIMM_ID7,  read_cpu_dimm_power, false, {0, 0, 0, 0, 0, 0, 0, 0}, POWER}, //0x67
  {"CPU0_DIMM_A4_PWR", DIMM_ID8,  read_cpu_dimm_power, false, {0, 0, 0, 0, 0, 0, 0, 0}, POWER}, //0x68
  {"CPU0_DIMM_C4_PWR", DIMM_ID9,  read_cpu_dimm_power, false, {0, 0, 0, 0, 0, 0, 0, 0}, POWER}, //0x69
  {"CPU0_DIMM_A5_PWR", DIMM_ID10, read_cpu_dimm_power, false, {0, 0, 0, 0, 0, 0, 0, 0}, POWER}, //0x6A
  {"CPU0_DIMM_C5_PWR", DIMM_ID11, read_cpu_dimm_power, false, {0, 0, 0, 0, 0, 0, 0, 0}, POWER}, //0x6B
  {"CPU0_DIMM_A6_PWR", DIMM_ID12, read_cpu_dimm_power, false, {0, 0, 0, 0, 0, 0, 0, 0}, POWER}, //0x6C
  {"CPU0_DIMM_C6_PWR", DIMM_ID13, read_cpu_dimm_power, false, {0, 0, 0, 0, 0, 0, 0, 0}, POWER}, //0x6D
  {"CPU0_DIMM_A7_PWR", DIMM_ID14, read_cpu_dimm_power, false, {0, 0, 0, 0, 0, 0, 0, 0}, POWER}, //0x6E
  {"CPU0_DIMM_C7_PWR", DIMM_ID15, read_cpu_dimm_power, false, {0, 0, 0, 0, 0, 0, 0, 0}, POWER}, //0x6F

  {"CPU1_DIMM_B0_PWR", DIMM_ID16, read_cpu_dimm_power, false, {0, 0, 0, 0, 0, 0, 0, 0}, POWER}, //0x70
  {"CPU1_DIMM_D0_PWR", DIMM_ID17, read_cpu_dimm_power, false, {0, 0, 0, 0, 0, 0, 0, 0}, POWER}, //0x71
  {"CPU1_DIMM_B1_PWR", DIMM_ID18, read_cpu_dimm_power, false, {0, 0, 0, 0, 0, 0, 0, 0}, POWER}, //0x72
  {"CPU1_DIMM_D1_PWR", DIMM_ID19, read_cpu_dimm_power, false, {0, 0, 0, 0, 0, 0, 0, 0}, POWER}, //0x73
  {"CPU1_DIMM_B2_PWR", DIMM_ID20, read_cpu_dimm_power, false, {0, 0, 0, 0, 0, 0, 0, 0}, POWER}, //0x74
  {"CPU1_DIMM_D2_PWR", DIMM_ID21, read_cpu_dimm_power, false, {0, 0, 0, 0, 0, 0, 0, 0}, POWER}, //0x75
  {"CPU1_DIMM_B3_PWR", DIMM_ID22, read_cpu_dimm_power, false, {0, 0, 0, 0, 0, 0, 0, 0}, POWER}, //0x76
  {"CPU1_DIMM_D3_PWR", DIMM_ID23, read_cpu_dimm_power, false, {0, 0, 0, 0, 0, 0, 0, 0}, POWER}, //0x77
  {"CPU1_DIMM_B4_PWR", DIMM_ID24, read_cpu_dimm_power, false, {0, 0, 0, 0, 0, 0, 0, 0}, POWER}, //0x78
  {"CPU1_DIMM_D4_PWR", DIMM_ID25, read_cpu_dimm_power, false, {0, 0, 0, 0, 0, 0, 0, 0}, POWER}, //0x79
  {"CPU1_DIMM_B5_PWR", DIMM_ID26, read_cpu_dimm_power, false, {0, 0, 0, 0, 0, 0, 0, 0}, POWER}, //0x7A
  {"CPU1_DIMM_D5_PWR", DIMM_ID27, read_cpu_dimm_power, false, {0, 0, 0, 0, 0, 0, 0, 0}, POWER}, //0x7B
  {"CPU1_DIMM_B6_PWR", DIMM_ID28, read_cpu_dimm_power, false, {0, 0, 0, 0, 0, 0, 0, 0}, POWER}, //0x7C
  {"CPU1_DIMM_D6_PWR", DIMM_ID29, read_cpu_dimm_power, false, {0, 0, 0, 0, 0, 0, 0, 0}, POWER}, //0x7D
  {"CPU1_DIMM_B7_PWR", DIMM_ID30, read_cpu_dimm_power, false, {0, 0, 0, 0, 0, 0, 0, 0}, POWER}, //0x7E
  {"CPU1_DIMM_D7_PWR", DIMM_ID31, read_cpu_dimm_power, false, {0, 0, 0, 0, 0, 0, 0, 0}, POWER}, //0x7F

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
  {"PROCESSOR_FAIL", FRU_MB, read_frb3, 0, {0, 0, 0, 0, 0, 0, 0, 0}, STATE}, //0xA1
  {"CPLD_HEALTH", 0, read_cpld_health, 0, {0, 0, 0, 0, 0, 0, 0, 0}, STATE}, //0xA2
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

  {"CPU0_DIMM_A0_STATE", DIMM_ID0,  read_cpu_dimm_state, false, {0, 0, 0, 0, 0, 0, 0, 0}, STATE}, //0xB0
  {"CPU0_DIMM_C0_STATE", DIMM_ID1,  read_cpu_dimm_state, false, {0, 0, 0, 0, 0, 0, 0, 0}, STATE}, //0xB1
  {"CPU0_DIMM_A1_STATE", DIMM_ID2,  read_cpu_dimm_state, false, {0, 0, 0, 0, 0, 0, 0, 0}, STATE}, //0xB2
  {"CPU0_DIMM_C1_STATE", DIMM_ID3,  read_cpu_dimm_state, false, {0, 0, 0, 0, 0, 0, 0, 0}, STATE}, //0xB3
  {"CPU0_DIMM_A2_STATE", DIMM_ID4,  read_cpu_dimm_state, false, {0, 0, 0, 0, 0, 0, 0, 0}, STATE}, //0xB4
  {"CPU0_DIMM_C2_STATE", DIMM_ID5,  read_cpu_dimm_state, false, {0, 0, 0, 0, 0, 0, 0, 0}, STATE}, //0xB5
  {"CPU0_DIMM_A3_STATE", DIMM_ID6,  read_cpu_dimm_state, false, {0, 0, 0, 0, 0, 0, 0, 0}, STATE}, //0xB6
  {"CPU0_DIMM_C3_STATE", DIMM_ID7,  read_cpu_dimm_state, false, {0, 0, 0, 0, 0, 0, 0, 0}, STATE}, //0xB7
  {"CPU0_DIMM_A4_STATE", DIMM_ID8,  read_cpu_dimm_state, false, {0, 0, 0, 0, 0, 0, 0, 0}, STATE}, //0xB8
  {"CPU0_DIMM_C4_STATE", DIMM_ID9,  read_cpu_dimm_state, false, {0, 0, 0, 0, 0, 0, 0, 0}, STATE}, //0xB9
  {"CPU0_DIMM_A5_STATE", DIMM_ID10, read_cpu_dimm_state, false, {0, 0, 0, 0, 0, 0, 0, 0}, STATE}, //0xBA
  {"CPU0_DIMM_C5_STATE", DIMM_ID11, read_cpu_dimm_state, false, {0, 0, 0, 0, 0, 0, 0, 0}, STATE}, //0xBB
  {"CPU0_DIMM_A6_STATE", DIMM_ID12, read_cpu_dimm_state, false, {0, 0, 0, 0, 0, 0, 0, 0}, STATE}, //0xBC
  {"CPU0_DIMM_C6_STATE", DIMM_ID13, read_cpu_dimm_state, false, {0, 0, 0, 0, 0, 0, 0, 0}, STATE}, //0xBD
  {"CPU0_DIMM_A7_STATE", DIMM_ID14, read_cpu_dimm_state, false, {0, 0, 0, 0, 0, 0, 0, 0}, STATE}, //0xBE
  {"CPU0_DIMM_C7_STATE", DIMM_ID15, read_cpu_dimm_state, false, {0, 0, 0, 0, 0, 0, 0, 0}, STATE}, //0xBF

  {"CPU1_DIMM_B0_STATE", DIMM_ID16, read_cpu_dimm_state, false, {0, 0, 0, 0, 0, 0, 0, 0}, STATE}, //0xC0
  {"CPU1_DIMM_D0_STATE", DIMM_ID17, read_cpu_dimm_state, false, {0, 0, 0, 0, 0, 0, 0, 0}, STATE}, //0xC1
  {"CPU1_DIMM_B1_STATE", DIMM_ID18, read_cpu_dimm_state, false, {0, 0, 0, 0, 0, 0, 0, 0}, STATE}, //0xC2
  {"CPU1_DIMM_D1_STATE", DIMM_ID19, read_cpu_dimm_state, false, {0, 0, 0, 0, 0, 0, 0, 0}, STATE}, //0xC3
  {"CPU1_DIMM_B2_STATE", DIMM_ID20, read_cpu_dimm_state, false, {0, 0, 0, 0, 0, 0, 0, 0}, STATE}, //0xC4
  {"CPU1_DIMM_D2_STATE", DIMM_ID21, read_cpu_dimm_state, false, {0, 0, 0, 0, 0, 0, 0, 0}, STATE}, //0xC5
  {"CPU1_DIMM_B3_STATE", DIMM_ID22, read_cpu_dimm_state, false, {0, 0, 0, 0, 0, 0, 0, 0}, STATE}, //0xC6
  {"CPU1_DIMM_D3_STATE", DIMM_ID23, read_cpu_dimm_state, false, {0, 0, 0, 0, 0, 0, 0, 0}, STATE}, //0xC7
  {"CPU1_DIMM_B4_STATE", DIMM_ID24, read_cpu_dimm_state, false, {0, 0, 0, 0, 0, 0, 0, 0}, STATE}, //0xC8
  {"CPU1_DIMM_D4_STATE", DIMM_ID25, read_cpu_dimm_state, false, {0, 0, 0, 0, 0, 0, 0, 0}, STATE}, //0xC9
  {"CPU1_DIMM_B5_STATE", DIMM_ID26, read_cpu_dimm_state, false, {0, 0, 0, 0, 0, 0, 0, 0}, STATE}, //0xCA
  {"CPU1_DIMM_D5_STATE", DIMM_ID27, read_cpu_dimm_state, false, {0, 0, 0, 0, 0, 0, 0, 0}, STATE}, //0xCB
  {"CPU1_DIMM_B6_STATE", DIMM_ID28, read_cpu_dimm_state, false, {0, 0, 0, 0, 0, 0, 0, 0}, STATE}, //0xCC
  {"CPU1_DIMM_D6_STATE", DIMM_ID29, read_cpu_dimm_state, false, {0, 0, 0, 0, 0, 0, 0, 0}, STATE}, //0xCD
  {"CPU1_DIMM_B7_STATE", DIMM_ID30, read_cpu_dimm_state, false, {0, 0, 0, 0, 0, 0, 0, 0}, STATE}, //0xCE
  {"CPU1_DIMM_D7_STATE", DIMM_ID31, read_cpu_dimm_state, false, {0, 0, 0, 0, 0, 0, 0, 0}, STATE}, //0xCF

};

extern struct snr_map sensor_map[];

size_t mb_sensor_cnt = sizeof(mb_sensor_list)/sizeof(uint8_t);
size_t hsc_sensor_cnt = sizeof(hsc_sensor_list)/sizeof(uint8_t);
size_t mb_discrete_sensor_cnt = sizeof(mb_discrete_sensor_list)/sizeof(uint8_t);

int
read_adc_val(uint8_t fru, uint8_t sensor_num, float *value) {
  uint8_t adc_id = sensor_map[fru].map[sensor_num].id;
  if (adc_id >= ADC_NUM_CNT) {
    return -1;
  }
  return sensors_read_adc(sensor_map[fru].map[sensor_num].snr_name, value);
}

static int
read_bat_val(uint8_t fru, uint8_t sensor_num, float *value) {
  int ret = -1;

  gpio_desc_t *gp_batt = gpio_open_by_shadow(GPIO_P3V_BAT_SCALED_EN);
  if (!gp_batt) {
    return -1;
  }
  if (gpio_set_value(gp_batt, GPIO_VALUE_HIGH)) {
    goto bail;
  }

#ifdef DEBUG
  syslog(LOG_DEBUG, "%s %s\n", __func__, path);
#endif
  msleep(20);
  ret = read_adc_val(fru, sensor_num, value);
  if (gpio_set_value(gp_batt, GPIO_VALUE_LOW)) {
    goto bail;
  }

bail:
  gpio_close(gp_batt);
  return ret;
}

static bool is_max11617_chip(void) {
  uint8_t id;
  static bool val=false;
  static bool cached=false;

  if (!cached) {
    if (pal_get_platform_id(&id))
      return false;

    if (GETBIT(id, 1)) {
       adc_chips = max11617_devs;
       val = true;
    }
    cached = true;
  }
  return val;
}

static int sensors_read_maxim(const char *dev, int channel, float *data)
{
  int val = 0;
  char ain_name[30] = {0};
  char dev_dir[LARGEST_DEVICE_NAME] = {0};
  float R1 = max11617_ch_info[channel].r1;
  float R2 = max11617_ch_info[channel].r2;


  snprintf(ain_name, sizeof(ain_name), IIO_AIN_NAME, channel);
  snprintf(dev_dir, sizeof(dev_dir), dev, ain_name);

  if(access(dev_dir, F_OK)) {
    return ERR_SENSOR_NA;
  }

  if (read_device(dev_dir, &val) < 0) {
    syslog(LOG_ERR, "%s: dev_dir: %s read fail", __func__, dev_dir);
    return ERR_FAILURE;
  }


  *data = (float)val * 2048 / 4096 * (R1 + R2) / R2 /1000;
  if(channel == ADC_CH1 || channel == ADC_CH2)
    *data = 0.0056 * pow(*data, 2) + 1.158 * (*data) + 0.07;
  return 0;
}

int
read_iic_adc_val(uint8_t fru, uint8_t sensor_num, float *value) {
  int ret;
  uint8_t ch_id = sensor_map[fru].map[sensor_num].id;

  if(is_max11617_chip())
    ret = sensors_read_maxim(adc_chips[ch_id/8], ch_id, value);
  else
    ret = sensors_read(adc_chips[ch_id/8], sensor_map[fru].map[sensor_num].snr_name, value);
  return ret;
}

int
oper_iic_adc_power(uint8_t fru, uint8_t volt_snr_num, uint8_t curr_snr_num, float *value) {
  int ret;
  float iout=0;
  float vout=0;

  ret = read_iic_adc_val(fru, curr_snr_num, &iout);
  if(ret)
    return -1;

  ret = read_hsc_vin(FRU_MB, volt_snr_num, &vout);
  if(ret)
    return -1;

  *value = iout * vout;
  return 0;
}

static int
read_e1s_power(uint8_t fru, uint8_t sensor_num, float *value) {
  int ret;

  ret = oper_iic_adc_power(fru, MB_SNR_HSC_VIN, MB_SNR_E1S_P12V_IOUT, value);
  return ret;
}

static int
read_e1s_temp(uint8_t fru, uint8_t sensor_num, float *value) {
  int fd = 0, ret = -1;
  char fn[32];
  uint8_t tlen, rlen, addr, bus;
  uint8_t tbuf[16] = {0};
  uint8_t rbuf[16] = {0};
  uint8_t hd_id = sensor_map[fru].map[sensor_num].id;
  static uint8_t retry=0;

  bus = e1s_info_list[hd_id].bus;
  addr = e1s_info_list[hd_id].slv_addr;

  snprintf(fn, sizeof(fn), "/dev/i2c-%d", bus);
  fd = open(fn, O_RDWR);
  if (fd < 0) {
    return -1;
  }

  //Temp Register
  tbuf[0] = 0x03;
  tlen = 1;
  rlen = 1;

  ret = i2c_rdwr_msg_transfer(fd, addr, tbuf, tlen, rbuf, rlen);

#ifdef DEBUG
  syslog(LOG_DEBUG, "%s Temp[%d]=%x bus=%x slavaddr=%x\n", __func__, hd_id, rbuf[0], bus, addr);
#endif

  if( ret < 0 || (rbuf[0] >= 0x80) ) {
    retry++;
    ret = retry_err_handle(retry, 5);
    goto err_exit;
  }

  *value = rbuf[0];
  retry=0;
err_exit:
  if (fd > 0) {
    close(fd);
  }
  return ret;
}

static int
read_cpu_tjmax(uint8_t fru, uint8_t sensor_num, float *value) {
  uint8_t cpu_id = sensor_map[fru].map[sensor_num].id;

  if(!is_cpu_socket_occupy(cpu_id))
    return READING_NA;

  return lib_get_cpu_tjmax(cpu_id, value);
}

static int
read_cpu_thermal_margin(uint8_t fru, uint8_t sensor_num, float *value) {
  uint8_t cpu_id = sensor_map[fru].map[sensor_num].id;

  if(!is_cpu_socket_occupy(cpu_id))
    return READING_NA;

  if(pal_bios_completed(fru) != true) {
    return READING_NA;
  }

  return lib_get_cpu_thermal_margin(cpu_id, value);
}

static int
read_cpu_pkg_pwr(uint8_t fru, uint8_t sensor_num, float *value) {
  uint8_t rbuf[32] = {0x00};
  uint8_t domain_id = NMS_SIGNAL_COMPONENT | NMS_DOMAIN_ID_CPU_SUBSYSTEM;
  uint8_t policy_id = sensor_map[fru].map[sensor_num].id;
  static uint8_t retry = 0;
  int ret = 0;
  NM_RW_INFO info;

  get_nm_rw_info(&info.bus, &info.nm_addr, &info.bmc_addr);
  ret = cmd_NM_get_nm_statistics(info, NMS_GLOBAL_POWER, domain_id, policy_id, rbuf);

  if ( (ret != 0) || (rbuf[6] != CC_SUCCESS) ) {
    retry++;
    return retry_err_handle(retry, 5);
  }

  *value = rbuf[11] << 8 | rbuf[10];
  retry = 0;
  return 0;
}

static int
read_cpu_temp(uint8_t fru, uint8_t sensor_num, float *value) {
  uint8_t cpu_id = sensor_map[fru].map[sensor_num].id;
  uint8_t cpu_addr = cpu_info_list[cpu_id].cpu_addr;

  if(!is_cpu_socket_occupy(cpu_id))
    return READING_NA;

  return lib_get_cpu_temp(cpu_id, cpu_addr, value);
}

static int
read_cpu_dimm_temp(uint8_t fru, uint8_t sensor_num, float *value) {
  int ret;
  uint8_t channel = sensor_map[fru].map[sensor_num].id;  // channel:0-15
  uint8_t ch_num = channel%PER_CPU_DIMM_CHANNEL_MAX;     // 8 channel per cpu
  uint8_t cpu_id = channel/PER_CPU_DIMM_CHANNEL_MAX;     // cpu_id:0-1
  uint8_t dimm_id = channel*2;                           // dimm_id:0-31
  static uint8_t retry[MAX_DIMM_CHANNEL] = {0};
  uint8_t cpu_addr[2] = {PECI_CPU0_ADDR, PECI_CPU1_ADDR};


  if(!is_dimm_present(dimm_id))
    return READING_NA;

  if(pal_bios_completed(fru) != true) {
    return READING_NA;
  }

  ret = lib_get_cpu_dimm_temp(cpu_addr[cpu_id], ch_num, value);
  if ( ret != 0 || *value == 255 ) {
    retry[channel]++;
    return retry_err_handle(retry[channel], 5);
  }

#ifdef DEBUG
  syslog(LOG_DEBUG, "%s CPU%d DIMM%d CH=%d Temp=%f\n",
                    __func__, cpu_id, dimm_id, channel, *value);
#endif
  retry[channel] = 0;
  return 0;
}

static int
get_nm_rw_info(uint8_t* nm_bus, uint8_t* nm_addr, uint16_t* bmc_addr) {
  *nm_bus = nm_info_list[NM_ID0].bus;
  *nm_addr = nm_info_list[NM_ID0].slv_addr;
  *bmc_addr = BMC_DEF_SLAVE_ADDR;
  return 0;
}


static int
read_dimm_power(uint8_t fru, uint8_t sensor_num, float *value,
                uint8_t dimm_num, uint8_t cpu_id, bool* cached) {
  int ret;
  NM_RW_INFO info;
  NM_DIMM_SMB_DEV dev;
  uint8_t rxbuf[16];
  uint8_t len;

  get_nm_rw_info(&info.bus, &info.nm_addr, &info.bmc_addr);
  dev.cpu = cpu_id;
  dev.bus_id = dimm_pmic_list[dimm_num].identify;
  dev.addr = dimm_pmic_list[dimm_num].addr;
  dev.offset_len = 1;
  len = 1;

  do {
    dev.offset = 0x0000000C;
    ret = retry_cond(!cmd_NM_read_dimm_smbus(&info, &dev, len, rxbuf), 10, 50);
  } while(0);

  if( ret == 0 )
    *value = ((float)(rxbuf[0] * 125))/1000;

#ifdef DEBUG
  syslog(LOG_DEBUG, "%s CPU%d DIMM%d Addr=%x\n", __func__, cpu_id, dimm_num, dev.addr);
  syslog(LOG_DEBUG, "%s DIMM Power=%f RAW=%x\n", __func__, *value, rxbuf[0]);
#endif

  return ret;
}

static int
read_cpu_dimm_power(uint8_t fru, uint8_t sensor_num, float *value) {
  int ret;
  static uint8_t retry[MAX_DIMM_NUM] = {0};
  static bool cached[MAX_DIMM_NUM] = {false};
  uint8_t dimm_id = sensor_map[fru].map[sensor_num].id; // dimm_id 0-31
  uint8_t dimm_num = dimm_id%PER_CPU_DIMM_NUMBER_MAX;   // per cpu dimm number 0-15
  uint8_t cpu_id = dimm_id/PER_CPU_DIMM_NUMBER_MAX;     // cpu_id:CPU0/CPU1

  if(!is_dimm_present(dimm_id))
    return READING_NA;

  if(pal_bios_completed(fru) != true) {
    return READING_NA;
  }

  ret = read_dimm_power(fru, sensor_num, value, dimm_num, cpu_id, cached);
  if ( ret != 0 ) {
    retry[dimm_id]++;
    return retry_err_handle(retry[dimm_id], 5);
  }

  retry[dimm_id] = 0;
  return 0;
}

static
int read_cpu_dimm_state(uint8_t fru, uint8_t sensor_num, float *value) {
  uint8_t dimm_id = sensor_map[fru].map[sensor_num].id; // dimm_id 0-31
  uint8_t dimm_num = dimm_id%PER_CPU_DIMM_NUMBER_MAX;   // per cpu dimm number 0-15
  uint8_t cpu_id = dimm_id/PER_CPU_DIMM_NUMBER_MAX;     // cpu_id:CPU0/CPU1
  uint8_t err_cnt = 0;
  const char *err_list[MAX_PMIC_ERR_TYPE] = {0};
  int ret, i, index=0;
  static bool flag[MAX_DIMM_NUM][MAX_PMIC_ERR_TYPE]= {false};
  bool curr[MAX_PMIC_ERR_TYPE] = {false};
  bool tag=false;
  char name[64] = {0};

  if(!is_dimm_present(dimm_id))
    return READING_NA;

  *value = 0;
  ret = pmic_list_err(fru, cpu_id, dimm_num, err_list, &err_cnt);
  if(!ret) {
    for (i = 0; i < err_cnt; i++) {
      index = pmic_err_index(err_list[i]);
      curr[index] = true;
#ifdef DEBUG
      syslog(LOG_DEBUG, "%s cpu%d dimm%d err_num%d %s\n", __func__, cpu_id, dimm_id, index, err_list[i]);
#endif
    }
    for (i = 0; i < MAX_PMIC_ERR_TYPE; i++) {
      tag = curr[i] ^ flag[dimm_id][i];
#ifdef DEBUG
      syslog(LOG_DEBUG, "%s dimm%d curr=%d flag%d",
                         __func__, dimm_id, curr[i], flag[dimm_id][i] );
#endif
      if(tag) {
        pmic_err_name(i, name);
        if(curr[i] == true)
           syslog(LOG_CRIT, "ASSERT DIMM_LABEL=%s Error %s", 
		get_dimm_label(cpu_id, dimm_num), name);
        else
	   syslog(LOG_CRIT, "DEASSERT DIMM_LABEL=%s Error %s", 
	        get_dimm_label(cpu_id, dimm_num), name);

        flag[dimm_id][i] = curr[i];
      }
    }
  }
  return ret;
}

//Sensor HSC
static int
read_hsc_vin(uint8_t fru, uint8_t sensor_num, float *value) {
  return sensors_read(NULL, sensor_map[fru].map[sensor_num].snr_name, value);
}

static int
read_hsc_iout(uint8_t fru, uint8_t sensor_num, float *value) {
  return sensors_read(NULL, sensor_map[fru].map[sensor_num].snr_name, value);
}

static int
read_hsc_pin(uint8_t fru, uint8_t sensor_num, float *value) {
  return sensors_read(NULL, sensor_map[fru].map[sensor_num].snr_name, value);
}

static int
read_hsc_temp(uint8_t fru, uint8_t sensor_num, float *value) {
  return sensors_read(NULL, sensor_map[fru].map[sensor_num].snr_name, value);
}

static int
read_hsc_peak_pin(uint8_t fru, uint8_t sensor_num, float *value) {
  return sensors_read(NULL, sensor_map[fru].map[sensor_num].snr_name, value);
}

static int
read_NM_pch_temp(uint8_t fru, uint8_t sensor_num, float *value) {
  uint8_t rlen = 0;
  uint8_t rbuf[32] = {0x00};
  int ret = 0;
  static uint8_t retry = 0;
  NM_RW_INFO info;

  get_nm_rw_info(&info.bus, &info.nm_addr, &info.bmc_addr);
  ret = cmd_NM_sensor_reading(info, NM_PCH_TEMP, rbuf, &rlen);
  if (ret) {
    retry++;
    return retry_err_handle(retry, 5);
  }

  *value = (float) rbuf[7];
  retry = 0;
  return ret;
}

static int
read_mb_temp (uint8_t fru, uint8_t sensor_num, float *value) {
  return sensors_read(NULL, sensor_map[fru].map[sensor_num].snr_name, value);
}

int
read_isl28022(uint8_t fru, uint8_t sensor_num, float *value) {
  int fd = 0, ret = -1;
  char fn[32];
  uint8_t tlen, rlen, addr, bus;
  float scale;
  uint8_t tbuf[16] = {0};
  uint8_t rbuf[16] = {0};
  uint8_t dpm_id = sensor_map[fru].map[sensor_num].id;

  bus = dpm_info_list[dpm_id].bus;
  addr = dpm_info_list[dpm_id].slv_addr;
  scale = dpm_info_list[dpm_id].vbus_scale;

  snprintf(fn, sizeof(fn), "/dev/i2c-%d", bus);
  fd = open(fn, O_RDWR);
  if (fd < 0) {
    goto err_exit;
  }

  //Voltage BUS Register
  tbuf[0] = 0x02;
  tlen = 1;
  rlen = 2;

  ret = i2c_rdwr_msg_transfer(fd, addr, tbuf, tlen, rbuf, rlen);

#ifdef DEBUG
  syslog(LOG_DEBUG, "%s Voltage [%d] =%x %x bus=%x slavaddr=%x\n", __func__, dpm_id,
         rbuf[1], rbuf[0], bus, addr);
#endif

  *value = ((rbuf[0] << 8 | rbuf[1]) >> 2) * scale;
err_exit:
  if (fd > 0) {
    close(fd);
  }
  return ret;
}

int
read_dpm_vout(uint8_t fru, uint8_t sensor_num, float *value) {
  int ret = 0;
  uint8_t dpm_id = sensor_map[fru].map[sensor_num].id;
  static uint8_t retry[DPM_NUM_CNT] = {0};
  uint8_t mb_rev;

  if(pal_get_board_rev_id(&mb_rev))
    return -1;

  if (mb_rev <= 2) {
    ret = read_isl28022(fru, sensor_num, value);
  } else {
    ret = sensors_read(NULL, sensor_map[fru].map[sensor_num].snr_name, value);
  }

  if (ret || *value == 0) {
    retry[dpm_id]++;
    return retry_err_handle(retry[dpm_id], 2);
  }

  retry[dpm_id] = 0;
  return ret;
}

static int
read_vr_temp(uint8_t fru, uint8_t sensor_num, float *value) {
  int ret = 0;
  uint8_t vr_id = sensor_map[fru].map[sensor_num].id;
  static uint8_t retry[VR_NUM_CNT] = {0};
  uint8_t cpu_id = vr_id/5;

  if(!is_cpu_socket_occupy(cpu_id))
    return READING_NA;

  ret = sensors_read(NULL, sensor_map[fru].map[sensor_num].snr_name, value);
  if (*value == 0) {
    retry[vr_id]++;
    return retry_err_handle(retry[vr_id], 5);
  }

  retry[vr_id] = 0;
  return ret;
}

static int
read_vr_vout(uint8_t fru, uint8_t sensor_num, float *value) {
  int ret = 0;
  uint8_t vr_id = sensor_map[fru].map[sensor_num].id;
  static uint8_t retry[VR_NUM_CNT] = {0};
  uint8_t cpu_id = vr_id/5;

  if(!is_cpu_socket_occupy(cpu_id))
    return READING_NA;

   ret = sensors_read(NULL, sensor_map[fru].map[sensor_num].snr_name, value);
  if (*value == 0) {
    retry[vr_id]++;
    return retry_err_handle(retry[vr_id], 5);
  }

  retry[vr_id] = 0;
  return ret;
}


static int
read_vr_iout(uint8_t fru, uint8_t sensor_num, float *value) {
  uint8_t cpu_id = sensor_map[fru].map[sensor_num].id/5;

  if(!is_cpu_socket_occupy(cpu_id))
    return READING_NA;

  return sensors_read(NULL, sensor_map[fru].map[sensor_num].snr_name, value);
}

static int
read_vr_pout(uint8_t fru, uint8_t sensor_num, float *value) {
  uint8_t cpu_id = sensor_map[fru].map[sensor_num].id/5;

  if(!is_cpu_socket_occupy(cpu_id))
    return READING_NA;

  return sensors_read(NULL, sensor_map[fru].map[sensor_num].snr_name, value);
}

static void
_print_sensor_discrete_log(uint8_t fru, uint8_t snr_num, char *snr_name,
    uint8_t val, char *event) {
  if (val) {
    syslog(LOG_CRIT, "ASSERT: %s discrete - raised - FRU: %d", event, fru);
  } else {
    syslog(LOG_CRIT, "DEASSERT: %s discrete - settled - FRU: %d", event, fru);
  }
  pal_update_ts_sled();
}

static int
check_frb3(uint8_t fru_id, uint8_t sensor_num, float *value) {
  static unsigned int retry = 0;
  static uint8_t frb3_fail = 0x10; // bit 4: FRB3 failure
  static time_t rst_time = 0;
  uint8_t postcodes[256] = {0};
  struct stat file_stat;
  int rc;
  size_t len = 0;
  char sensor_name[32] = {0};
  char error[32] = {0};

  if (fru_id != FRU_MB) {
    syslog(LOG_ERR, "Not Supported Operation for fru %d", fru_id);
    return READING_NA;
  }

  if (stat("/tmp/rst_touch", &file_stat) == 0 && file_stat.st_mtime > rst_time) {
    rst_time = file_stat.st_mtime;
    // assume fail till we know it is not
    frb3_fail = 0x10; // bit 4: FRB3 failure
    retry = 0;
    // cache current postcode buffer
    if (stat("/tmp/DWR", &file_stat) != 0) {
      memset(postcodes_last, 0, sizeof(postcodes_last));
      pal_get_80port_record(FRU_MB, postcodes_last, sizeof(postcodes_last), &len);
    }
  }

  if (frb3_fail) {
    // KCS transaction
    if (stat("/tmp/kcs_touch", &file_stat) == 0 && file_stat.st_mtime > rst_time)
      frb3_fail = 0;

    // Port 80 updated
    memset(postcodes, 0, sizeof(postcodes));
    rc = pal_get_80port_record(FRU_MB, postcodes, sizeof(postcodes), &len);
    if (rc == PAL_EOK && memcmp(postcodes_last, postcodes, 256) != 0) {
      frb3_fail = 0;
    }

    // BIOS POST COMPLT, in case BMC reboot when system idle in OS
    if (pal_bios_completed(FRU_MB))
      frb3_fail = 0;
  }

  if (frb3_fail)
    retry++;
  else
    retry = 0;

  if (retry == FRB3_MAX_READ_RETRY) {
    pal_get_sensor_name(fru_id, sensor_num, sensor_name);
    snprintf(error, sizeof(error), "FRB3 failure");
    _print_sensor_discrete_log(fru_id, sensor_num, sensor_name, frb3_fail, error);
  }

  return 0;
}

static
int read_frb3(uint8_t fru, uint8_t sensor_num, float *value) {
  int ret = 0;
  uint8_t fru_id = sensor_map[fru].map[sensor_num].id;

#ifdef DEBUG
  syslog(LOG_INFO, "%s\n", __func__);
#endif
  ret = check_frb3(fru_id, MB_SNR_PROCESSOR_FAIL, value);
  *value = 0;
  return ret;
}
