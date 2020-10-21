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
#include "pal.h"

//#define DEBUG
#define GPIO_P3V_BAT_SCALED_EN "P3V_BAT_SCALED_EN"
#define GPIO_CPU0_PRESENT "FM_CPU0_SKTOCC_LVT3_PLD_N"
#define GPIO_CPU1_PRESENT "FM_CPU1_SKTOCC_LVT3_PLD_N"
#define MAX_READ_RETRY 10
#define POLLING_DELAY_TIME  (10)

#define PECI_MUX_SELECT_BMC                     (GPIO_VALUE_HIGH)
#define PECI_MUX_SELECT_PCH                     (GPIO_VALUE_LOW)

uint8_t pwr_polling_flag=false;
uint8_t DIMM_SLOT_CNT = 0;
size_t pal_pwm_cnt = FAN_PWM_ALL_NUM;
size_t pal_tach_cnt = FAN_TACH_ALL_NUM;
const char pal_pwm_list[] = "0,1,2,3";
const char pal_tach_list[] = "0,1,2,3,4,5,6,7";

bool pal_bios_completed(uint8_t fru);

static int read_adc_val(uint8_t adc_id, float *value);
static int read_battery_val(uint8_t adc_id, float *value);
static int read_sensor(uint8_t snr_id, float *value);
static int read_nic_temp(uint8_t nic_id, float *value);
static int read_hd_temp(uint8_t hd_id, float *value);
static int read_cpu_temp(uint8_t cpu_id, float *value);
static int read_cpu_pkg_pwr(uint8_t cpu_id, float *value);
static int read_cpu_tjmax(uint8_t cpu_id, float *value);
static int read_cpu_thermal_margin(uint8_t cpu_id, float *value);
static int read_hsc_iout(uint8_t hsc_id, float *value);
static int read_hsc_vin(uint8_t hsc_id, float *value);
static int read_hsc_pin(uint8_t hsc_id, float *value);
static int read_hsc_temp(uint8_t hsc_id, float *value);
static int read_hsc_peak_pin(uint8_t hsc_id, float *value);
static int read_cpu0_dimm_temp(uint8_t dimm_id, float *value);
static int read_cpu1_dimm_temp(uint8_t dimm_id, float *value);
static int read_cpu2_dimm_temp(uint8_t dimm_id, float *value);
static int read_cpu3_dimm_temp(uint8_t dimm_id, float *value);
static int read_cpu4_dimm_temp(uint8_t dimm_id, float *value);
static int read_cpu5_dimm_temp(uint8_t dimm_id, float *value);
static int read_cpu6_dimm_temp(uint8_t dimm_id, float *value);
static int read_cpu7_dimm_temp(uint8_t dimm_id, float *value);
static int read_NM_pch_temp(uint8_t nm_snr_id, float *value);
static int read_ina260_vol(uint8_t ina260_id, float *value);
static int read_vr_vout(uint8_t vr_id, float *value);
static int read_vr_temp(uint8_t vr_id, float  *value);
static int read_vr_iout(uint8_t vr_id, float  *value);
static int read_vr_pout(uint8_t vr_id, float  *value);
static int read_cm_sensor(uint8_t id, float *value);
static int read_frb3(uint8_t fru_id, float *value);
static int get_nm_rw_info(uint8_t nm_id, uint8_t* nm_bus, uint8_t* nm_addr, uint16_t* bmc_addr);

static uint8_t m_TjMax[CPU_ID_NUM] = {0};
static float m_Dts[CPU_ID_NUM] = {0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF};
static uint8_t postcodes_last[256] = {0};

//8S Master BMC Sensor List
const uint8_t mb_8s_m_sensor_list[] = {
  MB_SNR_INLET_TEMP,
  MB_SNR_OUTLET_TEMP_R,
  MB_SNR_OUTLET_TEMP_L,
  MB_SNR_INLET_REMOTE_TEMP,
  MB_SNR_OUTLET_REMOTE_TEMP_R,
  MB_SNR_OUTLET_REMOTE_TEMP_L,
  MB_SNR_P5V,
  MB_SNR_P5V_STBY,
  MB_SNR_P3V3_STBY,
  MB_SNR_P3V3,
  MB_SNR_P3V_BAT,
  MB_SNR_CPU_P1V8,
  MB_SNR_PCH_P1V8,
  MB_SNR_CPU0_PVPP_ABC,
  MB_SNR_CPU0_PVPP_DEF,
  MB_SNR_CPU0_PVTT_ABC,
  MB_SNR_CPU0_PVTT_DEF,
  MB_SNR_CPU1_PVPP_ABC,
  MB_SNR_CPU1_PVPP_DEF,
  MB_SNR_CPU1_PVTT_ABC,
  MB_SNR_CPU1_PVTT_DEF,
  MB_SNR_HSC_VIN,
  MB_SNR_HSC_IOUT,
  MB_SNR_HSC_PIN,
  MB_SNR_HSC_TEMP,
  MB_SNR_HSC_PEAK_PIN,
  MB1_SNR_HSC_VIN,
  MB1_SNR_HSC_IOUT,
  MB1_SNR_HSC_PIN,
  MB1_SNR_HSC_TEMP,
  MB1_SNR_HSC_PEAK_PIN,
  MB2_SNR_HSC_VIN,
  MB2_SNR_HSC_IOUT,
  MB2_SNR_HSC_PIN,
  MB2_SNR_HSC_TEMP,
  MB2_SNR_HSC_PEAK_PIN,
  MB3_SNR_HSC_VIN,
  MB3_SNR_HSC_IOUT,
  MB3_SNR_HSC_PIN,
  MB3_SNR_HSC_TEMP,
  MB3_SNR_HSC_PEAK_PIN,
  MB_SNR_PCH_TEMP,
  MB_SNR_CPU0_TEMP,
  MB_SNR_CPU1_TEMP,
  MB_SNR_CPU2_TEMP,
  MB_SNR_CPU3_TEMP,
  MB_SNR_CPU4_TEMP,
  MB_SNR_CPU5_TEMP,
  MB_SNR_CPU6_TEMP,
  MB_SNR_CPU7_TEMP,
  MB_SNR_CPU0_DIMM_GRPA_TEMP,
  MB_SNR_CPU0_DIMM_GRPB_TEMP,
  MB_SNR_CPU0_DIMM_GRPC_TEMP,
  MB_SNR_CPU0_DIMM_GRPD_TEMP,
  MB_SNR_CPU0_DIMM_GRPE_TEMP,
  MB_SNR_CPU0_DIMM_GRPF_TEMP,
  MB_SNR_CPU1_DIMM_GRPA_TEMP,
  MB_SNR_CPU1_DIMM_GRPB_TEMP,
  MB_SNR_CPU1_DIMM_GRPC_TEMP,
  MB_SNR_CPU1_DIMM_GRPD_TEMP,
  MB_SNR_CPU1_DIMM_GRPE_TEMP,
  MB_SNR_CPU1_DIMM_GRPF_TEMP,
  MB_SNR_CPU2_DIMM_GRPA_TEMP,
  MB_SNR_CPU2_DIMM_GRPB_TEMP,
  MB_SNR_CPU2_DIMM_GRPC_TEMP,
  MB_SNR_CPU2_DIMM_GRPD_TEMP,
  MB_SNR_CPU2_DIMM_GRPE_TEMP,
  MB_SNR_CPU2_DIMM_GRPF_TEMP,
  MB_SNR_CPU3_DIMM_GRPA_TEMP,
  MB_SNR_CPU3_DIMM_GRPB_TEMP,
  MB_SNR_CPU3_DIMM_GRPC_TEMP,
  MB_SNR_CPU3_DIMM_GRPD_TEMP,
  MB_SNR_CPU3_DIMM_GRPE_TEMP,
  MB_SNR_CPU3_DIMM_GRPF_TEMP,
  MB_SNR_CPU4_DIMM_GRPA_TEMP,
  MB_SNR_CPU4_DIMM_GRPB_TEMP,
  MB_SNR_CPU4_DIMM_GRPC_TEMP,
  MB_SNR_CPU4_DIMM_GRPD_TEMP,
  MB_SNR_CPU4_DIMM_GRPE_TEMP,
  MB_SNR_CPU4_DIMM_GRPF_TEMP,
  MB_SNR_CPU5_DIMM_GRPA_TEMP,
  MB_SNR_CPU5_DIMM_GRPB_TEMP,
  MB_SNR_CPU5_DIMM_GRPC_TEMP,
  MB_SNR_CPU5_DIMM_GRPD_TEMP,
  MB_SNR_CPU5_DIMM_GRPE_TEMP,
  MB_SNR_CPU5_DIMM_GRPF_TEMP,
  MB_SNR_CPU6_DIMM_GRPA_TEMP,
  MB_SNR_CPU6_DIMM_GRPB_TEMP,
  MB_SNR_CPU6_DIMM_GRPC_TEMP,
  MB_SNR_CPU6_DIMM_GRPD_TEMP,
  MB_SNR_CPU6_DIMM_GRPE_TEMP,
  MB_SNR_CPU6_DIMM_GRPF_TEMP,
  MB_SNR_CPU7_DIMM_GRPA_TEMP,
  MB_SNR_CPU7_DIMM_GRPB_TEMP,
  MB_SNR_CPU7_DIMM_GRPC_TEMP,
  MB_SNR_CPU7_DIMM_GRPD_TEMP,
  MB_SNR_CPU7_DIMM_GRPE_TEMP,
  MB_SNR_CPU7_DIMM_GRPF_TEMP,
  MB_SNR_DATA0_DRIVE_TEMP,
  MB_SNR_DATA1_DRIVE_TEMP,
  MB_SNR_CPU0_PKG_POWER,
  MB_SNR_CPU1_PKG_POWER,
  MB_SNR_CPU2_PKG_POWER,
  MB_SNR_CPU3_PKG_POWER,
  MB_SNR_CPU4_PKG_POWER,
  MB_SNR_CPU5_PKG_POWER,
  MB_SNR_CPU6_PKG_POWER,
  MB_SNR_CPU7_PKG_POWER,
  MB_SNR_CPU0_TJMAX,
  MB_SNR_CPU1_TJMAX,
  MB_SNR_CPU2_TJMAX,
  MB_SNR_CPU3_TJMAX,
  MB_SNR_CPU4_TJMAX,
  MB_SNR_CPU5_TJMAX,
  MB_SNR_CPU6_TJMAX,
  MB_SNR_CPU7_TJMAX,
  MB_SNR_CPU0_THERM_MARGIN,
  MB_SNR_CPU1_THERM_MARGIN,
  MB_SNR_CPU2_THERM_MARGIN,
  MB_SNR_CPU3_THERM_MARGIN,
  MB_SNR_CPU4_THERM_MARGIN,
  MB_SNR_CPU5_THERM_MARGIN,
  MB_SNR_CPU6_THERM_MARGIN,
  MB_SNR_CPU7_THERM_MARGIN,
  MB_SNR_P12V_STBY_INA260_VOL,
  MB_SNR_P3V3_M2_1_INA260_VOL,
  MB_SNR_P3V3_M2_2_INA260_VOL,
  MB_SNR_P3V3_M2_3_INA260_VOL,
  MB_SNR_VR_CPU0_VCCIN_VOLT,
  MB_SNR_VR_CPU0_VCCIN_TEMP,
  MB_SNR_VR_CPU0_VCCIN_CURR,
  MB_SNR_VR_CPU0_VCCIN_POWER,
  MB_SNR_VR_CPU0_VSA_VOLT,
  MB_SNR_VR_CPU0_VSA_TEMP,
  MB_SNR_VR_CPU0_VSA_CURR,
  MB_SNR_VR_CPU0_VSA_POWER,
  MB_SNR_VR_CPU0_VCCIO_VOLT,
  MB_SNR_VR_CPU0_VCCIO_TEMP,
  MB_SNR_VR_CPU0_VCCIO_CURR,
  MB_SNR_VR_CPU0_VCCIO_POWER,
  MB_SNR_VR_CPU0_VDDQ_GRPABC_VOLT,
  MB_SNR_VR_CPU0_VDDQ_GRPABC_TEMP,
  MB_SNR_VR_CPU0_VDDQ_GRPABC_CURR,
  MB_SNR_VR_CPU0_VDDQ_GRPABC_POWER,
  MB_SNR_VR_CPU0_VDDQ_GRPDEF_VOLT,
  MB_SNR_VR_CPU0_VDDQ_GRPDEF_TEMP,
  MB_SNR_VR_CPU0_VDDQ_GRPDEF_CURR,
  MB_SNR_VR_CPU0_VDDQ_GRPDEF_POWER,
  MB_SNR_VR_CPU1_VCCIN_VOLT,
  MB_SNR_VR_CPU1_VCCIN_TEMP,
  MB_SNR_VR_CPU1_VCCIN_CURR,
  MB_SNR_VR_CPU1_VCCIN_POWER,
  MB_SNR_VR_CPU1_VSA_VOLT,
  MB_SNR_VR_CPU1_VSA_TEMP,
  MB_SNR_VR_CPU1_VSA_CURR,
  MB_SNR_VR_CPU1_VSA_POWER,
  MB_SNR_VR_CPU1_VCCIO_VOLT,
  MB_SNR_VR_CPU1_VCCIO_TEMP,
  MB_SNR_VR_CPU1_VCCIO_CURR,
  MB_SNR_VR_CPU1_VCCIO_POWER,
  MB_SNR_VR_CPU1_VDDQ_GRPABC_VOLT,
  MB_SNR_VR_CPU1_VDDQ_GRPABC_TEMP,
  MB_SNR_VR_CPU1_VDDQ_GRPABC_CURR,
  MB_SNR_VR_CPU1_VDDQ_GRPABC_POWER,
  MB_SNR_VR_CPU1_VDDQ_GRPDEF_VOLT,
  MB_SNR_VR_CPU1_VDDQ_GRPDEF_TEMP,
  MB_SNR_VR_CPU1_VDDQ_GRPDEF_CURR,
  MB_SNR_VR_CPU1_VDDQ_GRPDEF_POWER,
  MB_SNR_VR_PCH_P1V05_VOLT,
  MB_SNR_VR_PCH_P1V05_TEMP,
  MB_SNR_VR_PCH_P1V05_CURR,
  MB_SNR_VR_PCH_P1V05_POWER,
  MB_SNR_VR_PCH_PVNN_VOLT,
  MB_SNR_VR_PCH_PVNN_TEMP,
  MB_SNR_VR_PCH_PVNN_CURR,
  MB_SNR_VR_PCH_PVNN_POWER,
};

//4S Master BMC Sensor List
const uint8_t mb_4s_m_sensor_list[] = {
  MB_SNR_INLET_TEMP,
  MB_SNR_OUTLET_TEMP_R,
  MB_SNR_OUTLET_TEMP_L,
  MB_SNR_INLET_REMOTE_TEMP,
  MB_SNR_OUTLET_REMOTE_TEMP_R,
  MB_SNR_OUTLET_REMOTE_TEMP_L,
  MB_SNR_P5V,
  MB_SNR_P5V_STBY,
  MB_SNR_P3V3_STBY,
  MB_SNR_P3V3,
  MB_SNR_P3V_BAT,
  MB_SNR_CPU_P1V8,
  MB_SNR_PCH_P1V8,
  MB_SNR_CPU0_PVPP_ABC,
  MB_SNR_CPU0_PVPP_DEF,
  MB_SNR_CPU0_PVTT_ABC,
  MB_SNR_CPU0_PVTT_DEF,
  MB_SNR_CPU1_PVPP_ABC,
  MB_SNR_CPU1_PVPP_DEF,
  MB_SNR_CPU1_PVTT_ABC,
  MB_SNR_CPU1_PVTT_DEF,
  MB_SNR_HSC_VIN,
  MB_SNR_HSC_IOUT,
  MB_SNR_HSC_PIN,
  MB_SNR_HSC_TEMP,
  MB_SNR_HSC_PEAK_PIN,
  MB1_SNR_HSC_VIN,
  MB1_SNR_HSC_IOUT,
  MB1_SNR_HSC_PIN,
  MB1_SNR_HSC_TEMP,
  MB1_SNR_HSC_PEAK_PIN,
  MB_SNR_PCH_TEMP,
  MB_SNR_CPU0_TEMP,
  MB_SNR_CPU1_TEMP,
  MB_SNR_CPU2_TEMP,
  MB_SNR_CPU3_TEMP,
  MB_SNR_CPU0_DIMM_GRPA_TEMP,
  MB_SNR_CPU0_DIMM_GRPB_TEMP,
  MB_SNR_CPU0_DIMM_GRPC_TEMP,
  MB_SNR_CPU0_DIMM_GRPD_TEMP,
  MB_SNR_CPU0_DIMM_GRPE_TEMP,
  MB_SNR_CPU0_DIMM_GRPF_TEMP,
  MB_SNR_CPU1_DIMM_GRPA_TEMP,
  MB_SNR_CPU1_DIMM_GRPB_TEMP,
  MB_SNR_CPU1_DIMM_GRPC_TEMP,
  MB_SNR_CPU1_DIMM_GRPD_TEMP,
  MB_SNR_CPU1_DIMM_GRPE_TEMP,
  MB_SNR_CPU1_DIMM_GRPF_TEMP,
  MB_SNR_CPU2_DIMM_GRPA_TEMP,
  MB_SNR_CPU2_DIMM_GRPB_TEMP,
  MB_SNR_CPU2_DIMM_GRPC_TEMP,
  MB_SNR_CPU2_DIMM_GRPD_TEMP,
  MB_SNR_CPU2_DIMM_GRPE_TEMP,
  MB_SNR_CPU2_DIMM_GRPF_TEMP,
  MB_SNR_CPU3_DIMM_GRPA_TEMP,
  MB_SNR_CPU3_DIMM_GRPB_TEMP,
  MB_SNR_CPU3_DIMM_GRPC_TEMP,
  MB_SNR_CPU3_DIMM_GRPD_TEMP,
  MB_SNR_CPU3_DIMM_GRPE_TEMP,
  MB_SNR_CPU3_DIMM_GRPF_TEMP,
  MB_SNR_DATA0_DRIVE_TEMP,
  MB_SNR_DATA1_DRIVE_TEMP,
  MB_SNR_CPU0_PKG_POWER,
  MB_SNR_CPU1_PKG_POWER,
  MB_SNR_CPU2_PKG_POWER,
  MB_SNR_CPU3_PKG_POWER,
  MB_SNR_CPU0_TJMAX,
  MB_SNR_CPU1_TJMAX,
  MB_SNR_CPU2_TJMAX,
  MB_SNR_CPU3_TJMAX,
  MB_SNR_CPU0_THERM_MARGIN,
  MB_SNR_CPU1_THERM_MARGIN,
  MB_SNR_CPU2_THERM_MARGIN,
  MB_SNR_CPU3_THERM_MARGIN,
  MB_SNR_P12V_STBY_INA260_VOL,
  MB_SNR_P3V3_M2_1_INA260_VOL,
  MB_SNR_P3V3_M2_2_INA260_VOL,
  MB_SNR_P3V3_M2_3_INA260_VOL,
  MB_SNR_VR_CPU0_VCCIN_VOLT,
  MB_SNR_VR_CPU0_VCCIN_TEMP,
  MB_SNR_VR_CPU0_VCCIN_CURR,
  MB_SNR_VR_CPU0_VCCIN_POWER,
  MB_SNR_VR_CPU0_VSA_VOLT,
  MB_SNR_VR_CPU0_VSA_TEMP,
  MB_SNR_VR_CPU0_VSA_CURR,
  MB_SNR_VR_CPU0_VSA_POWER,
  MB_SNR_VR_CPU0_VCCIO_VOLT,
  MB_SNR_VR_CPU0_VCCIO_TEMP,
  MB_SNR_VR_CPU0_VCCIO_CURR,
  MB_SNR_VR_CPU0_VCCIO_POWER,
  MB_SNR_VR_CPU0_VDDQ_GRPABC_VOLT,
  MB_SNR_VR_CPU0_VDDQ_GRPABC_TEMP,
  MB_SNR_VR_CPU0_VDDQ_GRPABC_CURR,
  MB_SNR_VR_CPU0_VDDQ_GRPABC_POWER,
  MB_SNR_VR_CPU0_VDDQ_GRPDEF_VOLT,
  MB_SNR_VR_CPU0_VDDQ_GRPDEF_TEMP,
  MB_SNR_VR_CPU0_VDDQ_GRPDEF_CURR,
  MB_SNR_VR_CPU0_VDDQ_GRPDEF_POWER,
  MB_SNR_VR_CPU1_VCCIN_VOLT,
  MB_SNR_VR_CPU1_VCCIN_TEMP,
  MB_SNR_VR_CPU1_VCCIN_CURR,
  MB_SNR_VR_CPU1_VCCIN_POWER,
  MB_SNR_VR_CPU1_VSA_VOLT,
  MB_SNR_VR_CPU1_VSA_TEMP,
  MB_SNR_VR_CPU1_VSA_CURR,
  MB_SNR_VR_CPU1_VSA_POWER,
  MB_SNR_VR_CPU1_VCCIO_VOLT,
  MB_SNR_VR_CPU1_VCCIO_TEMP,
  MB_SNR_VR_CPU1_VCCIO_CURR,
  MB_SNR_VR_CPU1_VCCIO_POWER,
  MB_SNR_VR_CPU1_VDDQ_GRPABC_VOLT,
  MB_SNR_VR_CPU1_VDDQ_GRPABC_TEMP,
  MB_SNR_VR_CPU1_VDDQ_GRPABC_CURR,
  MB_SNR_VR_CPU1_VDDQ_GRPABC_POWER,
  MB_SNR_VR_CPU1_VDDQ_GRPDEF_VOLT,
  MB_SNR_VR_CPU1_VDDQ_GRPDEF_TEMP,
  MB_SNR_VR_CPU1_VDDQ_GRPDEF_CURR,
  MB_SNR_VR_CPU1_VDDQ_GRPDEF_POWER,
  MB_SNR_VR_PCH_P1V05_VOLT,
  MB_SNR_VR_PCH_P1V05_TEMP,
  MB_SNR_VR_PCH_P1V05_CURR,
  MB_SNR_VR_PCH_P1V05_POWER,
  MB_SNR_VR_PCH_PVNN_VOLT,
  MB_SNR_VR_PCH_PVNN_TEMP,
  MB_SNR_VR_PCH_PVNN_CURR,
  MB_SNR_VR_PCH_PVNN_POWER,
};

