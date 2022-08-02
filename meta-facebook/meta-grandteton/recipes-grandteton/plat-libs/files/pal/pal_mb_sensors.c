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
#include "pal.h"


//#define DEBUG
#define GPIO_P3V_BAT_SCALED_EN    "BATTERY_DETECT"
#define FRB3_MAX_READ_RETRY       (10)
#define HSC_12V_BUS               (2)

uint8_t DIMM_SLOT_CNT = 0;
//static float InletCalibration = 0;

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
static int read_cpu0_dimm_temp(uint8_t fru, uint8_t sensor_num, float *value);
static int read_cpu1_dimm_temp(uint8_t fru, uint8_t sensor_num, float *value);
static int read_cpu0_dimm_power(uint8_t fru, uint8_t sensor_num, float *value);
static int read_cpu1_dimm_power(uint8_t fru, uint8_t sensor_num, float *value);
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
  MB_SNR_HSC_VIN,
  MB_SNR_HSC_IOUT,
  MB_SNR_HSC_PIN,
  MB_SNR_HSC_TEMP,
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

// List of MB discrete sensors to be monitored
const uint8_t mb_discrete_sensor_list[] = {
//  MB_SENSOR_POWER_FAIL,
//  MB_SENSOR_MEMORY_LOOP_FAIL,
  MB_SNR_PROCESSOR_FAIL,
};

//CPU
COMMON_CPU_INFO cpu_info_list[] = {
  {CPU_ID0, PECI_CPU0_ADDR},
  {CPU_ID1, PECI_CPU1_ADDR},
};

//12V HSC
//MP5990
PAL_ATTR_INFO mp5990_info_list[] = {
  {HSC_VOLTAGE, 32, 0, 1},
  {HSC_CURRENT, 16, 0, 1},
  {HSC_POWER, 1, 0, 1},
  {HSC_TEMP, 1, 0, 1},
};

PAL_HSC_INFO hsc_info_list[] = {
  {HSC_ID0, HSC_12V_BUS, MP5990_SLAVE_ADDR, mp5990_info_list },
};
PAL_HSC_INFO* hsc_binding = &hsc_info_list[0];

//NM
PAL_I2C_BUS_INFO nm_info_list[] = {
  {NM_ID0, NM_IPMB_BUS_ID, NM_SLAVE_ADDR},
};


