#include <stdio.h>
#include <math.h>
#include <stdint.h>
#include <stdbool.h>
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
#include "pal.h"

//#define DEBUG
#define GPIO_P3V_BAT_SCALED_EN "BATTERY_DETECT"
#define MAX_READ_RETRY (10)
#define POLLING_DELAY_TIME (10)

#define PECI_MUX_SELECT_BMC (GPIO_VALUE_HIGH)
#define PECI_MUX_SELECT_PCH (GPIO_VALUE_LOW)
#define SENSOR_SKIP_MAX (1)

#define HSC_12V_BUS  (2)

#define FAN_PRESNT_TEMPLATE     "FAN%d_PRESENT"

//I2C type
enum {
  I2C_BYTE = 0,
  I2C_WORD,
};

uint8_t pwr_polling_flag=false;
uint8_t DIMM_SLOT_CNT = 0;
size_t pal_pwm_cnt = FAN_PWM_CNT;
size_t pal_tach_cnt = FAN_TACH_CNT;
const char pal_pwm_list[] = "0,1,2,3,4,5,6,7,8,9,10,11,12,13,14,15";
const char pal_tach_list[] = "0,1,2,3,4,5,6,7,8,9,10,11,12,13,14,15,16,17,18,19,20,21,22,23,24,25,26,27,28,29,30,31";
//static float InletCalibration = 0;

bool pal_bios_completed(uint8_t fru);
static int read_adc_val(uint8_t sensor_num, float *value);
static int read_bat_val(uint8_t sensor_num, float *value);
static int read_temp(uint8_t sensor_num, float *value);
static int read_cpu_temp(uint8_t sensor_num, float *value);
static int read_cpu_pkg_pwr(uint8_t sensor_num, float *value);
static int read_cpu_tjmax(uint8_t sensor_num, float *value);
static int read_cpu_thermal_margin(uint8_t sensor_num, float *value);
static int read_hsc_iout(uint8_t sensor_num, float *value);
static int read_hsc_vin(uint8_t sensor_num, float *value);
static int read_hsc_pin(uint8_t sensor_num, float *value);
static int read_hsc_temp(uint8_t sensor_num, float *value);
static int read_hsc_peak_pin(uint8_t sensor_num, float *value);
static int read_cpu0_dimm_temp(uint8_t sensor_num, float *value);
static int read_cpu1_dimm_temp(uint8_t sensor_num, float *value);
static int read_NM_pch_temp(uint8_t nm_id, float *value);
static int read_vr_vout(uint8_t sensor_num, float *value);
static int read_vr_temp(uint8_t sensor_num, float  *value);
static int read_vr_iout(uint8_t sensor_num, float  *value);
static int read_vr_pout(uint8_t sensor_num, float  *value);
static int read_frb3(uint8_t sensor_num, float *value);
static int read_fan_speed(uint8_t sensor_num, float *value);
static int read_48v_hsc_vin(uint8_t sensor_num, float *value);
static int read_48v_hsc_iout(uint8_t sensor_num, float *value);
static int read_48v_hsc_pin(uint8_t sensor_num, float *value);
static int read_48v_hsc_peak_pin(uint8_t sensor_num, float *value);
static int read_48v_hsc_temp(uint8_t sensor_num, float *value);
static int read_brick_vout(uint8_t sensor_num, float *value);
static int read_brick_iout(uint8_t sensor_num, float *value);
static int read_brick_temp(uint8_t sensor_num, float *value);
static int read_dpm_vout(uint8_t sensor_num, float *value);
static int get_nm_rw_info(uint8_t nm_id, uint8_t* nm_bus, uint8_t* nm_addr, uint16_t* bmc_addr);

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
  MB_SNR_P12V,
  MB_SNR_P5V,
  MB_SNR_P3V3,
  MB_SNR_P2V5,
  MB_SNR_P1V8,
  MB_SNR_PGPPA,
  MB_SNR_P1V2,
  MB_SNR_P3V_BAT,
  MB_SNR_P1V0,
  MB_SNR_DPM_M2_P3V3_VOUT,
  MB_SNR_DPM_E1S_P3V3_VOUT,
};

const uint8_t nic0_sensor_list[] = {
  NIC0_SNR_MEZZ0_TEMP,
  NIC0_SNR_DPM_MEZZ0_P3V3_VOUT,
};

const uint8_t nic1_sensor_list[] = {
  NIC1_SNR_MEZZ1_TEMP,
  NIC1_SNR_DPM_MEZZ1_P3V3_VOUT,
};