//Slave BMC Sensor List
const uint8_t mb_slv_sensor_list[] = {
  MB_SNR_INLET_TEMP,
  MB_SNR_OUTLET_TEMP_R,
  MB_SNR_OUTLET_TEMP_L,
  MB_SNR_INLET_REMOTE_TEMP,
  MB_SNR_OUTLET_REMOTE_TEMP_R,
  MB_SNR_OUTLET_REMOTE_TEMP_L,
  MB_SNR_P5V,
  MB_SNR_P5V_STBY,
  MB_SNR_P3V3_STBY,
  MB_SNR_P3V3,
  MB_SNR_P3V_BAT,
  MB_SNR_CPU_P1V8,
  MB_SNR_PCH_P1V8,
  MB_SNR_CPU0_PVPP_ABC,
  MB_SNR_CPU0_PVPP_DEF,
  MB_SNR_CPU0_PVTT_ABC,
  MB_SNR_CPU0_PVTT_DEF,
  MB_SNR_CPU1_PVPP_ABC,
  MB_SNR_CPU1_PVPP_DEF,
  MB_SNR_CPU1_PVTT_ABC,
  MB_SNR_CPU1_PVTT_DEF,
  MB_SNR_DATA0_DRIVE_TEMP,
  MB_SNR_DATA1_DRIVE_TEMP,
  MB_SNR_P12V_STBY_INA260_VOL,
  MB_SNR_P3V3_M2_1_INA260_VOL,
  MB_SNR_P3V3_M2_2_INA260_VOL,
  MB_SNR_P3V3_M2_3_INA260_VOL,
  MB_SNR_VR_CPU0_VCCIN_VOLT,
  MB_SNR_VR_CPU0_VCCIN_TEMP,
  MB_SNR_VR_CPU0_VCCIN_CURR,
  MB_SNR_VR_CPU0_VCCIN_POWER,
  MB_SNR_VR_CPU0_VSA_VOLT,
  MB_SNR_VR_CPU0_VSA_TEMP,
  MB_SNR_VR_CPU0_VSA_CURR,
  MB_SNR_VR_CPU0_VSA_POWER,
  MB_SNR_VR_CPU0_VCCIO_VOLT,
  MB_SNR_VR_CPU0_VCCIO_TEMP,
  MB_SNR_VR_CPU0_VCCIO_CURR,
  MB_SNR_VR_CPU0_VCCIO_POWER,
  MB_SNR_VR_CPU0_VDDQ_GRPABC_VOLT,
  MB_SNR_VR_CPU0_VDDQ_GRPABC_TEMP,
  MB_SNR_VR_CPU0_VDDQ_GRPABC_CURR,
  MB_SNR_VR_CPU0_VDDQ_GRPABC_POWER,
  MB_SNR_VR_CPU0_VDDQ_GRPDEF_VOLT,
  MB_SNR_VR_CPU0_VDDQ_GRPDEF_TEMP,
  MB_SNR_VR_CPU0_VDDQ_GRPDEF_CURR,
  MB_SNR_VR_CPU0_VDDQ_GRPDEF_POWER,
  MB_SNR_VR_CPU1_VCCIN_VOLT,
  MB_SNR_VR_CPU1_VCCIN_TEMP,
  MB_SNR_VR_CPU1_VCCIN_CURR,
  MB_SNR_VR_CPU1_VCCIN_POWER,
  MB_SNR_VR_CPU1_VSA_VOLT,
  MB_SNR_VR_CPU1_VSA_TEMP,
  MB_SNR_VR_CPU1_VSA_CURR,
  MB_SNR_VR_CPU1_VSA_POWER,
  MB_SNR_VR_CPU1_VCCIO_VOLT,
  MB_SNR_VR_CPU1_VCCIO_TEMP,
  MB_SNR_VR_CPU1_VCCIO_CURR,
  MB_SNR_VR_CPU1_VCCIO_POWER,
  MB_SNR_VR_CPU1_VDDQ_GRPABC_VOLT,
  MB_SNR_VR_CPU1_VDDQ_GRPABC_TEMP,
  MB_SNR_VR_CPU1_VDDQ_GRPABC_CURR,
  MB_SNR_VR_CPU1_VDDQ_GRPABC_POWER,
  MB_SNR_VR_CPU1_VDDQ_GRPDEF_VOLT,
  MB_SNR_VR_CPU1_VDDQ_GRPDEF_TEMP,
  MB_SNR_VR_CPU1_VDDQ_GRPDEF_CURR,
  MB_SNR_VR_CPU1_VDDQ_GRPDEF_POWER,
  MB_SNR_VR_PCH_P1V05_VOLT,
  MB_SNR_VR_PCH_P1V05_TEMP,
  MB_SNR_VR_PCH_P1V05_CURR,
  MB_SNR_VR_PCH_P1V05_POWER,
  MB_SNR_VR_PCH_PVNN_VOLT,
  MB_SNR_VR_PCH_PVNN_TEMP,
  MB_SNR_VR_PCH_PVNN_CURR,
  MB_SNR_VR_PCH_PVNN_POWER,
};

const uint8_t mb_2s_sensor_list[] = {
  MB_SNR_INLET_TEMP,
  MB_SNR_OUTLET_TEMP_R,
  MB_SNR_OUTLET_TEMP_L,
  MB_SNR_INLET_REMOTE_TEMP,
  MB_SNR_OUTLET_REMOTE_TEMP_R,
  MB_SNR_OUTLET_REMOTE_TEMP_L,
  MB_SNR_P5V,
  MB_SNR_P5V_STBY,
  MB_SNR_P3V3_STBY,
  MB_SNR_P3V3,
  MB_SNR_P3V_BAT,
  MB_SNR_CPU_P1V8,
  MB_SNR_PCH_P1V8,
  MB_SNR_CPU0_PVPP_ABC,
  MB_SNR_CPU0_PVPP_DEF,
  MB_SNR_CPU0_PVTT_ABC,
  MB_SNR_CPU0_PVTT_DEF,
  MB_SNR_CPU1_PVPP_ABC,
  MB_SNR_CPU1_PVPP_DEF,
  MB_SNR_CPU1_PVTT_ABC,
  MB_SNR_CPU1_PVTT_DEF,
  MB_SNR_CPU0_TEMP,
  MB_SNR_CPU1_TEMP,
  MB_SNR_HSC_VIN,
  MB_SNR_HSC_IOUT,
  MB_SNR_HSC_PIN,
  MB_SNR_HSC_TEMP,
  MB_SNR_HSC_PEAK_PIN,
  MB_SNR_PCH_TEMP,
  MB_SNR_CPU0_DIMM_GRPA_TEMP,
  MB_SNR_CPU0_DIMM_GRPB_TEMP,
  MB_SNR_CPU0_DIMM_GRPC_TEMP,
  MB_SNR_CPU0_DIMM_GRPD_TEMP,
  MB_SNR_CPU0_DIMM_GRPE_TEMP,
  MB_SNR_CPU0_DIMM_GRPF_TEMP,
  MB_SNR_CPU1_DIMM_GRPA_TEMP,
  MB_SNR_CPU1_DIMM_GRPB_TEMP,
  MB_SNR_CPU1_DIMM_GRPC_TEMP,
  MB_SNR_CPU1_DIMM_GRPD_TEMP,
  MB_SNR_CPU1_DIMM_GRPE_TEMP,
  MB_SNR_CPU1_DIMM_GRPF_TEMP,
  MB_SNR_DATA0_DRIVE_TEMP,
  MB_SNR_DATA1_DRIVE_TEMP,
  MB_SNR_CPU0_PKG_POWER,
  MB_SNR_CPU1_PKG_POWER,
  MB_SNR_CPU0_TJMAX,
  MB_SNR_CPU1_TJMAX,
  MB_SNR_CPU0_THERM_MARGIN,
  MB_SNR_CPU1_THERM_MARGIN,
  MB_SNR_P12V_STBY_INA260_VOL,
  MB_SNR_P3V3_M2_1_INA260_VOL,
  MB_SNR_P3V3_M2_2_INA260_VOL,
  MB_SNR_P3V3_M2_3_INA260_VOL,
  MB_SNR_VR_CPU0_VCCIN_VOLT,
  MB_SNR_VR_CPU0_VCCIN_TEMP,
  MB_SNR_VR_CPU0_VCCIN_CURR,
  MB_SNR_VR_CPU0_VCCIN_POWER,
  MB_SNR_VR_CPU0_VSA_VOLT,
  MB_SNR_VR_CPU0_VSA_TEMP,
  MB_SNR_VR_CPU0_VSA_CURR,
  MB_SNR_VR_CPU0_VSA_POWER,
  MB_SNR_VR_CPU0_VCCIO_VOLT,
  MB_SNR_VR_CPU0_VCCIO_TEMP,
  MB_SNR_VR_CPU0_VCCIO_CURR,
  MB_SNR_VR_CPU0_VCCIO_POWER,
  MB_SNR_VR_CPU0_VDDQ_GRPABC_VOLT,
  MB_SNR_VR_CPU0_VDDQ_GRPABC_TEMP,
  MB_SNR_VR_CPU0_VDDQ_GRPABC_CURR,
  MB_SNR_VR_CPU0_VDDQ_GRPABC_POWER,
  MB_SNR_VR_CPU0_VDDQ_GRPDEF_VOLT,
  MB_SNR_VR_CPU0_VDDQ_GRPDEF_TEMP,
  MB_SNR_VR_CPU0_VDDQ_GRPDEF_CURR,
  MB_SNR_VR_CPU0_VDDQ_GRPDEF_POWER,
  MB_SNR_VR_CPU1_VCCIN_VOLT,
  MB_SNR_VR_CPU1_VCCIN_TEMP,
  MB_SNR_VR_CPU1_VCCIN_CURR,
  MB_SNR_VR_CPU1_VCCIN_POWER,
  MB_SNR_VR_CPU1_VSA_VOLT,
  MB_SNR_VR_CPU1_VSA_TEMP,
  MB_SNR_VR_CPU1_VSA_CURR,
  MB_SNR_VR_CPU1_VSA_POWER,
  MB_SNR_VR_CPU1_VCCIO_VOLT,
  MB_SNR_VR_CPU1_VCCIO_TEMP,
  MB_SNR_VR_CPU1_VCCIO_CURR,
  MB_SNR_VR_CPU1_VCCIO_POWER,
  MB_SNR_VR_CPU1_VDDQ_GRPABC_VOLT,
  MB_SNR_VR_CPU1_VDDQ_GRPABC_TEMP,
  MB_SNR_VR_CPU1_VDDQ_GRPABC_CURR,
  MB_SNR_VR_CPU1_VDDQ_GRPABC_POWER,
  MB_SNR_VR_CPU1_VDDQ_GRPDEF_VOLT,
  MB_SNR_VR_CPU1_VDDQ_GRPDEF_TEMP,
  MB_SNR_VR_CPU1_VDDQ_GRPDEF_CURR,
  MB_SNR_VR_CPU1_VDDQ_GRPDEF_POWER,
  MB_SNR_VR_PCH_P1V05_VOLT,
  MB_SNR_VR_PCH_P1V05_TEMP,
  MB_SNR_VR_PCH_P1V05_CURR,
  MB_SNR_VR_PCH_P1V05_POWER,
  MB_SNR_VR_PCH_PVNN_VOLT,
  MB_SNR_VR_PCH_PVNN_TEMP,
  MB_SNR_VR_PCH_PVNN_CURR,
  MB_SNR_VR_PCH_PVNN_POWER,
};

const uint8_t nic0_sensor_list[] = {
  NIC_MEZZ0_SNR_TEMP,
};

const uint8_t nic1_sensor_list[] = {
  NIC_MEZZ1_SNR_TEMP,
};

const uint8_t pdb_sensor_list[] = {
  PDB_SNR_FAN0_INLET_SPEED,
  PDB_SNR_FAN1_INLET_SPEED,
  PDB_SNR_FAN2_INLET_SPEED,
  PDB_SNR_FAN3_INLET_SPEED,
  PDB_SNR_FAN0_OUTLET_SPEED,
  PDB_SNR_FAN1_OUTLET_SPEED,
  PDB_SNR_FAN2_OUTLET_SPEED,
  PDB_SNR_FAN3_OUTLET_SPEED,
  PDB_SNR_FAN0_VOLTAGE,
  PDB_SNR_FAN1_VOLTAGE,
  PDB_SNR_FAN2_VOLTAGE,
  PDB_SNR_FAN3_VOLTAGE,
  PDB_SNR_FAN0_CURRENT,
  PDB_SNR_FAN1_CURRENT,
  PDB_SNR_FAN2_CURRENT,
  PDB_SNR_FAN3_CURRENT,
  PDB_SNR_FAN0_POWER,
  PDB_SNR_FAN1_POWER,
  PDB_SNR_FAN2_POWER,
  PDB_SNR_FAN3_POWER, 
  PDB_SNR_HSC_VIN,
  PDB_SNR_HSC_VOUT,
  PDB_SNR_HSC_TEMP,
  PDB_SNR_HSC_PIN,
  PDB_SNR_HSC_PEAK_IOUT,
  PDB_SNR_HSC_PEAK_PIN,
  PDB_SNR_HSC_P12V,
  PDB_SNR_HSC_P3V,
};

// List of MB discrete sensors to be monitored
const uint8_t mb_discrete_sensor_list[] = {
//  MB_SENSOR_POWER_FAIL,
//  MB_SENSOR_MEMORY_LOOP_FAIL,
  MB_SNR_PROCESSOR_FAIL,
};

//CPU
PAL_CPU_INFO cpu_info_list[] = {
  {CPU_ID0, PECI_CPU0_ADDR},
  {CPU_ID1, PECI_CPU1_ADDR},
  {CPU_ID2, PECI_CPU2_ADDR},
  {CPU_ID3, PECI_CPU3_ADDR},
  {CPU_ID4, PECI_CPU4_ADDR},
  {CPU_ID5, PECI_CPU5_ADDR},
  {CPU_ID6, PECI_CPU6_ADDR},
  {CPU_ID7, PECI_CPU7_ADDR},

};

//ADM1278
PAL_ADM1278_INFO adm1278_info_list[] = {
  {ADM1278_VOLTAGE, 19599, 0, 100},
  {ADM1278_CURRENT, 842 * ADM1278_RSENSE, 20475, 10},
  {ADM1278_POWER, 6442 * ADM1278_RSENSE, 0, 100},
  {ADM1278_TEMP, 42, 31880, 10},
};

//HSC
PAL_HSC_INFO hsc_info_list[] = {
  {HSC_ID0, ADM1278_SLAVE_ADDR, PCA9545_CH1, adm1278_info_list },
  {HSC_ID1, ADM1278_SLAVE_ADDR, PCA9545_CH2, adm1278_info_list },
  {HSC_ID2, ADM1278_SLAVE_ADDR, PCA9545_CH3, adm1278_info_list },
  {HSC_ID3, ADM1278_SLAVE_ADDR, PCA9545_CH4, adm1278_info_list },
};

//NM
PAL_I2C_BUS_INFO nm_info_list[] = {
  {NM_ID0, NM_IPMB_BUS_ID, NM_SLAVE_ADDR},
};

PAL_I2C_BUS_INFO nic_info_list[] = {
  {MEZZ0, I2C_BUS_17, 0x3E},
  {MEZZ1, I2C_BUS_18, 0x3E},
};

PAL_I2C_BUS_INFO disk_info_list[] = {
  {DISK_BOOT, I2C_BUS_20, 0xD4},
  {DISK_DATA0, I2C_BUS_21, 0xD4},
  {DISK_DATA1, I2C_BUS_22, 0xD4},
};

//INA260
PAL_I2C_BUS_INFO ina260_info_list[] = {
  {INA260_ID0, I2C_BUS_1, 0x80},
  {INA260_ID1, I2C_BUS_1, 0x82},
  {INA260_ID2, I2C_BUS_1, 0x84},
  {INA260_ID3, I2C_BUS_1, 0x86},
};

//CM SENSOR
PAL_CM_SENSOR_HEAD cm_snr_head_list[] = {
 {CM_FAN0_INLET_SPEED,  CM_SNR_FAN0_INLET_SPEED},
 {CM_FAN1_INLET_SPEED,  CM_SNR_FAN1_INLET_SPEED},
 {CM_FAN2_INLET_SPEED,  CM_SNR_FAN2_INLET_SPEED},
 {CM_FAN3_INLET_SPEED,  CM_SNR_FAN3_INLET_SPEED},
 {CM_FAN0_OUTLET_SPEED, CM_SNR_FAN0_OUTLET_SPEED},
 {CM_FAN1_OUTLET_SPEED, CM_SNR_FAN1_OUTLET_SPEED},
 {CM_FAN2_OUTLET_SPEED, CM_SNR_FAN2_OUTLET_SPEED},
 {CM_FAN3_OUTLET_SPEED, CM_SNR_FAN3_OUTLET_SPEED},
 {CM_HSC_VIN,  CM_SNR_HSC_VIN},
 {CM_HSC_IOUT, CM_SNR_HSC_IOUT},
 {CM_HSC_TEMP, CM_SNR_HSC_TEMP},
 {CM_HSC_PIN,  CM_SNR_HSC_PIN},
 {CM_HSC_PEAK_IOUT, CM_SNR_HSC_PEAK_IOUT},
 {CM_HSC_PEAK_PIN,  CM_SNR_HSC_PEAK_PIN},
 {CM_P12V, CM_SNR_P12V},
 {CM_P3V, CM_SNR_P3V},
 {CM_FAN0_VOLT, CM_SNR_FAN0_VOLT},
 {CM_FAN1_VOLT, CM_SNR_FAN1_VOLT},
 {CM_FAN2_VOLT, CM_SNR_FAN2_VOLT},
 {CM_FAN3_VOLT, CM_SNR_FAN3_VOLT},
 {CM_FAN0_CURR, CM_SNR_FAN0_CURR},
 {CM_FAN1_CURR, CM_SNR_FAN1_CURR},
 {CM_FAN2_CURR, CM_SNR_FAN2_CURR},
 {CM_FAN3_CURR, CM_SNR_FAN3_CURR},
 {CM_FAN0_POWER, CM_SNR_FAN0_POWER},
 {CM_FAN1_POWER, CM_SNR_FAN1_POWER},
 {CM_FAN2_POWER, CM_SNR_FAN2_POWER},
 {CM_FAN3_POWER, CM_SNR_FAN3_POWER},
};

char *vr_ti_chips[] = {
  "tps53688-i2c-1-60",  // CPU0_VCCIN
  "tps53688-i2c-1-60",  // CPU0_VCCSA
  "tps53688-i2c-1-62",  // CPU0_VCCIO
  "tps53688-i2c-1-66",  // CPU0_VDDQ_ABC
  "tps53688-i2c-1-68",  // CPU0_VDDQ_DEF
  "tps53688-i2c-1-70",  // CPU1_VCCIN
  "tps53688-i2c-1-70",  // CPU1_VCCSA
  "tps53688-i2c-1-72",  // CPU1_VCCIO
  "tps53688-i2c-1-76",  // CPU1_VDDQ_ABC
  "tps53688-i2c-1-6c",  // CPU1_VDDQ_DEF
  "pxe1110c-i2c-1-4a",  // PCH PVNN
  "pxe1110c-i2c-1-4a",  // PCH 1V5
};

char *vr_infineon_chips[] = {
  "xdpe12284-i2c-1-60",  // CPU0_VCCIN
  "xdpe12284-i2c-1-60",  // CPU0_VCCSA
  "xdpe12284-i2c-1-62",  // CPU0_VCCIO
  "xdpe12284-i2c-1-66",  // CPU0_VDDQ_ABC
  "xdpe12284-i2c-1-68",  // CPU0_VDDQ_DEF
  "xdpe12284-i2c-1-70",  // CPU1_VCCIN
  "xdpe12284-i2c-1-70",  // CPU1_VCCSA
  "xdpe12284-i2c-1-72",  // CPU1_VCCIO
  "xdpe12284-i2c-1-76",  // CPU1_VDDQ_ABC
  "xdpe12284-i2c-1-6c",  // CPU1_VDDQ_DEF
  "pxe1110c-i2c-1-4a",   // PCH PVNN
  "pxe1110c-i2c-1-4a",   // PCH 1V5
};


char **vr_chips = vr_ti_chips;

struct fsc_monitor
{
  uint8_t sensor_num;
  char *sensor_name;
  bool (*check_sensor_sts)(uint8_t);
  bool is_alive;
  uint8_t init_count;
  uint8_t retry;
};