PAL_DPM_DEV_INFO dpm_info_list[] = {
  {DPM_0, I2C_BUS_34, 0x84, 0.004, 0, 0 },
  {DPM_1, I2C_BUS_34, 0x88, 0.004, 0, 0 },
  {DPM_2, I2C_BUS_34, 0x82, 0.004, 0, 0 },
  {DPM_3, I2C_BUS_34, 0x86, 0.004, 0, 0 },
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

//VR CHIP
char *vr_isl_chips[VR_NUM_CNT] = {
  "isl69260-i2c-20-60",  // CPU0_VCCIN
  "isl69260-i2c-20-60",  // CPU0_VCCFA_FIVRA
  "isl69260-i2c-20-61",  // CPU0_VCCIN_FAVON
  "isl69260-i2c-20-61",  // CPU0_VCCFA
  "isl69260-i2c-20-63",  // CPU0_VCCD_HV
  "isl69260-i2c-20-72",  // CPU1_VCCIN
  "isl69260-i2c-20-72",  // CPU1_VCCFA_FIVRA
  "isl69260-i2c-20-74",  // CPU1_VCCIN_FAVON
  "isl69260-i2c-20-74",  // CPU1_VCCFA
  "isl69260-i2c-20-76",  // CPU1_VCCD_HV
};

char **vr_chips = vr_isl_chips;

//{SensorName, ID, FUNCTION, PWR_STATUS, {UCR, UNC, UNR, LCR, LNC, LNR, Pos, Neg}
PAL_SENSOR_MAP mb_sensor_map[] = {
  {NULL, 0, NULL, 0, {0, 0, 0, 0, 0, 0, 0, 0}, 0}, //0x00
  {"E1S_P3V3_VOLT", DPM_0, read_dpm_vout, false, {0, 0, 0, 0, 0, 0, 0, 0}, VOLT}, //0x01
  {NULL, 0, NULL, 0, {0, 0, 0, 0, 0, 0, 0, 0}, 0}, //0x02
  {"E1S_P12V_CURR", E1S_0, read_adc128_val, false, {0, 0, 0, 0, 0, 0, 0, 0}, CURR}, //0x03
  {"E1S_P12V_PWR", E1S_0, read_e1s_power, false, {0, 0, 0, 0, 0, 0, 0, 0}, POWER}, //0x04
  {"E1S_TEMP", E1S_0, read_e1s_temp, false, {0, 0, 0, 0, 0, 0, 0, 0}, TEMP}, //0x05
  {NULL, 0, NULL, 0, {0, 0, 0, 0, 0, 0, 0, 0}, 0}, //0x06
  {NULL, 0, NULL, 0, {0, 0, 0, 0, 0, 0, 0, 0}, 0}, //0x07

  {NULL, 0, NULL, 0, {0, 0, 0, 0, 0, 0, 0, 0}, 0}, //0x08
  {NULL, 0, NULL, 0, {0, 0, 0, 0, 0, 0, 0, 0}, 0}, //0x09
  {"PROCESSOR_FAIL", FRU_MB, read_frb3, 0, {0, 0, 0, 0, 0, 0, 0, 0}, STATE}, //0x0A
  {"HSC_VOLT",  HSC_ID0, read_hsc_vin,  true, {0, 0, 0, 0, 0, 0, 0, 0}, VOLT}, //0x0B
  {"HSC_CURR",  HSC_ID0, read_hsc_iout, true, {0, 0, 0, 0, 0, 0, 0, 0}, CURR}, //0x0C
  {"HSC_PWR",   HSC_ID0, read_hsc_pin,  true, {0, 0, 0, 0, 0, 0, 0, 0}, POWER}, //0x0D
  {"HSC_TEMP",  HSC_ID0, read_hsc_temp, true, {0, 0, 0, 0, 0, 0, 0, 0}, TEMP}, //0x0E
  {"HSC_PEAK_PIN",  HSC_ID0, read_hsc_peak_pin, true, {0, 0, 0, 0, 0, 0, 0, 0}, POWER}, //0x0F

  {"OUTLET_L_TEMP", TEMP_OUTLET_L, read_mb_temp, true, {0, 0, 0, 0, 0, 0, 0, 0}, TEMP}, //0x10
  {"INLET_L_TEMP", TEMP_INLET_L, read_mb_temp, true, {0, 0, 0, 0, 0, 0, 0, 0}, TEMP}, //0x11
  {"OUTLET_R_TEMP", TEMP_OUTLET_R, read_mb_temp, true, {0, 0, 0, 0, 0, 0, 0, 0}, TEMP}, //0x12
  {"INLET_R_TEMP", TEMP_INLET_R, read_mb_temp, true, {0, 0, 0, 0, 0, 0, 0, 0}, TEMP}, //0x13
  {NULL, 0, NULL, 0, {0, 0, 0, 0, 0, 0, 0, 0}, 0}, //0x14
  {NULL, 0, NULL, 0, {0, 0, 0, 0, 0, 0, 0, 0}, 0}, //0x15
  {NULL, 0, NULL, 0, {0, 0, 0, 0, 0, 0, 0, 0}, 0}, //0x16
  {NULL, 0, NULL, 0, {0, 0, 0, 0, 0, 0, 0, 0}, 0}, //0x17

  {"CPU0_TEMP", CPU_ID0, read_cpu_temp, false, {0, 0, 0, 0, 0, 0, 0, 0}, TEMP}, //0x18
  {"CPU1_TEMP", CPU_ID1, read_cpu_temp, false, {0, 0, 0, 0, 0, 0, 0, 0}, TEMP}, //0x19
  {"CPU0_THERM_MARGIN", CPU_ID0, read_cpu_thermal_margin, false, {0, 0, 0, 0, 0, 0, 0, 0}, TEMP}, //0x1A
  {"CPU1_THERM_MARGIN", CPU_ID1, read_cpu_thermal_margin, false, {0, 0, 0, 0, 0, 0, 0, 0}, TEMP}, //0x1B
  {"CPU0_TJMAX", CPU_ID0, read_cpu_tjmax, false, {0, 0, 0, 0, 0, 0, 0, 0}, TEMP}, //0x1C
  {"CPU1_TJMAX", CPU_ID1, read_cpu_tjmax, false, {0, 0, 0, 0, 0, 0, 0, 0}, TEMP}, //0x1D
  {"CPU0_PKG_PWR", CPU_ID0, read_cpu_pkg_pwr, false, {0, 0, 0, 0, 0, 0, 0, 0}, POWER}, //0x1E
  {"CPU1_PKG_PWR", CPU_ID1, read_cpu_pkg_pwr, false, {0, 0, 0, 0, 0, 0, 0, 0}, POWER}, //0x1F

  {"CPU0_DIMM_A0_C0_TEMP", DIMM_CRPA, read_cpu0_dimm_temp, false, {0, 0, 0, 0, 0, 0, 0, 0}, TEMP}, //0x20
  {"CPU0_DIMM_A1_C1_TEMP", DIMM_CRPB, read_cpu0_dimm_temp, false, {0, 0, 0, 0, 0, 0, 0, 0}, TEMP}, //0x21
  {"CPU0_DIMM_A2_C2_TEMP", DIMM_CRPC, read_cpu0_dimm_temp, false, {0, 0, 0, 0, 0, 0, 0, 0}, TEMP}, //0x22
  {"CPU0_DIMM_A3_C3_TEMP", DIMM_CRPD, read_cpu0_dimm_temp, false, {0, 0, 0, 0, 0, 0, 0, 0}, TEMP}, //0x23
  {"CPU0_DIMM_A4_C4_TEMP", DIMM_CRPE, read_cpu0_dimm_temp, false, {0, 0, 0, 0, 0, 0, 0, 0}, TEMP}, //0x24
  {"CPU0_DIMM_A5_C5_TEMP", DIMM_CRPF, read_cpu0_dimm_temp, false, {0, 0, 0, 0, 0, 0, 0, 0}, TEMP}, //0x25
  {"CPU0_DIMM_A6_C6_TEMP", DIMM_CRPG, read_cpu0_dimm_temp, false, {0, 0, 0, 0, 0, 0, 0, 0}, TEMP}, //0x26
  {"CPU0_DIMM_A7_C7_TEMP", DIMM_CRPH, read_cpu0_dimm_temp, false, {0, 0, 0, 0, 0, 0, 0, 0}, TEMP}, //0x27

  {"CPU1_DIMM_B0_D0_TEMP", DIMM_CRPA, read_cpu1_dimm_temp, false, {0, 0, 0, 0, 0, 0, 0, 0}, TEMP}, //0x28
  {"CPU1_DIMM_B1_D1_TEMP", DIMM_CRPB, read_cpu1_dimm_temp, false, {0, 0, 0, 0, 0, 0, 0, 0}, TEMP}, //0x29
  {"CPU1_DIMM_B2_D2_TEMP", DIMM_CRPC, read_cpu1_dimm_temp, false, {0, 0, 0, 0, 0, 0, 0, 0}, TEMP}, //0x2A
  {"CPU1_DIMM_B3_D3_TEMP", DIMM_CRPD, read_cpu1_dimm_temp, false, {0, 0, 0, 0, 0, 0, 0, 0}, TEMP}, //0x2B
  {"CPU1_DIMM_B4_D4_TEMP", DIMM_CRPE, read_cpu1_dimm_temp, false, {0, 0, 0, 0, 0, 0, 0, 0}, TEMP}, //0x2C
  {"CPU1_DIMM_B5_D5_TEMP", DIMM_CRPF, read_cpu1_dimm_temp, false, {0, 0, 0, 0, 0, 0, 0, 0}, TEMP}, //0x2D
  {"CPU1_DIMM_B6_D6_TEMP", DIMM_CRPG, read_cpu1_dimm_temp, false, {0, 0, 0, 0, 0, 0, 0, 0}, TEMP}, //0x2E
  {"CPU1_DIMM_B7_D7_TEMP", DIMM_CRPH, read_cpu1_dimm_temp, false, {0, 0, 0, 0, 0, 0, 0, 0}, TEMP}, //0x2F

  {"VR_CPU0_VCCIN_VOLT", VR_ID0, read_vr_vout, false, {0, 0, 0, 0, 0, 0, 0, 0}, VOLT}, //0x30
  {"VR_CPU0_VCCIN_TEMP", VR_ID0, read_vr_temp, false, {0, 0, 0, 0, 0, 0, 0, 0}, TEMP}, //0x31
  {"VR_CPU0_VCCIN_CURR", VR_ID0, read_vr_iout, false, {0, 0, 0, 0, 0, 0, 0, 0}, CURR}, //0X32
  {"VR_CPU0_VCCIN_PWR",  VR_ID0, read_vr_pout, false, {0, 0, 0, 0, 0, 0, 0, 0}, POWER}, //0x33
  {"VR_CPU0_FIVRA_VOLT", VR_ID1, read_vr_vout, false, {0, 0, 0, 0, 0, 0, 0, 0}, VOLT}, //0x34
  {"VR_CPU0_FIVRA_TEMP", VR_ID1, read_vr_temp, false, {0, 0, 0, 0, 0, 0, 0, 0}, TEMP}, //0x35
  {"VR_CPU0_FIVRA_CURR", VR_ID1, read_vr_iout, false, {0, 0, 0, 0, 0, 0, 0, 0}, CURR}, //0X36
  {"VR_CPU0_FIVRA_PWR",  VR_ID1, read_vr_pout, false, {0, 0, 0, 0, 0, 0, 0, 0}, POWER}, //0x37
  {"VR_CPU0_FAON_VOLT",  VR_ID2, read_vr_vout, false, {0, 0, 0, 0, 0, 0, 0, 0}, VOLT}, //0x38
  {"VR_CPU0_FAON_TEMP",  VR_ID2, read_vr_temp, false, {0, 0, 0, 0, 0, 0, 0, 0}, TEMP}, //0x39
  {"VR_CPU0_FAON_CURR",  VR_ID2, read_vr_iout, false, {0, 0, 0, 0, 0, 0, 0, 0}, CURR}, //0X3A
  {"VR_CPU0_FAON_PWR",   VR_ID2, read_vr_pout, false, {0, 0, 0, 0, 0, 0, 0, 0}, POWER}, //0x3B
  {"VR_CPU0_VCCFA_VOLT", VR_ID3, read_vr_vout, false, {0, 0, 0, 0, 0, 0, 0, 0}, VOLT}, //0x3C
  {"VR_CPU0_VCCFA_TEMP", VR_ID3, read_vr_temp, false, {0, 0, 0, 0, 0, 0, 0, 0}, TEMP}, //0x3D
  {"VR_CPU0_VCCFA_CURR", VR_ID3, read_vr_iout, false, {0, 0, 0, 0, 0, 0, 0, 0}, CURR}, //0X3E
  {"VR_CPU0_VCCFA_PWR",  VR_ID3, read_vr_pout, false, {0, 0, 0, 0, 0, 0, 0, 0}, POWER}, //0x3F
  {"VR_CPU0_VCCD_HV_VOLT", VR_ID4, read_vr_vout, false, {0, 0, 0, 0, 0, 0, 0, 0}, VOLT}, //0x40
  {"VR_CPU0_VCCD_HV_TEMP", VR_ID4, read_vr_temp, false, {0, 0, 0, 0, 0, 0, 0, 0}, TEMP}, //0x41
  {"VR_CPU0_VCCD_HV_CURR", VR_ID4, read_vr_iout, false, {0, 0, 0, 0, 0, 0, 0, 0}, CURR}, //0X42
  {"VR_CPU0_VCCD_HV_PWR", VR_ID4, read_vr_pout, false, {0, 0, 0, 0, 0, 0, 0, 0}, POWER}, //0x43
  {"P12V_AUX_IN0_VOLT", ADC128_0, read_adc128_val, true, {0, 0, 0, 0, 0, 0, 0, 0}, VOLT}, //0x44
  {"P5V_IN3_VOLT",      ADC128_0, read_adc128_val, false, {0, 0, 0, 0, 0, 0, 0, 0}, VOLT}, //0x45
  {"P3V3_IN4_VOLT",     ADC128_0, read_adc128_val, false, {0, 0, 0, 0, 0, 0, 0, 0}, VOLT}, //0x46
  {"P3V3_AUX_IN5_VOLT", ADC128_0, read_adc128_val, true, {0, 0, 0, 0, 0, 0, 0, 0}, VOLT}, //0x47

  {"VR_CPU1_VCCIN_VOLT", VR_ID5, read_vr_vout, false, {0, 0, 0, 0, 0, 0, 0, 0}, VOLT}, //0x48
  {"VR_CPU1_VCCIN_TEMP", VR_ID5, read_vr_temp, false, {0, 0, 0, 0, 0, 0, 0, 0}, TEMP}, //0x49
  {"VR_CPU1_VCCIN_CURR", VR_ID5, read_vr_iout, false, {0, 0, 0, 0, 0, 0, 0, 0}, CURR}, //0X4A
  {"VR_CPU1_VCCIN_PWR",  VR_ID5, read_vr_pout, false, {0, 0, 0, 0, 0, 0, 0, 0}, POWER}, //0x4B
  {"VR_CPU1_FIVRA_VOLT", VR_ID6, read_vr_vout, false, {0, 0, 0, 0, 0, 0, 0, 0}, VOLT}, //0x4C
  {"VR_CPU1_FIVRA_TEMP", VR_ID6, read_vr_temp, false, {0, 0, 0, 0, 0, 0, 0, 0}, TEMP}, //0x4D
  {"VR_CPU1_FIVRA_CURR", VR_ID6, read_vr_iout, false, {0, 0, 0, 0, 0, 0, 0, 0}, CURR}, //0X4E
  {"VR_CPU1_FIVRA_PWR", VR_ID6, read_vr_pout, false, {0, 0, 0, 0, 0, 0, 0, 0}, POWER}, //0x4F
  {"VR_CPU1_FAON_VOLT", VR_ID7, read_vr_vout, false, {0, 0, 0, 0, 0, 0, 0, 0}, VOLT}, //0x50
  {"VR_CPU1_FAON_TEMP", VR_ID7, read_vr_temp, false, {0, 0, 0, 0, 0, 0, 0, 0}, TEMP}, //0x51
  {"VR_CPU1_FAON_CURR", VR_ID7, read_vr_iout, false, {0, 0, 0, 0, 0, 0, 0, 0}, CURR}, //0X52
  {"VR_CPU1_FAON_PWR",  VR_ID7, read_vr_pout, false, {0, 0, 0, 0, 0, 0, 0, 0}, POWER}, //0x53
  {"VR_CPU1_VCCFA_VOLT", VR_ID8, read_vr_vout, false, {0, 0, 0, 0, 0, 0, 0, 0}, VOLT}, //0x54
  {"VR_CPU1_VCCFA_TEMP", VR_ID8, read_vr_temp, false, {0, 0, 0, 0, 0, 0, 0, 0}, TEMP}, //0x55
  {"VR_CPU1_VCCFA_CURR", VR_ID8, read_vr_iout, false, {0, 0, 0, 0, 0, 0, 0, 0}, CURR}, //0X56
  {"VR_CPU1_VCCFA_PWR", VR_ID8, read_vr_pout, false, {0, 0, 0, 0, 0, 0, 0, 0}, POWER}, //0x57
  {"VR_CPU1_VCCD_HV_VOLT", VR_ID9, read_vr_vout, false, {0, 0, 0, 0, 0, 0, 0, 0}, VOLT}, //0x58
  {"VR_CPU1_VCCD_HV_TEMP", VR_ID9, read_vr_temp, false, {0, 0, 0, 0, 0, 0, 0, 0}, TEMP}, //0x59
  {"VR_CPU1_VCCD_HV_CURR", VR_ID9, read_vr_iout, false, {0, 0, 0, 0, 0, 0, 0, 0}, CURR}, //0X5A
  {"VR_CPU1_VCCD_HV_PWR", VR_ID9, read_vr_pout, false, {0, 0, 0, 0, 0, 0, 0, 0}, POWER}, //0x5B
  {NULL, 0, NULL, 0, {0, 0, 0, 0, 0, 0, 0, 0}, 0}, //0x5C
  {NULL, 0, NULL, 0, {0, 0, 0, 0, 0, 0, 0, 0}, 0}, //0x5D
  {"P3V_BAT_VOLT", ADC7, read_bat_val, false, {0, 0, 0, 0, 0, 0, 0, 0}, VOLT}, //0x5E
  {"PCH_TEMP", NM_ID0, read_NM_pch_temp, false, {0, 0, 0, 0, 0, 0, 0, 0}, TEMP}, //0x5F

  {"CPU0_DIMM_A0_PWR", DIMM_ID0,  read_cpu0_dimm_power, 0, {0, 0, 0, 0, 0, 0, 0, 0}, POWER}, //0x60
  {"CPU0_DIMM_C0_PWR", DIMM_ID1,  read_cpu0_dimm_power, 0, {0, 0, 0, 0, 0, 0, 0, 0}, POWER}, //0x61
  {"CPU0_DIMM_A1_PWR", DIMM_ID2,  read_cpu0_dimm_power, 0, {0, 0, 0, 0, 0, 0, 0, 0}, POWER}, //0x62
  {"CPU0_DIMM_C1_PWR", DIMM_ID3,  read_cpu0_dimm_power, 0, {0, 0, 0, 0, 0, 0, 0, 0}, POWER}, //0x63
  {"CPU0_DIMM_A2_PWR", DIMM_ID4,  read_cpu0_dimm_power, 0, {0, 0, 0, 0, 0, 0, 0, 0}, POWER}, //0x64
  {"CPU0_DIMM_C2_PWR", DIMM_ID5,  read_cpu0_dimm_power, 0, {0, 0, 0, 0, 0, 0, 0, 0}, POWER}, //0x65
  {"CPU0_DIMM_A3_PWR", DIMM_ID6,  read_cpu0_dimm_power, 0, {0, 0, 0, 0, 0, 0, 0, 0}, POWER}, //0x66
  {"CPU0_DIMM_C3_PWR", DIMM_ID7,  read_cpu0_dimm_power, 0, {0, 0, 0, 0, 0, 0, 0, 0}, POWER}, //0x67
  {"CPU0_DIMM_A4_PWR", DIMM_ID8,  read_cpu0_dimm_power, 0, {0, 0, 0, 0, 0, 0, 0, 0}, POWER}, //0x68
  {"CPU0_DIMM_C4_PWR", DIMM_ID9,  read_cpu0_dimm_power, 0, {0, 0, 0, 0, 0, 0, 0, 0}, POWER}, //0x69
  {"CPU0_DIMM_A5_PWR", DIMM_ID10, read_cpu0_dimm_power, 0, {0, 0, 0, 0, 0, 0, 0, 0}, POWER}, //0x6A
  {"CPU0_DIMM_C5_PWR", DIMM_ID11, read_cpu0_dimm_power, 0, {0, 0, 0, 0, 0, 0, 0, 0}, POWER}, //0x6B
  {"CPU0_DIMM_A6_PWR", DIMM_ID12, read_cpu0_dimm_power, 0, {0, 0, 0, 0, 0, 0, 0, 0}, POWER}, //0x6C
  {"CPU0_DIMM_C6_PWR", DIMM_ID13, read_cpu0_dimm_power, 0, {0, 0, 0, 0, 0, 0, 0, 0}, POWER}, //0x6D
  {"CPU0_DIMM_A7_PWR", DIMM_ID14, read_cpu0_dimm_power, 0, {0, 0, 0, 0, 0, 0, 0, 0}, POWER}, //0x6E
  {"CPU0_DIMM_C7_PWR", DIMM_ID15, read_cpu0_dimm_power, 0, {0, 0, 0, 0, 0, 0, 0, 0}, POWER}, //0x6F

  {"CPU1_DIMM_B0_PWR", DIMM_ID0,  read_cpu1_dimm_power, 0, {0, 0, 0, 0, 0, 0, 0, 0}, POWER}, //0x70
  {"CPU1_DIMM_D0_PWR", DIMM_ID1,  read_cpu1_dimm_power, 0, {0, 0, 0, 0, 0, 0, 0, 0}, POWER}, //0x71
  {"CPU1_DIMM_B1_PWR", DIMM_ID2,  read_cpu1_dimm_power, 0, {0, 0, 0, 0, 0, 0, 0, 0}, POWER}, //0x72
  {"CPU1_DIMM_D1_PWR", DIMM_ID3,  read_cpu1_dimm_power, 0, {0, 0, 0, 0, 0, 0, 0, 0}, POWER}, //0x73
  {"CPU1_DIMM_B2_PWR", DIMM_ID4,  read_cpu1_dimm_power, 0, {0, 0, 0, 0, 0, 0, 0, 0}, POWER}, //0x74
  {"CPU1_DIMM_D2_PWR", DIMM_ID5,  read_cpu1_dimm_power, 0, {0, 0, 0, 0, 0, 0, 0, 0}, POWER}, //0x75
  {"CPU1_DIMM_B3_PWR", DIMM_ID6,  read_cpu1_dimm_power, 0, {0, 0, 0, 0, 0, 0, 0, 0}, POWER}, //0x76
  {"CPU1_DIMM_D3_PWR", DIMM_ID7,  read_cpu1_dimm_power, 0, {0, 0, 0, 0, 0, 0, 0, 0}, POWER}, //0x77
  {"CPU1_DIMM_B4_PWR", DIMM_ID8,  read_cpu1_dimm_power, 0, {0, 0, 0, 0, 0, 0, 0, 0}, POWER}, //0x78
  {"CPU1_DIMM_D4_PWR", DIMM_ID9,  read_cpu1_dimm_power, 0, {0, 0, 0, 0, 0, 0, 0, 0}, POWER}, //0x79
  {"CPU1_DIMM_B5_PWR", DIMM_ID10, read_cpu1_dimm_power, 0, {0, 0, 0, 0, 0, 0, 0, 0}, POWER}, //0x7A
  {"CPU1_DIMM_D5_PWR", DIMM_ID11, read_cpu1_dimm_power, 0, {0, 0, 0, 0, 0, 0, 0, 0}, POWER}, //0x7B
  {"CPU1_DIMM_B6_PWR", DIMM_ID12, read_cpu1_dimm_power, 0, {0, 0, 0, 0, 0, 0, 0, 0}, POWER}, //0x7C
  {"CPU1_DIMM_D6_PWR", DIMM_ID13, read_cpu1_dimm_power, 0, {0, 0, 0, 0, 0, 0, 0, 0}, POWER}, //0x7D
  {"CPU1_DIMM_B7_PWR", DIMM_ID14, read_cpu1_dimm_power, 0, {0, 0, 0, 0, 0, 0, 0, 0}, POWER}, //0x7E
  {"CPU1_DIMM_D7_PWR", DIMM_ID15, read_cpu1_dimm_power, 0, {0, 0, 0, 0, 0, 0, 0, 0}, POWER}, //0x7F
};

extern struct snr_map sensor_map[];

size_t mb_sensor_cnt = sizeof(mb_sensor_list)/sizeof(uint8_t);
size_t mb_discrete_sensor_cnt = sizeof(mb_discrete_sensor_list)/sizeof(uint8_t);

bool check_pwron_time(int time) {
  char str[MAX_VALUE_LEN] = {0};
  struct timespec ts;
  long pwron_time;

  clock_gettime(CLOCK_MONOTONIC, &ts);
  if (!kv_get("snr_pwron_flag", str, NULL, 0)) {
    pwron_time = strtoul(str, NULL, 10);
   // syslog(LOG_WARNING, "power on time =%ld\n", pwron_time);
    if ( (ts.tv_sec - pwron_time > time ) && ( pwron_time != 0 ) ) {
      return true;
    }
  } else {
     sprintf(str, "%ld", ts.tv_sec);
     kv_set("snr_pwron_flag", str, 0, 0);
  }

  return false;
}

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

int
read_adc128_val(uint8_t fru, uint8_t sensor_num, float *value) {
  int ret;
  uint8_t snr_id = sensor_map[fru].map[sensor_num].id;

  char *devs[] = {
    "adc128d818-i2c-20-1d",
  };

  if (snr_id >= ARRAY_SIZE(devs)) {
    return -1;
  }

  ret = sensors_read(devs[snr_id], sensor_map[fru].map[sensor_num].snr_name, value);
  return ret;
}

int
oper_adc128_power(uint8_t fru, uint8_t volt_snr_num, uint8_t curr_snr_num, float *value) {
  int ret;
  float iout=0;
  float vout=0;

  ret = read_adc128_val(fru, curr_snr_num, &iout);
  if(ret)
    return -1;

  ret = read_hsc_vin(fru, volt_snr_num, &vout);
  if(ret)
    return -1;

  *value = iout * vout;
  return 0;
}

static int
read_e1s_power(uint8_t fru, uint8_t sensor_num, float *value) {
  int ret;

  ret = oper_adc128_power(fru, MB_SNR_HSC_VIN, MB_SNR_E1S_P12V_IOUT, value);
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

  return lib_get_cpu_tjmax(cpu_id, value);
}

static int
read_cpu_thermal_margin(uint8_t fru, uint8_t sensor_num, float *value) {
  uint8_t cpu_id = sensor_map[fru].map[sensor_num].id;

  return lib_get_cpu_thermal_margin(cpu_id, value);
}

static int
read_cpu_pkg_pwr(uint8_t fru, uint8_t sensor_num, float *value) {
  uint8_t cpu_id = sensor_map[fru].map[sensor_num].id;
  uint8_t cpu_addr = cpu_info_list[cpu_id].cpu_addr;

  return lib_get_cpu_pkg_pwr(cpu_id, cpu_addr, value);
}

static int
read_cpu_temp(uint8_t fru, uint8_t sensor_num, float *value) {
  uint8_t cpu_id = sensor_map[fru].map[sensor_num].id;
  uint8_t cpu_addr = cpu_info_list[cpu_id].cpu_addr;

  return lib_get_cpu_temp(cpu_id, cpu_addr, value);
}

static int
read_cpu0_dimm_temp(uint8_t fru, uint8_t sensor_num, float *value) {
  int ret;
  uint8_t dimm_id = sensor_map[fru].map[sensor_num].id;
  static uint8_t retry[DIMM_CNT] = {0};

  if(pal_bios_completed(fru) != true) {
    return READING_NA;
  }

  ret = lib_get_cpu_dimm_temp(PECI_CPU0_ADDR, dimm_id, value);
  if ( ret != 0 || *value == 255 ) {
    retry[dimm_id]++;
    return retry_err_handle(retry[dimm_id], 5);
  }

#ifdef DEBUG
  syslog(LOG_DEBUG, "%s DIMM Temp=%f id=%d\n", __func__, *value, dimm_id);
#endif
  retry[dimm_id] = 0;
  return 0;
}

static int
read_cpu1_dimm_temp(uint8_t fru, uint8_t sensor_num, float *value) {
  int ret;
  uint8_t dimm_id = sensor_map[fru].map[sensor_num].id;
  static uint8_t retry[DIMM_CNT] = {0};

  if(pal_bios_completed(fru) != true) {
    return READING_NA;
  }

  ret = lib_get_cpu_dimm_temp(PECI_CPU1_ADDR, dimm_id, value);
  if ( ret != 0 || *value == 255 ) {
    retry[dimm_id]++;
    return retry_err_handle(retry[dimm_id], 5);
  }
#ifdef DEBUG
  syslog(LOG_DEBUG, "%s DIMM Temp=%f id=%d\n", __func__, *value, dimm_id);
#endif
  retry[dimm_id] = 0;
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
                uint8_t dimm_id, uint8_t cpu_id, bool* cached) {
  int ret;
  NM_RW_INFO info;
  NM_DIMM_SMB_DEV dev;
  uint8_t rxbuf[16];
  uint8_t txbuf[4];
  uint8_t len;

  get_nm_rw_info(&info.bus, &info.nm_addr, &info.bmc_addr);
  dev.cpu = cpu_id;
  dev.bus_id = dimm_pmic_list[dimm_id].identify;
  dev.addr = dimm_pmic_list[dimm_id].addr;
  dev.offset_len = 1;
  len = 1;

  do {
    if(!cached[dimm_id]) {
      dev.offset = 0x00000030;
      txbuf[0] = 0x80;
      if ( (ret = retry_cond(!cmd_NM_write_dimm_smbus(&info, &dev, len, txbuf), 10, 50)) )
        break;

      //1.VIN_BULK_POWER_GOOD_THRESHOLD_VOLTAGE:4.25V
      //2.Report total power
      dev.offset = 0x0000001A;
      txbuf[0] = 0xC2;
      if ( (ret = retry_cond(!cmd_NM_write_dimm_smbus(&info, &dev, len, txbuf), 10, 50)) )
        break;

      //Report Power Measurements in reg
      dev.offset = 0x0000001B;
      txbuf[0] = 0x45;
      if ( (ret = retry_cond(!cmd_NM_write_dimm_smbus(&info, &dev, len, txbuf), 10, 50)) )
        break;
      cached[dimm_id] = true;
    }
    dev.offset = 0x0000000C;
    ret = retry_cond(!cmd_NM_read_dimm_smbus(&info, &dev, len, rxbuf), 10, 50);
  } while(0);

  if( ret == 0 )
    *value = ((float)(rxbuf[0] * 125))/1000;

#ifdef DEBUG
  syslog(LOG_DEBUG, "%s DIMM addr=%x val=%d\n", __func__, dev.addr, rxbuf[0]);
  syslog(LOG_DEBUG, "%s DIMM Power=%f id=%d\n", __func__, *value, dimm_id);
#endif

  return ret;
}

static int
read_cpu0_dimm_power(uint8_t fru, uint8_t sensor_num, float *value) {
  int ret;
  static uint8_t retry[DIMM_ID_MAX] = {0};
  static bool cached[DIMM_ID_MAX] = {false};
  uint8_t dimm_id = sensor_map[fru].map[sensor_num].id;

  if(pal_bios_completed(fru) != true) {
    return READING_NA;
  }

  ret = read_dimm_power(fru, sensor_num, value, dimm_id, CPU_ID0, cached);
  if ( ret != 0 ) {
    retry[dimm_id]++;
    return retry_err_handle(retry[dimm_id], 5);
  }

  retry[dimm_id] = 0;
  return 0;
}

static int
read_cpu1_dimm_power(uint8_t fru, uint8_t sensor_num, float *value) {
  int ret;
  static uint8_t retry[DIMM_ID_MAX] = {0};
  static bool cached[DIMM_ID_MAX] = {false};
  uint8_t dimm_id = sensor_map[fru].map[sensor_num].id;

  if(pal_bios_completed(fru) != true) {
    return READING_NA;
  }

  ret = read_dimm_power(fru, sensor_num, value, dimm_id, CPU_ID1, cached);
  if ( ret != 0 ) {
    retry[dimm_id]++;
    return retry_err_handle(retry[dimm_id], 5);
  }

  retry[dimm_id] = 0;
  return 0;
}

//Sensor HSC
static int
get_hsc_reading(uint8_t hsc_id, uint8_t reading_type, uint8_t type, uint8_t cmd, float *value) {
  uint8_t hsc_bus = hsc_info_list[hsc_id].bus;
  uint8_t addr = hsc_info_list[hsc_id].addr;
  int fd;
  uint8_t rbuf[255] = {0x00};
  uint8_t rlen = 0;
  uint8_t tlen = 1;
  int ret = -1;

  if (value == NULL) {
    return READING_NA;
  }

  fd = i2c_cdev_slave_open(hsc_bus, addr >> 1, I2C_SLAVE_FORCE_CLAIM);
  if ( fd < 0 ) {
    syslog(LOG_WARNING, "Failed to open bus %d", hsc_bus);
    return READING_NA;
  }

  switch(reading_type) {
    case I2C_BYTE:
      rlen = 1;
      break;
    case I2C_WORD:
      rlen = 2;
      break;
    default:
      rlen = 1;
  }

  ret = i2c_rdwr_msg_transfer(fd, addr, &cmd, tlen, rbuf, rlen);
  if ( ret < 0 ) {
    ret = READING_NA;
    goto exit;
  }

  float m = hsc_binding->info[type].m;
  float b = hsc_binding->info[type].b;
  float r = hsc_binding->info[type].r;

  *value = ((float)(rbuf[1] << 8 | rbuf[0]) * r - b) / m;
exit:
  if ( fd >= 0 ) {
    close(fd);
    fd = -1;
  }
  return ret;
}


static int
read_hsc_vin(uint8_t fru, uint8_t sensor_num, float *value) {
  uint8_t hsc_id = sensor_map[fru].map[sensor_num].id;

  if ( get_hsc_reading(hsc_id, I2C_WORD, HSC_VOLTAGE, PMBUS_READ_VIN, value) < 0 ) {
    return READING_NA;
  }
  return 0;
}

static int
read_hsc_iout(uint8_t fru, uint8_t sensor_num, float *value) {
  uint8_t hsc_id = sensor_map[fru].map[sensor_num].id;

  if ( get_hsc_reading(hsc_id, I2C_WORD, HSC_CURRENT, PMBUS_READ_IOUT, value) < 0 ) {
    return READING_NA;
  }
  return 0;
}

static int
read_hsc_pin(uint8_t fru, uint8_t sensor_num, float *value) {
  uint8_t hsc_id = sensor_map[fru].map[sensor_num].id;

  if ( get_hsc_reading(hsc_id, I2C_WORD, HSC_POWER, PMBUS_READ_PIN, value) < 0 ) {
    return READING_NA;
  }
  return 0;
}

static int
read_hsc_temp(uint8_t fru, uint8_t sensor_num, float *value) {
  uint8_t hsc_id = sensor_map[fru].map[sensor_num].id;

  if ( get_hsc_reading(hsc_id, I2C_WORD, HSC_TEMP, PMBUS_READ_TEMPERATURE_1, value) < 0 ) {
    return READING_NA;
  }
  return 0;
}

static int
read_hsc_peak_pin(uint8_t fru, uint8_t sensor_num, float *value) {
  uint8_t hsc_id = sensor_map[fru].map[sensor_num].id;

  if ( get_hsc_reading(hsc_id, I2C_WORD, HSC_POWER, HSC_PEAK_PIN, value) < 0 ) {
    return READING_NA;
  }
  return 0;
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
  int ret;
  uint8_t snr_id = sensor_map[fru].map[sensor_num].id;

  char *devs[] = {
    "stlm75-i2c-21-48",
    "stlm75-i2c-22-48",
    "stlm75-i2c-23-48",
    "stlm75-i2c-24-48",
  };

  if (snr_id >= ARRAY_SIZE(devs)) {
    return -1;
  }

  ret = sensors_read(devs[snr_id], sensor_map[fru].map[sensor_num].snr_name, value);
  return ret;
}

static int
read_vr_vout(uint8_t fru, uint8_t sensor_num, float *value) {
  int ret = 0;
  uint8_t vr_id = sensor_map[fru].map[sensor_num].id;
  static uint8_t retry[VR_NUM_CNT] = {0};

  if (vr_id >= VR_NUM_CNT)
    return -1;

  ret = sensors_read(vr_chips[vr_id], sensor_map[fru].map[sensor_num].snr_name, value);
  if (*value == 0) {
    retry[vr_id]++;
    return retry_err_handle(retry[vr_id], 5);
  }

  retry[vr_id] = 0;
  return ret;
}

int
read_dpm_vout(uint8_t fru, uint8_t sensor_num, float *value) {
  int fd = 0, ret = -1;
  char fn[32];
  uint8_t tlen, rlen, addr, bus;
  float scale;
  uint8_t tbuf[16] = {0};
  uint8_t rbuf[16] = {0};
  uint8_t dpm_id = sensor_map[fru].map[sensor_num].id;
  static uint8_t retry=0;

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

  if( ret < 0 ) {
    retry++;
    ret = retry_err_handle(retry, 2);
    goto err_exit;
  }

  *value = ((rbuf[0] << 8 | rbuf[1]) >> 2) * scale;
  retry=0;
err_exit:
  if (fd > 0) {
    close(fd);
  }
  return ret;
}

static int
read_vr_temp(uint8_t fru, uint8_t sensor_num, float *value) {
  int ret = 0;
  uint8_t vr_id = sensor_map[fru].map[sensor_num].id;
  static uint8_t retry[VR_NUM_CNT] = {0};

  if (vr_id >= VR_NUM_CNT) {
    return -1;
  }

  ret = sensors_read(vr_chips[vr_id], sensor_map[fru].map[sensor_num].snr_name, value);
  if (*value == 0) {
    retry[vr_id]++;
    return retry_err_handle(retry[vr_id], 5);
  }

  retry[vr_id] = 0;
  return ret;
}

static int
read_vr_iout(uint8_t fru, uint8_t sensor_num, float *value) {
  uint8_t vr_id = sensor_map[fru].map[sensor_num].id;

  if (vr_id >= VR_NUM_CNT) {
    return -1;
  }
  return sensors_read(vr_chips[vr_id], sensor_map[fru].map[sensor_num].snr_name, value);
}

static int
read_vr_pout(uint8_t fru, uint8_t sensor_num, float *value) {
  uint8_t vr_id = sensor_map[fru].map[sensor_num].id;

  if (vr_id >= VR_NUM_CNT) {
    return -1;
  }
  return sensors_read(vr_chips[vr_id], sensor_map[fru].map[sensor_num].snr_name, value);
}

bool
pal_bios_completed(uint8_t fru)
{
  gpio_desc_t *desc;
  gpio_value_t value;
  bool ret = false;

  if ( FRU_MB != fru )
  {
    syslog(LOG_WARNING, "[%s]incorrect fru id: %d", __func__, fru);
    return false;
  }

//BIOS COMPLT need time to inital when platform reset.
  if( !check_pwron_time(10) ) {
    return false;
  }

  desc = gpio_open_by_shadow("FM_BIOS_POST_CMPLT_BMC_N");
  if (!desc)
    return false;

  if (gpio_get_value(desc, &value) == 0 && value == GPIO_VALUE_LOW)
    ret = true;
  gpio_close(desc);
  return ret;
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

  *value = (float)frb3_fail;
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
  return ret;
}

void
get_dimm_present_info(uint8_t fru, bool *dimm_sts_list) {
  char key[MAX_KEY_LEN] = {0};
  char value[MAX_VALUE_LEN] = {0};
  int i;
  size_t ret;

  //check dimm info from /mnt/data/sys_config/
  for (i=0; i<DIMM_SLOT_CNT; i++) {
    sprintf(key, "sys_config/fru%d_dimm%d_location", fru, i);
    if(kv_get(key, value, &ret, KV_FPERSIST) != 0 || ret < 4) {
      syslog(LOG_WARNING,"[%s]Cannot get dimm_slot%d present info", __func__, i);
      return;
    }

    if ( 0xff == value[0] ) {
      dimm_sts_list[i] = false;
    } else {
      dimm_sts_list[i] = true;
    }
  }
}

bool
pal_is_dimm_present(uint8_t dimm_id)
{
  static bool is_check = false;
  static bool dimm_sts_list[96] = {0};
  uint8_t fru = FRU_MB;

  if (!pal_bios_completed(fru) ) {
    return false;
  }

  if ( is_check == false ) {
    is_check = true;
    get_dimm_present_info(fru, dimm_sts_list);
  }

  if( dimm_sts_list[dimm_id] == true) {
    return true;
  }
  return false;
}
