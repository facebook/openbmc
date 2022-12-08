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
#include <esmi_mailbox.h>
#include "pal.h"
#include "pal_common.h"
#include <openbmc/kv.h>

//#define DEBUG
#define GPIO_P3V_BAT_SCALED_EN    "BATTERY_DETECT"
#define GPIO_APML_MUX2_SEL        "FM_APML_MUX2_SEL_R"
#define FRB3_MAX_READ_RETRY       (10)
#define HSC_12V_BUS               (2)

#define IIO_DEV_DIR(device, bus, addr, index) \
  "/sys/bus/i2c/drivers/"#device"/"#bus"-00"#addr"/iio:device"#index"/%s"
#define IIO_AIN_NAME       "in_voltage%d_raw"

#define MAX11617_DIR     IIO_DEV_DIR(max1363, 20, 35, 2)

#define SCALING_FACTOR	0.25
#define GROUP_OF_DIMM_NUM  2
#define CHANNEL_OF_DIMM_NUM  6

uint8_t DIMM_SLOT_CNT = 0;
//static float InletCalibration = 0;

static int read_bat_val(uint8_t fru, uint8_t sensor_num, float *value);
static int read_mb_temp(uint8_t fru, uint8_t sensor_num, float *value);
static int read_cpu_temp(uint8_t fru, uint8_t sensor_num, float *value);
static int read_cpu_pkg_pwr(uint8_t fru, uint8_t sensor_num, float *value);
static int read_hsc_iout(uint8_t fru, uint8_t sensor_num, float *value);
static int read_hsc_vin(uint8_t fru, uint8_t sensor_num, float *value);
static int read_hsc_pin(uint8_t fru, uint8_t sensor_num, float *value);
static int read_hsc_temp(uint8_t fru, uint8_t sensor_num, float *value);
static int read_hsc_peak_pin(uint8_t fru, uint8_t sensor_num, float *value);
static int read_cpu0_dimm_temp(uint8_t fru, uint8_t sensor_num, float *value);
static int read_cpu1_dimm_temp(uint8_t fru, uint8_t sensor_num, float *value);
static int read_cpu0_dimm_power(uint8_t fru, uint8_t sensor_num, float *value);
static int read_cpu1_dimm_power(uint8_t fru, uint8_t sensor_num, float *value);
static int read_vr_vout(uint8_t fru, uint8_t sensor_num, float *value);
static int read_vr_temp(uint8_t fru, uint8_t sensor_num, float  *value);
static int read_vr_iout(uint8_t fru, uint8_t sensor_num, float  *value);
static int read_vr_pout(uint8_t fru, uint8_t sensor_num, float  *value);
static int read_frb3(uint8_t fru, uint8_t sensor_num, float *value);
static int read_e1s_power(uint8_t fru, uint8_t sensor_num, float *value);
static int read_e1s_temp(uint8_t fru, uint8_t sensor_num, float *value);
bool pal_bios_completed(uint8_t fru);
static uint8_t postcodes_last[256] = {0};
char g_has_mux[MAX_VALUE_LEN] = {0};

const uint8_t mb_sensor_list[] = {
  MB_SNR_INLET_TEMP_R,
  MB_SNR_INLET_TEMP_L,
  MB_SNR_OUTLET_TEMP_R,
  MB_SNR_OUTLET_TEMP_L,
  MB_SNR_DIMM_CPU0_A0_TEMP,
  MB_SNR_DIMM_CPU0_A1_TEMP,
  MB_SNR_DIMM_CPU0_A2_TEMP,
  MB_SNR_DIMM_CPU0_A3_TEMP,
  MB_SNR_DIMM_CPU0_A4_TEMP,
  MB_SNR_DIMM_CPU0_A5_TEMP,
  MB_SNR_DIMM_CPU0_A6_TEMP,
  MB_SNR_DIMM_CPU0_A7_TEMP,
  MB_SNR_DIMM_CPU0_A8_TEMP,
  MB_SNR_DIMM_CPU0_A9_TEMP,
  MB_SNR_DIMM_CPU0_A10_TEMP,
  MB_SNR_DIMM_CPU0_A11_TEMP,
  MB_SNR_DIMM_CPU1_B0_TEMP,
  MB_SNR_DIMM_CPU1_B1_TEMP,
  MB_SNR_DIMM_CPU1_B2_TEMP,
  MB_SNR_DIMM_CPU1_B3_TEMP,
  MB_SNR_DIMM_CPU1_B4_TEMP,
  MB_SNR_DIMM_CPU1_B5_TEMP,
  MB_SNR_DIMM_CPU1_B6_TEMP,
  MB_SNR_DIMM_CPU1_B7_TEMP,
  MB_SNR_DIMM_CPU1_B8_TEMP,
  MB_SNR_DIMM_CPU1_B9_TEMP,
  MB_SNR_DIMM_CPU1_B10_TEMP,
  MB_SNR_DIMM_CPU1_B11_TEMP,
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
};

const uint8_t hsc_sensor_list[] = {
  MB_SNR_HSC_VIN,
  MB_SNR_HSC_IOUT,
  MB_SNR_HSC_PIN,
  MB_SNR_HSC_TEMP,
};