//{SensorName, ID, FUNCTION, PWR_STATUS, {UCR, UNC, UNR, LCR, LNC, LNR, Pos, Neg}
PAL_SENSOR_MAP sensor_map[] = {
  {NULL, 0, NULL, 0, {0, 0, 0, 0, 0, 0, 0, 0}, 0}, //0x00
  {NULL, 0, NULL, 0, {0, 0, 0, 0, 0, 0, 0, 0}, 0}, //0x01
  {NULL, 0, NULL, 0, {0, 0, 0, 0, 0, 0, 0, 0}, 0}, //0x02
  {NULL, 0, NULL, 0, {0, 0, 0, 0, 0, 0, 0, 0}, 0}, //0x03
  {NULL, 0, NULL, 0, {0, 0, 0, 0, 0, 0, 0, 0}, 0}, //0x04
  {NULL, 0, NULL, 0, {0, 0, 0, 0, 0, 0, 0, 0}, 0}, //0x05
  {NULL, 0, NULL, 0, {0, 0, 0, 0, 0, 0, 0, 0}, 0}, //0x06
  {NULL, 0, NULL, 0, {0, 0, 0, 0, 0, 0, 0, 0}, 0}, //0x07
  {"AL_MB_PCH_TEMP", NM_ID0, read_NM_pch_temp, false, {100, 0, 0, 10, 0, 0, 0, 0}, TEMP}, //0x08
  {NULL, 0, NULL, 0, {0, 0, 0, 0, 0, 0, 0, 0}, 0}, //0x09
  {NULL, 0, NULL, 0, {0, 0, 0, 0, 0, 0, 0, 0}, 0}, //0x0A
  {NULL, 0, NULL, 0, {0, 0, 0, 0, 0, 0, 0, 0}, 0}, //0x0B
  {NULL, 0, NULL, 0, {0, 0, 0, 0, 0, 0, 0, 0}, 0}, //0x0C
  {NULL, 0, NULL, 0, {0, 0, 0, 0, 0, 0, 0, 0}, 0}, //0x0D
  {NULL, 0, NULL, 0, {0, 0, 0, 0, 0, 0, 0, 0}, 0}, //0x0E
  {NULL, 0, NULL, 0, {0, 0, 0, 0, 0, 0, 0, 0}, 0}, //0x0F

  {"AL_NIC_MEZZ0_TEMP", MEZZ0, read_nic_temp, true, {95, 0, 0, 10, 0, 0, 0, 0}, TEMP},  //0x10
  {"AL_NIC_MEZZ1_TEMP", MEZZ1, read_nic_temp, true, {95, 0, 0, 10, 0, 0, 0, 0}, TEMP},  //0x11
  {"AL_MB_BOOT_DRIVE_TEMP",  DISK_BOOT,  read_hd_temp, false, {70, 0, 0, 10, 0, 0, 0, 0}, TEMP}, //0x12
  {"AL_MB_DATA0_DRIVE_TEMP", DISK_DATA0, read_hd_temp, false, {70, 0, 0, 10, 0, 0, 0, 0}, TEMP}, //0x13
  {"AL_MB_DATA1_DRIVE_TEMP", DISK_DATA1, read_hd_temp, false, {70, 0, 0, 10, 0, 0, 0, 0}, TEMP}, //0x14
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

  {"AL_PDB_HSC_VIN",  CM_HSC_VIN,  read_cm_sensor, true, {13.2, 0, 0, 10.8, 0, 0, 0, 0}, VOLT}, //0x20
  {"AL_PDB_HSC_IOUT", CM_HSC_IOUT, read_cm_sensor, true, {36, 0, 0, 0, 0, 0, 0, 0}, CURR}, //0x21
  {"AL_PDB_HSC_TEMP", CM_HSC_TEMP, read_cm_sensor, true, {85, 0, 0, 10, 0, 0, 0, 0}, TEMP}, //0x22
  {"AL_PDB_HSC_PIN",  CM_HSC_PIN,  read_cm_sensor, true, {0, 0, 0, 0, 0, 0, 0, 0}, POWER}, //0x23
  {"AL_PDB_HSC_PEAK_IOUT", CM_HSC_PEAK_IOUT, read_cm_sensor, true, {0, 0, 0, 0, 0, 0, 0, 0}, CURR}, //0x24
  {"AL_PDB_HSC_PEAK_PIN",  CM_HSC_PEAK_PIN,  read_cm_sensor, true, {0, 0, 0, 0, 0, 0, 0, 0}, POWER}, //0x25
  {"AL_PDB_P12V", CM_P12V, read_cm_sensor, false, {13.2, 0, 0, 10.8, 0, 0, 0, 0}, VOLT}, //0x26
  {"AL_PDB_P3V3", CM_P3V,  read_cm_sensor, false, {3.47, 0, 0, 3.14, 0, 0, 0, 0}, VOLT}, //0x27

  {"AL_MB_CPU0_TJMAX", CPU_ID0, read_cpu_tjmax, false, {0, 0, 0, 0, 0, 0, 0, 0}, TEMP}, //0x28
  {"AL_MB_CPU1_TJMAX", CPU_ID1, read_cpu_tjmax, false, {0, 0, 0, 0, 0, 0, 0, 0}, TEMP}, //0x29
  {"AL_MB_CPU2_TJMAX", CPU_ID2, read_cpu_tjmax, false, {0, 0, 0, 0, 0, 0, 0, 0}, TEMP}, //0x2A
  {"AL_MB_CPU3_TJMAX", CPU_ID3, read_cpu_tjmax, false, {0, 0, 0, 0, 0, 0, 0, 0}, TEMP}, //0x2B
  {"AL_MB_CPU4_TJMAX", CPU_ID4, read_cpu_tjmax, false, {0, 0, 0, 0, 0, 0, 0, 0}, TEMP}, //0x2C
  {"AL_MB_CPU5_TJMAX", CPU_ID5, read_cpu_tjmax, false, {0, 0, 0, 0, 0, 0, 0, 0}, TEMP}, //0x2D
  {"AL_MB_CPU6_TJMAX", CPU_ID6, read_cpu_tjmax, false, {0, 0, 0, 0, 0, 0, 0, 0}, TEMP}, //0x2E
  {"AL_MB_CPU7_TJMAX", CPU_ID7, read_cpu_tjmax, false, {0, 0, 0, 0, 0, 0, 0, 0}, TEMP}, //0x2F

  {"AL_MB_CPU0_PKG_POWER", CPU_ID0, read_cpu_pkg_pwr, false, {208, 0, 0, 0, 0, 0, 0, 0}, POWER}, //0x30
  {"AL_MB_CPU1_PKG_POWER", CPU_ID1, read_cpu_pkg_pwr, false, {208, 0, 0, 0, 0, 0, 0, 0}, POWER}, //0x31
  {"AL_MB_CPU2_PKG_POWER", CPU_ID2, read_cpu_pkg_pwr, false, {208, 0, 0, 0, 0, 0, 0, 0}, POWER}, //0x32
  {"AL_MB_CPU3_PKG_POWER", CPU_ID3, read_cpu_pkg_pwr, false, {208, 0, 0, 0, 0, 0, 0, 0}, POWER}, //0x33
  {"AL_MB_CPU4_PKG_POWER", CPU_ID4, read_cpu_pkg_pwr, false, {208, 0, 0, 0, 0, 0, 0, 0}, POWER}, //0x34
  {"AL_MB_CPU5_PKG_POWER", CPU_ID5, read_cpu_pkg_pwr, false, {208, 0, 0, 0, 0, 0, 0, 0}, POWER}, //0x35
  {"AL_MB_CPU6_PKG_POWER", CPU_ID6, read_cpu_pkg_pwr, false, {208, 0, 0, 0, 0, 0, 0, 0}, POWER}, //0x36
  {"AL_MB_CPU7_PKG_POWER", CPU_ID7, read_cpu_pkg_pwr, false, {208, 0, 0, 0, 0, 0, 0, 0}, POWER}, //0x37

  {"AL_MB_CPU0_THERM_MARGIN", CPU_ID0, read_cpu_thermal_margin, false, {-5, 0, 0, -81, 0, 0, 0, 0}, TEMP}, //0x38
  {"AL_MB_CPU1_THERM_MARGIN", CPU_ID1, read_cpu_thermal_margin, false, {-5, 0, 0, -81, 0, 0, 0, 0}, TEMP}, //0x39
  {"AL_MB_CPU2_THERM_MARGIN", CPU_ID2, read_cpu_thermal_margin, false, {-5, 0, 0, -81, 0, 0, 0, 0}, TEMP}, //0x3A
  {"AL_MB_CPU3_THERM_MARGIN", CPU_ID3, read_cpu_thermal_margin, false, {-5, 0, 0, -81, 0, 0, 0, 0}, TEMP}, //0x3B
  {"AL_MB_CPU4_THERM_MARGIN", CPU_ID4, read_cpu_thermal_margin, false, {-5, 0, 0, -81, 0, 0, 0, 0}, TEMP}, //0x3C
  {"AL_MB_CPU5_THERM_MARGIN", CPU_ID5, read_cpu_thermal_margin, false, {-5, 0, 0, -81, 0, 0, 0, 0}, TEMP}, //0x3D
  {"AL_MB_CPU6_THERM_MARGIN", CPU_ID6, read_cpu_thermal_margin, false, {-5, 0, 0, -81, 0, 0, 0, 0}, TEMP}, //0x3E
  {"AL_MB_CPU7_THERM_MARGIN", CPU_ID7, read_cpu_thermal_margin, false, {-5, 0, 0, -81, 0, 0, 0, 0}, TEMP}, //0x3F
  {"AL_MB_HSC_VIN",   HSC_ID0, read_hsc_vin,  true, {13.2, 0, 0, 10.8, 0, 0, 0, 0}, VOLT}, //0x40
  {"AL_MB_HSC_IOUT",  HSC_ID0, read_hsc_iout, true, {100, 0, 0, 0, 0, 0, 0, 0}, CURR}, //0x41
  {"AL_MB_HSC_PIN",   HSC_ID0, read_hsc_pin,  true, {0, 0, 0, 0, 0, 0, 0, 0}, POWER}, //0x42
  {"AL_MB_HSC_TEMP",  HSC_ID0, read_hsc_temp, true, {85, 0, 0, 10, 0, 0, 0, 0}, TEMP}, //0x43
  {"AL_MB1_HSC_VIN",  HSC_ID1, read_hsc_vin,  true, {13.2, 0, 0, 10.8, 0, 0, 0, 0}, VOLT}, //0x44
  {"AL_MB1_HSC_IOUT", HSC_ID1, read_hsc_iout, true, {100, 0, 0, 0, 0, 0, 0, 0}, CURR}, //0x45
  {"AL_MB1_HSC_PIN",  HSC_ID1, read_hsc_pin,  true, {0, 0, 0, 0, 0, 0, 0, 0}, POWER}, //0x46
  {"AL_MB1_HSC_TEMP", HSC_ID1, read_hsc_temp, true, {85, 0, 0, 10, 0, 0, 0, 0}, TEMP}, //0x47
  {"AL_MB2_HSC_VIN",  HSC_ID2, read_hsc_vin,  true, {13.2, 0, 0, 10.8, 0, 0, 0, 0}, VOLT}, //0x48
  {"AL_MB2_HSC_IOUT", HSC_ID2, read_hsc_iout, true, {0, 0, 0, 0, 0, 0, 0, 0}, CURR}, //0x49
  {"AL_MB2_HSC_PIN",  HSC_ID2, read_hsc_pin,  true, {0, 0, 0, 0, 0, 0, 0, 0}, POWER}, //0x4A
  {"AL_MB2_HSC_TEMP", HSC_ID2, read_hsc_temp, true, {85, 0, 0, 10, 0, 0, 0, 0}, TEMP}, //0x4B
  {"AL_MB3_HSC_VIN",  HSC_ID3, read_hsc_vin,  true, {13.2, 0, 0, 10.8, 0, 0, 0, 0}, VOLT}, //0x4C
  {"AL_MB3_HSC_IOUT", HSC_ID3, read_hsc_iout, true, {0, 0, 0, 0, 0, 0, 0, 0}, CURR}, //0x4D
  {"AL_MB3_HSC_PIN",  HSC_ID3, read_hsc_pin,  true, {0, 0, 0, 0, 0, 0, 0, 0}, POWER}, //0x4E
  {"AL_MB3_HSC_TEMP", HSC_ID3, read_hsc_temp, true, {85, 0, 0, 10, 0, 0, 0, 0}, TEMP}, //0x4F
  {"AL_MB_HSC_PEAK_PIN",  HSC_ID0, read_hsc_peak_pin, true, {0, 0, 0, 0, 0, 0, 0, 0}, POWER}, //0x50
  {"AL_MB1_HSC_PEAK_PIN", HSC_ID1, read_hsc_peak_pin, true, {0, 0, 0, 0, 0, 0, 0, 0}, POWER}, //0x51
  {"AL_MB2_HSC_PEAK_PIN", HSC_ID2, read_hsc_peak_pin, true, {0, 0, 0, 0, 0, 0, 0, 0}, POWER}, //0x52
  {"AL_MB3_HSC_PEAK_PIN", HSC_ID3, read_hsc_peak_pin, true, {0, 0, 0, 0, 0, 0, 0, 0}, POWER}, //0x53

  {NULL, 0, NULL, 0, {0, 0, 0, 0, 0, 0, 0, 0}, 0}, //0x54
  {NULL, 0, NULL, 0, {0, 0, 0, 0, 0, 0, 0, 0}, 0}, //0x55
  {NULL, 0, NULL, 0, {0, 0, 0, 0, 0, 0, 0, 0}, 0}, //0x56
  {NULL, 0, NULL, 0, {0, 0, 0, 0, 0, 0, 0, 0}, 0}, //0x57
  {NULL, 0, NULL, 0, {0, 0, 0, 0, 0, 0, 0, 0}, 0}, //0x58
  {NULL, 0, NULL, 0, {0, 0, 0, 0, 0, 0, 0, 0}, 0}, //0x59
  {NULL, 0, NULL, 0, {0, 0, 0, 0, 0, 0, 0, 0}, 0}, //0x5A
  {NULL, 0, NULL, 0, {0, 0, 0, 0, 0, 0, 0, 0}, 0}, //0x5B

  {"AL_MB_P12V_STBY_INA260_VOL", INA260_ID0, read_ina260_vol, true, {13.2, 0, 0, 10.8, 0, 0, 0, 0}, VOLT}, //0x5C
  {"AL_MB_P3V3_M2_1_INA260_VOL", INA260_ID1, read_ina260_vol, false, {3.47, 0, 0, 3.14, 0, 0, 0, 0}, VOLT}, //0x5D
  {"AL_MB_P3V3_M2_2_INA260_VOL", INA260_ID2, read_ina260_vol, false, {3.47, 0, 0, 3.14, 0, 0, 0, 0}, VOLT}, //0x5E
  {"AL_MB_P3V3_M2_3_INA260_VOL", INA260_ID3, read_ina260_vol, false, {3.47, 0, 0, 3.14, 0, 0, 0, 0}, VOLT}, //0x5F

  {"AL_PDB_FAN0_INLET_SPEED", CM_FAN0_INLET_SPEED, read_cm_sensor, true, {13500, 0, 0, 500, 0, 0, 0, 0}, FAN}, //0x60
  {"AL_PDB_FAN1_INLET_SPEED", CM_FAN1_INLET_SPEED, read_cm_sensor, true, {13500, 0, 0, 500, 0, 0, 0, 0}, FAN}, //0x61
  {"AL_PDB_FAN2_INLET_SPEED", CM_FAN2_INLET_SPEED, read_cm_sensor, true, {13500, 0, 0, 500, 0, 0, 0, 0}, FAN}, //0x62
  {"AL_PDB_FAN3_INLET_SPEED", CM_FAN3_INLET_SPEED, read_cm_sensor, true, {13500, 0, 0, 500, 0, 0, 0, 0}, FAN}, //0x63
  {"AL_PDB_FAN0_OUTLET_SPEED", CM_FAN0_OUTLET_SPEED, read_cm_sensor, true, {12500, 0, 0, 500, 0, 0, 0, 0}, FAN}, //0x64
  {"AL_PDB_FAN1_OUTLET_SPEED", CM_FAN1_OUTLET_SPEED, read_cm_sensor, true, {12500, 0, 0, 500, 0, 0, 0, 0}, FAN}, //0x65
  {"AL_PDB_FAN2_OUTLET_SPEED", CM_FAN2_OUTLET_SPEED, read_cm_sensor, true, {12500, 0, 0, 500, 0, 0, 0, 0}, FAN}, //0x66
  {"AL_PDB_FAN3_OUTLET_SPEED", CM_FAN3_OUTLET_SPEED, read_cm_sensor, true, {12500, 0, 0, 500, 0, 0, 0, 0}, FAN}, //0x67
  {"AL_PDB_FAN0_VOLT", CM_FAN0_VOLT, read_cm_sensor, true, {13.2, 0, 0, 10.8, 0, 0, 0, 0}, VOLT}, //0x68
  {"AL_PDB_FAN1_VOLT", CM_FAN1_VOLT, read_cm_sensor, true, {13.2, 0, 0, 10.8, 0, 0, 0, 0}, VOLT}, //0x69
  {"AL_PDB_FAN2_VOLT", CM_FAN2_VOLT, read_cm_sensor, true, {13.2, 0, 0, 10.8, 0, 0, 0, 0}, VOLT}, //0x6A
  {"AL_PDB_FAN3_VOLT", CM_FAN3_VOLT, read_cm_sensor, true, {13.2, 0, 0, 10.8, 0, 0, 0, 0}, VOLT}, //0x6B
  {"AL_PDB_FAN0_CURR", CM_FAN0_CURR, read_cm_sensor, true, {0, 0, 0, 0, 0, 0, 0, 0}, CURR}, //0x6C
  {"AL_PDB_FAN1_CURR", CM_FAN1_CURR, read_cm_sensor, true, {0, 0, 0, 0, 0, 0, 0, 0}, CURR}, //0x6D
  {"AL_PDB_FAN2_CURR", CM_FAN2_CURR, read_cm_sensor, true, {0, 0, 0, 0, 0, 0, 0, 0}, CURR}, //0x6E
  {"AL_PDB_FAN3_CURR", CM_FAN3_CURR, read_cm_sensor, true, {0, 0, 0, 0, 0, 0, 0, 0}, CURR}, //0x6F

  {"AL_MB_CPU0_DIMM_A0_C0_TEMP", DIMM_CRPA, read_cpu0_dimm_temp, false, {85, 0, 0, 10, 0, 0, 0, 0}, TEMP}, //0x70
  {"AL_MB_CPU0_DIMM_A1_C1_TEMP", DIMM_CRPB, read_cpu0_dimm_temp, false, {85, 0, 0, 10, 0, 0, 0, 0}, TEMP}, //0x71
  {"AL_MB_CPU0_DIMM_A2_C2_TEMP", DIMM_CRPC, read_cpu0_dimm_temp, false, {85, 0, 0, 10, 0, 0, 0, 0}, TEMP}, //0x72
  {"AL_MB_CPU0_DIMM_A3_C3_TEMP", DIMM_CRPD, read_cpu0_dimm_temp, false, {85, 0, 0, 10, 0, 0, 0, 0}, TEMP}, //0x73
  {"AL_MB_CPU0_DIMM_A4_C4_TEMP", DIMM_CRPE, read_cpu0_dimm_temp, false, {85, 0, 0, 10, 0, 0, 0, 0}, TEMP}, //0x74
  {"AL_MB_CPU0_DIMM_A5_C5_TEMP", DIMM_CRPF, read_cpu0_dimm_temp, false, {85, 0, 0, 10, 0, 0, 0, 0}, TEMP}, //0x75_
  {"AL_MB_CPU1_DIMM_B0_D0_TEMP", DIMM_CRPA, read_cpu1_dimm_temp, false, {85, 0, 0, 10, 0, 0, 0, 0}, TEMP}, //0x76
  {"AL_MB_CPU1_DIMM_B1_D1_TEMP", DIMM_CRPB, read_cpu1_dimm_temp, false, {85, 0, 0, 10, 0, 0, 0, 0}, TEMP}, //0x77
  {"AL_MB_CPU1_DIMM_B2_D2_TEMP", DIMM_CRPC, read_cpu1_dimm_temp, false, {85, 0, 0, 10, 0, 0, 0, 0}, TEMP}, //0x78
  {"AL_MB_CPU1_DIMM_B3_D3_TEMP", DIMM_CRPD, read_cpu1_dimm_temp, false, {85, 0, 0, 10, 0, 0, 0, 0}, TEMP}, //0x79
  {"AL_MB_CPU1_DIMM_B4_D4_TEMP", DIMM_CRPE, read_cpu1_dimm_temp, false, {85, 0, 0, 10, 0, 0, 0, 0}, TEMP}, //0x7A
  {"AL_MB_CPU1_DIMM_B5_D5_TEMP", DIMM_CRPF, read_cpu1_dimm_temp, false, {85, 0, 0, 10, 0, 0, 0, 0}, TEMP}, //0x7B
  {"AL_MB_CPU2_DIMM_A0_C0_TEMP", DIMM_CRPA, read_cpu2_dimm_temp, false, {85, 0, 0, 10, 0, 0, 0, 0}, TEMP}, //0x7C
  {"AL_MB_CPU2_DIMM_A1_C1_TEMP", DIMM_CRPB, read_cpu2_dimm_temp, false, {85, 0, 0, 10, 0, 0, 0, 0}, TEMP}, //0x7D
  {"AL_MB_CPU2_DIMM_A2_C2_TEMP", DIMM_CRPC, read_cpu2_dimm_temp, false, {85, 0, 0, 10, 0, 0, 0, 0}, TEMP}, //0x7E
  {"AL_MB_CPU2_DIMM_A3_C3_TEMP", DIMM_CRPD, read_cpu2_dimm_temp, false, {85, 0, 0, 10, 0, 0, 0, 0}, TEMP}, //0x7F
  {"AL_MB_CPU2_DIMM_A4_C4_TEMP", DIMM_CRPE, read_cpu2_dimm_temp, false, {85, 0, 0, 10, 0, 0, 0, 0}, TEMP}, //0x80
  {"AL_MB_CPU2_DIMM_A5_C5_TEMP", DIMM_CRPF, read_cpu2_dimm_temp, false, {85, 0, 0, 10, 0, 0, 0, 0}, TEMP}, //0x81

  {"AL_MB_CPU3_DIMM_B0_D0_TEMP", DIMM_CRPA, read_cpu3_dimm_temp, false, {85, 0, 0, 10, 0, 0, 0, 0}, TEMP}, //0x82
  {"AL_MB_CPU3_DIMM_B1_D1_TEMP", DIMM_CRPB, read_cpu3_dimm_temp, false, {85, 0, 0, 10, 0, 0, 0, 0}, TEMP}, //0x83
  {"AL_MB_CPU3_DIMM_B2_D2_TEMP", DIMM_CRPC, read_cpu3_dimm_temp, false, {85, 0, 0, 10, 0, 0, 0, 0}, TEMP}, //0x84
  {"AL_MB_CPU3_DIMM_B3_D3_TEMP", DIMM_CRPD, read_cpu3_dimm_temp, false, {85, 0, 0, 10, 0, 0, 0, 0}, TEMP}, //0x85
  {"AL_MB_CPU3_DIMM_B4_D4_TEMP", DIMM_CRPE, read_cpu3_dimm_temp, false, {85, 0, 0, 10, 0, 0, 0, 0}, TEMP}, //0x86
  {"AL_MB_CPU3_DIMM_B5_D5_TEMP", DIMM_CRPF, read_cpu3_dimm_temp, false, {85, 0, 0, 10, 0, 0, 0, 0}, TEMP}, //0x87
  {"AL_MB_CPU4_DIMM_A0_C0_TEMP", DIMM_CRPA, read_cpu4_dimm_temp, false, {85, 0, 0, 10, 0, 0, 0, 0}, TEMP}, //0x88
  {"AL_MB_CPU4_DIMM_A1_C1_TEMP", DIMM_CRPB, read_cpu4_dimm_temp, false, {85, 0, 0, 10, 0, 0, 0, 0}, TEMP}, //0x89
  {"AL_MB_CPU4_DIMM_A2_C2_TEMP", DIMM_CRPC, read_cpu4_dimm_temp, false, {85, 0, 0, 10, 0, 0, 0, 0}, TEMP}, //0x8A
  {"AL_MB_CPU4_DIMM_A3_C3_TEMP", DIMM_CRPD, read_cpu4_dimm_temp, false, {85, 0, 0, 10, 0, 0, 0, 0}, TEMP}, //0x8B
  {"AL_MB_CPU4_DIMM_A4_C4_TEMP", DIMM_CRPE, read_cpu4_dimm_temp, false, {85, 0, 0, 10, 0, 0, 0, 0}, TEMP}, //0x8C
  {"AL_MB_CPU4_DIMM_A5_C5_TEMP", DIMM_CRPF, read_cpu4_dimm_temp, false, {85, 0, 0, 10, 0, 0, 0, 0}, TEMP}, //0x8D
  {"AL_MB_CPU5_DIMM_B0_D0_TEMP", DIMM_CRPA, read_cpu5_dimm_temp, false, {85, 0, 0, 10, 0, 0, 0, 0}, TEMP}, //0x8E
  {"AL_MB_CPU5_DIMM_B1_D1_TEMP", DIMM_CRPB, read_cpu5_dimm_temp, false, {85, 0, 0, 10, 0, 0, 0, 0}, TEMP}, //0x8F
  {"AL_MB_CPU5_DIMM_B2_D2_TEMP", DIMM_CRPC, read_cpu5_dimm_temp, false, {85, 0, 0, 10, 0, 0, 0, 0}, TEMP}, //0x90
  {"AL_MB_CPU5_DIMM_B3_D3_TEMP", DIMM_CRPD, read_cpu5_dimm_temp, false, {85, 0, 0, 10, 0, 0, 0, 0}, TEMP}, //0x91
  {"AL_MB_CPU5_DIMM_B4_D4_TEMP", DIMM_CRPE, read_cpu5_dimm_temp, false, {85, 0, 0, 10, 0, 0, 0, 0}, TEMP}, //0x92
  {"AL_MB_CPU5_DIMM_B5_D5_TEMP", DIMM_CRPF, read_cpu5_dimm_temp, false, {85, 0, 0, 10, 0, 0, 0, 0}, TEMP}, //0x93

  {"AL_MB_CPU6_DIMM_A0_C0_TEMP", DIMM_CRPA, read_cpu6_dimm_temp, false, {85, 0, 0, 10, 0, 0, 0, 0}, TEMP}, //0x94
  {"AL_MB_CPU6_DIMM_A1_C1_TEMP", DIMM_CRPB, read_cpu6_dimm_temp, false, {85, 0, 0, 10, 0, 0, 0, 0}, TEMP}, //0x95
  {"AL_MB_CPU6_DIMM_A2_C2_TEMP", DIMM_CRPC, read_cpu6_dimm_temp, false, {85, 0, 0, 10, 0, 0, 0, 0}, TEMP}, //0x96
  {"AL_MB_CPU6_DIMM_A3_C3_TEMP", DIMM_CRPD, read_cpu6_dimm_temp, false, {85, 0, 0, 10, 0, 0, 0, 0}, TEMP}, //0x97
  {"AL_MB_CPU6_DIMM_A4_C4_TEMP", DIMM_CRPE, read_cpu6_dimm_temp, false, {85, 0, 0, 10, 0, 0, 0, 0}, TEMP}, //0x98
  {"AL_MB_CPU6_DIMM_A5_C5_TEMP", DIMM_CRPF, read_cpu6_dimm_temp, false, {85, 0, 0, 10, 0, 0, 0, 0}, TEMP}, //0x99
  {"AL_MB_CPU7_DIMM_B0_D0_TEMP", DIMM_CRPA, read_cpu7_dimm_temp, false, {85, 0, 0, 10, 0, 0, 0, 0}, TEMP}, //0x9A
  {"AL_MB_CPU7_DIMM_B1_D1_TEMP", DIMM_CRPB, read_cpu7_dimm_temp, false, {85, 0, 0, 10, 0, 0, 0, 0}, TEMP}, //0x9B
  {"AL_MB_CPU7_DIMM_B2_D2_TEMP", DIMM_CRPC, read_cpu7_dimm_temp, false, {85, 0, 0, 10, 0, 0, 0, 0}, TEMP}, //0x9C
  {"AL_MB_CPU7_DIMM_B3_D3_TEMP", DIMM_CRPD, read_cpu7_dimm_temp, false, {85, 0, 0, 10, 0, 0, 0, 0}, TEMP}, //0x9D
  {"AL_MB_CPU7_DIMM_B4_D4_TEMP", DIMM_CRPE, read_cpu7_dimm_temp, false, {85, 0, 0, 10, 0, 0, 0, 0}, TEMP}, //0x9E
  {"AL_MB_CPU7_DIMM_B5_D5_TEMP", DIMM_CRPF, read_cpu7_dimm_temp, false, {85, 0, 0, 10, 0, 0, 0, 0}, TEMP}, //0x9F
  {"AL_MB_INLET_TEMP",    TEMP_INLET,    read_sensor, true, {55, 0, 0, 10, 0, 0, 0, 0}, TEMP}, //0xA0
  {"AL_MB_OUTLET_TEMP_R", TEMP_OUTLET_R, read_sensor, true, {70, 0, 0, 10, 0, 0, 0, 0}, TEMP}, //0xA1
  {"AL_MB_OUTLET_TEMP_L", TEMP_OUTLET_L, read_sensor, true, {70, 0, 0, 10, 0, 0, 0, 0}, TEMP}, //0xA2
  {"AL_MB_INLET_REMOTE_TEMP", TEMP_REMOTE_INLET, read_sensor, true, {55, 0, 0, 10, 0, 0, 0, 0}, TEMP}, //0xA3
  {"AL_MB_OUTLET_R_REMOTE_TEMP", TEMP_REMOTE_OUTLET_R, read_sensor, true, {75, 0, 0, 10, 0, 0, 0, 0}, TEMP}, //0xA4
  {"AL_MB_OUTLET_L_REMOTE_TEMP", TEMP_REMOTE_OUTLET_L, read_sensor, true, {75, 0, 0, 10, 0, 0, 0, 0}, TEMP}, //0xA5
  {NULL, 0, NULL, 0, {0, 0, 0, 0, 0, 0, 0, 0}, 0}, //0xA6
  {NULL, 0, NULL, 0, {0, 0, 0, 0, 0, 0, 0, 0}, 0}, //0xA7
  {"AL_MB_CPU0_TEMP", CPU_ID0, read_cpu_temp, false, {94, 0, 0, 10, 0, 0, 0, 0}, TEMP}, //0xA8
  {"AL_MB_CPU1_TEMP", CPU_ID1, read_cpu_temp, false, {94, 0, 0, 10, 0, 0, 0, 0}, TEMP}, //0xA9
  {"AL_MB_CPU2_TEMP", CPU_ID2, read_cpu_temp, false, {94, 0, 0, 10, 0, 0, 0, 0}, TEMP}, //0xAA
  {"AL_MB_CPU3_TEMP", CPU_ID3, read_cpu_temp, false, {94, 0, 0, 10, 0, 0, 0, 0}, TEMP}, //0xAB
  {"AL_MB_CPU4_TEMP", CPU_ID4, read_cpu_temp, false, {94, 0, 0, 10, 0, 0, 0, 0}, TEMP}, //0xAC
  {"AL_MB_CPU5_TEMP", CPU_ID5, read_cpu_temp, false, {94, 0, 0, 10, 0, 0, 0, 0}, TEMP}, //0xAD
  {"AL_MB_CPU6_TEMP", CPU_ID6, read_cpu_temp, false, {94, 0, 0, 10, 0, 0, 0, 0}, TEMP}, //0xAE
  {"AL_MB_CPU7_TEMP", CPU_ID7, read_cpu_temp, false, {94, 0, 0, 10, 0, 0, 0, 0}, TEMP}, //0xAF

  {"AL_MB_VR_CPU0_VCCIN_VOUT", VR_ID0, read_vr_vout, false, {2.1, 0, 0, 1.33, 0, 0, 0, 0}, VOLT}, //0xB0
  {"AL_MB_VR_CPU0_VCCIN_TEMP", VR_ID0, read_vr_temp, false, {115, 0, 0, 10, 0, 0, 0, 0}, TEMP}, //0xB1
  {"AL_MB_VR_CPU0_VCCIN_IOUT", VR_ID0, read_vr_iout, false, {440, 0, 0, 0, 0, 0, 0, 0}, CURR}, //0XB2
  {"AL_MB_VR_CPU0_VCCIN_POUT", VR_ID0, read_vr_pout, false, {1008, 0, 0, 0, 0, 0, 0, 0}, POWER}, //0xB3
  {"AL_MB_VR_CPU0_VCCSA_VOUT", VR_ID1, read_vr_vout, false, {1.25, 0, 0, 0.44, 0, 0, 0, 0}, VOLT}, //0xB4
  {"AL_MB_VR_CPU0_VCCSA_TEMP", VR_ID1, read_vr_temp, false, {115, 0, 0, 10, 0, 0, 0, 0}, TEMP}, //0xB5
  {"AL_MB_VR_CPU0_VCCSA_IOUT", VR_ID1, read_vr_iout, false, {19.6, 0, 0, 0, 0, 0, 0, 0}, CURR}, //0XB6
  {"AL_MB_VR_CPU0_VCCSA_POUT", VR_ID1, read_vr_pout, false, {28.75, 0, 0, 0, 0, 0, 0, 0}, POWER}, //0xB7
  {"AL_MB_VR_CPU0_VCCIO_VOUT", VR_ID2, read_vr_vout, false, {1.25, 0, 0, 0.85, 0, 0, 0, 0}, VOLT}, //0xB8
  {"AL_MB_VR_CPU0_VCCIO_TEMP", VR_ID2, read_vr_temp, false, {115, 0, 0, 10, 0, 0, 0, 0}, TEMP}, //0xB9
  {"AL_MB_VR_CPU0_VCCIO_IOUT", VR_ID2, read_vr_iout, false, {28.8, 0, 0, 0, 0, 0, 0, 0}, CURR}, //0XBA
  {"AL_MB_VR_CPU0_VCCIO_POUT", VR_ID2, read_vr_pout, false, {40.7, 0, 0, 0, 0, 0, 0, 0}, POWER}, //0xBB
  {"AL_MB_VR_CPU0_VDDQ_ABC_VOUT", VR_ID3, read_vr_vout, false, {1.4, 0, 0, 1.13, 0, 0, 0, 0}, VOLT}, //0xBC
  {"AL_MB_VR_CPU0_VDDQ_ABC_TEMP", VR_ID3, read_vr_temp, false, {115, 0, 0, 10, 0, 0, 0, 0}, TEMP}, //0xBD
  {"AL_MB_VR_CPU0_VDDQ_ABC_IOUT", VR_ID3, read_vr_iout, false, {77, 0, 0, 0, 0, 0, 0, 0}, CURR}, //0XBE
  {"AL_MB_VR_CPU0_VDDQ_ABC_POUT", VR_ID3, read_vr_pout, false, {121.8, 0, 0, 0, 0, 0, 0, 0}, POWER}, //0xBF

  {"AL_MB_VR_CPU0_VDDQ_DEF_VOUT", VR_ID4, read_vr_vout, false, {1.4, 0, 0, 1.13, 0, 0, 0, 0}, VOLT}, //0xC0
  {"AL_MB_VR_CPU0_VDDQ_DEF_TEMP", VR_ID4, read_vr_temp, false, {115, 0, 0, 10, 0, 0, 0, 0}, TEMP}, //0xC1
  {"AL_MB_VR_CPU0_VDDQ_DEF_IOUT", VR_ID4, read_vr_iout, false, {77, 0, 0, 0, 0, 0, 0, 0}, CURR}, //0XC2
  {"AL_MB_VR_CPU0_VDDQ_DEF_POUT", VR_ID4, read_vr_pout, false, {121.8, 0, 0, 0, 0, 0, 0, 0}, POWER}, //0xC3
  {"AL_MB_VR_PCH_P1V05_VOLT", VR_ID10, read_vr_vout, true, {1.2, 0, 0, 1.01, 0, 0, 0, 0}, VOLT}, //0xC4
  {"AL_MB_VR_PCH_P1V05_TEMP", VR_ID10, read_vr_temp, true, {115, 0, 0, 10, 0, 0, 0, 0}, TEMP}, //0xC5
  {"AL_MB_VR_PCH_P1V05_IOUT", VR_ID10, read_vr_iout, true, {18, 0, 0, 0, 0, 0, 0, 0}, CURR}, //0xC6
  {"AL_MB_VR_PCH_P1V05_POUT", VR_ID10, read_vr_pout, true, {24, 0, 0, 0, 0, 0, 0, 0}, POWER}, //0xC7

  {"AL_MB_VR_PCH_PVNN_VOLT", VR_ID11, read_vr_vout, true, {1.2, 0, 0, 0.80, 0, 0, 0, 0}, VOLT}, //0xC8
  {"AL_MB_VR_PCH_PVNN_TEMP", VR_ID11, read_vr_temp, true, {115, 0, 0, 10, 0, 0, 0, 0}, TEMP}, //0xC9
  {"AL_MB_VR_PCH_PVNN_IOUT", VR_ID11, read_vr_iout, true, {26, 0, 0, 0, 0, 0, 0, 0}, CURR}, //0xCA
  {"AL_MB_VR_PCH_PVNN_POUT", VR_ID11, read_vr_pout, true, {33.6, 0, 0, 0, 0, 0, 0, 0}, POWER}, //0xCB
  
  {"AL_PDB_FAN0_POWER", CM_FAN0_POWER, read_cm_sensor, true, {0, 0, 0, 0, 0, 0, 0, 0}, POWER}, //0xCC
  {"AL_PDB_FAN1_POWER", CM_FAN1_POWER, read_cm_sensor, true, {0, 0, 0, 0, 0, 0, 0, 0}, POWER}, //0xCD
  {"AL_PDB_FAN2_POWER", CM_FAN2_POWER, read_cm_sensor, true, {0, 0, 0, 0, 0, 0, 0, 0}, POWER}, //0xCE
  {"AL_PDB_FAN3_POWER", CM_FAN3_POWER, read_cm_sensor, true, {0, 0, 0, 0, 0, 0, 0, 0}, POWER}, //0xCF

  {"AL_MB_P5V",       ADC0, read_adc_val, false, {5.25, 0, 0, 4.75, 0, 0, 0, 0}, VOLT}, //0xD0
  {"AL_MB_P5V_STBY",  ADC1, read_adc_val, true,  {5.25, 0, 0, 4.75, 0, 0, 0, 0}, VOLT}, //0xD1
  {"AL_MB_P3V3_STBY", ADC2, read_adc_val, true,  {3.47, 0, 0, 3.14, 0, 0, 0, 0}, VOLT}, //0xD2
  {"AL_MB_P3V3",      ADC3, read_adc_val, false, {3.47, 0, 0, 3.14, 0, 0, 0, 0}, VOLT}, //0xD3
  {"AL_MB_P3V_BAT",   ADC4, read_battery_val, true, {3.4, 0, 0, 2.6, 0, 0, 0, 0}, VOLT}, //0xD4
  {"AL_MB_CPU_P1V8",   ADC5, read_adc_val, false, {1.89, 0, 0, 1.71, 0, 0, 0, 0}, VOLT}, //0xD5
  {"AL_MB_PCH_P1V8",   ADC6, read_adc_val, false, {1.89, 0, 0, 1.71, 0, 0, 0, 0}, VOLT}, //0xD6
  {"AL_MB_CPU0_PVPP_ABC", ADC7,  read_adc_val, false, {2.75, 0, 0, 2.38, 0, 0, 0, 0}, VOLT}, //0xD7
  {"AL_MB_CPU1_PVPP_ABC", ADC8,  read_adc_val, false, {2.75, 0, 0, 2.38, 0, 0, 0, 0}, VOLT}, //0xD8
  {"AL_MB_CPU0_PVPP_DEF", ADC9,  read_adc_val, false, {2.75, 0, 0, 2.38, 0, 0, 0, 0}, VOLT}, //0xD9
  {"AL_MB_CPU1_PVPP_DEF", ADC10, read_adc_val, false, {2.75, 0, 0, 2.38, 0, 0, 0, 0}, VOLT}, //0xDA
  {"AL_MB_CPU0_PVTT_ABC", ADC11, read_adc_val, false, {0.66, 0, 0, 0.55, 0, 0, 0, 0}, VOLT}, //0xDB
  {"AL_MB_CPU1_PVTT_ABC", ADC12, read_adc_val, false, {0.66, 0, 0, 0.55, 0, 0, 0, 0}, VOLT}, //0xDC
  {"AL_MB_CPU0_PVTT_DEF", ADC13, read_adc_val, false, {0.66, 0, 0, 0.55, 0, 0, 0, 0}, VOLT}, //0xDD
  {"AL_MB_CPU1_PVTT_DEF", ADC14, read_adc_val, false, {0.66, 0, 0, 0.55, 0, 0, 0, 0}, VOLT}, //0xDE
  {"AL_MB_PROCESSOR_FAIL", FRU_MB, read_frb3, 0, {0, 0, 0, 0, 0, 0, 0, 0}, STATE}, //0xDF

  {"AL_MB_VR_CPU1_VCCIN_VOUT", VR_ID5, read_vr_vout, false, {2.1, 0, 0, 1.33, 0, 0, 0, 0}, VOLT}, //0xE0
  {"AL_MB_VR_CPU1_VCCIN_TEMP", VR_ID5, read_vr_temp, false, {115, 0, 0, 10, 0, 0, 0, 0}, TEMP}, //0xE1
  {"AL_MB_VR_CPU1_VCCIN_IOUT", VR_ID5, read_vr_iout, false, {440, 0, 0, 0, 0, 0, 0, 0}, CURR}, //0XE2
  {"AL_MB_VR_CPU1_VCCIN_POUT", VR_ID5, read_vr_pout, false, {1008, 0, 0, 0, 0, 0, 0, 0}, POWER}, //0xE3
  {"AL_MB_VR_CPU1_VCCSA_VOUT", VR_ID6, read_vr_vout, false, {1.25, 0, 0, 0.44, 0, 0, 0, 0}, VOLT}, //0xE4
  {"AL_MB_VR_CPU1_VCCSA_TEMP", VR_ID6, read_vr_temp, false, {115, 0, 0, 10, 0, 0, 0, 0}, TEMP}, //0xE5
  {"AL_MB_VR_CPU1_VCCSA_IOUT", VR_ID6, read_vr_iout, false, {19.6, 0, 0, 0, 0, 0, 0, 0}, CURR}, //0XE6
  {"AL_MB_VR_CPU1_VCCSA_POUT", VR_ID6, read_vr_pout, false, {28.75, 0, 0, 0, 0, 0, 0, 0}, POWER}, //0xE7
  {"AL_MB_VR_CPU1_VCCIO_VOUT", VR_ID7, read_vr_vout, false, {1.25, 0, 0, 0.85, 0, 0, 0, 0}, VOLT}, //0xE8
  {"AL_MB_VR_CPU1_VCCIO_TEMP", VR_ID7, read_vr_temp, false, {115, 0, 0, 10, 0, 0, 0, 0}, TEMP}, //0xE9
  {"AL_MB_VR_CPU1_VCCIO_IOUT", VR_ID7, read_vr_iout, false, {28.8, 0, 0, 0, 0, 0, 0, 0}, CURR}, //0XEA
  {"AL_MB_VR_CPU1_VCCIO_POUT", VR_ID7, read_vr_pout, false, {40.7, 0, 0, 0, 0, 0, 0, 0}, POWER}, //0xEB
  {"AL_MB_VR_CPU1_VDDQ_ABC_VOUT", VR_ID8, read_vr_vout, false, {1.4, 0, 0, 1.13, 0, 0, 0, 0}, VOLT}, //0xEC
  {"AL_MB_VR_CPU1_VDDQ_ABC_TEMP", VR_ID8, read_vr_temp, false, {115, 0, 0, 10, 0, 0, 0, 0}, TEMP}, //0xED
  {"AL_MB_VR_CPU1_VDDQ_ABC_IOUT", VR_ID8, read_vr_iout, false, {77, 0, 0, 0, 0, 0, 0, 0}, CURR}, //0XEE
  {"AL_MB_VR_CPU1_VDDQ_ABC_POUT", VR_ID8, read_vr_pout, false, {121.8, 0, 0, 0, 0, 0, 0, 0}, POWER}, //0xEF

  {"AL_MB_VR_CPU1_VDDQ_DEF_VOUT", VR_ID9, read_vr_vout, false, {1.4, 0, 0, 1.13, 0, 0, 0, 0}, VOLT}, //0xF0
  {"AL_MB_VR_CPU1_VDDQ_DEF_TEMP", VR_ID9, read_vr_temp, false, {115, 0, 0, 10, 0, 0, 0, 0}, TEMP}, //0xF1
  {"AL_MB_VR_CPU1_VDDQ_DEF_IOUT", VR_ID9, read_vr_iout, false, {77, 0, 0, 0, 0, 0, 0, 0}, CURR}, //0XF2
  {"AL_MB_VR_CPU1_VDDQ_DEF_POUT", VR_ID9, read_vr_pout, false, {121.8, 0, 0, 0, 0, 0, 0, 0}, POWER}, //0xF3
  {NULL, 0, NULL, 0, {0, 0, 0, 0, 0, 0, 0, 0}, 0}, //0xF4
  {NULL, 0, NULL, 0, {0, 0, 0, 0, 0, 0, 0, 0}, 0}, //0xF5
  {NULL, 0, NULL, 0, {0, 0, 0, 0, 0, 0, 0, 0}, 0}, //0xF6
  {NULL, 0, NULL, 0, {0, 0, 0, 0, 0, 0, 0, 0}, 0}, //0xF7
  {NULL, 0, NULL, 0, {0, 0, 0, 0, 0, 0, 0, 0}, 0}, //0xF8
  {NULL, 0, NULL, 0, {0, 0, 0, 0, 0, 0, 0, 0}, 0}, //0xF9
  {NULL, 0, NULL, 0, {0, 0, 0, 0, 0, 0, 0, 0}, 0}, //0xFA
  {NULL, 0, NULL, 0, {0, 0, 0, 0, 0, 0, 0, 0}, 0}, //0xFB
  {NULL, 0, NULL, 0, {0, 0, 0, 0, 0, 0, 0, 0}, 0}, //0xFC
  {NULL, 0, NULL, 0, {0, 0, 0, 0, 0, 0, 0, 0}, 0}, //0xFD
  {NULL, 0, NULL, 0, {0, 0, 0, 0, 0, 0, 0, 0}, 0}, //0xFE
  {NULL, 0, NULL, 0, {0, 0, 0, 0, 0, 0, 0, 0}, 0}, //0xFF
};