const uint8_t pdbv_sensor_list[] = {
  PDBV_SNR_HSC0_VIN,
  PDBV_SNR_HSC0_IOUT,
  PDBV_SNR_HSC0_PIN,
  PDBV_SNR_HSC0_TEMP,
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

const uint8_t pdbh_sensor_list[] = {
  PDBH_SNR_HSC1_VIN,
  PDBH_SNR_HSC1_IOUT,
  PDBH_SNR_HSC1_PIN,
  PDBH_SNR_HSC1_TEMP,
  PDBH_SNR_HSC2_VIN,
  PDBH_SNR_HSC2_IOUT,
  PDBH_SNR_HSC2_PIN,
  PDBH_SNR_HSC2_TEMP,
};

const uint8_t bp0_sensor_list[] = {
  BP0_SNR_FAN0_INLET_SPEED,
  BP0_SNR_FAN0_OUTLET_SPEED,
  BP0_SNR_FAN1_INLET_SPEED,
  BP0_SNR_FAN1_OUTLET_SPEED,
  BP0_SNR_FAN4_INLET_SPEED,
  BP0_SNR_FAN4_OUTLET_SPEED,
  BP0_SNR_FAN5_INLET_SPEED,
  BP0_SNR_FAN5_OUTLET_SPEED,
  BP0_SNR_FAN8_INLET_SPEED,
  BP0_SNR_FAN8_OUTLET_SPEED,
  BP0_SNR_FAN9_INLET_SPEED,
  BP0_SNR_FAN9_OUTLET_SPEED,
  BP0_SNR_FAN12_INLET_SPEED,
  BP0_SNR_FAN12_OUTLET_SPEED,
  BP0_SNR_FAN13_INLET_SPEED,
  BP0_SNR_FAN13_OUTLET_SPEED,
};

const uint8_t bp1_sensor_list[] = {
  BP1_SNR_FAN2_INLET_SPEED,
  BP1_SNR_FAN2_OUTLET_SPEED,
  BP1_SNR_FAN3_INLET_SPEED,
  BP1_SNR_FAN3_OUTLET_SPEED,
  BP1_SNR_FAN6_INLET_SPEED,
  BP1_SNR_FAN6_OUTLET_SPEED,
  BP1_SNR_FAN7_INLET_SPEED,
  BP1_SNR_FAN7_OUTLET_SPEED,
  BP1_SNR_FAN10_INLET_SPEED,
  BP1_SNR_FAN10_OUTLET_SPEED,
  BP1_SNR_FAN11_INLET_SPEED,
  BP1_SNR_FAN11_OUTLET_SPEED,
  BP1_SNR_FAN14_INLET_SPEED,
  BP1_SNR_FAN14_OUTLET_SPEED,
  BP1_SNR_FAN15_INLET_SPEED,
  BP1_SNR_FAN15_OUTLET_SPEED,
};

const uint8_t scm_sensor_list[] = {
  SCM_SNR_DPM_BMC_P12V_VOUT,
  SCM_SNR_BMC_TEMP,
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


//48V HSC
char *hsc_adm1272_chips[HSC_48V_CNT] = {
    "adm1272-i2c-38-10",
    "adm1272-i2c-39-13",
    "adm1272-i2c-39-1c",
};

char **hsc_48v_chips = hsc_adm1272_chips;


//BRICK
char *brick_pmbus_chips[BRICK_CNT] = {
    "pmbus-i2c-38-69",
    "pmbus-i2c-38-6a",
    "pmbus-i2c-38-6b",
};

//NM
PAL_I2C_BUS_INFO nm_info_list[] = {
  {NM_ID0, NM_IPMB_BUS_ID, NM_SLAVE_ADDR},
};

//NM
PAL_I2C_BUS_INFO fan_info_list[] = {
  {FAN_CHIP_ID0, I2C_BUS_40, 0x5E},
  {FAN_CHIP_ID1, I2C_BUS_40, 0x40},
  {FAN_CHIP_ID2, I2C_BUS_41, 0x5E},
  {FAN_CHIP_ID3, I2C_BUS_41, 0x40},
};

PAL_I2C_BUS_INFO disk_info_list[] = {
  {DISK_BOOT, I2C_BUS_26, 0xD4},
};

PAL_DPM_DEV_INFO dpm_info_list[] = {
  {DPM_0, I2C_BUS_34, 0x84, 0.004, 0, 0 },
  {DPM_1, I2C_BUS_34, 0x88, 0.004, 0, 0 },
  {DPM_2, I2C_BUS_34, 0x82, 0.004, 0, 0 },
  {DPM_3, I2C_BUS_34, 0x86, 0.004, 0, 0 },
  {DPM_4, I2C_BUS_34, 0x8A, 0.004, 0, 0 },
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


//FAN CHIP
char *max31790_chips[FAN_CHIP_CNT] = {
  "max31790-i2c-40-2f",  // BP0
  "max31790-i2c-40-20",  // BP0
  "max31790-i2c-41-2f",  // BP1
  "max31790-i2c-41-20",  // BP1
};
char **fan_chips = max31790_chips;

//{SensorName, ID, FUNCTION, PWR_STATUS, {UCR, UNC, UNR, LCR, LNC, LNR, Pos, Neg}
PAL_SENSOR_MAP sensor_map[] = {
  {NULL, 0, NULL, 0, {0, 0, 0, 0, 0, 0, 0, 0}, 0}, //0x00
  {"E1S_DPM_P3V3_VOUT", DPM_0, read_dpm_vout, false, {0, 0, 0, 0, 0, 0, 0, 0}, VOLT}, //0x01
  {NULL, 0, NULL, 0, {0, 0, 0, 0, 0, 0, 0, 0}, 0}, //0x02
  {NULL, 0, NULL, 0, {0, 0, 0, 0, 0, 0, 0, 0}, 0}, //0x03
  {NULL, 0, NULL, 0, {0, 0, 0, 0, 0, 0, 0, 0}, 0}, //0x04
  {"M2_DPM_P3V3_VOUT", DPM_1, read_dpm_vout, false, {0, 0, 0, 0, 0, 0, 0, 0}, VOLT}, //0x05
  {NULL, 0, NULL, 0, {0, 0, 0, 0, 0, 0, 0, 0}, 0}, //0x06
  {NULL, 0, NULL, 0, {0, 0, 0, 0, 0, 0, 0, 0}, 0}, //0x07

  {NULL, 0, NULL, 0, {0, 0, 0, 0, 0, 0, 0, 0}, 0}, //0x08
  {NULL, 0, NULL, 0, {0, 0, 0, 0, 0, 0, 0, 0}, 0}, //0x09
  {"PROCESSOR_FAIL", FRU_MB, read_frb3, 0, {0, 0, 0, 0, 0, 0, 0, 0}, STATE}, //0x0A
  {"HSC_VIN",   HSC_ID0, read_hsc_vin,  true, {0, 0, 0, 0, 0, 0, 0, 0}, VOLT}, //0x0B
  {"HSC_IOUT",  HSC_ID0, read_hsc_iout, true, {0, 0, 0, 0, 0, 0, 0, 0}, CURR}, //0x0C
  {"HSC_PIN",   HSC_ID0, read_hsc_pin,  true, {0, 0, 0, 0, 0, 0, 0, 0}, POWER}, //0x0D
  {"HSC_TEMP",  HSC_ID0, read_hsc_temp, true, {0, 0, 0, 0, 0, 0, 0, 0}, TEMP}, //0x0E
  {"HSC_PEAK_PIN",  HSC_ID0, read_hsc_peak_pin, true, {0, 0, 0, 0, 0, 0, 0, 0}, POWER}, //0x0F

  {"INLET_TEMP_R", TEMP_INLET_R, read_temp, true, {0, 0, 0, 0, 0, 0, 0, 0}, TEMP}, //0x10
  {"INLET_TEMP_L", TEMP_INLET_L, read_temp, true, {0, 0, 0, 0, 0, 0, 0, 0}, TEMP}, //0x11
  {"OUTLET_TEMP_R", TEMP_OUTLET_R, read_temp, true, {0, 0, 0, 0, 0, 0, 0, 0}, TEMP}, //0x12
  {"OUTLET_TEMP_L", TEMP_OUTLET_L, read_temp, true, {0, 0, 0, 0, 0, 0, 0, 0}, TEMP}, //0x13
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
  {"CPU0_PKG_POWER", CPU_ID0, read_cpu_pkg_pwr, false, {0, 0, 0, 0, 0, 0, 0, 0}, POWER}, //0x1E
  {"CPU1_PKG_POWER", CPU_ID1, read_cpu_pkg_pwr, false, {0, 0, 0, 0, 0, 0, 0, 0}, POWER}, //0x1F

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

  {"VR_CPU0_VCCIN_VOUT", VR_ID0, read_vr_vout, false, {0, 0, 0, 0, 0, 0, 0, 0}, VOLT}, //0x30
  {"VR_CPU0_VCCIN_TEMP", VR_ID0, read_vr_temp, false, {0, 0, 0, 0, 0, 0, 0, 0}, TEMP}, //0x31
  {"VR_CPU0_VCCIN_IOUT", VR_ID0, read_vr_iout, false, {0, 0, 0, 0, 0, 0, 0, 0}, CURR}, //0X32
  {"VR_CPU0_VCCIN_POUT", VR_ID0, read_vr_pout, false, {0, 0, 0, 0, 0, 0, 0, 0}, POWER}, //0x33
  {"VR_CPU0_VCCFA_FIVRA_VOUT", VR_ID1, read_vr_vout, false, {0, 0, 0, 0, 0, 0, 0, 0}, VOLT}, //0x34
  {"VR_CPU0_VCCFA_FIVRA_TEMP", VR_ID1, read_vr_temp, false, {0, 0, 0, 0, 0, 0, 0, 0}, TEMP}, //0x35
  {"VR_CPU0_VCCFA_FIVRA_IOUT", VR_ID1, read_vr_iout, false, {0, 0, 0, 0, 0, 0, 0, 0}, CURR}, //0X36
  {"VR_CPU0_VCCFA_FIVRA_POUT", VR_ID1, read_vr_pout, false, {0, 0, 0, 0, 0, 0, 0, 0}, POWER}, //0x37
  {"VR_CPU0_VCCIN_FAON_VOUT", VR_ID2, read_vr_vout, false, {0, 0, 0, 0, 0, 0, 0, 0}, VOLT}, //0x38
  {"VR_CPU0_VCCIN_FAON_TEMP", VR_ID2, read_vr_temp, false, {0, 0, 0, 0, 0, 0, 0, 0}, TEMP}, //0x39
  {"VR_CPU0_VCCIN_FAON_IOUT", VR_ID2, read_vr_iout, false, {0, 0, 0, 0, 0, 0, 0, 0}, CURR}, //0X3A
  {"VR_CPU0_VCCIN_FAON_POUT", VR_ID2, read_vr_pout, false, {0, 0, 0, 0, 0, 0, 0, 0}, POWER}, //0x3B
  {"VR_CPU0_VCCFA_VOUT", VR_ID3, read_vr_vout, false, {0, 0, 0, 0, 0, 0, 0, 0}, VOLT}, //0x3C
  {"VR_CPU0_VCCFA_TEMP", VR_ID3, read_vr_temp, false, {0, 0, 0, 0, 0, 0, 0, 0}, TEMP}, //0x3D
  {"VR_CPU0_VCCFA_IOUT", VR_ID3, read_vr_iout, false, {0, 0, 0, 0, 0, 0, 0, 0}, CURR}, //0X3E
  {"VR_CPU0_VCCFA_POUT", VR_ID3, read_vr_pout, false, {0, 0, 0, 0, 0, 0, 0, 0}, POWER}, //0x3F
  {"VR_CPU0_VCCD_HV_VOUT", VR_ID4, read_vr_vout, false, {0, 0, 0, 0, 0, 0, 0, 0}, VOLT}, //0x40
  {"VR_CPU0_VCCD_HV_TEMP", VR_ID4, read_vr_temp, false, {0, 0, 0, 0, 0, 0, 0, 0}, TEMP}, //0x41
  {"VR_CPU0_VCCD_HV_IOUT", VR_ID4, read_vr_iout, false, {0, 0, 0, 0, 0, 0, 0, 0}, CURR}, //0X42
  {"VR_CPU0_VCCD_HV_POUT", VR_ID4, read_vr_pout, false, {0, 0, 0, 0, 0, 0, 0, 0}, POWER}, //0x43
  {NULL, 0, NULL, 0, {0, 0, 0, 0, 0, 0, 0, 0}, 0}, //0x44
  {NULL, 0, NULL, 0, {0, 0, 0, 0, 0, 0, 0, 0}, 0}, //0x45
  {NULL, 0, NULL, 0, {0, 0, 0, 0, 0, 0, 0, 0}, 0}, //0x46
  {NULL, 0, NULL, 0, {0, 0, 0, 0, 0, 0, 0, 0}, 0}, //0x47

  {"VR_CPU1_VCCIN_VOUT", VR_ID5, read_vr_vout, false, {0, 0, 0, 0, 0, 0, 0, 0}, VOLT}, //0x48
  {"VR_CPU1_VCCIN_TEMP", VR_ID5, read_vr_temp, false, {0, 0, 0, 0, 0, 0, 0, 0}, TEMP}, //0x49
  {"VR_CPU1_VCCIN_IOUT", VR_ID5, read_vr_iout, false, {0, 0, 0, 0, 0, 0, 0, 0}, CURR}, //0X4A
  {"VR_CPU1_VCCIN_POUT", VR_ID5, read_vr_pout, false, {0, 0, 0, 0, 0, 0, 0, 0}, POWER}, //0x4B
  {"VR_CPU1_VCCFA_FIVRA_VOUT", VR_ID6, read_vr_vout, false, {0, 0, 0, 0, 0, 0, 0, 0}, VOLT}, //0x4C
  {"VR_CPU1_VCCFA_FIVRA_TEMP", VR_ID6, read_vr_temp, false, {0, 0, 0, 0, 0, 0, 0, 0}, TEMP}, //0x4D
  {"VR_CPU1_VCCFA_FIVRA_IOUT", VR_ID6, read_vr_iout, false, {0, 0, 0, 0, 0, 0, 0, 0}, CURR}, //0X4E
  {"VR_CPU1_VCCFA_FIVRA_POUT", VR_ID6, read_vr_pout, false, {0, 0, 0, 0, 0, 0, 0, 0}, POWER}, //0x4F
  {"VR_CPU1_VCCIN_FAON_VOUT", VR_ID7, read_vr_vout, false, {0, 0, 0, 0, 0, 0, 0, 0}, VOLT}, //0x50
  {"VR_CPU1_VCCIN_FAON_TEMP", VR_ID7, read_vr_temp, false, {0, 0, 0, 0, 0, 0, 0, 0}, TEMP}, //0x51
  {"VR_CPU1_VCCIN_FAON_IOUT", VR_ID7, read_vr_iout, false, {0, 0, 0, 0, 0, 0, 0, 0}, CURR}, //0X52
  {"VR_CPU1_VCCIN_FAON_POUT", VR_ID7, read_vr_pout, false, {0, 0, 0, 0, 0, 0, 0, 0}, POWER}, //0x53
  {"VR_CPU1_VCCFA_VOUT", VR_ID8, read_vr_vout, false, {0, 0, 0, 0, 0, 0, 0, 0}, VOLT}, //0x54
  {"VR_CPU1_VCCFA_TEMP", VR_ID8, read_vr_temp, false, {0, 0, 0, 0, 0, 0, 0, 0}, TEMP}, //0x55
  {"VR_CPU1_VCCFA_IOUT", VR_ID8, read_vr_iout, false, {0, 0, 0, 0, 0, 0, 0, 0}, CURR}, //0X56
  {"VR_CPU1_VCCFA_POUT", VR_ID8, read_vr_pout, false, {0, 0, 0, 0, 0, 0, 0, 0}, POWER}, //0x57
  {"VR_CPU1_VCCD_HV_VOUT", VR_ID9, read_vr_vout, false, {0, 0, 0, 0, 0, 0, 0, 0}, VOLT}, //0x58
  {"VR_CPU1_VCCD_HV_TEMP", VR_ID9, read_vr_temp, false, {0, 0, 0, 0, 0, 0, 0, 0}, TEMP}, //0x59
  {"VR_CPU1_VCCD_HV_IOUT", VR_ID9, read_vr_iout, false, {0, 0, 0, 0, 0, 0, 0, 0}, CURR}, //0X5A
  {"VR_CPU1_VCCD_HV_POUT", VR_ID9, read_vr_pout, false, {0, 0, 0, 0, 0, 0, 0, 0}, POWER}, //0x5B
  {NULL, 0, NULL, 0, {0, 0, 0, 0, 0, 0, 0, 0}, 0}, //0x5C
  {NULL, 0, NULL, 0, {0, 0, 0, 0, 0, 0, 0, 0}, 0}, //0x5D
  {NULL, 0, NULL, 0, {0, 0, 0, 0, 0, 0, 0, 0}, 0}, //0x5E
  {"PCH_TEMP", NM_ID0, read_NM_pch_temp, false, {0, 0, 0, 0, 0, 0, 0, 0}, TEMP}, //0x5F

  {"P12V",    ADC0, read_adc_val, false, {0, 0, 0, 0, 0, 0, 0, 0}, VOLT}, //0x60
  {"P5V",     ADC1, read_adc_val, false, {0, 0, 0, 0, 0, 0, 0, 0}, VOLT}, //0x61
  {"P3V3",    ADC2, read_adc_val, false, {0, 0, 0, 0, 0, 0, 0, 0}, VOLT}, //0x62
  {"P2V5",    ADC3, read_adc_val, false, {0, 0, 0, 0, 0, 0, 0, 0}, VOLT}, //0x63
  {"P1V8",    ADC4, read_adc_val, false, {0, 0, 0, 0, 0, 0, 0, 0}, VOLT}, //0x64
  {"PGPPA",   ADC5, read_adc_val, false, {0, 0, 0, 0, 0, 0, 0, 0}, VOLT}, //0x65
  {"P1V2",    ADC6, read_adc_val, false, {0, 0, 0, 0, 0, 0, 0, 0}, VOLT}, //0x66
  {"P3V_BAT", ADC7, read_bat_val, false, {0, 0, 0, 0, 0, 0, 0, 0}, VOLT}, //0x67
  {"P1V0",    ADC8, read_adc_val, false, {0, 0, 0, 0, 0, 0, 0, 0}, VOLT}, //0x68
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

  {"DPM_MEZZ0_P3V3_VOUT", DPM_2, read_dpm_vout, false, {0, 0, 0, 0, 0, 0, 0, 0}, VOLT}, //0x80
  {NULL, 0, NULL, 0, {0, 0, 0, 0, 0, 0, 0, 0}, 0}, //0x81
  {NULL, 0, NULL, 0, {0, 0, 0, 0, 0, 0, 0, 0}, 0}, //0x82
  {"MEZZ0_TEMP", TEMP_MEZZ0, read_temp, true, {0, 0, 0, 0, 0, 0, 0, 0}, TEMP},  //0x83
  {"DPM_MEZZ1_P3V3_VOUT", DPM_3, read_dpm_vout, false, {0, 0, 0, 0, 0, 0, 0, 0}, VOLT}, //0x84
  {NULL, 0, NULL, 0, {0, 0, 0, 0, 0, 0, 0, 0}, 0}, //0x85
  {NULL, 0, NULL, 0, {0, 0, 0, 0, 0, 0, 0, 0}, 0}, //0x86
  {"MEZZ1_TEMP", TEMP_MEZZ1, read_temp, true, {0, 0, 0, 0, 0, 0, 0, 0}, TEMP},  //0x87

  {NULL, 0, NULL, 0, {0, 0, 0, 0, 0, 0, 0, 0}, 0}, //0x88
  {NULL, 0, NULL, 0, {0, 0, 0, 0, 0, 0, 0, 0}, 0}, //0x89
  {NULL, 0, NULL, 0, {0, 0, 0, 0, 0, 0, 0, 0}, 0}, //0x8A
  {NULL, 0, NULL, 0, {0, 0, 0, 0, 0, 0, 0, 0}, 0}, //0x8B
  {NULL, 0, NULL, 0, {0, 0, 0, 0, 0, 0, 0, 0}, 0}, //0x8C
  {NULL, 0, NULL, 0, {0, 0, 0, 0, 0, 0, 0, 0}, 0}, //0x8D
  {NULL, 0, NULL, 0, {0, 0, 0, 0, 0, 0, 0, 0}, 0}, //0x8E
  {NULL, 0, NULL, 0, {0, 0, 0, 0, 0, 0, 0, 0}, 0}, //0x8F

  {"HSC0_VIN",      HSC_48V_ID0, read_48v_hsc_vin,      true, {0, 0, 0, 0, 0, 0, 0, 0}, VOLT},  //0x90
  {"HSC0_IOUT",     HSC_48V_ID0, read_48v_hsc_iout,     true, {0, 0, 0, 0, 0, 0, 0, 0}, CURR},  //0x91
  {"HSC0_PIN",      HSC_48V_ID0, read_48v_hsc_pin,      true, {0, 0, 0, 0, 0, 0, 0, 0}, POWER}, //0x92
  {"HSC0_TEMP",     HSC_48V_ID0, read_48v_hsc_temp,     true, {0, 0, 0, 0, 0, 0, 0, 0}, TEMP},  //0x93
  {"HSC0_PEAK_PIN", HSC_48V_ID0, read_48v_hsc_peak_pin, true, {0, 0, 0, 0, 0, 0, 0, 0}, POWER}, //0x94

  {"BRICK0_VOUT", BRICK_ID0, read_brick_vout, true, {0, 0, 0, 0, 0, 0, 0, 0}, VOLT}, //0x95
  {"BRICK0_IOUT", BRICK_ID0, read_brick_iout, true, {0, 0, 0, 0, 0, 0, 0, 0}, CURR}, //0x96
  {"BRICK0_TEMP", BRICK_ID0, read_brick_temp, true, {0, 0, 0, 0, 0, 0, 0, 0}, TEMP}, //0x97
  {NULL, 0, NULL, 0, {0, 0, 0, 0, 0, 0, 0, 0}, 0}, //0x98
  {"BRICK1_VOUT", BRICK_ID1, read_brick_vout, true, {0, 0, 0, 0, 0, 0, 0, 0}, VOLT}, //0x99
  {"BRICK1_IOUT", BRICK_ID1, read_brick_iout, true, {0, 0, 0, 0, 0, 0, 0, 0}, CURR}, //0x9A
  {"BRICK1_TEMP", BRICK_ID1, read_brick_temp, true, {0, 0, 0, 0, 0, 0, 0, 0}, TEMP}, //0x9B
  {NULL, 0, NULL, 0, {0, 0, 0, 0, 0, 0, 0, 0}, 0}, //0x9C
  {"BRICK2_VOUT", BRICK_ID2, read_brick_vout, true, {0, 0, 0, 0, 0, 0, 0, 0}, VOLT}, //0x9D
  {"BRICK2_IOUT", BRICK_ID2, read_brick_iout, true, {0, 0, 0, 0, 0, 0, 0, 0}, CURR}, //0x9E
  {"BRICK2_TEMP", BRICK_ID2, read_brick_temp, true, {0, 0, 0, 0, 0, 0, 0, 0}, TEMP}, //0x9F

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

  {"HSC1_VIN",      HSC_48V_ID1, read_48v_hsc_vin,      true, {0, 0, 0, 0, 0, 0, 0, 0}, VOLT},  //0xB0
  {"HSC1_IOUT",     HSC_48V_ID1, read_48v_hsc_iout,     true, {0, 0, 0, 0, 0, 0, 0, 0}, CURR},  //0xB1
  {"HSC1_PIN",      HSC_48V_ID1, read_48v_hsc_pin,      true, {0, 0, 0, 0, 0, 0, 0, 0}, POWER}, //0xB2
  {"HSC1_TEMP",     HSC_48V_ID1, read_48v_hsc_temp,     true, {0, 0, 0, 0, 0, 0, 0, 0}, TEMP},  //0xB3
  {"HSC1_PEAK_PIN", HSC_48V_ID1, read_48v_hsc_peak_pin, true, {0, 0, 0, 0, 0, 0, 0, 0}, POWER}, //0xB4
  {"HSC2_VIN",      HSC_48V_ID2, read_48v_hsc_vin,      true, {0, 0, 0, 0, 0, 0, 0, 0}, VOLT},  //0xB5
  {"HSC2_IOUT",     HSC_48V_ID2, read_48v_hsc_iout,     true, {0, 0, 0, 0, 0, 0, 0, 0}, CURR},  //0xB6
  {"HSC2_PIN",      HSC_48V_ID2, read_48v_hsc_pin,      true, {0, 0, 0, 0, 0, 0, 0, 0}, POWER}, //0xB7
  {"HSC2_TEMP",     HSC_48V_ID2, read_48v_hsc_temp,     true, {0, 0, 0, 0, 0, 0, 0, 0}, TEMP},  //0xB8
  {"HSC2_PEAK_PIN", HSC_48V_ID2, read_48v_hsc_peak_pin, true, {0, 0, 0, 0, 0, 0, 0, 0}, POWER}, //0xB9

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
  {NULL, 0, NULL, 0, {0, 0, 0, 0, 0, 0, 0, 0}, 0}, //0xCE
  {NULL, 0, NULL, 0, {0, 0, 0, 0, 0, 0, 0, 0}, 0}, //0xCF

  {"FAN0_INLET_SPEED",  FAN_TACH_ID0,  read_fan_speed, true, {0, 0, 0, 0, 0, 0, 0, 0}, FAN}, //0xD0
  {"FAN0_OUTLET_SPEED", FAN_TACH_ID1,  read_fan_speed, true, {0, 0, 0, 0, 0, 0, 0, 0}, FAN}, //0xD1
  {"FAN1_INLET_SPEED",  FAN_TACH_ID2,  read_fan_speed, true, {0, 0, 0, 0, 0, 0, 0, 0}, FAN}, //0xD2
  {"FAN1_OUTLET_SPEED", FAN_TACH_ID3,  read_fan_speed, true, {0, 0, 0, 0, 0, 0, 0, 0}, FAN}, //0xD3
  {"FAN2_INLET_SPEED",  FAN_TACH_ID4,  read_fan_speed, true, {0, 0, 0, 0, 0, 0, 0, 0}, FAN}, //0xD4
  {"FAN2_OUTLET_SPEED", FAN_TACH_ID5,  read_fan_speed, true, {0, 0, 0, 0, 0, 0, 0, 0}, FAN}, //0xD5
  {"FAN3_INLET_SPEED",  FAN_TACH_ID6,  read_fan_speed, true, {0, 0, 0, 0, 0, 0, 0, 0}, FAN}, //0xD6
  {"FAN3_OUTLET_SPEED", FAN_TACH_ID7,  read_fan_speed, true, {0, 0, 0, 0, 0, 0, 0, 0}, FAN}, //0xD7
  {"FAN4_INLET_SPEED",  FAN_TACH_ID8,  read_fan_speed, true, {0, 0, 0, 0, 0, 0, 0, 0}, FAN}, //0xD8
  {"FAN4_OUTLET_SPEED", FAN_TACH_ID9,  read_fan_speed, true, {0, 0, 0, 0, 0, 0, 0, 0}, FAN}, //0xD9
  {"FAN5_INLET_SPEED",  FAN_TACH_ID10, read_fan_speed, true, {0, 0, 0, 0, 0, 0, 0, 0}, FAN}, //0xDA
  {"FAN5_OUTLET_SPEED", FAN_TACH_ID11, read_fan_speed, true, {0, 0, 0, 0, 0, 0, 0, 0}, FAN}, //0xDB
  {"FAN6_INLET_SPEED",  FAN_TACH_ID12, read_fan_speed, true, {0, 0, 0, 0, 0, 0, 0, 0}, FAN}, //0xDC
  {"FAN6_OUTLET_SPEED", FAN_TACH_ID13, read_fan_speed, true, {0, 0, 0, 0, 0, 0, 0, 0}, FAN}, //0xDD
  {"FAN7_INLET_SPEED",  FAN_TACH_ID14, read_fan_speed, true, {0, 0, 0, 0, 0, 0, 0, 0}, FAN}, //0xDE
  {"FAN7_OUTLET_SPEED", FAN_TACH_ID15, read_fan_speed, true, {0, 0, 0, 0, 0, 0, 0, 0}, FAN}, //0xDF

  {"FAN8_INLET_SPEED",  FAN_TACH_ID16, read_fan_speed, true, {0, 0, 0, 0, 0, 0, 0, 0}, FAN}, //0xE0
  {"FAN8_OUTLET_SPEED", FAN_TACH_ID17, read_fan_speed, true, {0, 0, 0, 0, 0, 0, 0, 0}, FAN}, //0xE1
  {"FAN9_INLET_SPEED",  FAN_TACH_ID18, read_fan_speed, true, {0, 0, 0, 0, 0, 0, 0, 0}, FAN}, //0xE2
  {"FAN9_OUTLET_SPEED", FAN_TACH_ID19, read_fan_speed, true, {0, 0, 0, 0, 0, 0, 0, 0}, FAN}, //0xE3
  {"FAN10_INLET_SPEED",  FAN_TACH_ID20, read_fan_speed, true, {0, 0, 0, 0, 0, 0, 0, 0}, FAN}, //0xE4
  {"FAN10_OUTLET_SPEED", FAN_TACH_ID21, read_fan_speed, true, {0, 0, 0, 0, 0, 0, 0, 0}, FAN}, //0xE5
  {"FAN11_INLET_SPEED",  FAN_TACH_ID22, read_fan_speed, true, {0, 0, 0, 0, 0, 0, 0, 0}, FAN}, //0xE6
  {"FAN11_OUTLET_SPEED", FAN_TACH_ID23, read_fan_speed, true, {0, 0, 0, 0, 0, 0, 0, 0}, FAN}, //0xE7
  {"FAN12_INLET_SPEED",  FAN_TACH_ID24, read_fan_speed, true, {0, 0, 0, 0, 0, 0, 0, 0}, FAN}, //0xE8
  {"FAN12_OUTLET_SPEED", FAN_TACH_ID25, read_fan_speed, true, {0, 0, 0, 0, 0, 0, 0, 0}, FAN}, //0xE9
  {"FAN13_INLET_SPEED",  FAN_TACH_ID26, read_fan_speed, true, {0, 0, 0, 0, 0, 0, 0, 0}, FAN}, //0xEA
  {"FAN13_OUTLET_SPEED", FAN_TACH_ID27, read_fan_speed, true, {0, 0, 0, 0, 0, 0, 0, 0}, FAN}, //0xEB
  {"FAN14_INLET_SPEED",  FAN_TACH_ID28, read_fan_speed, true, {0, 0, 0, 0, 0, 0, 0, 0}, FAN}, //0xEC
  {"FAN14_OUTLET_SPEED", FAN_TACH_ID29, read_fan_speed, true, {0, 0, 0, 0, 0, 0, 0, 0}, FAN}, //0xED
  {"FAN15_INLET_SPEED",  FAN_TACH_ID30, read_fan_speed, true, {0, 0, 0, 0, 0, 0, 0, 0}, FAN}, //0xEE
  {"FAN15_OUTLET_SPEED", FAN_TACH_ID31, read_fan_speed, true, {0, 0, 0, 0, 0, 0, 0, 0}, FAN}, //0xEF

  {NULL, 0, NULL, 0, {0, 0, 0, 0, 0, 0, 0, 0}, 0}, //0xF0
  {NULL, 0, NULL, 0, {0, 0, 0, 0, 0, 0, 0, 0}, 0}, //0xF1
  {NULL, 0, NULL, 0, {0, 0, 0, 0, 0, 0, 0, 0}, 0}, //0xF2
  {NULL, 0, NULL, 0, {0, 0, 0, 0, 0, 0, 0, 0}, 0}, //0xF3
  {NULL, 0, NULL, 0, {0, 0, 0, 0, 0, 0, 0, 0}, 0}, //0xF4
  {NULL, 0, NULL, 0, {0, 0, 0, 0, 0, 0, 0, 0}, 0}, //0xF5
  {NULL, 0, NULL, 0, {0, 0, 0, 0, 0, 0, 0, 0}, 0}, //0xF6
  {NULL, 0, NULL, 0, {0, 0, 0, 0, 0, 0, 0, 0}, 0}, //0xF7
  {"DPM_BMC_P12V_VOUT", DPM_4, read_dpm_vout, false, {0, 0, 0, 0, 0, 0, 0, 0}, VOLT}, //0xF8
  {NULL, 0, NULL, 0, {0, 0, 0, 0, 0, 0, 0, 0}, 0}, //0xF9
  {NULL, 0, NULL, 0, {0, 0, 0, 0, 0, 0, 0, 0}, 0}, //0xFA
  {"BMC_TEMP", TEMP_BMC, read_temp, true, {0, 0, 0, 0, 0, 0, 0, 0}, TEMP}, //0xFB
  {NULL, 0, NULL, 0, {0, 0, 0, 0, 0, 0, 0, 0}, 0}, //0xFC
  {NULL, 0, NULL, 0, {0, 0, 0, 0, 0, 0, 0, 0}, 0}, //0xFD
  {NULL, 0, NULL, 0, {0, 0, 0, 0, 0, 0, 0, 0}, 0}, //0xFE
  {NULL, 0, NULL, 0, {0, 0, 0, 0, 0, 0, 0, 0}, 0}, //0xFF
};

size_t mb_sensor_cnt = sizeof(mb_sensor_list)/sizeof(uint8_t);
size_t nic0_sensor_cnt = sizeof(nic0_sensor_list)/sizeof(uint8_t);
size_t nic1_sensor_cnt = sizeof(nic1_sensor_list)/sizeof(uint8_t);
size_t mb_discrete_sensor_cnt = sizeof(mb_discrete_sensor_list)/sizeof(uint8_t);
size_t pdbv_sensor_cnt = sizeof(pdbv_sensor_list)/sizeof(uint8_t);
size_t pdbh_sensor_cnt = sizeof(pdbh_sensor_list)/sizeof(uint8_t);
size_t bp0_sensor_cnt = sizeof(bp0_sensor_list)/sizeof(uint8_t);
size_t bp1_sensor_cnt = sizeof(bp1_sensor_list)/sizeof(uint8_t);
size_t scm_sensor_cnt = sizeof(scm_sensor_list)/sizeof(uint8_t);


int
pal_get_fru_sensor_list(uint8_t fru, uint8_t **sensor_list, int *cnt) {
  int ret=0;

  if (fru == FRU_MB) {
    *sensor_list = (uint8_t *) mb_sensor_list;
    *cnt = mb_sensor_cnt;
  } else if (fru == FRU_NIC0) {
    *sensor_list = (uint8_t *) nic0_sensor_list;
    *cnt = nic0_sensor_cnt;
  } else if (fru == FRU_NIC1) {
    *sensor_list = (uint8_t *) nic1_sensor_list;
    *cnt = nic1_sensor_cnt;
  } else if (fru == FRU_PDBV) {
    *sensor_list = (uint8_t *) pdbv_sensor_list;
    *cnt = pdbv_sensor_cnt;
  } else if (fru == FRU_PDBH) {
    *sensor_list = (uint8_t *) pdbh_sensor_list;
    *cnt = pdbh_sensor_cnt;
  } else if (fru == FRU_BP0) {
    *sensor_list = (uint8_t *) bp0_sensor_list;
    *cnt = bp0_sensor_cnt;
  } else if (fru == FRU_BP1) {
    *sensor_list = (uint8_t *) bp1_sensor_list;
    *cnt = bp1_sensor_cnt;
  } else if (fru == FRU_SCM) {
    *sensor_list = (uint8_t *) scm_sensor_list;
    *cnt = scm_sensor_cnt;
  } else if (fru > MAX_NUM_FRUS) {
    return -1;
  } else {
    *sensor_list = NULL;
    *cnt = 0;
  }
  return ret;
}

static int retry_err_handle(uint8_t retry_curr, uint8_t retry_max) {

  if( retry_curr <= retry_max) {
    return READING_SKIP;
  }
  return READING_NA;
}

static int retry_skip_handle(uint8_t retry_curr, uint8_t retry_max) {

  if( retry_curr <= retry_max) {
    return READING_SKIP;
  }
  return 0;
}

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
pal_get_fru_discrete_list(uint8_t fru, uint8_t **sensor_list, int *cnt) {
  if (fru == FRU_MB) {
    *sensor_list = (uint8_t *) mb_discrete_sensor_list;
    *cnt = mb_discrete_sensor_cnt;
  } else if (fru > MAX_NUM_FRUS) {
      return -1;
  } else {
    // Nothing to read yet.
    *sensor_list = NULL;
    *cnt = 0;
  }
  return 0;
}

/*==========================================
Read adc voltage sensor value.
  adc_id: ASPEED adc channel
  *value: real voltage
  return: error code
============================================*/
static int
read_adc_val(uint8_t sensor_num, float *value) {
  uint8_t adc_id = sensor_map[sensor_num].id;
  if (adc_id >= ADC_NUM_CNT) {
    return -1;
  }
  return sensors_read_adc(sensor_map[sensor_num].snr_name, value);
}

static int
read_bat_val(uint8_t sensor_num, float *value) {
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
  ret = read_adc_val(sensor_num, value);
  if (gpio_set_value(gp_batt, GPIO_VALUE_LOW)) {
    goto bail;
  }

bail:
  gpio_close(gp_batt);
  return ret;
}

static int
read_cpu_tjmax(uint8_t sensor_num, float *value) {
  uint8_t cpu_id = sensor_map[sensor_num].id;

  return lib_get_cpu_tjmax(cpu_id, value);
}

static int
read_cpu_thermal_margin(uint8_t sensor_num, float *value) {
  uint8_t cpu_id = sensor_map[sensor_num].id;

  return lib_get_cpu_thermal_margin(cpu_id, value);
}

static int
read_cpu_pkg_pwr(uint8_t sensor_num, float *value) {
  uint8_t cpu_id = sensor_map[sensor_num].id;
  uint8_t cpu_addr = cpu_info_list[cpu_id].cpu_addr;

  return lib_get_cpu_pkg_pwr(cpu_id, cpu_addr, value);
}

static int
read_cpu_temp(uint8_t sensor_num, float *value) {
  uint8_t cpu_id = sensor_map[sensor_num].id;
  uint8_t cpu_addr = cpu_info_list[cpu_id].cpu_addr;

  return lib_get_cpu_temp(cpu_id, cpu_addr, value);
}

static int
read_cpu0_dimm_temp(uint8_t sensor_num, float *value) {
  int ret;
  uint8_t dimm_id = sensor_map[sensor_num].id;
  static uint8_t retry[DIMM_CNT] = {0};

  if(pal_bios_completed(FRU_MB) != true) {
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
read_cpu1_dimm_temp(uint8_t sensor_num, float *value) {
  int ret;
  uint8_t dimm_id = sensor_map[sensor_num].id;
  static uint8_t retry[DIMM_CNT] = {0};

  if(pal_bios_completed(FRU_MB) != true) {
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
get_nm_rw_info(uint8_t nm_id, uint8_t* nm_bus, uint8_t* nm_addr, uint16_t* bmc_addr) {
  *nm_bus = nm_info_list[nm_id].bus;
  *nm_addr = nm_info_list[nm_id].slv_addr;
  *bmc_addr = BMC_DEF_SLAVE_ADDR;
  return 0;
}

//Sensor HSC
static int
get_hsc_reading(uint8_t hsc_id, uint8_t reading_type, uint8_t type, uint8_t cmd, float *value) {
  uint8_t hsc_bus = hsc_info_list[hsc_id].bus;
  uint8_t addr = hsc_info_list[hsc_id].addr;
  int fd = -1;
  uint8_t rbuf[255] = {0x00};
  uint8_t rlen = 0;
  uint8_t tlen = 1;
  int ret = -1;

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

/*
static int
set_hsc_iout_warn_limit(uint8_t hsc_id, float value) {
  int ret=0;
  float m, b, r;
  uint16_t data;

  get_hsc_info(hsc_id, HSC_CURRENT, &m, &b, &r);

  data =(uint16_t) ((value * m + b)/r);
  ret = set_NM_hsc_word_data(hsc_id, PMBUS_IOUT_OC_WARN_LIMIT, data);
  if (ret != 0) {
    return ret;
  }

  return 0;
}
*/

static int
read_hsc_vin(uint8_t sensor_num, float *value) {
  uint8_t hsc_id = sensor_map[sensor_num].id;

  if ( get_hsc_reading(hsc_id, I2C_WORD, HSC_VOLTAGE, PMBUS_READ_VIN, value) < 0 ) {
    return READING_NA;
  }
  return 0;
}

static int
read_hsc_iout(uint8_t sensor_num, float *value) {
  uint8_t hsc_id = sensor_map[sensor_num].id;

  if ( get_hsc_reading(hsc_id, I2C_WORD, HSC_CURRENT, PMBUS_READ_IOUT, value) < 0 ) {
    return READING_NA;
  }
  return 0;
}

static int
read_hsc_pin(uint8_t sensor_num, float *value) {
  uint8_t hsc_id = sensor_map[sensor_num].id;

  if ( get_hsc_reading(hsc_id, I2C_WORD, HSC_POWER, PMBUS_READ_PIN, value) < 0 ) {
    return READING_NA;
  }
  return 0;
}

static int
read_hsc_temp(uint8_t sensor_num, float *value) {
  uint8_t hsc_id = sensor_map[sensor_num].id;

  if ( get_hsc_reading(hsc_id, I2C_WORD, HSC_TEMP, PMBUS_READ_TEMPERATURE_1, value) < 0 ) {
    return READING_NA;
  }
  return 0;
}

static int
read_hsc_peak_pin(uint8_t sensor_num, float *value) {
  uint8_t hsc_id = sensor_map[sensor_num].id;

  if ( get_hsc_reading(hsc_id, I2C_WORD, HSC_POWER, HSC_PEAK_PIN, value) < 0 ) {
    return READING_NA;
  }
  return 0;
}

static int
read_NM_pch_temp(uint8_t sensor_num, float *value) {
  uint8_t rlen = 0;
  uint8_t rbuf[32] = {0x00};
  uint8_t nm_id = sensor_map[sensor_num].id;
  int ret = 0;
  static uint8_t retry = 0;
  NM_RW_INFO info;

  get_nm_rw_info(nm_id, &info.bus, &info.nm_addr, &info.bmc_addr);
  ret = cmd_NM_sensor_reading(info, NM_PCH_TEMP, rbuf, &rlen);
  if (ret) {
    retry++;
    return retry_err_handle(retry, 5);
  }

  *value = (float) rbuf[7];
  retry = 0;
  return ret;
}

/*==========================================
Read temperature sensor TMP421 value.
Interface: snr_id: temperature id
           *value: real temperature value
           return: error code
============================================*/
static int
read_temp (uint8_t sensor_num, float *value) {
  int ret;
  uint8_t snr_id = sensor_map[sensor_num].id;

  char *devs[] = {
    "stlm75-i2c-21-48",
    "stlm75-i2c-22-48",
    "stlm75-i2c-23-48",
    "stlm75-i2c-24-48",
    "tmp75-i2c-1-4b",
    "tmp421-i2c-13-1f",
    "tmp421-i2c-4-1f",
  };

  if (snr_id >= ARRAY_SIZE(devs)) {
    return -1;
  }

  ret = sensors_read(devs[snr_id], sensor_map[sensor_num].snr_name, value);
  return ret;
}

static int
read_48v_hsc_vin(uint8_t sensor_num, float *value) {
  int ret;
  uint8_t hsc_48v_id = sensor_map[sensor_num].id;
  static int retry[HSC_48V_CNT];


  if (hsc_48v_id >= HSC_48V_CNT)
    return -1;

  ret = sensors_read(hsc_48v_chips[hsc_48v_id], sensor_map[sensor_num].snr_name, value);
  if (*value == 0) {
    retry[hsc_48v_id]++;
    return retry_err_handle(retry[hsc_48v_id], 5);
  }

  retry[hsc_48v_id] = 0;
  return ret;
}

static int
read_48v_hsc_iout(uint8_t sensor_num, float *value) {
  int ret;
  uint8_t hsc_48v_id = sensor_map[sensor_num].id;
  static int retry[HSC_48V_CNT];


  if (hsc_48v_id >= HSC_48V_CNT)
    return -1;

  ret = sensors_read(hsc_48v_chips[hsc_48v_id], sensor_map[sensor_num].snr_name, value);
  if (*value == 0) {
    retry[hsc_48v_id]++;
    return retry_err_handle(retry[hsc_48v_id], 5);
  }

  retry[hsc_48v_id] = 0;
  return ret;
}

static int
read_48v_hsc_pin(uint8_t sensor_num, float *value) {
  int ret;
  uint8_t hsc_48v_id = sensor_map[sensor_num].id;
  static int retry[HSC_48V_CNT];


  if (hsc_48v_id >= HSC_48V_CNT)
    return -1;

  ret = sensors_read(hsc_48v_chips[hsc_48v_id], sensor_map[sensor_num].snr_name, value);
  if (*value == 0) {
    retry[hsc_48v_id]++;
    return retry_err_handle(retry[hsc_48v_id], 5);
  }

  retry[hsc_48v_id] = 0;
  return ret;
}

static int
read_48v_hsc_temp(uint8_t sensor_num, float *value) {
  int ret;
  uint8_t hsc_48v_id = sensor_map[sensor_num].id;
  static int retry[HSC_48V_CNT];


  if (hsc_48v_id >= HSC_48V_CNT)
    return -1;

  ret = sensors_read(hsc_48v_chips[hsc_48v_id], sensor_map[sensor_num].snr_name, value);
  if (*value == 0) {
    retry[hsc_48v_id]++;
    return retry_err_handle(retry[hsc_48v_id], 5);
  }

  retry[hsc_48v_id] = 0;
  return ret;
}

static int
read_48v_hsc_peak_pin(uint8_t sensor_num, float *value) {
  int ret;
  uint8_t hsc_48v_id = sensor_map[sensor_num].id;
  static int retry[HSC_48V_CNT];


  if (hsc_48v_id >= HSC_48V_CNT)
    return -1;

  ret = sensors_read(hsc_48v_chips[hsc_48v_id], sensor_map[sensor_num].snr_name, value);
  if (*value == 0) {
    retry[hsc_48v_id]++;
    return retry_err_handle(retry[hsc_48v_id], 5);
  }

  retry[hsc_48v_id] = 0;
  return ret;
}

static int
read_brick_vout(uint8_t sensor_num, float *value) {
  int ret;
  uint8_t brick_id = sensor_map[sensor_num].id;
  static int retry[BRICK_CNT];


  if (brick_id >= BRICK_CNT)
    return -1;

  ret = sensors_read(brick_pmbus_chips[brick_id], sensor_map[sensor_num].snr_name, value);
  if (*value == 0) {
    retry[brick_id]++;
    return retry_err_handle(retry[brick_id], 5);
  }

  retry[brick_id] = 0;
  return ret;
}

static int
read_brick_iout(uint8_t sensor_num, float *value) {
  int ret;
  uint8_t brick_id = sensor_map[sensor_num].id;
  static int retry[BRICK_CNT];


  if (brick_id >= BRICK_CNT)
    return -1;

  ret = sensors_read(brick_pmbus_chips[brick_id], sensor_map[sensor_num].snr_name, value);
  if (*value == 0) {
    retry[brick_id]++;
    return retry_err_handle(retry[brick_id], 5);
  }

  retry[brick_id] = 0;
  return ret;
}

static int
read_brick_temp(uint8_t sensor_num, float *value) {
  int ret;
  uint8_t brick_id = sensor_map[sensor_num].id;
  static int retry[BRICK_CNT];


  if (brick_id >= BRICK_CNT)
    return -1;

  ret = sensors_read(brick_pmbus_chips[brick_id], sensor_map[sensor_num].snr_name, value);
  if (*value == 0) {
    retry[brick_id]++;
    return retry_err_handle(retry[brick_id], 5);
  }

  retry[brick_id] = 0;
  return ret;
}

static int
read_vr_vout(uint8_t sensor_num, float *value) {
  int ret = 0;
  uint8_t vr_id = sensor_map[sensor_num].id;
  static uint8_t retry[VR_NUM_CNT] = {0};


  if (pal_is_fw_update_ongoing(FRU_MB)) {
    return READING_SKIP;
  }

  if (vr_id >= VR_NUM_CNT)
    return -1;

  ret = sensors_read(vr_chips[vr_id], sensor_map[sensor_num].snr_name, value);
  if (*value == 0) {
    retry[vr_id]++;
    return retry_err_handle(retry[vr_id], 5);
  }

  retry[vr_id] = 0;
  return ret;
}


static int
read_dpm_vout(uint8_t sensor_num, float *value) {
  int fd = 0, ret = -1;
  char fn[32];
  uint8_t tlen, rlen, addr, bus;
  float scale;
  uint8_t tbuf[16] = {0};
  uint8_t rbuf[16] = {0};
  uint8_t dpm_id = sensor_map[sensor_num].id;
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
read_vr_temp(uint8_t sensor_num, float *value) {
  int ret = 0;
  uint8_t vr_id = sensor_map[sensor_num].id;
  static uint8_t retry[VR_NUM_CNT] = {0};

  if (vr_id >= VR_NUM_CNT) {
    return -1;
  }

  ret = sensors_read(vr_chips[vr_id], sensor_map[sensor_num].snr_name, value);
  if (*value == 0) {
    retry[vr_id]++;
    return retry_err_handle(retry[vr_id], 5);
  }

  retry[vr_id] = 0;
  return ret;
}

static int
read_vr_iout(uint8_t sensor_num, float *value) {
  uint8_t vr_id = sensor_map[sensor_num].id;


  if (pal_is_fw_update_ongoing(FRU_MB)) {
    return READING_SKIP;
  }

  if (vr_id >= VR_NUM_CNT) {
    return -1;
  }
  return sensors_read(vr_chips[vr_id], sensor_map[sensor_num].snr_name, value);
}

static int
read_vr_pout(uint8_t sensor_num, float *value) {
  uint8_t vr_id = sensor_map[sensor_num].id;


  if (pal_is_fw_update_ongoing(FRU_MB)) {
    return READING_SKIP;
  }

  if (vr_id >= VR_NUM_CNT) {
    return -1;
  }
  return sensors_read(vr_chips[vr_id], sensor_map[sensor_num].snr_name, value);
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

  if (retry == MAX_READ_RETRY) {
    pal_get_sensor_name(fru_id, sensor_num, sensor_name);
    snprintf(error, sizeof(error), "FRB3 failure");
    _print_sensor_discrete_log(fru_id, sensor_num, sensor_name, frb3_fail, error);
  }

  *value = (float)frb3_fail;
  return 0;
}

static
int read_frb3(uint8_t sensor_num, float *value) {
  int ret = 0;
  uint8_t fru_id = sensor_map[sensor_num].id;

#ifdef DEBUG
  syslog(LOG_INFO, "%s\n", __func__);
#endif
  ret = check_frb3(fru_id, MB_SNR_PROCESSOR_FAIL, value);
  return ret;
}

int
pal_sensor_read_raw(uint8_t fru, uint8_t sensor_num, void *value) {
  char key[MAX_KEY_LEN] = {0};
  char str[MAX_VALUE_LEN] = {0};
  char fru_name[32];
  int ret=0;
  bool server_off;
  static uint8_t retry[256] = {0};

  pal_get_fru_name(fru, fru_name);
  sprintf(key, "%s_sensor%d", fru_name, sensor_num);
  server_off = is_server_off();
  if (fru == FRU_MB || fru == FRU_SCM ||
      fru == FRU_NIC0 || fru == FRU_NIC1 ||
      fru == FRU_PDBV || fru == FRU_PDBH ||
      fru == FRU_BP0 || fru == FRU_BP1) {
    if (server_off) {
      if (sensor_map[sensor_num].stby_read == true) {
        ret = sensor_map[sensor_num].read_sensor(sensor_num, (float*) value);
      } else {
        ret = READING_NA;
      }
    } else {
      ret = sensor_map[sensor_num].read_sensor(sensor_num, (float*) value);
    }

    if ( ret == 0 ) {
      if( (sensor_map[sensor_num].snr_thresh.ucr_thresh <= *(float*)value) &&
          (sensor_map[sensor_num].snr_thresh.ucr_thresh != 0) ) {
        ret = retry_skip_handle(retry[sensor_num], SENSOR_SKIP_MAX);
        if ( ret == READING_SKIP ) {
          retry[sensor_num]++;
#ifdef DEBUG
          syslog(LOG_CRIT,"sensor retry=%d touch ucr thres=%f snrnum=0x%x value=%f\n",
                 retry[sensor_num],
                 sensor_map[sensor_num].snr_thresh.ucr_thresh,
                 sensor_num,
                  *(float*)value );
#endif
        }

      } else if( (sensor_map[sensor_num].snr_thresh.lcr_thresh >= *(float*)value) &&
                 (sensor_map[sensor_num].snr_thresh.lcr_thresh != 0) ) {
        ret = retry_skip_handle(retry[sensor_num], SENSOR_SKIP_MAX);
        if ( ret == READING_SKIP ) {
          retry[sensor_num]++;
#ifdef DEBUG
          syslog(LOG_CRIT,"sensor retry=%d touch lcr thres=%f snrnum=0x%x value=%f\n",
                 retry[sensor_num],
                 sensor_map[sensor_num].snr_thresh.lcr_thresh,
                 sensor_num,
                 *(float*)value );
#endif
        }
      } else {
        retry[sensor_num] = 0;
      }
    }
  } else {
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
  char fru_name[32];

  if (fru == FRU_MB || fru == FRU_SCM ||
      fru == FRU_NIC0 || fru == FRU_NIC1 ||
      fru == FRU_PDBV || fru == FRU_PDBH ||
      fru == FRU_BP0 || fru == FRU_BP1) {

    pal_get_fru_name(fru, fru_name);
    if (fru_name != NULL)
      for (int i = 0; i < strlen(fru_name); i++)
        fru_name[i] = toupper(fru_name[i]);

    sprintf(name, "%s_%s_%s", PLATFORM_NAME, fru_name, sensor_map[sensor_num].snr_name);
  } else {
    return -1;
  }
  return 0;
}

int
pal_get_sensor_threshold(uint8_t fru, uint8_t sensor_num, uint8_t thresh, void *value) {
  float *val = (float*) value;

  if (fru != FRU_MB && fru != FRU_SCM &&
      fru != FRU_NIC0 && fru != FRU_NIC1 &&
      fru != FRU_PDBV && fru != FRU_PDBH &&
      fru != FRU_BP0 && fru != FRU_BP1) {
    syslog(LOG_WARNING, "Threshold type error value=%d\n", thresh);
    return -1;
  }

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
      return -1;
  }
  return 0;
}

int
pal_get_sensor_units(uint8_t fru, uint8_t sensor_num, char *units) {
  uint8_t scale = sensor_map[sensor_num].units;

  if (fru != FRU_MB && fru != FRU_SCM &&
      fru != FRU_NIC0 && fru != FRU_NIC1 &&
      fru != FRU_PDBV && fru != FRU_PDBH &&
      fru != FRU_BP0 && fru != FRU_BP1) {
    return -1;
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
      return -1;
  }
  return 0;
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

static int
read_fan_speed(uint8_t sensor_num, float *value) {
  int ret = 0;
  uint8_t tach_id = sensor_map[sensor_num].id;
  static uint8_t retry[FAN_TACH_CNT] = {0};

  if (tach_id >= FAN_TACH_CNT || !is_fan_present(tach_id/2))
    return -1;

  ret = sensors_read(fan_chips[tach_id%8/2], sensor_map[sensor_num].snr_name, value);
  if (*value == 0) {
    retry[tach_id]++;
    return retry_err_handle(retry[tach_id], 2);
  }

  retry[tach_id] = 0;
  return ret;
}

//MAX31790 Controller
int
pal_get_fan_name(uint8_t num, char *name) {
  if (num >= pal_tach_cnt) {
    syslog(LOG_WARNING, "%s: invalid fan#:%d", __func__, num);
    return -1;
  }

  sprintf(name, "Fan %d %s", num/2, num%2==0? "In":"Out");
  return 0;
}

int
pal_set_fan_speed(uint8_t fan, uint8_t pwm) {

  int pwm_map[4] = {1, 3, 4, 6};
  char label[32] = {0};

  if (fan >= pal_pwm_cnt || !is_fan_present(fan)) {
    syslog(LOG_INFO, "%s: fan number is invalid - %d", __func__, fan);
    return -1;
  }

  snprintf(label, sizeof(label), "pwm%d", pwm_map[fan/4]);
  return sensors_write(fan_chips[fan%4], label, (float)pwm);
}

int
pal_get_fan_speed(uint8_t tach, int *rpm) {
  int ret=0;
  uint8_t sensor_num = FAN_SNR_START_INDEX + tach;
  float speed = 0;

  if (tach >= pal_tach_cnt || !is_fan_present(tach/2)) {
    syslog(LOG_INFO, "%s: tach number is invalid - %d", __func__, tach);
    return -1;
  }

  ret = read_fan_speed(sensor_num, &speed);
  *rpm = (int)speed;
  return ret;
}

int
pal_get_pwm_value(uint8_t tach, uint8_t *value) {
  int pwm_map[4] = {1, 3, 4, 6};
  char label[32] = {0};
  uint8_t fan = tach/2;
  float pwm_val;

  if (fan >= pal_pwm_cnt || !is_fan_present(fan)) {
    syslog(LOG_INFO, "%s: fan number is invalid - %d", __func__, fan);
    return -1;
  }

  snprintf(label, sizeof(label), "pwm%d", pwm_map[fan/4]);
  sensors_read(fan_chips[fan%4], label, &pwm_val);
  *value = (uint8_t) pwm_val;

  return 0;
}

static int
gt_sensor_name(uint8_t fru, uint8_t sensor_num, char *name) {

  switch(fru) {
    case FRU_SWB:
      switch(sensor_num) {
        default:
          return -1;
      }
      break;
    default:
      return -1;
  }
  return 0;
}

int
pal_get_event_sensor_name(uint8_t fru, uint8_t *sel, char *name) {
  uint8_t snr_type = sel[10];
  uint8_t snr_num = sel[11];

  // If SNR_TYPE is OS_BOOT, sensor name is OS
  switch (snr_type) {
    case OS_BOOT:
      // OS_BOOT used by OS
      sprintf(name, "OS");
      return 0;
    default:
      if (gt_sensor_name(fru, snr_num, name) != 0) {
        break;
      }
      return 0;
  }

  // Otherwise, translate it based on snr_num
  return pal_get_x86_event_sensor_name(fru, snr_num, name);
}

int
pal_parse_sel(uint8_t fru, uint8_t *sel, char *error_log)
{
  uint8_t snr_num = sel[11];
  //uint8_t *event_data = &sel[10];
  //uint8_t *ed = &event_data[3];
  bool parsed = true;

  strcpy(error_log, "");
  switch(snr_num) {
    default:
      parsed = false;
      break;
  }

  if (parsed == true) {
    return 0;
  }

  pal_parse_sel_helper(fru, sel, error_log);
  return 0;
}

static int pal_set_peci_mux(uint8_t select) {
  gpio_desc_t *desc;
  bool ret = false;

  desc = gpio_open_by_shadow("PECI_MUX_SELECT");
  if (!desc)
    return ret;

  gpio_set_value(desc, select);
  gpio_close(desc);
  return 0;
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

void
pal_sensor_assert_handle(uint8_t fru, uint8_t snr_num, float val, uint8_t thresh) {
  char cmd[128];
  char thresh_name[10];
  uint8_t fan_id;
  uint8_t cpu_id;

  switch (thresh) {
    case UNR_THRESH:
        sprintf(thresh_name, "UNR");
      break;
    case UCR_THRESH:
        sprintf(thresh_name, "UCR");
      break;
    case UNC_THRESH:
        sprintf(thresh_name, "UNCR");
      break;
    case LNR_THRESH:
        sprintf(thresh_name, "LNR");
      break;
    case LCR_THRESH:
        sprintf(thresh_name, "LCR");
      break;
    case LNC_THRESH:
        sprintf(thresh_name, "LNCR");
      break;
    default:
      syslog(LOG_WARNING, "%s: wrong thresh enum value", __func__);
      exit(-1);
  }

  switch(snr_num) {
    case MB_SNR_CPU0_TEMP:
    case MB_SNR_CPU1_TEMP:
      cpu_id = snr_num - MB_SNR_CPU0_TEMP;
      sprintf(cmd, "P%d Temp %s %3.0fC - Assert",cpu_id, thresh_name, val);
      break;
    case MB_SNR_P12V:
    case MB_SNR_P5V:
    case MB_SNR_P3V3:
    case MB_SNR_P2V5:
    case MB_SNR_P1V8:
    case MB_SNR_PGPPA:
    case MB_SNR_P1V2:
    case MB_SNR_P3V_BAT:
    case MB_SNR_P1V0:
    case MB_SNR_HSC_VIN:
    case MB_SNR_VR_CPU0_VCCIN_VOLT:
    case MB_SNR_VR_CPU0_VCCFA_FIVRA_VOLT:
    case MB_SNR_VR_CPU0_VCCIN_FAON_VOLT:
    case MB_SNR_VR_CPU0_VCCFA_VOLT:
    case MB_SNR_VR_CPU0_VCCD_HV_VOLT:
    case MB_SNR_VR_CPU1_VCCIN_VOLT:
    case MB_SNR_VR_CPU1_VCCFA_FIVRA_VOLT:
    case MB_SNR_VR_CPU1_VCCIN_FAON_VOLT:
    case MB_SNR_VR_CPU1_VCCFA_VOLT:
    case MB_SNR_VR_CPU1_VCCD_HV_VOLT:
    case BP0_SNR_FAN0_INLET_SPEED:
    case BP0_SNR_FAN1_INLET_SPEED:
    case BP1_SNR_FAN2_INLET_SPEED:
    case BP1_SNR_FAN3_INLET_SPEED:
    case BP0_SNR_FAN4_INLET_SPEED:
    case BP0_SNR_FAN5_INLET_SPEED:
    case BP1_SNR_FAN6_INLET_SPEED:
    case BP1_SNR_FAN7_INLET_SPEED:
    case BP0_SNR_FAN8_INLET_SPEED:
    case BP0_SNR_FAN9_INLET_SPEED:
    case BP1_SNR_FAN10_INLET_SPEED:
    case BP1_SNR_FAN11_INLET_SPEED:
    case BP0_SNR_FAN12_INLET_SPEED:
    case BP0_SNR_FAN13_INLET_SPEED:
    case BP1_SNR_FAN14_INLET_SPEED:
    case BP1_SNR_FAN15_INLET_SPEED:
      fan_id = snr_num-FAN_SNR_START_INDEX;
      sprintf(cmd, "FAN%d %s %dRPM - Assert",fan_id ,thresh_name, (int)val);
      break;
    case BP0_SNR_FAN0_OUTLET_SPEED:
    case BP0_SNR_FAN1_OUTLET_SPEED:
    case BP1_SNR_FAN2_OUTLET_SPEED:
    case BP1_SNR_FAN3_OUTLET_SPEED:
    case BP0_SNR_FAN4_OUTLET_SPEED:
    case BP0_SNR_FAN5_OUTLET_SPEED:
    case BP1_SNR_FAN6_OUTLET_SPEED:
    case BP1_SNR_FAN7_OUTLET_SPEED:
    case BP0_SNR_FAN8_OUTLET_SPEED:
    case BP0_SNR_FAN9_OUTLET_SPEED:
    case BP1_SNR_FAN10_OUTLET_SPEED:
    case BP1_SNR_FAN11_OUTLET_SPEED:
    case BP0_SNR_FAN12_OUTLET_SPEED:
    case BP0_SNR_FAN13_OUTLET_SPEED:
    case BP1_SNR_FAN14_OUTLET_SPEED:
    case BP1_SNR_FAN15_OUTLET_SPEED:
      fan_id = snr_num-FAN_SNR_START_INDEX;
      sprintf(cmd, "FAN%d %s %dRPM - Assert",fan_id ,thresh_name, (int)val);
      break;
    default:
      return;
  }
  pal_add_cri_sel(cmd);
}

void
pal_sensor_deassert_handle(uint8_t fru, uint8_t snr_num, float val, uint8_t thresh) {
  char cmd[128];
  char sensor_name[32];
  char thresh_name[10];
  uint8_t fan_id;
  uint8_t cpu_id;

  switch (thresh) {
    case UNR_THRESH:
        sprintf(thresh_name, "UNR");
      break;
    case UCR_THRESH:
        sprintf(thresh_name, "UCR");
      break;
    case UNC_THRESH:
        sprintf(thresh_name, "UNCR");
      break;
    case LNR_THRESH:
        sprintf(thresh_name, "LNR");
      break;
    case LCR_THRESH:
        sprintf(thresh_name, "LCR");
      break;
    case LNC_THRESH:
        sprintf(thresh_name, "LNCR");
      break;
    default:
      syslog(LOG_WARNING, "%s: wrong thresh enum value", __func__);
      exit(-1);
  }

  switch(snr_num) {
    case MB_SNR_CPU0_TEMP:
    case MB_SNR_CPU1_TEMP:
      cpu_id = snr_num - MB_SNR_CPU0_TEMP;
      sprintf(cmd, "P%d Temp %s %3.0fC - Deassert",cpu_id, thresh_name, val);
      break;
    case MB_SNR_P12V:
    case MB_SNR_P5V:
    case MB_SNR_P3V3:
    case MB_SNR_P2V5:
    case MB_SNR_P1V8:
    case MB_SNR_PGPPA:
    case MB_SNR_P1V2:
    case MB_SNR_P3V_BAT:
    case MB_SNR_P1V0:
    case MB_SNR_HSC_VIN:
    case MB_SNR_VR_CPU0_VCCIN_VOLT:
    case MB_SNR_VR_CPU0_VCCFA_FIVRA_VOLT:
    case MB_SNR_VR_CPU0_VCCIN_FAON_VOLT:
    case MB_SNR_VR_CPU0_VCCFA_VOLT:
    case MB_SNR_VR_CPU0_VCCD_HV_VOLT:
    case MB_SNR_VR_CPU1_VCCIN_VOLT:
    case MB_SNR_VR_CPU1_VCCFA_FIVRA_VOLT:
    case MB_SNR_VR_CPU1_VCCIN_FAON_VOLT:
    case MB_SNR_VR_CPU1_VCCFA_VOLT:
    case MB_SNR_VR_CPU1_VCCD_HV_VOLT:
      pal_get_sensor_name(fru, snr_num, sensor_name);
      sprintf(cmd, "%s %s %.2fVolts - Deassert", sensor_name, thresh_name, val);
      break;
    case BP0_SNR_FAN0_INLET_SPEED:
    case BP0_SNR_FAN1_INLET_SPEED:
    case BP1_SNR_FAN2_INLET_SPEED:
    case BP1_SNR_FAN3_INLET_SPEED:
    case BP0_SNR_FAN4_INLET_SPEED:
    case BP0_SNR_FAN5_INLET_SPEED:
    case BP1_SNR_FAN6_INLET_SPEED:
    case BP1_SNR_FAN7_INLET_SPEED:
    case BP0_SNR_FAN8_INLET_SPEED:
    case BP0_SNR_FAN9_INLET_SPEED:
    case BP1_SNR_FAN10_INLET_SPEED:
    case BP1_SNR_FAN11_INLET_SPEED:
    case BP0_SNR_FAN12_INLET_SPEED:
    case BP0_SNR_FAN13_INLET_SPEED:
    case BP1_SNR_FAN14_INLET_SPEED:
    case BP1_SNR_FAN15_INLET_SPEED:
      fan_id= snr_num-FAN_SNR_START_INDEX;
      sprintf(cmd, "FAN%d %s %dRPM - Deassert",fan_id ,thresh_name, (int)val);
      break;
    case BP0_SNR_FAN0_OUTLET_SPEED:
    case BP0_SNR_FAN1_OUTLET_SPEED:
    case BP1_SNR_FAN2_OUTLET_SPEED:
    case BP1_SNR_FAN3_OUTLET_SPEED:
    case BP0_SNR_FAN4_OUTLET_SPEED:
    case BP0_SNR_FAN5_OUTLET_SPEED:
    case BP1_SNR_FAN6_OUTLET_SPEED:
    case BP1_SNR_FAN7_OUTLET_SPEED:
    case BP0_SNR_FAN8_OUTLET_SPEED:
    case BP0_SNR_FAN9_OUTLET_SPEED:
    case BP1_SNR_FAN10_OUTLET_SPEED:
    case BP1_SNR_FAN11_OUTLET_SPEED:
    case BP0_SNR_FAN12_OUTLET_SPEED:
    case BP0_SNR_FAN13_OUTLET_SPEED:
    case BP1_SNR_FAN14_OUTLET_SPEED:
    case BP1_SNR_FAN15_OUTLET_SPEED:
      fan_id = snr_num-FAN_SNR_START_INDEX;
      sprintf(cmd, "FAN%d %s %dRPM - Deassert",fan_id ,thresh_name, (int)val);
      break;
    default:
      return;
  }
  pal_add_cri_sel(cmd);

}

int pal_sensor_monitor_initial(void) {
//Initial
  syslog(LOG_DEBUG,"Sensor Initial\n");

//Config PECI Switch
  pal_set_peci_mux(PECI_MUX_SELECT_BMC);
  return 0;
}