// List of MB discrete sensors to be monitored
const uint8_t mb_discrete_sensor_list[] = {
//  MB_SENSOR_POWER_FAIL,
//  MB_SENSOR_MEMORY_LOOP_FAIL,
  MB_SNR_PROCESSOR_FAIL,
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
  {"E1S_P3V3_VOLT", DPM_0, read_dpm_vout, false, {3.465, 0, 0, 3.135, 0, 0, 0, 0}, VOLT}, //0x01
  {NULL, 0, NULL, 0, {0, 0, 0, 0, 0, 0, 0, 0}, 0}, //0x02
  {"E1S_P12V_CURR", ADC_CH7, read_iic_adc_val, false, {2.71, 0, 0, 0, 0, 0, 0, 0}, CURR}, //0x03
  {"E1S_P12V_PWR", E1S_0, read_e1s_power, false, {25.0, 0, 0, 0, 0, 0, 0, 0}, POWER}, //0x04
  {"E1S_TEMP", E1S_0, read_e1s_temp, false, {70.0, 0, 0, 10.0, 0, 0, 0, 0}, TEMP}, //0x05
  {NULL, 0, NULL, 0, {0, 0, 0, 0, 0, 0, 0, 0}, 0}, //0x06
  {NULL, 0, NULL, 0, {0, 0, 0, 0, 0, 0, 0, 0}, 0}, //0x07

  {NULL, 0, NULL, 0, {0, 0, 0, 0, 0, 0, 0, 0}, 0}, //0x08
  {NULL, 0, NULL, 0, {0, 0, 0, 0, 0, 0, 0, 0}, 0}, //0x09
  {NULL, 0, NULL, 0, {0, 0, 0, 0, 0, 0, 0, 0}, 0}, //0x0A
  {"HSC_VOLT", HSC_ID0, read_hsc_vin,  true, {13.3, 0, 0, 10.7, 0, 0, 0, 0}, VOLT}, //0x0B
  {"HSC_CURR", HSC_ID0, read_hsc_iout, true, {240.0, 0, 0, 0, 0, 0, 0, 0}, CURR}, //0x0C
  {"HSC_PWR", HSC_ID0, read_hsc_pin,  true, {2926.0, 0, 0, 0, 0, 0, 0, 0}, POWER}, //0x0D
  {"HSC_TEMP", HSC_ID0, read_hsc_temp, true, {120.0, 0, 0, 10.0, 0, 0, 0, 0}, TEMP}, //0x0E
  {"HSC_PEAK_PIN", HSC_ID0, read_hsc_peak_pin, true, {3285.0, 0, 0, 0, 0, 0, 0, 0}, POWER}, //0x0F

  {"OUTLET_L_TEMP", TEMP_OUTLET_L, read_mb_temp, true, {55.0, 0, 0, 10.0, 0, 0, 0, 0}, TEMP}, //0x10
  {"INLET_L_TEMP", TEMP_INLET_L, read_mb_temp, true, {55.0, 0, 0, 10.0, 0, 0, 0, 0}, TEMP}, //0x11
  {"OUTLET_R_TEMP", TEMP_OUTLET_R, read_mb_temp, true, {75.0, 0, 0, 10.0, 0, 0, 0, 0}, TEMP}, //0x12
  {"INLET_R_TEMP", TEMP_INLET_R, read_mb_temp, true, {75.0, 0, 0, 10.0, 0, 0, 0, 0}, TEMP}, //0x13
  {NULL, 0, NULL, 0, {0, 0, 0, 0, 0, 0, 0, 0}, 0}, //0x14
  {NULL, 0, NULL, 0, {0, 0, 0, 0, 0, 0, 0, 0}, 0}, //0x15
  {NULL, 0, NULL, 0, {0, 0, 0, 0, 0, 0, 0, 0}, 0}, //0x16
  {NULL, 0, NULL, 0, {0, 0, 0, 0, 0, 0, 0, 0}, 0}, //0x17

  {"CPU0_TEMP", CPU_ID0, read_cpu_temp, false, {90.0, 0, 0, 10.0, 0, 0, 0, 0}, TEMP}, //0x18
  {"CPU1_TEMP", CPU_ID1, read_cpu_temp, false, {90.0, 0, 0, 10.0, 0, 0, 0, 0}, TEMP}, //0x19
  {NULL, 0, NULL, 0, {0, 0, 0, 0, 0, 0, 0, 0}, 0}, //0x1A
  {NULL, 0, NULL, 0, {0, 0, 0, 0, 0, 0, 0, 0}, 0}, //0x1B
  {NULL, 0, NULL, 0, {0, 0, 0, 0, 0, 0, 0, 0}, 0}, //0x1C
  {NULL, 0, NULL, 0, {0, 0, 0, 0, 0, 0, 0, 0}, 0}, //0x1D
  {"CPU0_PKG_PWR", CPU_ID0, read_cpu_pkg_pwr, false, {420.0, 0, 0, 0.0, 0, 0, 0, 0}, POWER}, //0x1E
  {"CPU1_PKG_PWR", CPU_ID1, read_cpu_pkg_pwr, false, {420.0, 0, 0, 0.0, 0, 0, 0, 0}, POWER}, //0x1F

  {"CPU0_DIMM_A0_A1_TEMP", DIMM_CRPA, read_cpu0_dimm_temp, false, {85.0, 0, 0, 10.0, 0, 0, 0, 0}, TEMP}, //0x20
  {"CPU0_DIMM_A2_A3_TEMP", DIMM_CRPB, read_cpu0_dimm_temp, false, {85.0, 0, 0, 10.0, 0, 0, 0, 0}, TEMP}, //0x21
  {"CPU0_DIMM_A4_A5_TEMP", DIMM_CRPC, read_cpu0_dimm_temp, false, {85.0, 0, 0, 10.0, 0, 0, 0, 0}, TEMP}, //0x22
  {"CPU0_DIMM_A6_A7_TEMP", DIMM_CRPD, read_cpu0_dimm_temp, false, {85.0, 0, 0, 10.0, 0, 0, 0, 0}, TEMP}, //0x23
  {"CPU0_DIMM_A8_A9_TEMP", DIMM_CRPE, read_cpu0_dimm_temp, false, {85.0, 0, 0, 10.0, 0, 0, 0, 0}, TEMP}, //0x24
  {"CPU0_DIMM_A10_A11_TEMP", DIMM_CRPF, read_cpu0_dimm_temp, false, {85.0, 0, 0, 10.0, 0, 0, 0, 0}, TEMP}, //0x25
  {NULL, 0, NULL, 0, {0, 0, 0, 0, 0, 0, 0, 0}, 0}, //0x26
  {NULL, 0, NULL, 0, {0, 0, 0, 0, 0, 0, 0, 0}, 0}, //0x27

  {"CPU1_DIMM_B0_B1_TEMP", DIMM_CRPA, read_cpu1_dimm_temp, false, {85.0, 0, 0, 10.0, 0, 0, 0, 0}, TEMP}, //0x28
  {"CPU1_DIMM_B2_B3_TEMP", DIMM_CRPB, read_cpu1_dimm_temp, false, {85.0, 0, 0, 10.0, 0, 0, 0, 0}, TEMP}, //0x29
  {"CPU1_DIMM_B4_B5_TEMP", DIMM_CRPC, read_cpu1_dimm_temp, false, {85.0, 0, 0, 10.0, 0, 0, 0, 0}, TEMP}, //0x2A
  {"CPU1_DIMM_B6_B7_TEMP", DIMM_CRPD, read_cpu1_dimm_temp, false, {85.0, 0, 0, 10.0, 0, 0, 0, 0}, TEMP}, //0x2B
  {"CPU1_DIMM_B8_B9_TEMP", DIMM_CRPE, read_cpu1_dimm_temp, false, {85.0, 0, 0, 10.0, 0, 0, 0, 0}, TEMP}, //0x2C
  {"CPU1_DIMM_B10_B11_TEMP", DIMM_CRPF, read_cpu1_dimm_temp, false, {85.0, 0, 0, 10.0, 0, 0, 0, 0}, TEMP}, //0x2D
  {NULL, 0, NULL, 0, {0, 0, 0, 0, 0, 0, 0, 0}, 0}, //0x2E
  {NULL, 0, NULL, 0, {0, 0, 0, 0, 0, 0, 0, 0}, 0}, //0x2F

  {"VR_CPU0_VCORE0_VOLT", VR_ID0, read_vr_vout, false, {1.71, 0, 0, 0.34, 0, 0, 0, 0}, VOLT}, //0x30
  {"VR_CPU0_VCORE0_TEMP", VR_ID0, read_vr_temp, false, {105.0, 0, 0, 10.0, 0, 0, 0, 0}, TEMP}, //0x31
  {"VR_CPU0_VCORE0_CURR", VR_ID0, read_vr_iout, false, {240, 0, 0, 0, 0, 0, 0, 0}, CURR}, //0X32
  {"VR_CPU0_VCORE0_PWR",  VR_ID0, read_vr_pout, false, {410, 0, 0, 0, 0, 0, 0, 0}, POWER}, //0x33
  {"VR_CPU0_SOC_VOLT", VR_ID1, read_vr_vout, false, {1.36, 0, 0, 0.59, 0, 0, 0, 0}, VOLT}, //0x34
  {"VR_CPU0_SOC_TEMP", VR_ID1, read_vr_temp, false, {105.0, 0, 0, 10.0, 0, 0, 0, 0}, TEMP}, //0x35
  {"VR_CPU0_SOC_CURR", VR_ID1, read_vr_iout, false, {140, 0, 0, 0, 0, 0, 0, 0}, CURR}, //0X36
  {"VR_CPU0_SOC_PWR",  VR_ID1, read_vr_pout, false, {192, 0, 0, 0, 0, 0, 0, 0}, POWER}, //0x37
  {"VR_CPU0_VCORE1_VOLT",  VR_ID2, read_vr_vout, false, {1.71, 0, 0, 0.34, 0, 0, 0, 0}, VOLT}, //0x38
  {"VR_CPU0_VCORE1_TEMP",  VR_ID2, read_vr_temp, false, {105.0, 0, 0, 10.0, 0, 0, 0, 0}, TEMP}, //0x39
  {"VR_CPU0_VCORE1_CURR",  VR_ID2, read_vr_iout, false, {240, 0, 0, 0, 0, 0, 0, 0}, CURR}, //0X3A
  {"VR_CPU0_VCORE1_PWR",   VR_ID2, read_vr_pout, false, {410.0, 0, 0, 0, 0, 0, 0, 0}, POWER}, //0x3B
  {"VR_CPU0_PVDDIO_VOLT", VR_ID3, read_vr_vout, false, {1.29, 0, 0, 0.81, 0, 0, 0, 0}, VOLT}, //0x3C
  {"VR_CPU0_PVDDIO_TEMP", VR_ID3, read_vr_temp, false, {105.0, 0, 0, 10.0, 0, 0, 0, 0}, TEMP}, //0x3D
  {"VR_CPU0_PVDDIO_CURR", VR_ID3, read_vr_iout, false, {150, 0, 0, 0, 0, 0, 0, 0}, CURR}, //0X3E
  {"VR_CPU0_PVDDIO_PWR",  VR_ID3, read_vr_pout, false, {195, 0, 0, 0, 0, 0, 0, 0}, POWER}, //0x3F
  {"VR_CPU0_PVDD11_VOLT", VR_ID4, read_vr_vout, false, {1.17, 0, 0, 1.04, 0, 0, 0, 0}, VOLT}, //0x40
  {"VR_CPU0_PVDD11_TEMP", VR_ID4, read_vr_temp, false, {105.0, 0, 0, 10.0, 0, 0, 0, 0}, TEMP}, //0x41
  {"VR_CPU0_PVDD11_CURR", VR_ID4, read_vr_iout, false, {85, 0, 0, 0, 0, 0, 0, 0}, CURR}, //0X42
  {"VR_CPU0_PVDD11_PWR", VR_ID4, read_vr_pout, false, {99, 0, 0, 0, 0, 0, 0, 0}, POWER}, //0x43
  {"P12V_AUX_IN0_VOLT", ADC_CH0, read_iic_adc_val, true, {13.2, 0, 0, 10.8, 0, 0, 0, 0}, VOLT}, //0x44
  {"P5V_IN3_VOLT",      ADC_CH3, read_iic_adc_val, false, {5.25, 0, 0, 4.75, 0, 0, 0, 0}, VOLT}, //0x45
  {"P3V3_IN4_VOLT",     ADC_CH4, read_iic_adc_val, false, {3.465, 0, 0, 3.135, 0, 0, 0, 0}, VOLT}, //0x46
  {"P3V3_AUX_IN5_VOLT", ADC_CH5, read_iic_adc_val, true, {3.465, 0, 0, 3.135, 0, 0, 0, 0}, VOLT}, //0x47

  {"VR_CPU1_VCORE0_VOLT", VR_ID5, read_vr_vout, false, {1.71, 0, 0, 0.34, 0, 0, 0, 0}, VOLT}, //0x48
  {"VR_CPU1_VCORE0_TEMP", VR_ID5, read_vr_temp, false, {105.0, 0, 0, 10.0, 0, 0, 0, 0}, TEMP}, //0x49
  {"VR_CPU1_VCORE0_CURR", VR_ID5, read_vr_iout, false, {240, 0, 0, 0, 0, 0, 0, 0}, CURR}, //0X4A
  {"VR_CPU1_VCORE0_PWR", VR_ID5, read_vr_pout, false, {410, 0, 0, 0, 0, 0, 0, 0}, POWER}, //0x4B
  {"VR_CPU1_SOC_VOLT", VR_ID6, read_vr_vout, false, {1.36, 0, 0, 0.59, 0, 0, 0, 0}, VOLT}, //0x4C
  {"VR_CPU1_SOC_TEMP", VR_ID6, read_vr_temp, false, {105.0, 0, 0, 10.0, 0, 0, 0, 0}, TEMP}, //0x4D
  {"VR_CPU1_SOC_CURR", VR_ID6, read_vr_iout, false, {140, 0, 0, 0, 0, 0, 0, 0}, CURR}, //0X4E
  {"VR_CPU1_SOC_PWR", VR_ID6, read_vr_pout, false, {192, 0, 0, 0, 0, 0, 0, 0}, POWER}, //0x4F
  {"VR_CPU1_VCORE1_VOLT", VR_ID7, read_vr_vout, false, {1.71, 0, 0, 0.34, 0, 0, 0, 0}, VOLT}, //0x50
  {"VR_CPU1_VCORE1_TEMP", VR_ID7, read_vr_temp, false, {105.0, 0, 0, 10.0, 0, 0, 0, 0}, TEMP}, //0x51
  {"VR_CPU1_VCORE1_CURR", VR_ID7, read_vr_iout, false, {240, 0, 0, 0, 0, 0, 0, 0}, CURR}, //0X52
  {"VR_CPU1_VCORE1_PWR", VR_ID7, read_vr_pout, false, {410, 0, 0, 0, 0, 0, 0, 0}, POWER}, //0x53
  {"VR_CPU1_PVDDIO_VOLT", VR_ID8, read_vr_vout, false, {1.29, 0, 0, 0.81, 0, 0, 0, 0}, VOLT}, //0x54
  {"VR_CPU1_PVDDIO_TEMP", VR_ID8, read_vr_temp, false, {105.0, 0, 0, 10.0, 0, 0, 0, 0}, TEMP}, //0x55
  {"VR_CPU1_PVDDIO_CURR", VR_ID8, read_vr_iout, false, {150, 0, 0, 0, 0, 0, 0, 0}, CURR}, //0X56
  {"VR_CPU1_PVDDIO_PWR", VR_ID8, read_vr_pout, false, {195, 0, 0, 0, 0, 0, 0, 0}, POWER}, //0x57
  {"VR_CPU1_PVDD11_VOLT", VR_ID9, read_vr_vout, false, {1.17, 0, 0, 1.04, 0, 0, 0, 0}, VOLT}, //0x58
  {"VR_CPU1_PVDD11_TEMP", VR_ID9, read_vr_temp, false, {105.0, 0, 0, 10.0, 0, 0, 0, 0}, TEMP}, //0x59
  {"VR_CPU1_PVDD11_CURR", VR_ID9, read_vr_iout, false, {85, 0, 0, 0, 0, 0, 0, 0}, CURR}, //0X5A
  {"VR_CPU1_PVDD11_PWR", VR_ID9, read_vr_pout, false, {99, 0, 0, 0, 0, 0, 0, 0}, POWER}, //0x5B
  {NULL, 0, NULL, 0, {0, 0, 0, 0, 0, 0, 0, 0}, 0}, //0x5C
  {NULL, 0, NULL, 0, {0, 0, 0, 0, 0, 0, 0, 0}, 0}, //0x5D
  {"P3V_BAT_VOLT", ADC7, read_bat_val, true, {3.4, 0, 0, 2.6, 0, 0, 0, 0}, VOLT}, //0x5E
  {NULL, 0, NULL, 0, {0, 0, 0, 0, 0, 0, 0, 0}, 0}, //0x5F

  {"CPU0_DIMM_A0_PWR", DIMM_ID0,  read_cpu0_dimm_power, false, {30.0, 0, 0, 0, 0, 0, 0, 0}, POWER}, //0x60
  {"CPU0_DIMM_A1_PWR", DIMM_ID1,  read_cpu0_dimm_power, false, {30.0, 0, 0, 0, 0, 0, 0, 0}, POWER}, //0x61
  {"CPU0_DIMM_A2_PWR", DIMM_ID2,  read_cpu0_dimm_power, false, {30.0, 0, 0, 0, 0, 0, 0, 0}, POWER}, //0x62
  {"CPU0_DIMM_A3_PWR", DIMM_ID3,  read_cpu0_dimm_power, false, {30.0, 0, 0, 0, 0, 0, 0, 0}, POWER}, //0x63
  {"CPU0_DIMM_A4_PWR", DIMM_ID4,  read_cpu0_dimm_power, false, {30.0, 0, 0, 0, 0, 0, 0, 0}, POWER}, //0x64
  {"CPU0_DIMM_A5_PWR", DIMM_ID5,  read_cpu0_dimm_power, false, {30.0, 0, 0, 0, 0, 0, 0, 0}, POWER}, //0x65
  {"CPU0_DIMM_A6_PWR", DIMM_ID6,  read_cpu0_dimm_power, false, {30.0, 0, 0, 0, 0, 0, 0, 0}, POWER}, //0x66
  {"CPU0_DIMM_A7_PWR", DIMM_ID7,  read_cpu0_dimm_power, false, {30.0, 0, 0, 0, 0, 0, 0, 0}, POWER}, //0x67
  {"CPU0_DIMM_A8_PWR", DIMM_ID8,  read_cpu0_dimm_power, false, {30.0, 0, 0, 0, 0, 0, 0, 0}, POWER}, //0x68
  {"CPU0_DIMM_A9_PWR", DIMM_ID9,  read_cpu0_dimm_power, false, {30.0, 0, 0, 0, 0, 0, 0, 0}, POWER}, //0x69
  {"CPU0_DIMM_A10_PWR", DIMM_ID10, read_cpu0_dimm_power, false, {30.0, 0, 0, 0, 0, 0, 0, 0}, POWER}, //0x6A
  {"CPU0_DIMM_A11_PWR", DIMM_ID11, read_cpu0_dimm_power, false, {30.0, 0, 0, 0, 0, 0, 0, 0}, POWER}, //0x6B
  {NULL, 0, NULL, 0, {0, 0, 0, 0, 0, 0, 0, 0}, 0}, //0x6C
  {NULL, 0, NULL, 0, {0, 0, 0, 0, 0, 0, 0, 0}, 0}, //0x6D
  {NULL, 0, NULL, 0, {0, 0, 0, 0, 0, 0, 0, 0}, 0}, //0x6E
  {NULL, 0, NULL, 0, {0, 0, 0, 0, 0, 0, 0, 0}, 0}, //0x6F

  {"CPU1_DIMM_B0_PWR", DIMM_ID0,  read_cpu1_dimm_power, false, {30.0, 0, 0, 0, 0, 0, 0, 0}, POWER}, //0x70
  {"CPU1_DIMM_B1_PWR", DIMM_ID1,  read_cpu1_dimm_power, false, {30.0, 0, 0, 0, 0, 0, 0, 0}, POWER}, //0x71
  {"CPU1_DIMM_B2_PWR", DIMM_ID2,  read_cpu1_dimm_power, false, {30.0, 0, 0, 0, 0, 0, 0, 0}, POWER}, //0x72
  {"CPU1_DIMM_B3_PWR", DIMM_ID3,  read_cpu1_dimm_power, false, {30.0, 0, 0, 0, 0, 0, 0, 0}, POWER}, //0x73
  {"CPU1_DIMM_B4_PWR", DIMM_ID4,  read_cpu1_dimm_power, false, {30.0, 0, 0, 0, 0, 0, 0, 0}, POWER}, //0x74
  {"CPU1_DIMM_B5_PWR", DIMM_ID5,  read_cpu1_dimm_power, false, {30.0, 0, 0, 0, 0, 0, 0, 0}, POWER}, //0x75
  {"CPU1_DIMM_B6_PWR", DIMM_ID6,  read_cpu1_dimm_power, false, {30.0, 0, 0, 0, 0, 0, 0, 0}, POWER}, //0x76
  {"CPU1_DIMM_B7_PWR", DIMM_ID7,  read_cpu1_dimm_power, false, {30.0, 0, 0, 0, 0, 0, 0, 0}, POWER}, //0x77
  {"CPU1_DIMM_B8_PWR", DIMM_ID8,  read_cpu1_dimm_power, false, {30.0, 0, 0, 0, 0, 0, 0, 0}, POWER}, //0x78
  {"CPU1_DIMM_B9_PWR", DIMM_ID9,  read_cpu1_dimm_power, false, {30.0, 0, 0, 0, 0, 0, 0, 0}, POWER}, //0x79
  {"CPU1_DIMM_B10_PWR", DIMM_ID10, read_cpu1_dimm_power, false, {30.0, 0, 0, 0, 0, 0, 0, 0}, POWER}, //0x7A
  {"CPU1_DIMM_B11_PWR", DIMM_ID11, read_cpu1_dimm_power, false, {30.0, 0, 0, 0, 0, 0, 0, 0}, POWER}, //0x7B
  {NULL, 0, NULL, 0, {0, 0, 0, 0, 0, 0, 0, 0}, 0}, //0x7C
  {NULL, 0, NULL, 0, {0, 0, 0, 0, 0, 0, 0, 0}, 0}, //0x7D
  {NULL, 0, NULL, 0, {0, 0, 0, 0, 0, 0, 0, 0}, 0}, //0x7E
  {NULL, 0, NULL, 0, {0, 0, 0, 0, 0, 0, 0, 0}, 0}, //0x7F

  {"CPU0_DIMM_A0_TEMP", DIMM_ID0, read_cpu0_dimm_temp, false, {85.0, 0, 0, 10.0, 0, 0, 0, 0}, TEMP}, //0x80
  {"CPU0_DIMM_A1_TEMP", DIMM_ID1, read_cpu0_dimm_temp, false, {85.0, 0, 0, 10.0, 0, 0, 0, 0}, TEMP}, //0x81
  {"CPU0_DIMM_A2_TEMP", DIMM_ID2, read_cpu0_dimm_temp, false, {85.0, 0, 0, 10.0, 0, 0, 0, 0}, TEMP}, //0x82
  {"CPU0_DIMM_A3_TEMP", DIMM_ID3, read_cpu0_dimm_temp, false, {85.0, 0, 0, 10.0, 0, 0, 0, 0}, TEMP}, //0x83
  {"CPU0_DIMM_A4_TEMP", DIMM_ID4, read_cpu0_dimm_temp, false, {85.0, 0, 0, 10.0, 0, 0, 0, 0}, TEMP}, //0x84
  {"CPU0_DIMM_A5_TEMP", DIMM_ID5, read_cpu0_dimm_temp, false, {85.0, 0, 0, 10.0, 0, 0, 0, 0}, TEMP}, //0x85
  {"CPU0_DIMM_A6_TEMP", DIMM_ID6, read_cpu0_dimm_temp, false, {85.0, 0, 0, 10.0, 0, 0, 0, 0}, TEMP}, //0x86
  {"CPU0_DIMM_A7_TEMP", DIMM_ID7, read_cpu0_dimm_temp, false, {85.0, 0, 0, 10.0, 0, 0, 0, 0}, TEMP}, //0x87
  {"CPU0_DIMM_A8_TEMP",  DIMM_ID8, read_cpu0_dimm_temp, false, {85.0, 0, 0, 10.0, 0, 0, 0, 0}, TEMP}, //0x88
  {"CPU0_DIMM_A9_TEMP",  DIMM_ID9, read_cpu0_dimm_temp, false, {85.0, 0, 0, 10.0, 0, 0, 0, 0}, TEMP}, //0x89
  {"CPU0_DIMM_A10_TEMP", DIMM_ID10, read_cpu0_dimm_temp, false, {85.0, 0, 0, 10.0, 0, 0, 0, 0}, TEMP}, //0x8A
  {"CPU0_DIMM_A11_TEMP", DIMM_ID11, read_cpu0_dimm_temp, false, {85.0, 0, 0, 10.0, 0, 0, 0, 0}, TEMP}, //0x8B
  {NULL, 0, NULL, 0, {0, 0, 0, 0, 0, 0, 0, 0}, 0}, //0x8C
  {NULL, 0, NULL, 0, {0, 0, 0, 0, 0, 0, 0, 0}, 0}, //0x8D
  {NULL, 0, NULL, 0, {0, 0, 0, 0, 0, 0, 0, 0}, 0}, //0x8E
  {NULL, 0, NULL, 0, {0, 0, 0, 0, 0, 0, 0, 0}, 0}, //0x8F

  {"CPU1_DIMM_B0_TEMP", DIMM_ID0, read_cpu1_dimm_temp, false, {85.0, 0, 0, 10.0, 0, 0, 0, 0}, TEMP}, //0x90
  {"CPU1_DIMM_B1_TEMP", DIMM_ID1, read_cpu1_dimm_temp, false, {85.0, 0, 0, 10.0, 0, 0, 0, 0}, TEMP}, //0x91
  {"CPU1_DIMM_B2_TEMP", DIMM_ID2, read_cpu1_dimm_temp, false, {85.0, 0, 0, 10.0, 0, 0, 0, 0}, TEMP}, //0x92
  {"CPU1_DIMM_B3_TEMP", DIMM_ID3, read_cpu1_dimm_temp, false, {85.0, 0, 0, 10.0, 0, 0, 0, 0}, TEMP}, //0x93
  {"CPU1_DIMM_B4_TEMP", DIMM_ID4, read_cpu1_dimm_temp, false, {85.0, 0, 0, 10.0, 0, 0, 0, 0}, TEMP}, //0x94
  {"CPU1_DIMM_B5_TEMP", DIMM_ID5, read_cpu1_dimm_temp, false, {85.0, 0, 0, 10.0, 0, 0, 0, 0}, TEMP}, //0x95
  {"CPU1_DIMM_B6_TEMP", DIMM_ID6, read_cpu1_dimm_temp, false, {85.0, 0, 0, 10.0, 0, 0, 0, 0}, TEMP}, //0x96
  {"CPU1_DIMM_B7_TEMP", DIMM_ID7, read_cpu1_dimm_temp, false, {85.0, 0, 0, 10.0, 0, 0, 0, 0}, TEMP}, //0x97
  {"CPU1_DIMM_B8_TEMP",  DIMM_ID8, read_cpu1_dimm_temp, false, {85.0, 0, 0, 10.0, 0, 0, 0, 0}, TEMP}, //0x98
  {"CPU1_DIMM_B9_TEMP",  DIMM_ID9, read_cpu1_dimm_temp, false, {85.0, 0, 0, 10.0, 0, 0, 0, 0}, TEMP}, //0x99
  {"CPU1_DIMM_B10_TEMP", DIMM_ID10, read_cpu1_dimm_temp, false, {85.0, 0, 0, 10.0, 0, 0, 0, 0}, TEMP}, //0x9A
  {"CPU1_DIMM_B11_TEMP", DIMM_ID11, read_cpu1_dimm_temp, false, {85.0, 0, 0, 10.0, 0, 0, 0, 0}, TEMP}, //0x9B
  {NULL, 0, NULL, 0, {0, 0, 0, 0, 0, 0, 0, 0}, 0}, //0x9C
  {NULL, 0, NULL, 0, {0, 0, 0, 0, 0, 0, 0, 0}, 0}, //0x9D
  {NULL, 0, NULL, 0, {0, 0, 0, 0, 0, 0, 0, 0}, 0}, //0x9E
  {NULL, 0, NULL, 0, {0, 0, 0, 0, 0, 0, 0, 0}, 0}, //0x9F

  {NULL, 0, NULL, 0, {0, 0, 0, 0, 0, 0, 0, 0}, 0}, //0xA0
  {"PROCESSOR_FAIL", FRU_MB, read_frb3, 0, {0, 0, 0, 0, 0, 0, 0, 0}, STATE}, //0xA1
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

    if (GETBIT(id, 2)) {
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

static int set_apml_channel(uint8_t cpu_id) {
  int ret = 0;

  if (gpio_get_value_by_shadow(GPIO_APML_MUX2_SEL) == GPIO_VALUE_HIGH && cpu_id == CPU_ID0) {
    ret = gpio_set_value_by_shadow(GPIO_APML_MUX2_SEL, GPIO_VALUE_LOW);
  }
  else if (gpio_get_value_by_shadow(GPIO_APML_MUX2_SEL) == GPIO_VALUE_LOW && cpu_id == CPU_ID1) {
    ret = gpio_set_value_by_shadow(GPIO_APML_MUX2_SEL, GPIO_VALUE_HIGH);
  }
  else {
    //do nothing
  }

  if (ret) {
    syslog(LOG_WARNING, "%s, set apml sgpio fail", __FUNCTION__);
  }
  else {
    sleep(1);
  }

  return ret;
}

static int
read_cpu_pkg_pwr(uint8_t fru, uint8_t sensor_num, float *value) {
  uint8_t cpu_id = sensor_map[fru].map[sensor_num].id;
  int ret;
  char* cpu_chips[] = {
    "sbrmi-i2c-0-3c",
    "sbrmi-i2c-0-38",
  };
  static uint8_t retry[ARRAY_SIZE(cpu_chips)] = {0};

  if(!is_cpu_socket_occupy(cpu_id))
    return READING_NA;

  if(pal_bios_completed(fru) != true) {
    return READING_NA;
  }

  if (!strcmp( g_has_mux, "0")) {
    set_apml_channel(cpu_id);
  }

  ret = sensors_read(cpu_chips[cpu_id], sensor_map[fru].map[sensor_num].snr_name, value);
  if (ret) {
    retry[cpu_id]++;
    return retry_err_handle(retry[cpu_id], 3);
  }

  retry[cpu_id] = 0;
  return ret;
}

static int
read_cpu_temp(uint8_t fru, uint8_t sensor_num, float *value) {
  uint8_t cpu_id = sensor_map[fru].map[sensor_num].id;
  int ret;
  char* cpu_chips[] = {
    "sbtsi-i2c-0-4c",
    "sbtsi-i2c-0-48",
  };
  static uint8_t retry[ARRAY_SIZE(cpu_chips)] = {0};

  if(pal_bios_completed(fru) != true) {
    return READING_NA;
  }

  if(!is_cpu_socket_occupy(cpu_id))
    return READING_NA;

  if (!strcmp( g_has_mux, "0")) {
    set_apml_channel(cpu_id);
  }

  ret = sensors_read(cpu_chips[cpu_id], sensor_map[fru].map[sensor_num].snr_name, value);
  if (ret) {
    retry[cpu_id]++;
    return retry_err_handle(retry[cpu_id], 3);
  }

  retry[cpu_id] = 0;
  return ret;
}

static void decode_dimm_temp(uint16_t raw, float *temp)
{
  if (raw <= 0x3FF)
    *temp = raw * SCALING_FACTOR;
  else
    *temp = (raw - 0x800) * SCALING_FACTOR;
}

static int
read_dimm_temp(uint8_t fru, uint8_t sensor_num, float *value,
                uint8_t dimm_id, uint8_t cpu_id) {
  struct dimm_thermal d_sensor;
  float temp = 0;
  oob_status_t ret;

  uint8_t addr = (dimm_id/CHANNEL_OF_DIMM_NUM << 4) + (dimm_id%CHANNEL_OF_DIMM_NUM);

  ret = read_dimm_thermal_sensor(cpu_id, addr, &d_sensor);
  if(!ret) {
    decode_dimm_temp(d_sensor.sensor, &temp);
    *value = (float)temp;
  } else {
    return -1;
  }

  return 0;
}

static int
read_cpu0_dimm_temp(uint8_t fru, uint8_t sensor_num, float *value) {
  int ret;
  uint8_t dimm_id = sensor_map[fru].map[sensor_num].id;
  static uint8_t retry[DIMM_CNT] = {0};

  if(!is_cpu_socket_occupy(CPU_ID0))
    return READING_NA;

  if(pal_bios_completed(fru) != true) {
    return READING_NA;
  }

  kv_get("apml_mux", g_has_mux, 0, 0);
  if (!strcmp( g_has_mux, "0")) {
    set_apml_channel(CPU_ID0);
  }

  ret = read_dimm_temp(fru, sensor_num, value, dimm_id, CPU_ID0);
  if ( ret != 0 ) {
    retry[dimm_id]++;
    return retry_err_handle(retry[dimm_id], 3);
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

  if(!is_cpu_socket_occupy(CPU_ID1))
    return READING_NA;

  if(pal_bios_completed(fru) != true) {
    return READING_NA;
  }

  if (!strcmp( g_has_mux, "0")) {
    set_apml_channel(CPU_ID1);
  }

  ret = read_dimm_temp(fru, sensor_num, value, dimm_id, CPU_ID1);
  if ( ret != 0 ) {
    retry[dimm_id]++;
    return retry_err_handle(retry[dimm_id], 3);
  }

#ifdef DEBUG
  syslog(LOG_DEBUG, "%s DIMM Temp=%f id=%d\n", __func__, *value, dimm_id);
#endif
  retry[dimm_id] = 0;
  return 0;
}

static int
read_dimm_power(uint8_t fru, uint8_t sensor_num, float *value,
                uint8_t dimm_id, uint8_t cpu_id, bool* cached) {
  struct dimm_power d_power;
  oob_status_t ret;

  uint8_t addr = (dimm_id/CHANNEL_OF_DIMM_NUM << 4) + (dimm_id%CHANNEL_OF_DIMM_NUM);
  ret = read_dimm_power_consumption(cpu_id, addr, &d_power);

  if(ret)
    return -1;

  *value = ((float)d_power.power)/1000;
  return 0;
}

static int
read_cpu0_dimm_power(uint8_t fru, uint8_t sensor_num, float *value) {
  int ret;
  static uint8_t retry[MAX_DIMM_NUM] = {0};
  static bool cached[MAX_DIMM_NUM] = {false};
  uint8_t dimm_id = sensor_map[fru].map[sensor_num].id;

  if(!is_cpu_socket_occupy(CPU_ID0))
    return READING_NA;

  if(pal_bios_completed(fru) != true) {
    return READING_NA;
  }

  if (!strcmp( g_has_mux, "0")) {
    set_apml_channel(CPU_ID0);
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
  static uint8_t retry[MAX_DIMM_NUM] = {0};
  static bool cached[MAX_DIMM_NUM] = {false};
  uint8_t dimm_id = sensor_map[fru].map[sensor_num].id;

  if(!is_cpu_socket_occupy(CPU_ID1))
    return READING_NA;

  if(pal_bios_completed(fru) != true) {
    return READING_NA;
  }

  if (!strcmp( g_has_mux, "0")) {
    set_apml_channel(CPU_ID1);
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
read_mb_temp (uint8_t fru, uint8_t sensor_num, float *value) {
  return sensors_read(NULL, sensor_map[fru].map[sensor_num].snr_name, value);
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

  if (value == NULL || vr_id >= VR_NUM_CNT) {
    return -1;
  }

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
  return sensors_read(NULL, sensor_map[fru].map[sensor_num].snr_name, value);
}

static int
read_vr_pout(uint8_t fru, uint8_t sensor_num, float *value) {
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