size_t mb_8s_m_sensor_cnt = sizeof(mb_8s_m_sensor_list)/sizeof(uint8_t);
size_t mb_4s_m_sensor_cnt = sizeof(mb_4s_m_sensor_list)/sizeof(uint8_t);
size_t mb_slv_sensor_cnt = sizeof(mb_slv_sensor_list)/sizeof(uint8_t);
size_t mb_2s_sensor_cnt = sizeof(mb_2s_sensor_list)/sizeof(uint8_t);
size_t nic0_sensor_cnt = sizeof(nic0_sensor_list)/sizeof(uint8_t);
size_t nic1_sensor_cnt = sizeof(nic1_sensor_list)/sizeof(uint8_t);
size_t mb_discrete_sensor_cnt = sizeof(mb_discrete_sensor_list)/sizeof(uint8_t);
size_t pdb_sensor_cnt = sizeof(pdb_sensor_list)/sizeof(uint8_t);

static struct fsc_monitor fsc_2s_snr_list[] =
{
  {MB_SNR_INLET_REMOTE_TEMP  , "al_mb_inlet_remote_temp" , NULL, false, 5, 5},
  {MB_SNR_CPU0_THERM_MARGIN  , "al_mb_cpu0_therm_margin" , NULL, false, 5, 5},
  {MB_SNR_CPU1_THERM_MARGIN  , "al_mb_cpu1_therm_margin" , NULL, false, 5, 5},
  {NIC_MEZZ0_SNR_TEMP        , "al_nic_mezz0_temp"       , pal_check_nic_prsnt, false, 5, 5},
  {NIC_MEZZ1_SNR_TEMP        , "al_nic_mezz1_temp"       , pal_check_nic_prsnt, false, 5, 5},
  //dimm sensors wait for 240s. 240=80*3(fsc monitor interval)
  {MB_SNR_CPU0_DIMM_GRPA_TEMP, "al_mb_cpu0_dimm_a0_c0_temp", pal_check_dimm_prsnt, false, 80, 5},
  {MB_SNR_CPU0_DIMM_GRPB_TEMP, "al_mb_cpu0_dimm_a1_c1_temp", pal_check_dimm_prsnt, false, 80, 5},
  {MB_SNR_CPU0_DIMM_GRPC_TEMP, "al_mb_cpu0_dimm_a2_c2_temp", pal_check_dimm_prsnt, false, 80, 5},
  {MB_SNR_CPU0_DIMM_GRPD_TEMP, "al_mb_cpu0_dimm_a3_c3_temp", pal_check_dimm_prsnt, false, 80, 5},
  {MB_SNR_CPU0_DIMM_GRPE_TEMP, "al_mb_cpu0_dimm_a4_c4_temp", pal_check_dimm_prsnt, false, 80, 5},
  {MB_SNR_CPU0_DIMM_GRPF_TEMP, "al_mb_cpu0_dimm_a5_c5_temp", pal_check_dimm_prsnt, false, 80, 5},
  {MB_SNR_CPU1_DIMM_GRPA_TEMP, "al_mb_cpu1_dimm_b0_d0_temp", pal_check_dimm_prsnt, false, 80, 5},
  {MB_SNR_CPU1_DIMM_GRPB_TEMP, "al_mb_cpu1_dimm_b1_d1_temp", pal_check_dimm_prsnt, false, 80, 5},
  {MB_SNR_CPU1_DIMM_GRPC_TEMP, "al_mb_cpu1_dimm_b2_d2_temp", pal_check_dimm_prsnt, false, 80, 5},
  {MB_SNR_CPU1_DIMM_GRPD_TEMP, "al_mb_cpu1_dimm_b3_d3_temp", pal_check_dimm_prsnt, false, 80, 5},
  {MB_SNR_CPU1_DIMM_GRPE_TEMP, "al_mb_cpu1_dimm_b4_d4_temp", pal_check_dimm_prsnt, false, 80, 5},
  {MB_SNR_CPU1_DIMM_GRPF_TEMP, "al_mb_cpu1_dimm_b5_d5_temp", pal_check_dimm_prsnt, false, 80, 5},
  {MB_SNR_VR_CPU0_VCCIN_TEMP, "al_mb_vr_cpu0_vccin_temp", NULL, false, 80, 5},
  {MB_SNR_VR_CPU0_VSA_TEMP,   "al_mb_vr_cpu0_vccsa_temp", NULL, false, 80, 5},
  {MB_SNR_VR_CPU0_VCCIO_TEMP, "al_mb_vr_cpu0_vccio_temp", NULL, false, 80, 5},
  {MB_SNR_VR_CPU0_VDDQ_GRPABC_TEMP, "mb_vr_cpu0_vddq_abc_temp", NULL, false, 80, 5},
  {MB_SNR_VR_CPU0_VDDQ_GRPDEF_TEMP, "mb_vr_cpu0_vddq_def_temp", NULL, false, 80, 5},
  {MB_SNR_VR_CPU1_VCCIN_TEMP, "al_mb_vr_cpu1_vccin_temp", NULL, false, 80, 5},
  {MB_SNR_VR_CPU1_VSA_TEMP,   "al_mb_vr_cpu1_vccsa_temp", NULL, false, 80, 5},
  {MB_SNR_VR_CPU1_VCCIO_TEMP, "al_mb_vr_cpu1_vccio_temp", NULL, false, 80, 5},
  {MB_SNR_VR_CPU1_VDDQ_GRPABC_TEMP, "al_mb_vr_cpu1_vddq_abc_temp", NULL, false, 80, 5},
  {MB_SNR_VR_CPU1_VDDQ_GRPDEF_TEMP, "al_mb_vr_cpu1_vddq_def_temp", NULL, false, 80, 5},
  {MB_SNR_VR_PCH_P1V05_TEMP, "al_mb_vr_pch_p1v05_temp", NULL, false, 80, 5},
  {MB_SNR_VR_PCH_PVNN_TEMP, "al_mb_vr_pch_pvnn_temp", NULL, false, 80, 5},
};

static struct fsc_monitor fsc_4s_master_snr_list[] =
{
  {MB_SNR_INLET_REMOTE_TEMP  , "al_mb_inlet_remote_temp" , NULL, false, 5, 5},
  {MB_SNR_CPU0_THERM_MARGIN  , "al_mb_cpu0_therm_margin" , NULL, false, 5, 5},
  {MB_SNR_CPU1_THERM_MARGIN  , "al_mb_cpu1_therm_margin" , NULL, false, 5, 5},
  {MB_SNR_CPU2_THERM_MARGIN  , "al_mb_cpu2_therm_margin" , NULL, false, 5, 5},
  {MB_SNR_CPU3_THERM_MARGIN  , "al_mb_cpu3_therm_margin" , NULL, false, 5, 5},
  {NIC_MEZZ0_SNR_TEMP        , "al_nic_mezz0_temp"       , pal_check_nic_prsnt, false, 5, 5},
  {NIC_MEZZ1_SNR_TEMP        , "al_nic_mezz1_temp"       , pal_check_nic_prsnt, false, 5, 5},
  //dimm sensors wait for 240s. 240=80*3(fsc monitor interval)
  {MB_SNR_CPU0_DIMM_GRPA_TEMP, "al_mb_cpu0_dimm_a0_c0_temp", pal_check_dimm_prsnt, false, 80, 5},
  {MB_SNR_CPU0_DIMM_GRPB_TEMP, "al_mb_cpu0_dimm_a1_c1_temp", pal_check_dimm_prsnt, false, 80, 5},
  {MB_SNR_CPU0_DIMM_GRPC_TEMP, "al_mb_cpu0_dimm_a2_c2_temp", pal_check_dimm_prsnt, false, 80, 5},
  {MB_SNR_CPU0_DIMM_GRPD_TEMP, "al_mb_cpu0_dimm_a3_c3_temp", pal_check_dimm_prsnt, false, 80, 5},
  {MB_SNR_CPU0_DIMM_GRPE_TEMP, "al_mb_cpu0_dimm_a4_c4_temp", pal_check_dimm_prsnt, false, 80, 5},
  {MB_SNR_CPU0_DIMM_GRPF_TEMP, "al_mb_cpu0_dimm_a5_c5_temp", pal_check_dimm_prsnt, false, 80, 5},
  {MB_SNR_CPU1_DIMM_GRPA_TEMP, "al_mb_cpu1_dimm_b0_d0_temp", pal_check_dimm_prsnt, false, 80, 5},
  {MB_SNR_CPU1_DIMM_GRPB_TEMP, "al_mb_cpu1_dimm_b1_d1_temp", pal_check_dimm_prsnt, false, 80, 5},
  {MB_SNR_CPU1_DIMM_GRPC_TEMP, "al_mb_cpu1_dimm_b2_d2_temp", pal_check_dimm_prsnt, false, 80, 5},
  {MB_SNR_CPU1_DIMM_GRPD_TEMP, "al_mb_cpu1_dimm_b3_d3_temp", pal_check_dimm_prsnt, false, 80, 5},
  {MB_SNR_CPU1_DIMM_GRPE_TEMP, "al_mb_cpu1_dimm_b4_d4_temp", pal_check_dimm_prsnt, false, 80, 5},
  {MB_SNR_CPU1_DIMM_GRPF_TEMP, "al_mb_cpu1_dimm_b5_d5_temp", pal_check_dimm_prsnt, false, 80, 5},
  {MB_SNR_CPU2_DIMM_GRPA_TEMP, "al_mb_cpu2_dimm_a0_c0_temp", pal_check_dimm_prsnt, false, 80, 5},
  {MB_SNR_CPU2_DIMM_GRPB_TEMP, "al_mb_cpu2_dimm_a1_c1_temp", pal_check_dimm_prsnt, false, 80, 5},
  {MB_SNR_CPU2_DIMM_GRPC_TEMP, "al_mb_cpu2_dimm_a2_c2_temp", pal_check_dimm_prsnt, false, 80, 5},
  {MB_SNR_CPU2_DIMM_GRPD_TEMP, "al_mb_cpu2_dimm_a3_c3_temp", pal_check_dimm_prsnt, false, 80, 5},
  {MB_SNR_CPU2_DIMM_GRPE_TEMP, "al_mb_cpu2_dimm_a4_c4_temp", pal_check_dimm_prsnt, false, 80, 5},
  {MB_SNR_CPU2_DIMM_GRPF_TEMP, "al_mb_cpu2_dimm_a5_c5_temp", pal_check_dimm_prsnt, false, 80, 5},
  {MB_SNR_CPU3_DIMM_GRPA_TEMP, "al_mb_cpu3_dimm_b0_d0_temp", pal_check_dimm_prsnt, false, 80, 5},
  {MB_SNR_CPU3_DIMM_GRPB_TEMP, "al_mb_cpu3_dimm_b1_d1_temp", pal_check_dimm_prsnt, false, 80, 5},
  {MB_SNR_CPU3_DIMM_GRPC_TEMP, "al_mb_cpu3_dimm_b2_d2_temp", pal_check_dimm_prsnt, false, 80, 5},
  {MB_SNR_CPU3_DIMM_GRPD_TEMP, "al_mb_cpu3_dimm_b3_d3_temp", pal_check_dimm_prsnt, false, 80, 5},
  {MB_SNR_CPU3_DIMM_GRPE_TEMP, "al_mb_cpu3_dimm_b4_d4_temp", pal_check_dimm_prsnt, false, 80, 5},
  {MB_SNR_CPU3_DIMM_GRPF_TEMP, "al_mb_cpu3_dimm_b5_d5_temp", pal_check_dimm_prsnt, false, 80, 5},
  {MB_SNR_VR_CPU0_VCCIN_TEMP, "al_mb_cpu0_vr_vccin_temp", NULL, false, 80, 5},
  {MB_SNR_VR_CPU0_VCCIN_TEMP, "al_mb_vr_cpu0_vccin_temp", NULL, false, 80, 5},
  {MB_SNR_VR_CPU0_VSA_TEMP,   "al_mb_vr_cpu0_vccsa_temp", NULL, false, 80, 5},
  {MB_SNR_VR_CPU0_VCCIO_TEMP, "al_mb_vr_cpu0_vccio_temp", NULL, false, 80, 5},
  {MB_SNR_VR_CPU0_VDDQ_GRPABC_TEMP, "al_mb_vr_cpu0_vddq_abc_temp", NULL, false, 80, 5},
  {MB_SNR_VR_CPU0_VDDQ_GRPDEF_TEMP, "al_mb_vr_cpu0_vddq_def_temp", NULL, false, 80, 5},
  {MB_SNR_VR_CPU1_VCCIN_TEMP, "al_mb_vr_cpu1_vccin_temp", NULL, false, 80, 5},
  {MB_SNR_VR_CPU1_VSA_TEMP,   "al_mb_vr_cpu1_vccsa_temp", NULL, false, 80, 5},
  {MB_SNR_VR_CPU1_VCCIO_TEMP, "al_mb_vr_cpu1_vccio_temp", NULL, false, 80, 5},
  {MB_SNR_VR_CPU1_VDDQ_GRPABC_TEMP, "al_mb_vr_cpu1_vddq_abc_temp", NULL, false, 80, 5},
  {MB_SNR_VR_CPU1_VDDQ_GRPDEF_TEMP, "al_mb_vr_cpu1_vddq_def_temp", NULL, false, 80, 5},
  {MB_SNR_VR_PCH_P1V05_TEMP, "al_mb_vr_pch_p1v05_temp", NULL, false, 80, 5},
  {MB_SNR_VR_PCH_PVNN_TEMP, "al_mb_vr_pch_pvnn_temp", NULL, false, 80, 5},
};

static struct fsc_monitor fsc_8s_master_snr_list[] =
{
  {MB_SNR_INLET_TEMP         , "al_mb_inlet_temp"        , NULL, false, 5, 5},
  {MB_SNR_CPU0_THERM_MARGIN  , "al_mb_cpu0_therm_margin" , NULL, false, 5, 5},
  {MB_SNR_CPU1_THERM_MARGIN  , "al_mb_cpu1_therm_margin" , NULL, false, 5, 5},
  {MB_SNR_CPU2_THERM_MARGIN  , "al_mb_cpu2_therm_margin" , NULL, false, 5, 5},
  {MB_SNR_CPU3_THERM_MARGIN  , "al_mb_cpu3_therm_margin" , NULL, false, 5, 5},
  {MB_SNR_CPU4_THERM_MARGIN  , "al_mb_cpu4_therm_margin" , NULL, false, 5, 5},
  {MB_SNR_CPU5_THERM_MARGIN  , "al_mb_cpu5_therm_margin" , NULL, false, 5, 5},
  {MB_SNR_CPU6_THERM_MARGIN  , "al_mb_cpu6_therm_margin" , NULL, false, 5, 5},
  {MB_SNR_CPU7_THERM_MARGIN  , "al_mb_cpu7_therm_margin" , NULL, false, 5, 5},
  {NIC_MEZZ0_SNR_TEMP        , "al_nic_mezz0_temp"       , pal_check_nic_prsnt, false, 5, 5},
  {NIC_MEZZ1_SNR_TEMP        , "al_nic_mezz1_temp"       , pal_check_nic_prsnt, false, 5, 5},
  //dimm sensors wait for 240s. 240=80*3(fsc monitor interval)
  {MB_SNR_CPU0_DIMM_GRPA_TEMP, "al_mb_cpu0_dimm_a0_c0_temp", pal_check_dimm_prsnt, false, 80, 5},
  {MB_SNR_CPU0_DIMM_GRPB_TEMP, "al_mb_cpu0_dimm_a1_c1_temp", pal_check_dimm_prsnt, false, 80, 5},
  {MB_SNR_CPU0_DIMM_GRPC_TEMP, "al_mb_cpu0_dimm_a2_c2_temp", pal_check_dimm_prsnt, false, 80, 5},
  {MB_SNR_CPU0_DIMM_GRPD_TEMP, "al_mb_cpu0_dimm_a3_c3_temp", pal_check_dimm_prsnt, false, 80, 5},
  {MB_SNR_CPU0_DIMM_GRPE_TEMP, "al_mb_cpu0_dimm_a4_c4_temp", pal_check_dimm_prsnt, false, 80, 5},
  {MB_SNR_CPU0_DIMM_GRPF_TEMP, "al_mb_cpu0_dimm_a5_c5_temp", pal_check_dimm_prsnt, false, 80, 5},
  {MB_SNR_CPU1_DIMM_GRPA_TEMP, "al_mb_cpu1_dimm_b0_d0_temp", pal_check_dimm_prsnt, false, 80, 5},
  {MB_SNR_CPU1_DIMM_GRPB_TEMP, "al_mb_cpu1_dimm_b1_d1_temp", pal_check_dimm_prsnt, false, 80, 5},
  {MB_SNR_CPU1_DIMM_GRPC_TEMP, "al_mb_cpu1_dimm_b2_d2_temp", pal_check_dimm_prsnt, false, 80, 5},
  {MB_SNR_CPU1_DIMM_GRPD_TEMP, "al_mb_cpu1_dimm_b3_d3_temp", pal_check_dimm_prsnt, false, 80, 5},
  {MB_SNR_CPU1_DIMM_GRPE_TEMP, "al_mb_cpu1_dimm_b4_d4_temp", pal_check_dimm_prsnt, false, 80, 5},
  {MB_SNR_CPU1_DIMM_GRPF_TEMP, "al_mb_cpu1_dimm_b5_d5_temp", pal_check_dimm_prsnt, false, 80, 5},
  {MB_SNR_CPU2_DIMM_GRPA_TEMP, "al_mb_cpu2_dimm_a0_c0_temp", pal_check_dimm_prsnt, false, 80, 5},
  {MB_SNR_CPU2_DIMM_GRPB_TEMP, "al_mb_cpu2_dimm_a1_c1_temp", pal_check_dimm_prsnt, false, 80, 5},
  {MB_SNR_CPU2_DIMM_GRPC_TEMP, "al_mb_cpu2_dimm_a2_c2_temp", pal_check_dimm_prsnt, false, 80, 5},
  {MB_SNR_CPU2_DIMM_GRPD_TEMP, "al_mb_cpu2_dimm_a3_c3_temp", pal_check_dimm_prsnt, false, 80, 5},
  {MB_SNR_CPU2_DIMM_GRPE_TEMP, "al_mb_cpu2_dimm_a4_c4_temp", pal_check_dimm_prsnt, false, 80, 5},
  {MB_SNR_CPU2_DIMM_GRPF_TEMP, "al_mb_cpu2_dimm_a5_c5_temp", pal_check_dimm_prsnt, false, 80, 5},
  {MB_SNR_CPU3_DIMM_GRPA_TEMP, "al_mb_cpu3_dimm_b0_d0_temp", pal_check_dimm_prsnt, false, 80, 5},
  {MB_SNR_CPU3_DIMM_GRPB_TEMP, "al_mb_cpu3_dimm_b1_d1_temp", pal_check_dimm_prsnt, false, 80, 5},
  {MB_SNR_CPU3_DIMM_GRPC_TEMP, "al_mb_cpu3_dimm_b2_d2_temp", pal_check_dimm_prsnt, false, 80, 5},
  {MB_SNR_CPU3_DIMM_GRPD_TEMP, "al_mb_cpu3_dimm_b3_d3_temp", pal_check_dimm_prsnt, false, 80, 5},
  {MB_SNR_CPU3_DIMM_GRPE_TEMP, "al_mb_cpu3_dimm_b4_d4_temp", pal_check_dimm_prsnt, false, 80, 5},
  {MB_SNR_CPU3_DIMM_GRPF_TEMP, "al_mb_cpu3_dimm_b5_d5_temp", pal_check_dimm_prsnt, false, 80, 5},
  {MB_SNR_CPU4_DIMM_GRPA_TEMP, "al_mb_cpu4_dimm_a0_c0_temp", pal_check_dimm_prsnt, false, 80, 5},
  {MB_SNR_CPU4_DIMM_GRPB_TEMP, "al_mb_cpu4_dimm_a1_c1_temp", pal_check_dimm_prsnt, false, 80, 5},
  {MB_SNR_CPU4_DIMM_GRPC_TEMP, "al_mb_cpu4_dimm_a2_c2_temp", pal_check_dimm_prsnt, false, 80, 5},
  {MB_SNR_CPU4_DIMM_GRPD_TEMP, "al_mb_cpu4_dimm_a3_c3_temp", pal_check_dimm_prsnt, false, 80, 5},
  {MB_SNR_CPU4_DIMM_GRPE_TEMP, "al_mb_cpu4_dimm_a4_c4_temp", pal_check_dimm_prsnt, false, 80, 5},
  {MB_SNR_CPU4_DIMM_GRPF_TEMP, "al_mb_cpu4_dimm_a5_c5_temp", pal_check_dimm_prsnt, false, 80, 5},
  {MB_SNR_CPU5_DIMM_GRPA_TEMP, "al_mb_cpu5_dimm_b0_d0_temp", pal_check_dimm_prsnt, false, 80, 5},
  {MB_SNR_CPU5_DIMM_GRPB_TEMP, "al_mb_cpu5_dimm_b1_d1_temp", pal_check_dimm_prsnt, false, 80, 5},
  {MB_SNR_CPU5_DIMM_GRPC_TEMP, "al_mb_cpu5_dimm_b2_d2_temp", pal_check_dimm_prsnt, false, 80, 5},
  {MB_SNR_CPU5_DIMM_GRPD_TEMP, "al_mb_cpu5_dimm_b3_d3_temp", pal_check_dimm_prsnt, false, 80, 5},
  {MB_SNR_CPU5_DIMM_GRPE_TEMP, "al_mb_cpu5_dimm_b4_d4_temp", pal_check_dimm_prsnt, false, 80, 5},
  {MB_SNR_CPU5_DIMM_GRPF_TEMP, "al_mb_cpu5_dimm_b5_d5_temp", pal_check_dimm_prsnt, false, 80, 5},
  {MB_SNR_CPU6_DIMM_GRPA_TEMP, "al_mb_cpu6_dimm_a0_c0_temp", pal_check_dimm_prsnt, false, 80, 5},
  {MB_SNR_CPU6_DIMM_GRPB_TEMP, "al_mb_cpu6_dimm_a1_c1_temp", pal_check_dimm_prsnt, false, 80, 5},
  {MB_SNR_CPU6_DIMM_GRPC_TEMP, "al_mb_cpu6_dimm_a2_c2_temp", pal_check_dimm_prsnt, false, 80, 5},
  {MB_SNR_CPU6_DIMM_GRPD_TEMP, "al_mb_cpu6_dimm_a3_c3_temp", pal_check_dimm_prsnt, false, 80, 5},
  {MB_SNR_CPU6_DIMM_GRPE_TEMP, "al_mb_cpu6_dimm_a4_c4_temp", pal_check_dimm_prsnt, false, 80, 5},
  {MB_SNR_CPU6_DIMM_GRPF_TEMP, "al_mb_cpu6_dimm_a5_c5_temp", pal_check_dimm_prsnt, false, 80, 5},
  {MB_SNR_CPU7_DIMM_GRPA_TEMP, "al_mb_cpu7_dimm_b0_d0_temp", pal_check_dimm_prsnt, false, 80, 5},
  {MB_SNR_CPU7_DIMM_GRPB_TEMP, "al_mb_cpu7_dimm_b1_d1_temp", pal_check_dimm_prsnt, false, 80, 5},
  {MB_SNR_CPU7_DIMM_GRPC_TEMP, "al_mb_cpu7_dimm_b2_d2_temp", pal_check_dimm_prsnt, false, 80, 5},
  {MB_SNR_CPU7_DIMM_GRPD_TEMP, "al_mb_cpu7_dimm_b3_d3_temp", pal_check_dimm_prsnt, false, 80, 5},
  {MB_SNR_CPU7_DIMM_GRPE_TEMP, "al_mb_cpu7_dimm_b4_d4_temp", pal_check_dimm_prsnt, false, 80, 5},
  {MB_SNR_CPU7_DIMM_GRPF_TEMP, "al_mb_cpu7_dimm_b5_d5_temp", pal_check_dimm_prsnt, false, 80, 5},
  {MB_SNR_VR_CPU0_VCCIN_TEMP, "al_mb_vr_cpu0_vccin_temp", NULL, false, 80, 5},
  {MB_SNR_VR_CPU0_VSA_TEMP,   "al_mb_vr_cpu0_vccsa_temp", NULL, false, 80, 5},
  {MB_SNR_VR_CPU0_VCCIO_TEMP, "al_mb_vr_cpu0_vccio_temp", NULL, false, 80, 5},
  {MB_SNR_VR_CPU0_VDDQ_GRPABC_TEMP, "al_mb_vr_cpu0_vddq_abc_temp", NULL, false, 80, 5},
  {MB_SNR_VR_CPU0_VDDQ_GRPDEF_TEMP, "al_mb_vr_cpu0_vddq_def_temp", NULL, false, 80, 5},
  {MB_SNR_VR_CPU1_VCCIN_TEMP, "al_mb_vr_cpu1_vccin_temp", NULL, false, 80, 5},
  {MB_SNR_VR_CPU1_VSA_TEMP,   "al_mb_vr_cpu1_vccsa_temp", NULL, false, 80, 5},
  {MB_SNR_VR_CPU1_VCCIO_TEMP, "al_mb_vr_cpu1_vccio_temp", NULL, false, 80, 5},
  {MB_SNR_VR_CPU1_VDDQ_GRPABC_TEMP, "al_mb_vr_cpu1_vddq_abc_temp", NULL, false, 80, 5},
  {MB_SNR_VR_CPU1_VDDQ_GRPDEF_TEMP, "al_mb_vr_cpu1_vddq_def_temp", NULL, false, 80, 5},
  {MB_SNR_VR_PCH_P1V05_TEMP, "al_mb_vr_pch_p1v05_temp", NULL, false, 80, 5},
  {MB_SNR_VR_PCH_PVNN_TEMP, "al_mb_vr_pch_pvnn_temp", NULL, false, 80, 5},
};

static struct fsc_monitor fsc_slave_snr_list[] =
{
  {MB_SNR_INLET_REMOTE_TEMP, "al_mb_inlet_remote_temp" , NULL, false, 5, 5},
  {NIC_MEZZ0_SNR_TEMP, "al_nic_mezz0_temp", pal_check_nic_prsnt, false, 5, 5},
  {NIC_MEZZ1_SNR_TEMP, "al_nic_mezz1_temp", pal_check_nic_prsnt, false, 5, 5},
  {MB_SNR_VR_CPU0_VCCIN_TEMP, "al_mb_vr_cpu0_vccin_temp", NULL, false, 80, 5},
  {MB_SNR_VR_CPU0_VSA_TEMP,   "al_mb_vr_cpu0_vccsa_temp", NULL, false, 80, 5},
  {MB_SNR_VR_CPU0_VCCIO_TEMP, "al_mb_vr_cpu0_vccio_temp", NULL, false, 80, 5},
  {MB_SNR_VR_CPU0_VDDQ_GRPABC_TEMP, "al_mb_vr_cpu0_vddq_abc_temp", NULL, false, 80, 5},
  {MB_SNR_VR_CPU0_VDDQ_GRPDEF_TEMP, "al_mb_vr_cpu0_vddq_def_temp", NULL, false, 80, 5},
  {MB_SNR_VR_CPU1_VCCIN_TEMP, "al_mb_vr_cpu1_vccin_temp", NULL, false, 80, 5},
  {MB_SNR_VR_CPU1_VSA_TEMP,   "al_mb_vr_cpu1_vccsa_temp", NULL, false, 80, 5},
  {MB_SNR_VR_CPU1_VCCIO_TEMP, "al_mb_vr_cpu1_vccio_temp", NULL, false, 80, 5},
  {MB_SNR_VR_CPU1_VDDQ_GRPABC_TEMP, "al_mb_vr_cpu1_vddq_abc_temp", NULL, false, 80, 5},
  {MB_SNR_VR_CPU1_VDDQ_GRPDEF_TEMP, "al_mb_vr_cpu1_vddq_def_temp", NULL, false, 80, 5},
  {MB_SNR_VR_PCH_P1V05_TEMP, "al_mb_vr_pch_p1v05_temp", NULL, false, 80, 5},
  {MB_SNR_VR_PCH_PVNN_TEMP, "al_mb_vr_pch_pvnn_temp", NULL, false, 80, 5},
};

static int fsc_8s_master_snr_list_size = sizeof(fsc_8s_master_snr_list) / sizeof(struct fsc_monitor);
static int fsc_4s_master_snr_list_size = sizeof(fsc_4s_master_snr_list) / sizeof(struct fsc_monitor);
static int fsc_2s_snr_list_size = sizeof(fsc_2s_snr_list) / sizeof(struct fsc_monitor);
static int fsc_slave_snr_list_size = sizeof(fsc_slave_snr_list) / sizeof(struct fsc_monitor);

int
pal_fsc_get_target_snr(char *sname, struct fsc_monitor *fsc_fru_list, int fsc_fru_list_size)
{
  int i;
  for ( i=0;  i<fsc_fru_list_size; i++) {
    if ( 0 == strcmp(sname, fsc_fru_list[i].sensor_name) ) {
#ifdef FSC_DEBUG
      syslog(LOG_WARNING,"[%s]sensor is found:%s, idx:%d", __func__, sname, i);
#endif
      return i;
    }
  }

  syslog(LOG_WARNING,"[%s]Unknown sensor name:%s", __func__, sname);
  return PAL_ENOTSUP;
}

bool
pal_sensor_is_valid(char *fru_name, char *sensor_name)
{
  uint8_t fru_id;
  struct fsc_monitor *fsc_fru_list;
  int fsc_fru_list_size;
  int ret;
  uint8_t mode;
  uint8_t master = pal_get_config_is_master();

  //check the fru name is valid or not
  ret = pal_get_fru_id(fru_name, &fru_id);
  if ( ret < 0 ) {
    syslog(LOG_WARNING,"[%s] Wrong fru#%s", __func__, fru_name);
    return false;
  }

  ret = pal_get_host_system_mode(&mode);
  if (ret != 0) {
    syslog(LOG_WARNING,"%s Wrong get system mode\n", __func__);
    return false;
  }

  if( mode == MB_2S_MODE ) {
    //2S Master
    fsc_fru_list = fsc_2s_snr_list;
    fsc_fru_list_size = fsc_2s_snr_list_size;
  } else if(mode == MB_4S_MODE && master ==true) {
    //4S Master
    fsc_fru_list = fsc_4s_master_snr_list;
    fsc_fru_list_size = fsc_4s_master_snr_list_size;
  } else if(mode == MB_8S_MODE && master ==true) {
    //8S Master
    fsc_fru_list = fsc_8s_master_snr_list;
    fsc_fru_list_size = fsc_8s_master_snr_list_size;
  } else {
    //Slave
    fsc_fru_list = fsc_slave_snr_list;
    fsc_fru_list_size = fsc_slave_snr_list_size;
  }
  //get the target sensor
  ret = pal_fsc_get_target_snr(sensor_name, fsc_fru_list, fsc_fru_list_size);
  if ( ret < 0 ) {
    syslog(LOG_WARNING,"[%s] undefined sensor: %s", __func__, sensor_name);
    return false;
  }

  return true;
}

int
pal_get_fru_sensor_list(uint8_t fru, uint8_t **sensor_list, int *cnt) {
  int ret=0;
  uint8_t mode;
  uint8_t master;

  ret = pal_get_host_system_mode(&mode);
  if (ret != 0) {
    return ret;
  }
  master = pal_get_config_is_master();

  switch(fru) {
  case FRU_MB:
    if (mode == MB_2S_MODE ) {
      //2S Mode
      *sensor_list = (uint8_t *) mb_2s_sensor_list;
      *cnt = mb_2s_sensor_cnt;
    } else if(mode == MB_4S_MODE && master == true) {
      //4S Master Mode
      *sensor_list = (uint8_t *) mb_4s_m_sensor_list;
      *cnt = mb_4s_m_sensor_cnt;
    } else if(mode == MB_8S_MODE && master == true) {
      //8S Master Mode
      *sensor_list = (uint8_t *) mb_8s_m_sensor_list;
      *cnt = mb_8s_m_sensor_cnt;
    } else {
      //8S or 4S Slave Mode
      *sensor_list = (uint8_t *) mb_slv_sensor_list;
      *cnt = mb_slv_sensor_cnt;
    }
    break;

  case FRU_NIC0:
    *sensor_list = (uint8_t *) nic0_sensor_list;
    *cnt = nic0_sensor_cnt;
    break;

  case FRU_NIC1:
    *sensor_list = (uint8_t *) nic1_sensor_list;
    *cnt = nic1_sensor_cnt;
    break;

  case FRU_PDB:
    *sensor_list = (uint8_t *) pdb_sensor_list;
    *cnt = pdb_sensor_cnt;
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
  uint8_t master;
  master = pal_get_config_is_master();


  switch(fru) {
  case FRU_MB:
    if (master == true) {
      *sensor_list = (uint8_t *) mb_discrete_sensor_list;
      *cnt = mb_discrete_sensor_cnt;
    } else {
      *sensor_list = NULL;
      *cnt = 0;
    }
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

static int
get_cm_snr_num(uint8_t id) {
  int snr_num=0;

  snr_num = cm_snr_head_list[id].num;
  return snr_num;
}

bool
check_cpu_present_pin_gpio(uint8_t cpu_id) {
  int ret;
  gpio_desc_t *gdesc = NULL;
  gpio_value_t val;
  uint8_t status;

  if(cpu_id == 0)
    gdesc = gpio_open_by_shadow(GPIO_CPU0_PRESENT);
  else
    gdesc = gpio_open_by_shadow(GPIO_CPU1_PRESENT);

  if (gdesc == NULL)
    return -1;

  ret = gpio_get_value(gdesc, &val);

  gpio_close(gdesc);

  if(ret == 0)
    status = (int)val;
  else
      return -1;

  return status;
}

bool
check_cpu_present(const char *label) {
  uint8_t cpu_id;

  const char *cpu0_vr_sensor_list[] = {
    "MB_CPU0_PVPP_ABC",
    "MB_CPU0_PVPP_DEF",
    "MB_CPU0_PVTT_ABC",
    "MB_CPU0_PVTT_DEF",
    "MB_VR_CPU0_VCCIN_VOUT",
    "MB_VR_CPU0_VCCSA_VOUT",
    "MB_VR_CPU0_VCCIO_VOUT",
    "MB_VR_CPU0_VDDQ_ABC_VOUT",
    "MB_VR_CPU0_VDDQ_DEF_VOUT",
  };

  const char *cpu1_vr_sensor_list[] = {
    "MB_CPU1_PVPP_ABC",
    "MB_CPU1_PVPP_DEF",
    "MB_CPU1_PVTT_ABC",
    "MB_CPU1_PVTT_DEF",
    "MB_VR_CPU1_VCCIN_VOUT",
    "MB_VR_CPU1_VCCSA_VOUT",
    "MB_VR_CPU1_VCCIO_VOUT",
    "MB_VR_CPU1_VDDQ_ABC_VOUT",
    "MB_VR_CPU1_VDDQ_DEF_VOUT",
  };

  for(int i = 0; i < ARRAY_SIZE(cpu0_vr_sensor_list); i++) {
    if(!strcmp(label, cpu0_vr_sensor_list[i])) {
      cpu_id = 0;
      return !check_cpu_present_pin_gpio(cpu_id);
    }
  }

  for(int i = 0; i < ARRAY_SIZE(cpu1_vr_sensor_list); i++) {
    if(!strcmp(label, cpu1_vr_sensor_list[i])) {
      cpu_id = 1;
      return !check_cpu_present_pin_gpio(cpu_id);
    }
  }

  return true;
}

static int
read_cm_sensor(uint8_t id, float *value) {
  int sdr=0;
  int ret = 0;
  uint8_t rlen=0;
  uint8_t rbuf[16];
  static uint8_t retry=0;

  sdr = get_cm_snr_num(id);
  ret = cmd_cmc_get_sensor_value(sdr, rbuf, &rlen);
  if (ret != 0) {
    retry++;
    if (retry <= 3) {
      return READING_SKIP;
    }
    return READING_NA;
  } else {
    retry = 0;
  }

  *value = (float)(rbuf[0] | rbuf[1]<<8) + (float)(rbuf[2] | rbuf[3]<<8)/1000;
#ifdef DEBUG
{
  for(int i=0; i<*rlen; i++) {
    syslog(LOG_DEBUG, "%s buf[%d]=%d\n", __func__, i, rbuf[i]);
  }

  syslog(LOG_DEBUG, "%s value=%f\n", __func__, *value);
}
#endif
  return 0;
}


/*==========================================
Read adc voltage sensor value.
  adc_id: ASPEED adc channel
  *value: real voltage
  return: error code
============================================*/
static int
read_adc_val(uint8_t adc_id, float *value) {
  const char *adc_label[] = {
    "MB_P5V",
    "MB_P5V_STBY",
    "MB_P3V3_STBY",
    "MB_P3V3",
    "MB_P3V_BAT",
    "MB_CPU_P1V8",
    "MB_PCH_P1V8",
    "MB_CPU0_PVPP_ABC",
    "MB_CPU1_PVPP_ABC",
    "MB_CPU0_PVPP_DEF",
    "MB_CPU1_PVPP_DEF",
    "MB_CPU0_PVTT_ABC",
    "MB_CPU1_PVTT_ABC",
    "MB_CPU0_PVTT_DEF",
    "MB_CPU1_PVTT_DEF",
  };
  if (adc_id >= ARRAY_SIZE(adc_label)) {
    return -1;
  } else if (!check_cpu_present(adc_label[adc_id])) {
    return READING_NA;
  }
  return sensors_read_adc(adc_label[adc_id], value);
}

static int
read_battery_val(uint8_t adc_id, float *value) {
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
  msleep(10);
  ret = read_adc_val(adc_id, value);
  if (gpio_set_value(gp_batt, GPIO_VALUE_LOW)) {
    goto bail;
  }

bail:
  gpio_close(gp_batt);
  return ret;
}

static int
cmd_peci_rdpkgconfig(PECI_RD_PKG_CONFIG_INFO* info, uint8_t* rx_buf, uint8_t rx_len) {
  int ret;
  uint8_t tbuf[PECI_BUFFER_SIZE];

  tbuf[0] = PECI_CMD_RD_PKG_CONFIG;
  tbuf[1] = info->dev_info;
  tbuf[2] = info->index;
  tbuf[3] = info->para_l;
  tbuf[4] = info->para_h;
  ret = peci_raw(info->cpu_addr, rx_len, tbuf, 5, rx_buf, rx_len);
  if (ret != PECI_CC_SUCCESS) {
#ifdef DEBUG
    syslog(LOG_WARNING, "%s err addr=0x%x index=%x, cc=0x%x",
                        __func__, info->cpu_addr, info->index, ret);
#endif
    return -1;
  }

  return 0;
}

int
cmd_peci_get_thermal_margin(uint8_t cpu_addr, float* value) {
  PECI_RD_PKG_CONFIG_INFO info;
  int ret;
  uint8_t rx_len=5;
  uint8_t rx_buf[rx_len];
  int16_t tmp;

  info.cpu_addr = cpu_addr;
  info.dev_info = 0x00;
  info.index = PECI_INDEX_PKG_TEMP;
  info.para_l = 0x00;
  info.para_h = 0x00;

  ret = cmd_peci_rdpkgconfig(&info, rx_buf, rx_len);
  if (ret != 0) {
    return -1;
  }

  tmp = (rx_buf[2] << 8) | rx_buf[1];
#ifdef DEBUG
  syslog(LOG_DEBUG, "%s rxbuf[1]=%x, rxbuf[2]=%x, tmp=%x", __func__, rx_buf[1], rx_buf[2], tmp);
#endif
  if (tmp <= (int16_t)0x81ff) {  // 0x8000 ~ 0x81ff for error code
    return -1;
  }

  *value = (float)(tmp >> 6);
  return 0;
}

static int
cmd_peci_get_tjmax(uint8_t cpu_addr, int* tjmax) {
  PECI_RD_PKG_CONFIG_INFO info;
  int ret;
  uint8_t rx_len=5;
  uint8_t rx_buf[rx_len];

  info.cpu_addr = cpu_addr;
  info.dev_info = 0x00;
  info.index = PECI_INDEX_TEMP_TARGET;
  info.para_l = 0x00;
  info.para_h = 0x00;

  ret = cmd_peci_rdpkgconfig(&info, rx_buf, rx_len);
  if (ret != 0) {
    return -1;
  }

  *tjmax = rx_buf[3];
  return 0;
}

static int
cmd_peci_dimm_thermal_reading(uint8_t cpu_addr, uint8_t channel, uint8_t* temp) {
  PECI_RD_PKG_CONFIG_INFO info;
  int ret;
  uint8_t rx_len=5;
  uint8_t rx_buf[rx_len];

  info.cpu_addr = cpu_addr;
  info.dev_info = 0x00;
  info.index = PECI_INDEX_DIMM_THERMAL_RW;
  info.para_l = channel;
  info.para_h = 0x00;

  ret = cmd_peci_rdpkgconfig(&info, rx_buf, rx_len);
  if (ret != 0) {
    return -1;
  }

  if(rx_buf[PECI_THERMAL_DIMM0_BYTE] > rx_buf[PECI_THERMAL_DIMM1_BYTE]) {
    *temp = rx_buf[PECI_THERMAL_DIMM0_BYTE];
  } else {
    *temp = rx_buf[PECI_THERMAL_DIMM1_BYTE];
  }
  return 0;
}

static int
cmd_peci_accumulated_energy(uint8_t cpu_addr, uint32_t* value) {
  PECI_RD_PKG_CONFIG_INFO info;
  int ret=0;
  int i=0;
  uint8_t rx_len=5;
  uint8_t rx_buf[rx_len];

  info.cpu_addr= cpu_addr;
  info.dev_info = 0x00;
  info.index = PECI_INDEX_ACCUMULATED_ENERGY_STATUS;
  info.para_l = 0xFF;
  info.para_h = 0x00;

  ret = cmd_peci_rdpkgconfig(&info, rx_buf, rx_len);
  if (ret != 0) {
    return -1;
  }

  for (i=0; i<4; i++) {
    *value |= rx_buf[i] << (8*i);
  }
  return 0;
}

static int
cmd_peci_total_time(uint8_t cpu_addr, uint32_t* value) {
  PECI_RD_PKG_CONFIG_INFO info;
  int ret=0;
  int i=0;
  uint8_t rx_len=5;
  uint8_t rx_buf[rx_len];

  info.cpu_addr= cpu_addr;
  info.dev_info = 0x00;
  info.index = PECI_INDEX_TOTAL_TIME;
  info.para_l = 0x00;
  info.para_h = 0x00;

  ret = cmd_peci_rdpkgconfig(&info, rx_buf, rx_len);
  if (ret != 0) {
    return -1;
  }

  for (i=0; i<4; i++) {
    *value |= rx_buf[i] << (8*i);
  }
  return 0;
}

int
cmd_peci_get_cpu_err_num(int* num, uint8_t is_caterr) {
  PECI_RD_PKG_CONFIG_INFO info;
  int ret=0;
  uint8_t rx_len=5;
  uint8_t rx_buf[rx_len];
  int cpu_num=-1;

  info.cpu_addr= PECI_CPU0_ADDR;
  info.dev_info = 0x00;
  info.index = PECI_INDEX_PKG_IDENTIFIER;
  info.para_l = 0x05;
  info.para_h = 0x00;

  ret = cmd_peci_rdpkgconfig(&info, rx_buf, rx_len);
  if (ret != 0) {
    return -1;
  }

//CATERR ERROR
  if (is_caterr) {
    if ((rx_buf[3] & BOTH_CPU_ERROR_MASK) == BOTH_CPU_ERROR_MASK)
      cpu_num = 2; //Both
    else if ((rx_buf[3] & EXTERNAL_CPU_ERROR_MASK) > 0)
      cpu_num = 1; //CPU1
    else if ((rx_buf[3] & INTERNAL_CPU_ERROR_MASK) > 0)
      cpu_num = 0; //CPU0
  } else {
//MSMI
    if (((rx_buf[2] & BOTH_CPU_ERROR_MASK) == BOTH_CPU_ERROR_MASK))
      cpu_num = 2; //Both
    else if ((rx_buf[2] & EXTERNAL_CPU_ERROR_MASK) > 0)
      cpu_num = 1; //CPU1
    else if ((rx_buf[2] & INTERNAL_CPU_ERROR_MASK) > 0)
      cpu_num = 0; //CPU0
  }
  *num = cpu_num;
#ifdef DEBUG
  syslog(LOG_DEBUG,"%s cpu error number=%x\n",__func__, *num);
#endif
  return 0;
}

static int
read_cpu_tjmax(uint8_t cpu_id, float *value) {
  if (!m_TjMax[cpu_id]) {
    return READING_NA;
  }

  *value = (float)m_TjMax[cpu_id];
  return 0;
}

static int
read_cpu_thermal_margin(uint8_t cpu_id, float *value) {
  if (m_Dts[cpu_id] > 0) {
    return READING_NA;
  }

  *value = m_Dts[cpu_id];
  return 0;
}

static int
read_cpu_pkg_pwr(uint8_t cpu_id, float *value) {
  uint8_t rbuf[32] = {0x00};
  uint8_t domain_id = NMS_SIGNAL_COMPONENT | NMS_DOMAIN_ID_CPU_SUBSYSTEM;
  uint8_t policy_id = cpu_id;
  static int retry = 0;
  int ret = 0;
  NM_RW_INFO info;

  get_nm_rw_info(NM_ID0, &info.bus, &info.nm_addr, &info.bmc_addr);

  ret = cmd_NM_get_nm_statistics(info, NMS_GLOBAL_POWER, domain_id, policy_id, rbuf);

  if (ret != 0) {
    retry++;
    if (retry <= 5) {
      return READING_SKIP;
    }
    return READING_NA;
  }

  if (rbuf[6] == 0) {
    *value = rbuf[10];
//#ifdef DEBUG
    syslog(LOG_DEBUG, "%s value_l=%f\n", __func__, *value);
//#endif
    retry = 0;
  } else {
#ifdef DEBUG
    syslog(LOG_DEBUG, "%s cc=%x\n", __func__, rbuf[6]);
#endif
    retry++;
    if (retry <= 5) {
      return READING_SKIP;
    }
    return READING_NA;
  }

  return 0;
}

static int
read_cpu_temp(uint8_t cpu_id, float *value) {
  uint8_t cpu_addr = cpu_info_list[cpu_id].cpu_addr;
  int ret, tjmax;
  float dts;
  static uint8_t retry[CPU_ID_NUM] = {0};

  if (!m_TjMax[cpu_id]) {
    ret = cmd_peci_get_tjmax(cpu_addr, &tjmax);
    if (!ret) {
      m_TjMax[cpu_id] = tjmax;
    }
  }

  ret = cmd_peci_get_thermal_margin(cpu_addr, &dts);
  if (ret != 0) {
    retry[cpu_id]++;
    if (retry[cpu_id] <= 3) {
      return READING_SKIP;
    }
    m_Dts[cpu_id] = 0xFF;
    return READING_NA;
  } else {
    retry[cpu_id] = 0;
    m_Dts[cpu_id] = dts;
  }

  if (!m_TjMax[cpu_id]) {
    return READING_NA;
  }

  *value = (float)m_TjMax[cpu_id] + dts;
#ifdef DEBUG
  syslog(LOG_DEBUG, "%s cpu_id=%d value=%f", __func__, cpu_id, *value);
#endif
  return 0;
}

static int
get_dimm_temp(uint8_t cpu_addr, uint8_t dimm_id, float *value) {
  static int retry = 0;
  uint8_t temp;
  int ret;

  if(pal_bios_completed(FRU_MB) != true) {
    return READING_NA;
  }

  ret = cmd_peci_dimm_thermal_reading(cpu_addr, dimm_id, &temp);
  if (ret != 0) {
    retry++;
    if (retry <= 3) {
      return READING_SKIP;
    }
    return READING_NA;
  } else {
    retry = 0;
  }

  *value = (float)temp;
  return 0;
}

static int
read_cpu0_dimm_temp(uint8_t dimm_id, float *value) {
  int ret;

  ret = get_dimm_temp(PECI_CPU0_ADDR, dimm_id, value);
  if (ret != 0) {
    return ret;
  }
#ifdef DEBUG
  syslog(LOG_DEBUG, "%s DIMM Temp=%f id=%d\n", __func__, *value, dimm_id);
#endif
  return 0;
}

static int
read_cpu1_dimm_temp(uint8_t dimm_id, float *value) {
  int ret;

  ret = get_dimm_temp(PECI_CPU1_ADDR, dimm_id, value);
  if (ret != 0) {
    return ret;
  }
#ifdef DEBUG
  syslog(LOG_DEBUG, "%s DIMM Temp=%f id=%d\n", __func__, *value, dimm_id);
#endif
  return 0;
}

static int
read_cpu2_dimm_temp(uint8_t dimm_id, float *value) {
  int ret;

  ret = get_dimm_temp(PECI_CPU2_ADDR, dimm_id, value);
  if (ret != 0) {
    return ret;
  }
#ifdef DEBUG
  syslog(LOG_DEBUG, "%s DIMM Temp=%f id=%d\n", __func__, *value, dimm_id);
#endif
  return 0;
}

static int
read_cpu3_dimm_temp(uint8_t dimm_id, float *value) {
  int ret;

  ret = get_dimm_temp(PECI_CPU3_ADDR, dimm_id, value);
  if (ret != 0) {
    return ret;
  }
#ifdef DEBUG
  syslog(LOG_DEBUG, "%s DIMM Temp=%f id=%d\n", __func__, *value, dimm_id);
#endif
  return 0;
}

static int
read_cpu4_dimm_temp(uint8_t dimm_id, float *value) {
  int ret;

  ret = get_dimm_temp(PECI_CPU4_ADDR, dimm_id, value);
  if (ret != 0) {
    return ret;
  }
#ifdef DEBUG
  syslog(LOG_DEBUG, "%s DIMM Temp=%f id=%d\n", __func__, *value, dimm_id);
#endif
  return 0;
}

static int
read_cpu5_dimm_temp(uint8_t dimm_id, float *value) {
  int ret;

  ret = get_dimm_temp(PECI_CPU5_ADDR, dimm_id, value);
  if (ret != 0) {
    return ret;
  }
#ifdef DEBUG
  syslog(LOG_DEBUG, "%s DIMM Temp=%f id=%d\n", __func__, *value, dimm_id);
#endif
  return 0;
}

static int
read_cpu6_dimm_temp(uint8_t dimm_id, float *value) {
  int ret;

  ret = get_dimm_temp(PECI_CPU6_ADDR, dimm_id, value);
  if (ret != 0) {
    return ret;
  }
#ifdef DEBUG
  syslog(LOG_DEBUG, "%s DIMM Temp=%f id=%d\n", __func__, *value, dimm_id);
#endif
  return 0;
}

static int
read_cpu7_dimm_temp(uint8_t dimm_id, float *value) {
  int ret;

  ret = get_dimm_temp(PECI_CPU7_ADDR, dimm_id, value);
  if (ret != 0) {
    return ret;
  }
#ifdef DEBUG
  syslog(LOG_DEBUG, "%s DIMM Temp=%f id=%d\n", __func__, *value, dimm_id);
#endif
  return 0;
}

static int
get_nm_rw_info(uint8_t nm_id, uint8_t* nm_bus, uint8_t* nm_addr, uint16_t* bmc_addr) {
  int ret=0;

  *nm_bus= nm_info_list[nm_id].bus;
  *nm_addr= nm_info_list[nm_id].slv_addr;
  ret = pal_get_bmc_ipmb_slave_addr(bmc_addr, nm_info_list[nm_id].bus);
  if (ret != 0) {
    return ret;
  }

  return 0;
}

static int
get_hsc_rw_info(uint8_t* extend, uint8_t* mode) {
  int ret=0;

  ret = pal_get_host_system_mode(mode);
  if (ret != 0) {
    return ret;
  }

  if((*mode == MB_8S_MODE) || (*mode == MB_4S_MODE)) {
    *extend = true;
  } else {
    *extend = false;
  }
  return 0;
}

static void
get_adm1278_info(uint8_t hsc_id, uint8_t type, float* m, float* b, float* r) {

 *m = hsc_info_list[hsc_id].info[type].m;
 *b = hsc_info_list[hsc_id].info[type].b;
 *r = hsc_info_list[hsc_id].info[type].r;

 return;
}

//Sensor HSC
static int
get_hsc_val_from_me(uint8_t hsc_id, uint8_t pmbus_cmd, uint8_t *data) {
  uint8_t rbuf[32] = {0x00};
  uint8_t tmp[5] = {0x00};
  uint8_t extend=0;
  uint8_t mode=0;
  uint8_t mux_ch= hsc_info_list[hsc_id].ch;
  static int retry = 0;
  int ret = 0;
  NM_RW_INFO info;

  get_hsc_rw_info(&extend, &mode);

#ifdef DEBUG
  syslog(LOG_DEBUG, "%s extend=%d, mode=%d, ch=%d\n", __func__, extend, mode, mux_ch);
#endif
  get_nm_rw_info(NM_ID0, &info.bus, &info.nm_addr, &info.bmc_addr);

  tmp[0] = pmbus_cmd;                        // psu_cmd;
  tmp[1] = hsc_info_list[hsc_id].slv_addr;   // psu_addr;
  tmp[2] = NM_PSU_MUX_ADDR;                  // mux_addr;
  tmp[3] = mux_ch;                           // mux_ch;
  tmp[4] = NM_SENSOR_BUS_SMLINK1;            // sensor_bus;

  if(extend == 0) {
     ret = cmd_NM_pmbus_standard_read_word(info, tmp, rbuf);
  } else {
    ret = cmd_NM_pmbus_extend_read_word(info, tmp, rbuf);
  }

  if (ret != 0) {
    retry++;
    if (retry <= 5) {
      return READING_SKIP;
    }
    return READING_NA;
  }

  if (rbuf[6] == 0) {
    *data = rbuf[10];
    *(data+1) = rbuf[11];
#ifdef DEBUG
    syslog(LOG_DEBUG, "%s value_l=%x, value_h=%x\n", __func__, *data, *(data+1));
#endif
    retry = 0;
  } else {
#ifdef DEBUG
    syslog(LOG_DEBUG, "%s cc=%x\n", __func__, rbuf[6]);
#endif
    retry++;
    if (retry <= 5) {
      return READING_SKIP;
    }
    return READING_NA;
  }

  return 0;
}

int
set_NM_hsc_word_data(uint8_t hsc_id, uint8_t psu_cmd, uint16_t value) {
  uint8_t data[2] = {0x00};
  uint8_t buf[5] = {0x00};
  uint8_t extend=0;
  uint8_t mode=0;
    int ret = 0;
  NM_RW_INFO info;

  get_hsc_rw_info(&extend, &mode);

  buf[0] = psu_cmd;                          // psu_cmd;
  buf[1] = hsc_info_list[hsc_id].slv_addr;   // psu_addr;
  buf[2] = NM_PSU_MUX_ADDR;                  // mux_addr;
  buf[3] = hsc_info_list[hsc_id].ch;         // mux_ch;
  buf[4] = NM_SENSOR_BUS_SMLINK1;            // sensor_bus;

  get_nm_rw_info(NM_ID0, &info.bus, &info.nm_addr, &info.bmc_addr);

  data[0] = value & 0x00FF;   //PMON data low byte
  data[1] = value >> 8;       //PMON data high byte

#ifdef DEBUG
  syslog(LOG_DEBUG, "%s cmd=%x lb =%x hb=%x\n", __func__, psu_cmd, data[0], data[1]);
#endif
  if(extend == 0) {
    ret = cmd_NM_pmbus_standard_write_word(info, buf, data);
  } else {
    ret = cmd_NM_pmbus_extend_write_word(info, buf, data);
  }

  if (ret != 0) {
    return ret;
  }
  return 0;
}

int
get_NM_hsc_word_data(uint8_t hsc_id, uint8_t psu_cmd, uint8_t* value) {
  uint8_t rbuf[32] = {0x00};
  uint8_t buf[5] = {0x00};
  uint8_t extend=0;
  uint8_t mode=0;
    int ret = 0;
  NM_RW_INFO info;

  get_hsc_rw_info(&extend, &mode);

  buf[0] = psu_cmd;                          // psu_cmd;
  buf[1] = hsc_info_list[hsc_id].slv_addr;   // psu_addr;
  buf[2] = NM_PSU_MUX_ADDR;                  // mux_addr;
  buf[3] = hsc_info_list[hsc_id].ch;         // mux_ch;
  buf[4] = NM_SENSOR_BUS_SMLINK1;            // sensor_bus;

  get_nm_rw_info(NM_ID0, &info.bus, &info.nm_addr, &info.bmc_addr);

  if(extend == 0) {
    ret = cmd_NM_pmbus_standard_read_word(info, buf, rbuf);
  } else {
    ret = cmd_NM_pmbus_extend_read_word(info, buf, rbuf);
  }

  if (ret != 0) {
    return ret;
  }

  if (rbuf[6] == 0) {
    *value = rbuf[10];
    *(value+1) = rbuf[11];
  } else {
    syslog(LOG_DEBUG, "%s cc=%x\n", __func__, rbuf[6]);
  }
#ifdef DEBUG
  syslog(LOG_DEBUG, "%s id=%x cmd=%x data[0]=%x, [1]=%x\n",
          __func__, hsc_id, psu_cmd, value[0], value[1]);
#endif
  return 0;
}


static int
set_hsc_pmon_config(uint8_t hsc_id, uint16_t value) {
  int ret=0;
  ret = set_NM_hsc_word_data(hsc_id, ADM1278_PMON_CONFIG, value);
  if (ret != 0) {
    return ret;
  }

  return 0;
}

static int
set_hsc_clr_peak_pin(uint8_t hsc_id) {
  int ret=0;
  ret = set_NM_hsc_word_data(hsc_id, ADM1278_PEAK_PIN, 0x0000);
  if (ret != 0) {
    return ret;
  }

  return 0;
}

static int
set_hsc_iout_warn_limit(uint8_t hsc_id, float value) {
  int ret=0;
  float m, b, r;
  uint16_t data;

  get_adm1278_info(hsc_id, ADM1278_CURRENT, &m, &b, &r);

  data =(uint16_t) ((value * m + b)/r);
  ret = set_NM_hsc_word_data(hsc_id, PMBUS_IOUT_OC_WARN_LIMIT, data);
  if (ret != 0) {
    return ret;
  }

  return 0;
}

static int
set_alter1_config(uint8_t hsc_id, uint16_t value) {
  int ret=0;

  ret = set_NM_hsc_word_data(hsc_id, ADM1278_ALTER_CONFIG, value);
  if (ret != 0) {
    return ret;
  }

  return 0;
}

static int
read_hsc_iout(uint8_t hsc_id, float *value) {
  int ret;
  uint8_t data[4];
  float m, b, r;

  get_adm1278_info(hsc_id, ADM1278_CURRENT, &m, &b, &r);

  ret = get_hsc_val_from_me(hsc_id, PMBUS_READ_IOUT, data);
  if(ret != 0) {
    return ret;
  }
  *value = ((float)(data[1] << 8 | data[0]) * r - b) / m;

#ifdef DEBUG
  syslog(LOG_DEBUG, "%s: Value=%f\n", __func__, *value);
#endif
  return ret;
}

static int
read_hsc_vin(uint8_t hsc_id, float *value) {
  int ret;
  uint8_t data[4];
  float m, b, r;

  get_adm1278_info(hsc_id, ADM1278_VOLTAGE, &m, &b, &r);

  ret = get_hsc_val_from_me(hsc_id, PMBUS_READ_VIN, data);
  if(ret != 0) {
    return ret;
  }
  *value = ((float)(data[1] << 8 | data[0]) * r - b) / m;

#ifdef DEBUG
  syslog(LOG_DEBUG, "%s: Value=%f\n", __func__, *value);
#endif
  return ret;
}

static int
read_hsc_pin(uint8_t hsc_id, float *value) {
  int ret;
  uint8_t data[4];
  float m, b, r;

  get_adm1278_info(hsc_id, ADM1278_POWER, &m, &b, &r);

  ret = get_hsc_val_from_me(hsc_id, PMBUS_READ_PIN, data);
  if(ret != 0) {
    return ret;
  }
  *value = ((float)(data[1] << 8 | data[0]) * r - b) / m;

#ifdef DEBUG
  syslog(LOG_DEBUG, "%s: Value=%f\n", __func__, *value);
#endif
  return ret;
}

static int
read_hsc_temp(uint8_t hsc_id, float *value) {
  int ret;
  uint8_t data[4];
  float m, b, r;

  get_adm1278_info(hsc_id, ADM1278_TEMP, &m, &b, &r);

  //Self BMC Read TEMP HSC
  ret = get_hsc_val_from_me(hsc_id, PMBUS_READ_TEMPERATURE_1, data);
  if(ret != 0) {
    return ret;
  }
  *value = ((float)(data[1] << 8 | data[0]) * r - b) / m;

#ifdef DEBUG
  syslog(LOG_DEBUG, "%s: Value=%f\n", __func__, *value);
#endif
  return ret;
}

static int
read_hsc_peak_pin(uint8_t hsc_id, float *value) {
  int ret;
  uint8_t data[4];
  float m, b, r;

  get_adm1278_info(hsc_id, ADM1278_POWER , &m, &b, &r);

  //Self BMC Read PEAK PIN
  ret = get_hsc_val_from_me(hsc_id, ADM1278_PEAK_PIN, data);
  if(ret != 0) {
    return ret;
  }
  *value = ((float)(data[1] << 8 | data[0]) * r - b) / m;

#ifdef DEBUG
  syslog(LOG_DEBUG, "%s: Value=%f\n", __func__, *value);
#endif
  return ret;
}

static int
read_NM_pch_temp(uint8_t nm_id, float *value) {
  uint8_t rlen = 0;
  uint8_t rbuf[32] = {0x00};
  int ret = 0;
  static uint8_t retry = 0;
  NM_RW_INFO info;

  get_nm_rw_info(NM_ID0, &info.bus, &info.nm_addr, &info.bmc_addr);
  ret = cmd_NM_sensor_reading(info, NM_PCH_TEMP, rbuf, &rlen);
  if (!ret) {
    *value = (float) rbuf[7];
    retry = 0;
  } else {
    retry++;
    if (retry <= 3)
      ret = READING_SKIP;
  }
  return ret;
}


/*==========================================
Read temperature sensor TMP421 value.
Interface: temp_id: temperature id
           *value: real temperature value
           return: error code
============================================*/
static int
read_sensor(uint8_t id, float *value) {
  struct {
    const char *chip;
    const char *label;
  } devs[] = {
    {"tmp421-i2c-19-4c", "MB_INLET_TEMP"},
    {"tmp421-i2c-19-4e", "MB_OUTLET_L_TEMP"},
    {"tmp421-i2c-19-4f", "MB_OUTLET_R_TEMP"},
    {"tmp421-i2c-19-4c", "MB_INLET_REMOTE_TEMP"},
    {"tmp421-i2c-19-4e", "MB_OUTLET_L_REMOTE_TEMP"},
    {"tmp421-i2c-19-4f", "MB_OUTLET_R_REMOTE_TEMP"},
  };
  if (id >= ARRAY_SIZE(devs)) {
    return -1;
  }
  return sensors_read(devs[id].chip, devs[id].label, value);
}

static int
read_nic_temp(uint8_t nic_id, float *value) {
  int fd = 0, ret = -1;
  char fn[32];
  uint8_t tlen, rlen, addr, bus;
  static uint8_t retry=0;
  uint8_t tbuf[16] = {0};
  uint8_t rbuf[16] = {0};

  bus = nic_info_list[nic_id].bus;
  addr = nic_info_list[nic_id].slv_addr;

  snprintf(fn, sizeof(fn), "/dev/i2c-%d", bus);
  fd = open(fn, O_RDWR);
  if (fd < 0) {
    goto err_exit;
  }

  //Temp Register
  tbuf[0] = 0x01;
  tlen = 1;
  rlen = 1;

  ret = i2c_rdwr_msg_transfer(fd, addr, tbuf, tlen, rbuf, rlen);
  if( ret < 0 || (rbuf[0] == 0x80) ) {
    retry++;
    if (retry < 3) {
      ret = READING_SKIP;
    } else {
      ret = READING_NA;
    }
    goto err_exit;
  } else {
    retry=0;
  }

#ifdef DEBUG
  syslog(LOG_DEBUG, "%s Temp[%d]=%x bus=%x slavaddr=%x\n", __func__, nic_id, rbuf[0], bus, addr);
#endif

  *value = (float)rbuf[0];

err_exit:
  if (fd > 0) {
    close(fd);
  }
  return ret;
}

static int
read_hd_temp(uint8_t hd_id, float *value) {
  int fd = 0, ret = -1;
  char fn[32];
  uint8_t retry = 3, tlen, rlen, addr, bus;
  uint8_t tbuf[16] = {0};
  uint8_t rbuf[16] = {0};

  bus = disk_info_list[hd_id].bus;
  addr = disk_info_list[hd_id].slv_addr;

  snprintf(fn, sizeof(fn), "/dev/i2c-%d", bus);
  fd = open(fn, O_RDWR);
  if (fd < 0) {
    goto err_exit;
  }

  //Temp Register
  tbuf[0] = 0x03;
  tlen = 1;
  rlen = 1;

  while (ret < 0 && retry-- > 0) {
    ret = i2c_rdwr_msg_transfer(fd, addr, tbuf, tlen, rbuf, rlen);
  }

#ifdef DEBUG
  syslog(LOG_DEBUG, "%s Temp[%d]=%x bus=%x slavaddr=%x\n", __func__, hd_id, rbuf[0], bus, addr);
#endif

  if (ret < 0) {
    goto err_exit;
  }

  *value = rbuf[0];
  err_exit:
  if (fd > 0) {
    close(fd);
  }
  return ret;
}

static int
read_ina260_vol(uint8_t ina260_id, float *value) {
  int fd = 0, ret = -1;
  char fn[32];
  float scale;
  uint8_t retry = 3, tlen, rlen, addr, bus, cmd;
  uint8_t tbuf[16] = {0};
  uint8_t rbuf[16] = {0};
  uint16_t tmp;

  bus = ina260_info_list[ina260_id].bus;
  cmd = INA260_VOLTAGE;
  addr = ina260_info_list[ina260_id].slv_addr;
  scale = 0.00125;

  snprintf(fn, sizeof(fn), "/dev/i2c-%d", bus);
  fd = open(fn, O_RDWR);
  if (fd < 0) {
    goto err_exit;
  }

  tbuf[0] = cmd;
  tlen = 1;
  rlen = 2;

  while (ret < 0 && retry-- > 0) {
    ret = i2c_rdwr_msg_transfer(fd, addr, tbuf, tlen, rbuf, rlen);
  }

#ifdef DEBUG
  syslog(LOG_DEBUG, "%s bus=%x cmd=%x slavaddr=%x\n", __func__, bus, cmd, addr);
#endif

  if (ret < 0) {
    syslog(LOG_DEBUG, "ret=%d", ret);
    goto err_exit;
  }

  tmp = (rbuf[0] << 8) | (rbuf[1]);
  *value = (float)tmp * scale;

#ifdef DEBUG
  syslog(LOG_DEBUG, "%s tmp=%x val=%f\n", __func__, tmp, *value);
#endif

  err_exit:
  if (fd > 0) {
    close(fd);
  }
  return ret;
}

static int
read_vr_vout(uint8_t vr_id, float *value) {
  int ret=0;
  static uint8_t retry[VR_NUM_CNT] = {0};
  const char *label[] = {
    "MB_VR_CPU0_VCCIN_VOUT",
    "MB_VR_CPU0_VCCSA_VOUT",
    "MB_VR_CPU0_VCCIO_VOUT",
    "MB_VR_CPU0_VDDQ_ABC_VOUT",
    "MB_VR_CPU0_VDDQ_DEF_VOUT",
    "MB_VR_CPU1_VCCIN_VOUT",
    "MB_VR_CPU1_VCCSA_VOUT",
    "MB_VR_CPU1_VCCIO_VOUT",
    "MB_VR_CPU1_VDDQ_ABC_VOUT",
    "MB_VR_CPU1_VDDQ_DEF_VOUT",
    "MB_VR_PCH_P1V05_VOLT",
    "MB_VR_PCH_PVNN_VOLT",
  };
  if (vr_id >= ARRAY_SIZE(label)) {
    return -1;
  } else if (!check_cpu_present(label[vr_id])) {
    return READING_NA;
  }

  ret = sensors_read(vr_chips[vr_id], label[vr_id], value);
  if( *value == 0 ) {
    retry[vr_id]++;
    if(retry[vr_id] < 3 ){
      return READING_SKIP;
    }
  }

  retry[vr_id] = 0;
  return ret;
}

static int
read_vr_temp(uint8_t vr_id, float *value) {
  int ret=0;
  static uint8_t retry[VR_NUM_CNT] = {0};
  const char *label[] = {
    "MB_VR_CPU0_VCCIN_TEMP",
    "MB_VR_CPU0_VCCSA_TEMP",
    "MB_VR_CPU0_VCCIO_TEMP",
    "MB_VR_CPU0_VDDQ_ABC_TEMP",
    "MB_VR_CPU0_VDDQ_DEF_TEMP",
    "MB_VR_CPU1_VCCIN_TEMP",
    "MB_VR_CPU1_VCCSA_TEMP",
    "MB_VR_CPU1_VCCIO_TEMP",
    "MB_VR_CPU1_VDDQ_ABC_TEMP",
    "MB_VR_CPU1_VDDQ_DEF_TEMP",
    "MB_VR_PCH_P1V05_TEMP",
    "MB_VR_PCH_PVNN_TEMP",
  };
  if (vr_id >= ARRAY_SIZE(label)) {
    return -1;
  }
  ret = sensors_read(vr_chips[vr_id], label[vr_id], value);
  if( *value == 0 ) {
    retry[vr_id]++;
    if(retry[vr_id] < 3 ){
      return READING_SKIP;
    }
  }

  retry[vr_id] = 0;
  return ret;
}

static int
read_vr_iout(uint8_t vr_id, float *value) {
    const char *label[] = {
    "MB_VR_CPU0_VCCIN_IOUT",
    "MB_VR_CPU0_VCCSA_IOUT",
    "MB_VR_CPU0_VCCIO_IOUT",
    "MB_VR_CPU0_VDDQ_ABC_IOUT",
    "MB_VR_CPU0_VDDQ_DEF_IOUT",
    "MB_VR_CPU1_VCCIN_IOUT",
    "MB_VR_CPU1_VCCSA_IOUT",
    "MB_VR_CPU1_VCCIO_IOUT",
    "MB_VR_CPU1_VDDQ_ABC_IOUT",
    "MB_VR_CPU1_VDDQ_DEF_IOUT",
    "MB_VR_PCH_P1V05_IOUT",
    "MB_VR_PCH_PVNN_IOUT",
  };
  if (vr_id >= ARRAY_SIZE(label)) {
    return -1;
  }
  return sensors_read(vr_chips[vr_id], label[vr_id], value);
}

int
read_vr_iin(uint8_t vr_id, float *value) {
  return 0;
}

static int
read_vr_pout(uint8_t vr_id, float *value) {
  const char *label[] = {
    "MB_VR_CPU0_VCCIN_POUT",
    "MB_VR_CPU0_VCCSA_POUT",
    "MB_VR_CPU0_VCCIO_POUT",
    "MB_VR_CPU0_VDDQ_ABC_POUT",
    "MB_VR_CPU0_VDDQ_DEF_POUT",
    "MB_VR_CPU1_VCCIN_POUT",
    "MB_VR_CPU1_VCCSA_POUT",
    "MB_VR_CPU1_VCCIO_POUT",
    "MB_VR_CPU1_VDDQ_ABC_POUT",
    "MB_VR_CPU1_VDDQ_DEF_POUT",
    "MB_VR_PCH_P1V05_POUT",
    "MB_VR_PCH_PVNN_POUT",
  };
  if (vr_id >= ARRAY_SIZE(label)) {
    return -1;
  }
  return sensors_read(vr_chips[vr_id], label[vr_id], value);
}

int
read_vr_pin(uint8_t vr_id, float *value) {
 // int ret = 0;
 // uint8_t cmd;

 // cmd = PMBUS_READ_PIN;
 // ret = get_tps53688_linear(vr_id, cmd, value);
 // if (ret != 0) {
 //   return ret;
 // }
  return 0;
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
  if(pwr_polling_flag != true) {
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
  int ret = READING_NA, rc;
  size_t len = 0;
  char sensor_name[32] = {0};
  char error[32] = {0};

  if (fru_id != 1) {
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
  ret = 0;

  return ret;
}

static
int read_frb3(uint8_t fru_id, float *value) {
  int ret = 0;

#ifdef DEBUG
  syslog(LOG_INFO, "%s\n", __func__);
#endif
  ret = check_frb3(fru_id, MB_SNR_PROCESSOR_FAIL, value);
  if(ret != 0 ) {
    return ret;
  }

  return ret;
}

int
pal_sensor_read_raw(uint8_t fru, uint8_t sensor_num, void *value) {
  char key[MAX_KEY_LEN] = {0};
  char str[MAX_VALUE_LEN] = {0};
  char fru_name[32];
  int ret=0;
  bool server_off;
  uint8_t id=0;
  struct timespec ts;

  pal_get_fru_name(fru, fru_name);
  sprintf(key, "%s_sensor%d", fru_name, sensor_num);
  server_off = is_server_off();
  id = sensor_map[sensor_num].id;

  switch(fru) {
  case FRU_MB:
  case FRU_NIC0:
  case FRU_NIC1:
  case FRU_PDB:
    if (server_off) {
      pwr_polling_flag = false;
      if (sensor_map[sensor_num].stby_read == true) {
        ret = sensor_map[sensor_num].read_sensor(id, (float*) value);
      } else {
        ret = READING_NA;
      }
    } else {
      clock_gettime(CLOCK_MONOTONIC, &ts);
      if (!kv_get("snr_polling_flag", str, NULL, 0)) {
        if ( ts.tv_sec - strtoul(str, NULL, 10) > POLLING_DELAY_TIME ) {
          pwr_polling_flag = true;
        } else {
          pwr_polling_flag = false;
        }
      } else {
         sprintf(str, "%ld", ts.tv_sec);
         kv_set("snr_polling_flag", str, 0, 0);
         pwr_polling_flag = false;
      }
      ret = sensor_map[sensor_num].read_sensor(id, (float*) value);
    }

    if (ret != 0) {
      ret = READING_NA;
    }

    if (is_server_off() != server_off) {
      /* server power status changed while we were reading the sensor.
       * this sensor is potentially NA. */
      return pal_sensor_read_raw(fru, sensor_num, value);
    }
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
  switch(fru) {
  case FRU_MB:
  case FRU_NIC0:
  case FRU_NIC1:
  case FRU_PDB:
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
  switch(fru) {
  case FRU_MB:
  case FRU_NIC0:
  case FRU_NIC1:
  case FRU_PDB:
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
  }
  return 0;
}

int
pal_get_sensor_units(uint8_t fru, uint8_t sensor_num, char *units) {
  uint8_t scale = sensor_map[sensor_num].units;

  switch(fru) {
    case FRU_MB:
    case FRU_NIC0:
    case FRU_NIC1:
    case FRU_PDB:
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
      break;
    default:
      return -1;
  }
  return 0;
}

int pal_get_fan_name(uint8_t num, char *name)
{
  if (num >= pal_tach_cnt) {
    syslog(LOG_WARNING, "%s: invalid fan#:%d", __func__, num);
    return -1;
  }

  sprintf(name, "Fan %d %s", num/2, num%2==0? "In":"Out");

  return 0;
}

int
pal_set_fan_speed(uint8_t fan, uint8_t pwm) {
  int ret = -1;

  if (fan >= pal_pwm_cnt) {
    syslog(LOG_INFO, "%s: fan number is invalid - %d", __func__, fan);
    return -1;
  }

  ret = lib_cmc_set_fan_pwm(fan, pwm);
  return ret;
}

int
pal_get_fan_speed(uint8_t fan, int *rpm) {
  int ret;
  uint16_t speed = 0;

  if (fan >= pal_tach_cnt) {
    syslog(LOG_INFO, "%s: fan number is invalid - %d", __func__, fan);
    return -1;
  }

  ret = lib_cmc_get_fan_speed(fan, &speed);
  if (ret != 0 ) {
    syslog(LOG_INFO, "%s: invalid fan#:%d",__func__, fan);
    return -1;
  }

  *rpm = (int)speed;
  return ret;
}

int
pal_get_pwm_value(uint8_t fan_num, uint8_t *value) {
  int ret;
  uint8_t pwm = 0;
  uint8_t fan_id;

  if (fan_num >= pal_tach_cnt) {
    syslog(LOG_INFO, "%s: fan number is invalid - %d", __func__, fan_num);
    return -1;
  }

  fan_id = fan_num/2;
  ret = lib_cmc_get_fan_pwm(fan_id, &pwm);
  if (ret != 0 ) {
    syslog(LOG_INFO, "%s: invalid fan#:%d",__func__, fan_id);
    return -1;
  }

  *value = pwm;
  return ret;
}


static int
fbal_sensor_name(uint8_t fru, uint8_t sensor_num, char *name) {

  switch(fru) {
    case FRU_PDB:
      switch(sensor_num) {
        case PDB_EVENT_STATUS:
          sprintf(name, "PDB_EVENT_STATUS");
          break;
        case PDB_EVENT_FAN0_PRESENT:
          sprintf(name, "PDB_FAN0_PRESENT");
          break;
        case PDB_EVENT_FAN1_PRESENT:
          sprintf(name, "PDB_FAN1_PRESENT");
          break;
        case PDB_EVENT_FAN2_PRESENT:
          sprintf(name, "PDB_FAN2_PRESENT");
          break;
        case PDB_EVENT_FAN3_PRESENT:
          sprintf(name, "PDB_FAN3_PRESENT");
          break;
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
      if (fbal_sensor_name(fru, snr_num, name) != 0) {
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
  uint8_t *event_data = &sel[10];
  uint8_t *ed = &event_data[3];
  uint8_t fan_id=0;
  char temp_log[128] = {0};
  bool parsed = true;

  strcpy(error_log, "");
  switch(snr_num) {
    case PDB_EVENT_STATUS:
      switch (ed[0]) {
        case PDB_EVENT_CM_RESET:
          strcat(error_log, "PDB_Event_CM_Reset");
          break;
        case PDB_EVENT_PWR_CYCLE:
          sprintf(temp_log, "PDB_Event_Power_Cycle, MB%d", ed[1]);
          strcat(error_log, temp_log);
          break;
        case PDB_EVENT_SLED_CYCLE:
          sprintf(temp_log, "PDB_Event_SLED_Cycle, MB%d", ed[1]);
          strcat(error_log, temp_log);
          break;
        case PDB_EVENT_RECONFIG_SYSTEM:
          sprintf(temp_log, "PDB_Event_Reconfig_System, MB%d, Mode=%d", ed[1], ed[2]);
          strcat(error_log, temp_log);
          break;
        case PDB_EVENT_CM_EXTERNAL_RESET:
          strcat(error_log, "PDB_Event_CM_External_Reset");
          break;
        default:
          parsed = false;
          strcat(error_log, "Unknown");
          break;
      }
      break;
    case PDB_EVENT_FAN0_PRESENT:
    case PDB_EVENT_FAN1_PRESENT:
    case PDB_EVENT_FAN2_PRESENT:
    case PDB_EVENT_FAN3_PRESENT:
      fan_id = lib_cmc_get_fan_id(snr_num);
      sprintf(temp_log, "PDB_Event_Fan%d_%s", fan_id, ed[0] ? "Present": "Not_Present");
      strcat(error_log, temp_log);
      break;
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

bool
pal_check_dimm_prsnt(uint8_t snr_num) {
  uint8_t dimm_id = 0xFF;

  dimm_id = (snr_num - MB_SNR_CPU0_DIMM_GRPA_TEMP) *2;
  return pal_is_dimm_present(dimm_id);
}

bool
pal_check_nic_prsnt(uint8_t fru) {
  uint8_t status;
  pal_is_fru_prsnt(fru, &status);

  return status;
}

void
pal_sensor_assert_handle(uint8_t fru, uint8_t snr_num, float val, uint8_t thresh) {
  char cmd[128];
  char thresh_name[10];
  char snr_name[32];
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
    case MB_SNR_P5V:
    case MB_SNR_P5V_STBY:
    case MB_SNR_P3V3_STBY:
    case MB_SNR_P3V3:
    case MB_SNR_P3V_BAT:
      pal_get_sensor_name(fru, snr_num, snr_name);
      sprintf(cmd, "%s %s %.2fVolts - Assert", snr_name, thresh_name, val);
      break;
    case PDB_SNR_FAN0_INLET_SPEED:
    case PDB_SNR_FAN1_INLET_SPEED:
    case PDB_SNR_FAN2_INLET_SPEED:
    case PDB_SNR_FAN3_INLET_SPEED:
      fan_id = snr_num-PDB_SNR_FAN0_INLET_SPEED;
      sprintf(cmd, "FAN%d %s %dRPM - Assert",fan_id ,thresh_name, (int)val);
      break;
    case PDB_SNR_FAN0_OUTLET_SPEED:
    case PDB_SNR_FAN1_OUTLET_SPEED:
    case PDB_SNR_FAN2_OUTLET_SPEED:
    case PDB_SNR_FAN3_OUTLET_SPEED:
      fan_id = snr_num-PDB_SNR_FAN0_OUTLET_SPEED;
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
    case PDB_SNR_FAN0_INLET_SPEED:
    case PDB_SNR_FAN1_INLET_SPEED:
    case PDB_SNR_FAN2_INLET_SPEED:
    case PDB_SNR_FAN3_INLET_SPEED:
      fan_id = snr_num-PDB_SNR_FAN0_INLET_SPEED;
      sprintf(cmd, "FAN%d %s %dRPM - Deassert",fan_id ,thresh_name, (int)val);
      break;
    case PDB_SNR_FAN0_OUTLET_SPEED:
    case PDB_SNR_FAN1_OUTLET_SPEED:
    case PDB_SNR_FAN2_OUTLET_SPEED:
    case PDB_SNR_FAN3_OUTLET_SPEED:
      fan_id = snr_num-PDB_SNR_FAN0_OUTLET_SPEED;
      sprintf(cmd, "FAN%d %s %dRPM - Deassert",fan_id ,thresh_name, (int)val);
      break;
    default:
      return;
  }
  pal_add_cri_sel(cmd);

}

int pal_sensor_monitor_initial(void) {
  int i=0;
  uint16_t pmon_cfg = 0;
  uint16_t alert1_cfg = 0;
  float iout_oc_warn_limit = 123;
  uint8_t mode;
  uint8_t sku_id = 0xFF;
  uint8_t hsc_cnt=0;
  uint8_t master = pal_get_config_is_master();
  int ret;

//Get Config
  ret = pal_get_host_system_mode(&mode);
  if (ret != 0) {
    syslog(LOG_WARNING,"%s Wrong get system mode\n", __func__);
  }

//Initial
  syslog(LOG_DEBUG,"Sensor Initial\n");
  pmon_cfg = PMON_CFG_TEPM1_EN | PMON_CFG_CONTINUOUS_SAMPLE | PMON_CFG_VIN_EN |
             PMON_CFG_VI_AVG(CFG_SAMPLE_128) | PMON_CFG_AVG_PWR(CFG_SAMPLE_128);

  alert1_cfg = IOUT_OC_WARN_EN1;

  pal_get_platform_id(&sku_id);
  //DVT SKU_ID[2:1] = 00 (TI), 01 (INFINEON), TODO: 10 (3rd Source)
  if (sku_id & 0x2) {
    vr_chips = vr_infineon_chips;
  } else {
    vr_chips = vr_ti_chips;
  }

//Set HSC ControllerG
  if( mode == MB_2S_MODE ) {
    hsc_cnt = 1;
    DIMM_SLOT_CNT = 24;
  } else if(mode == MB_4S_MODE && master == true) {
    hsc_cnt = 2;
    DIMM_SLOT_CNT = 48;
  } else if(mode == MB_8S_MODE && master == true) {
    hsc_cnt = 4;
    DIMM_SLOT_CNT = 96;
  } else {
    hsc_cnt = 0;
  }

  for (i=0; i<hsc_cnt; i++) {
    if(set_hsc_pmon_config(i, pmon_cfg) != 0) {
      syslog(LOG_WARNING, "Set MB%d PMON CONFIG Fail\n", i);
    }
    if(set_hsc_iout_warn_limit(i, iout_oc_warn_limit) != 0) {
      syslog(LOG_WARNING, "Set MB%d Iout OC Warning Limit Fail\n", i);
    }
    if(set_alter1_config(i, alert1_cfg) != 0) {
      syslog(LOG_WARNING, "Set MB%d Alert1 Config Fail\n", i);
    }

    if(set_hsc_clr_peak_pin(i) != 0) {
      syslog(LOG_WARNING, "Set MB%d Clear PEAK PIN Fail\n", i);
    }
  }

//Config PECI Switch
  if (master == true) {
    pal_set_peci_mux(PECI_MUX_SELECT_BMC);
  } else {
    pal_set_peci_mux(PECI_MUX_SELECT_PCH);
  }

  return 0;
}
