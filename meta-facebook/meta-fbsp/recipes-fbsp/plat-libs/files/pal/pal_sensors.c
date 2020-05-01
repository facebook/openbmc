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
#include <openbmc/kv.h>
#include <openbmc/libgpio.h>
#include <openbmc/peci.h>
#include <openbmc/obmc-i2c.h>
#include <openbmc/nm.h>
#include <openbmc/ipmb.h>
#include <openbmc/obmc-sensors.h>
#include <pthread.h>
#include "pal.h"

//#define DEBUG
#define GPIO_P3V_BAT_SCALED_EN "P3V_BAT_SCALED_EN"
#define GPIO_FAN_BUF_PRESENT "FM_FAN%d_BUF_PRESENT_R"
#define NM_IPMB_BUS_ID   (5)
#define NM_SLAVE_ADDR    (0x2C)

#define FAN_LED_STATE "fan%d_led_state"
#define I2C_DEV_FAN_LED "/dev/i2c-8"
#define BLUE_LED_FUNC 0x06
#define YELLOW_LED_FUNC 0x07
#define FCB_0_ADDR 0xC0
#define FCB_1_ADDR 0xC2

#define PAL_FAN_CNT 8

#define FAN_DIR "/sys/bus/platform/devices/1e786000.pwm-tacho-controller/hwmon/hwmon0"

#define MAX_READ_RETRY 10
#define MAX_SNR_READ_RETRY 3

#define DIMM_SLOT_CNT 24

static int get_snr_state(int retry);
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
static int read_cpu0_dimm_temp(uint8_t dimm_id, float *value);
static int read_cpu1_dimm_temp(uint8_t dimm_id, float *value);
static int read_NM_pch_temp(uint8_t nm_snr_id, float *value);
static int read_ina260_sensor(uint8_t sensor_num, float *value);
static int read_vr_vout(uint8_t vr_id, float *value);
static int read_vr_temp(uint8_t vr_id, float  *value);
static int read_vr_iout(uint8_t vr_id, float  *value);
static int read_vr_pout(uint8_t vr_id, float  *value);
static int read_fan_volt(uint8_t fan_id, float *value);
static int read_fan_curr(uint8_t fan_id, float *value);
static int read_fan_pwr(uint8_t fan_id, float *value);
static bool is_fan_present(uint8_t fan_id);
static int read_fan_speed(uint8_t fan, float *rpm);
static bool is_ava_card_present(uint8_t riser_slot);
static int read_nvme_temp(uint8_t sensor_num, float *value);
static int read_ava_temp(uint8_t sensor_num, float *value);

size_t pal_pwm_cnt = 2;
size_t pal_tach_cnt = 16;
const char pal_pwm_list[] = "0, 1";
const char pal_tach_list[] = "0..15";
static float fan_volt[PAL_FAN_CNT];
static float fan_curr[PAL_FAN_CNT];
static uint8_t postcodes_last[256] = {0};

static const char *vr_chips[] = {
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
  "pxe1110c-i2c-1-4a",  //PCH PVNN, 1V5
};

const uint8_t mb_sensor_list[] = {
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
  MB_SNR_CPU_1V8,
  MB_SNR_PCH_1V8,
  MB_SNR_CPU0_PVPP_ABC,
  MB_SNR_CPU0_PVPP_DEF,
  MB_SNR_CPU0_PVTT_ABC,
  MB_SNR_CPU0_PVTT_DEF,
  MB_SNR_CPU1_PVPP_ABC,
  MB_SNR_CPU1_PVPP_DEF,
  MB_SNR_CPU1_PVTT_ABC,
  MB_SNR_CPU1_PVTT_DEF,
  MB_SNR_P12V_OCP_IMON,
  MB_SNR_CPU0_TEMP,
  MB_SNR_CPU1_TEMP,
  MB_SNR_HSC_VIN,
  MB_SNR_HSC_IOUT,
  MB_SNR_HSC_PIN,
  MB_SNR_HSC_TEMP,
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
  MB_SNR_FAN2_TACH_IN ,
  MB_SNR_FAN2_TACH_OUT,
  MB_SNR_FAN3_TACH_IN,
  MB_SNR_FAN3_TACH_OUT,
  MB_SNR_FAN4_TACH_IN ,
  MB_SNR_FAN4_TACH_OUT,
  MB_SNR_FAN5_TACH_IN,
  MB_SNR_FAN5_TACH_OUT,
  MB_SNR_DATA0_DRIVER_TEMP,
  MB_SNR_DATA1_DRIVER_TEMP,
  MB_SNR_CPU0_PKG_POWER,
  MB_SNR_CPU1_PKG_POWER,
  MB_SNR_CPU0_TJMAX,
  MB_SNR_CPU1_TJMAX,
  MB_SNR_CPU0_THERM_MARGIN,
  MB_SNR_CPU1_THERM_MARGIN,
  MB_SNR_P3V3_STBY_INA260_VOL,
  MB_SNR_P3V3_STBY_INA260_CURR,
  MB_SNR_P3V3_STBY_INA260_POWER,
  MB_SNR_P3V3_M2_1_INA260_VOL,
  MB_SNR_P3V3_M2_1_INA260_CURR,
  MB_SNR_P3V3_M2_1_INA260_POWER,
  MB_SNR_P3V3_M2_2_INA260_VOL,
  MB_SNR_P3V3_M2_2_INA260_CURR,
  MB_SNR_P3V3_M2_2_INA260_POWER,
  MB_SNR_P3V3_M2_3_INA260_VOL,
  MB_SNR_P3V3_M2_3_INA260_CURR,
  MB_SNR_P3V3_M2_3_INA260_POWER,
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
  MB_SNR_VR_PCH_P1V05_VOLT,
  MB_SNR_VR_PCH_P1V05_TEMP,
  MB_SNR_VR_PCH_P1V05_CURR,
  MB_SNR_VR_PCH_P1V05_POWER,
  MB_SNR_VR_PCH_PVNN_VOLT,
  MB_SNR_VR_PCH_PVNN_TEMP,
  MB_SNR_VR_PCH_PVNN_CURR,
  MB_SNR_VR_PCH_PVNN_POWER,
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
};

const uint8_t nic0_sensor_list[] = {
  NIC_MEZZ0_SNR_TEMP,
};

const uint8_t nic1_sensor_list[] = {
  NIC_MEZZ1_SNR_TEMP,
};

const uint8_t fcb_sensor_list[] = {
  FCB_FAN2_VOLT,
  FCB_FAN2_CURR,
  FCB_FAN2_PWR,
  FCB_FAN3_VOLT,
  FCB_FAN3_CURR,
  FCB_FAN3_PWR,
  FCB_FAN4_VOLT,
  FCB_FAN4_CURR,
  FCB_FAN4_PWR,
  FCB_FAN5_VOLT,
  FCB_FAN5_CURR,
  FCB_FAN5_PWR,
};

const uint8_t riser1_sensor_list[] = {
  RISER1_SNR_AVAII_FTEMP,
  RISER1_SNR_AVAII_RTEMP,
  RISER1_SNR_SLOT0_NVME_TEMP,
  RISER1_SNR_SLOT1_NVME_TEMP,
  RISER1_SNR_SLOT2_NVME_TEMP,
  RISER1_SNR_SLOT3_NVME_TEMP,
};

const uint8_t riser2_sensor_list[] = {
  RISER2_SNR_AVAII_FTEMP,
  RISER2_SNR_AVAII_RTEMP,
  RISER2_SNR_SLOT0_NVME_TEMP,
  RISER2_SNR_SLOT1_NVME_TEMP,
  RISER2_SNR_SLOT2_NVME_TEMP,
  RISER2_SNR_SLOT3_NVME_TEMP,
};

// List of MB discrete sensors to be monitored
const uint8_t mb_discrete_sensor_list[] = {
//  MB_SENSOR_POWER_FAIL,
//  MB_SENSOR_MEMORY_LOOP_FAIL,
  MB_SENSOR_PROCESSOR_FAIL,
};

//CPU
PAL_CPU_INFO cpu_info_list[] = {
  {CPU_ID0, PECI_CPU0_ADDR},
  {CPU_ID1, PECI_CPU1_ADDR},
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
  {HSC_ID0, ADM1278_SLAVE_ADDR, adm1278_info_list},
};

//NM
PAL_I2C_BUS_INFO nm_info_list[] = {
  {NM_ID0, NM_IPMB_BUS_ID, NM_SLAVE_ADDR},
};

//TPM421
PAL_I2C_BUS_INFO tmp421_info_list[] = {
  {TEMP_INLET, I2C_BUS_19, 0x4C},
  {TEMP_OUTLET_R, I2C_BUS_19, 0x4E},
  {TEMP_OUTLET_L, I2C_BUS_19, 0x4F},
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

typedef struct _inlet_corr_t {
  uint8_t duty;
  float delta_t;
} inlet_corr_t;

static inlet_corr_t g_ict[] = {
  // Inlet Sensor:
  // duty cycle vs delta_t
  { 10, 7.67 },
  { 16, 5.66 },
  { 30, 2.74 },
  { 40, 2.51 },
  { 50, 1.68 },
  { 60, 1.69 },
  { 70, 1.51 },
  { 80, 1.66 },
  { 90, 0.86 },
  {100, 1.26 },
};

static uint8_t g_ict_count = sizeof(g_ict)/sizeof(inlet_corr_t);

static inlet_corr_t g_remote_ict[] = {
  // Inlet Sensor:
  // duty cycle vs delta_t
  { 10, 2.87 },
  { 12, 3.15 },
  { 14, 1.67 },
  { 16, 1.22 },
  { 18, 1.09 },
  { 20, 0.78 },
  { 22, 0.81 },
  { 24, 0.86 },
  { 26, 0.73 },
  { 28, 0.62 },
  { 30, 0.53 },
  { 40, 0.37 },
  { 50, 0.21 },
  { 75, 0.16 },
  { 100, -0.82 },
};

static uint8_t g_remote_ict_count = sizeof(g_remote_ict)/sizeof(inlet_corr_t);

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
  {"MB_PCH_TEMP", NM_ID0, read_NM_pch_temp, false, {80, 0, 0, 10, 0, 0, 0, 0}, TEMP}, //0x08
  {NULL, 0, NULL, 0, {0, 0, 0, 0, 0, 0, 0, 0}, 0}, //0x09
  {NULL, 0, NULL, 0, {0, 0, 0, 0, 0, 0, 0, 0}, 0}, //0x0A
  {NULL, 0, NULL, 0, {0, 0, 0, 0, 0, 0, 0, 0}, 0}, //0x0B
  {NULL, 0, NULL, 0, {0, 0, 0, 0, 0, 0, 0, 0}, 0}, //0x0C
  {NULL, 0, NULL, 0, {0, 0, 0, 0, 0, 0, 0, 0}, 0}, //0x0D
  {NULL, 0, NULL, 0, {0, 0, 0, 0, 0, 0, 0, 0}, 0}, //0x0E
  {NULL, 0, NULL, 0, {0, 0, 0, 0, 0, 0, 0, 0}, 0}, //0x0F

  {"NIC_MEZZ0_TEMP", MEZZ0, read_nic_temp, true, {0, 0, 0, 10, 0, 0, 0, 0}, TEMP},  //0x10
  {"NIC_MEZZ1_TEMP", MEZZ1, read_nic_temp, true, {0, 0, 0, 10, 0, 0, 0, 0}, TEMP},  //0x11
  {"FCB_FAN0_VOLT", FAN_ID4, read_fan_volt, true, {13.2, 0, 0, 10.8, 0, 0, 0, 0}, VOLT}, //0x12
  {"FCB_FAN0_CURR", FAN_ID4, read_fan_curr, true, {0, 0, 0, 0, 0, 0, 0, 0}, CURR}, //0x13
  {"FCB_FAN0_PWR", FAN_ID4, read_fan_pwr, true, {0, 0, 0, 0, 0, 0, 0, 0}, POWER},  //0x14
  {"FCB_FAN1_VOLT", FAN_ID5, read_fan_volt, true, {13.2, 0, 0, 10.8, 0, 0, 0, 0}, VOLT}, //0x15
  {"FCB_FAN1_CURR", FAN_ID5, read_fan_curr, true, {0, 0, 0, 0, 0, 0, 0, 0}, CURR}, //0x16
  {"FCB_FAN1_PWR", FAN_ID5, read_fan_pwr, true, {0, 0, 0, 0, 0, 0, 0, 0}, POWER},  //0x17
  {"FCB_FAN2_VOLT", FAN_ID6, read_fan_volt, true, {13.2, 0, 0, 10.8, 0, 0, 0, 0}, VOLT}, //0x18
  {"FCB_FAN2_CURR", FAN_ID6, read_fan_curr, true, {0, 0, 0, 0, 0, 0, 0, 0}, CURR}, //0x19
  {"FCB_FAN2_PWR", FAN_ID6, read_fan_pwr, true, {0, 0, 0, 0, 0, 0, 0, 0}, POWER},  //0x1A
  {"FCB_FAN3_VOLT", FAN_ID7, read_fan_volt, true, {13.2, 0, 0, 10.8, 0, 0, 0, 0}, VOLT}, //0x1B
  {"FCB_FAN3_CURR", FAN_ID7, read_fan_curr, true, {0, 0, 0, 0, 0, 0, 0, 0}, CURR}, //0x1C
  {"FCB_FAN3_PWR", FAN_ID7, read_fan_pwr, true, {0, 0, 0, 0, 0, 0, 0, 0}, POWER},  //0x1D
  {"FCB_FAN4_VOLT", FAN_ID0, read_fan_volt, true, {13.2, 0, 0, 10.8, 0, 0, 0, 0}, VOLT}, //0x1E
  {"FCB_FAN4_CURR", FAN_ID0, read_fan_curr, true, {0, 0, 0, 0, 0, 0, 0, 0}, CURR}, //0x1F

  {"FCB_FAN4_PWR", FAN_ID0, read_fan_pwr, true, {0, 0, 0, 0, 0, 0, 0, 0}, POWER},  //0x20
  {"FCB_FAN5_VOLT", FAN_ID1, read_fan_volt, true, {13.2, 0, 0, 10.8, 0, 0, 0, 0}, VOLT}, //0x21
  {"FCB_FAN5_CURR", FAN_ID1, read_fan_curr, true, {0, 0, 0, 0, 0, 0, 0, 0}, CURR}, //0x22
  {"FCB_FAN5_PWR", FAN_ID1, read_fan_pwr, true, {0, 0, 0, 0, 0, 0, 0, 0}, POWER},  //0x23
  {"FCB_FAN6_VOLT", FAN_ID2, read_fan_volt, true, {13.2, 0, 0, 10.8, 0, 0, 0, 0}, VOLT}, //0x24
  {"FCB_FAN6_CURR", FAN_ID2, read_fan_curr, true, {0, 0, 0, 0, 0, 0, 0, 0}, CURR}, //0x25
  {"FCB_FAN6_PWR", FAN_ID2, read_fan_pwr, true, {0, 0, 0, 0, 0, 0, 0, 0}, POWER},  //0x26
  {"FCB_FAN7_VOLT", FAN_ID3, read_fan_volt, true, {13.2, 0, 0, 10.8, 0, 0, 0, 0}, VOLT}, //0x27
  {"FCB_FAN7_CURR", FAN_ID3, read_fan_curr, true, {0, 0, 0, 0, 0, 0, 0, 0}, CURR}, //0x28
  {"FCB_FAN7_PWR", FAN_ID3, read_fan_pwr, true, {0, 0, 0, 0, 0, 0, 0, 0}, POWER},  //0x29
  {NULL, 0, NULL, 0, {0, 0, 0, 0, 0, 0, 0, 0}, 0}, //0x2A
  {NULL, 0, NULL, 0, {0, 0, 0, 0, 0, 0, 0, 0}, 0}, //0x2B
  {NULL, 0, NULL, 0, {0, 0, 0, 0, 0, 0, 0, 0}, 0}, //0x2C
  {NULL, 0, NULL, 0, {0, 0, 0, 0, 0, 0, 0, 0}, 0}, //0x2D
  {NULL, 0, NULL, 0, {0, 0, 0, 0, 0, 0, 0, 0}, 0}, //0x2E
  {NULL, 0, NULL, 0, {0, 0, 0, 0, 0, 0, 0, 0}, 0}, //0x2F

  {"MB_CPU0_TJMAX", CPU_ID0, read_cpu_tjmax, false, {0, 0, 0, 10, 0, 0, 0, 0}, TEMP}, //0x30
  {"MB_CPU1_TJMAX", CPU_ID1, read_cpu_tjmax, false, {0, 0, 0, 10, 0, 0, 0, 0}, TEMP}, //0x31
  {"MB_CPU0_PKG_POWER", CPU_ID0, read_cpu_pkg_pwr, false, {0, 0, 0, 0, 0, 0, 0, 0}, POWER}, //0x32
  {"MB_CPU1_PKG_POWER", CPU_ID1, read_cpu_pkg_pwr, false, {0, 0, 0, 0, 0, 0, 0, 0}, POWER}, //0x33
  {"MB_CPU0_THERM_MARGIN", CPU_ID0, read_cpu_thermal_margin, false, {-5, 0, 0, -76, 0, 0, 0, 0}, TEMP}, //0x34
  {"MB_CPU1_THERM_MARGIN", CPU_ID1, read_cpu_thermal_margin, false, {-5, 0, 0, -76, 0, 0, 0, 0}, TEMP}, //0x35
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

  {"MB_HSC_VIN",  HSC_ID0, read_hsc_vin,  true, {13.2, 0, 0, 10.8, 0, 0, 0, 0}, VOLT}, //0x40
  {"MB_HSC_IOUT", HSC_ID0, read_hsc_iout, true, {0, 0, 0, 0, 0, 0, 0, 0}, CURR}, //0x41
  {"MB_HSC_PIN",  HSC_ID0, read_hsc_pin,  true, {0, 0, 0, 0, 0, 0, 0, 0}, POWER}, //0x42
  {"MB_HSC_TEMP", HSC_ID0, read_hsc_temp, true, {80, 0, 0, 10, 0, 0, 0, 0}, TEMP}, //0x43
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

  {"MB_CPU0_DIMM_A_TEMP", DIMM_CRPA, read_cpu0_dimm_temp, false, {80, 0, 0, -1, 0, 0, 0, 0}, TEMP}, //0x50
  {"MB_CPU0_DIMM_B_TEMP", DIMM_CRPB, read_cpu0_dimm_temp, false, {80, 0, 0, -1, 0, 0, 0, 0}, TEMP}, //0x51
  {"MB_CPU0_DIMM_C_TEMP", DIMM_CRPC, read_cpu0_dimm_temp, false, {80, 0, 0, -1, 0, 0, 0, 0}, TEMP}, //0x52
  {"MB_CPU0_DIMM_D_TEMP", DIMM_CRPD, read_cpu0_dimm_temp, false, {80, 0, 0, -1, 0, 0, 0, 0}, TEMP}, //0x53
  {"MB_CPU0_DIMM_E_TEMP", DIMM_CRPE, read_cpu0_dimm_temp, false, {80, 0, 0, -1, 0, 0, 0, 0}, TEMP}, //0x54
  {"MB_CPU0_DIMM_F_TEMP", DIMM_CRPF, read_cpu0_dimm_temp, false, {80, 0, 0, -1, 0, 0, 0, 0}, TEMP}, //0x55
  {"MB_CPU1_DIMM_A_TEMP", DIMM_CRPA, read_cpu1_dimm_temp, false, {80, 0, 0, -1, 0, 0, 0, 0}, TEMP}, //0x56
  {"MB_CPU1_DIMM_B_TEMP", DIMM_CRPB, read_cpu1_dimm_temp, false, {80, 0, 0, -1, 0, 0, 0, 0}, TEMP}, //0x57
  {"MB_CPU1_DIMM_C_TEMP", DIMM_CRPC, read_cpu1_dimm_temp, false, {80, 0, 0, -1, 0, 0, 0, 0}, TEMP}, //0x58
  {"MB_CPU1_DIMM_D_TEMP", DIMM_CRPD, read_cpu1_dimm_temp, false, {80, 0, 0, -1, 0, 0, 0, 0}, TEMP}, //0x59
  {"MB_CPU1_DIMM_E_TEMP", DIMM_CRPE, read_cpu1_dimm_temp, false, {80, 0, 0, -1, 0, 0, 0, 0}, TEMP}, //0x5A
  {"MB_CPU1_DIMM_F_TEMP", DIMM_CRPF, read_cpu1_dimm_temp, false, {80, 0, 0, -1, 0, 0, 0, 0}, TEMP}, //0x5B
  {NULL, 0, NULL, 0, {0, 0, 0, 0, 0, 0, 0, 0}, 0}, //0x5C
  {NULL, 0, NULL, 0, {0, 0, 0, 0, 0, 0, 0, 0}, 0}, //0x5D
  {NULL, 0, NULL, 0, {0, 0, 0, 0, 0, 0, 0, 0}, 0}, //0x5E
  {NULL, 0, NULL, 0, {0, 0, 0, 0, 0, 0, 0, 0}, 0}, //0x5F

  {"MB_FAN0_TACH_IN",  0, read_fan_speed, false, {27940, 0, 0, 2825, 0, 0, 0, 0}, FAN}, //0x60
  {"MB_FAN0_TACH_OUT", 1, read_fan_speed, false, {25630, 0, 0, 1795, 0, 0, 0, 0}, FAN}, //0x61
  {"MB_FAN1_TACH_IN",  2, read_fan_speed, false, {27940, 0, 0, 2825, 0, 0, 0, 0}, FAN}, //0x62
  {"MB_FAN1_TACH_OUT", 3, read_fan_speed, false, {25630, 0, 0, 1795, 0, 0, 0, 0}, FAN}, //0x63
  {"MB_FAN2_TACH_IN",  4, read_fan_speed, false, {27940, 0, 0, 2825, 0, 0, 0, 0}, FAN}, //0x64
  {"MB_FAN2_TACH_OUT", 5, read_fan_speed, false, {25630, 0, 0, 1795, 0, 0, 0, 0}, FAN}, //0x65
  {"MB_FAN3_TACH_IN",  6, read_fan_speed, false, {27940, 0, 0, 2825, 0, 0, 0, 0}, FAN}, //0x66
  {"MB_FAN3_TACH_OUT", 7, read_fan_speed, false, {25630, 0, 0, 1795, 0, 0, 0, 0}, FAN}, //0x67
  {"MB_FAN4_TACH_IN",  8, read_fan_speed, false, {27940, 0, 0, 2825, 0, 0, 0, 0}, FAN}, //0x68
  {"MB_FAN4_TACH_OUT", 9, read_fan_speed, false, {25630, 0, 0, 1795, 0, 0, 0, 0}, FAN}, //0x69
  {"MB_FAN5_TACH_IN", 10, read_fan_speed, false, {27940, 0, 0, 2825, 0, 0, 0, 0}, FAN}, //0x6A
  {"MB_FAN5_TACH_OUT",11, read_fan_speed, false, {25630, 0, 0, 1795, 0, 0, 0, 0}, FAN}, //0x6B
  {"MB_FAN6_TACH_IN", 12, read_fan_speed, false, {27940, 0, 0, 2825, 0, 0, 0, 0}, FAN}, //0x6C
  {"MB_FAN6_TACH_OUT",13, read_fan_speed, false, {25630, 0, 0, 1795, 0, 0, 0, 0}, FAN}, //0x6D
  {"MB_FAN7_TACH_IN", 14, read_fan_speed, false, {27940, 0, 0, 2825, 0, 0, 0, 0}, FAN}, //0x6E
  {"MB_FAN7_TACH_OUT",15, read_fan_speed, false, {25630, 0, 0, 1795, 0, 0, 0, 0}, FAN}, //0x6F

  {"MB_BOOT_DRIVER_TEMP",  DISK_BOOT,  read_hd_temp, true, {0, 0, 0, 0, 0, 0, 0, 0}, TEMP}, //0x70
  {"MB_DATA0_DRIVER_TEMP", DISK_DATA0, read_hd_temp, true, {70, 0, 0, 10, 0, 0, 0, 0}, TEMP}, //0x71
  {"MB_DATA1_DRIVER_TEMP", DISK_DATA1, read_hd_temp, true, {70, 0, 0, 10, 0, 0, 0, 0}, TEMP}, //0x72
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

  {"RISER1_AVAII_FTEMP"    , RISER1_SNR_AVAII_FTEMP    , read_ava_temp , true, {0, 0, 0, 0, 0, 0, 0, 0}, TEMP}, //0x80
  {"RISER1_AVAII_RTEMP"    , RISER1_SNR_AVAII_RTEMP    , read_ava_temp , true, {0, 0, 0, 0, 0, 0, 0, 0}, TEMP}, //0x81
  {"RISER2_AVAII_FTEMP"    , RISER2_SNR_AVAII_FTEMP    , read_ava_temp , true, {0, 0, 0, 0, 0, 0, 0, 0}, TEMP}, //0x82
  {"RISER2_AVAII_RTEMP"    , RISER2_SNR_AVAII_RTEMP    , read_ava_temp , true, {0, 0, 0, 0, 0, 0, 0, 0}, TEMP}, //0x83
  {"RISER1_SLOT0_NVME_TEMP", RISER1_SNR_SLOT0_NVME_TEMP, read_nvme_temp, true, {0, 0, 0, 0, 0, 0, 0, 0}, TEMP}, //0x84
  {"RISER1_SLOT1_NVME_TEMP", RISER1_SNR_SLOT1_NVME_TEMP, read_nvme_temp, true, {0, 0, 0, 0, 0, 0, 0, 0}, TEMP}, //0x85
  {"RISER1_SLOT2_NVME_TEMP", RISER1_SNR_SLOT2_NVME_TEMP, read_nvme_temp, true, {0, 0, 0, 0, 0, 0, 0, 0}, TEMP}, //0x86
  {"RISER1_SLOT3_NVME_TEMP", RISER1_SNR_SLOT3_NVME_TEMP, read_nvme_temp, true, {0, 0, 0, 0, 0, 0, 0, 0}, TEMP}, //0x87
  {"RISER2_SLOT0_NVME_TEMP", RISER2_SNR_SLOT0_NVME_TEMP, read_nvme_temp, true, {0, 0, 0, 0, 0, 0, 0, 0}, TEMP}, //0x88
  {"RISER2_SLOT1_NVME_TEMP", RISER2_SNR_SLOT1_NVME_TEMP, read_nvme_temp, true, {0, 0, 0, 0, 0, 0, 0, 0}, TEMP}, //0x89
  {"RISER2_SLOT2_NVME_TEMP", RISER2_SNR_SLOT2_NVME_TEMP, read_nvme_temp, true, {0, 0, 0, 0, 0, 0, 0, 0}, TEMP}, //0x8A
  {"RISER2_SLOT3_NVME_TEMP", RISER2_SNR_SLOT3_NVME_TEMP, read_nvme_temp, true, {0, 0, 0, 0, 0, 0, 0, 0}, TEMP}, //0x8B
  {NULL, 0, NULL, 0, {0, 0, 0, 0, 0, 0, 0, 0}, 0}, //0x8C
  {NULL, 0, NULL, 0, {0, 0, 0, 0, 0, 0, 0, 0}, 0}, //0x8D
  {NULL, 0, NULL, 0, {0, 0, 0, 0, 0, 0, 0, 0}, 0}, //0x8E
  {NULL, 0, NULL, 0, {0, 0, 0, 0, 0, 0, 0, 0}, 0}, //0x8F

  {"MB_P3V3_STBY_INA260_VOL", MB_SNR_P3V3_STBY_INA260_VOL, read_ina260_sensor, true, {13.2, 0, 0, 10.8, 0, 0, 0, 0}, VOLT}, //0x90
  {"MB_P3V3_STBY_INA260_CURR", MB_SNR_P3V3_STBY_INA260_CURR, read_ina260_sensor, true, {0, 0, 0, 0, 0, 0, 0, 0}, CURR}, //0x91
  {"MB_P3V3_STBY_INA260_POWER", MB_SNR_P3V3_STBY_INA260_POWER, read_ina260_sensor, true, {0, 0, 0, 0, 0, 0, 0, 0}, POWER},  //0x92
  {"MB_P3V3_M2_1_INA260_VOL", MB_SNR_P3V3_M2_1_INA260_VOL, read_ina260_sensor, false, {3.465, 0, 0, 3.135, 0, 0, 0, 0}, VOLT}, //0x93
  {"MB_P3V3_M2_1_INA260_CURR", MB_SNR_P3V3_M2_1_INA260_CURR, read_ina260_sensor, false, {0, 0, 0, 0, 0, 0, 0, 0}, CURR},  //0x94
  {"MB_P3V3_M2_1_INA260_POWER", MB_SNR_P3V3_M2_1_INA260_POWER, read_ina260_sensor, false, {0, 0, 0, 0, 0, 0, 0, 0}, POWER}, //0x95
  {"MB_P3V3_M2_2_INA260_VOL", MB_SNR_P3V3_M2_2_INA260_VOL, read_ina260_sensor, false, {3.465, 0, 0, 3.135, 0, 0, 0, 0}, VOLT}, //0x96
  {"MB_P3V3_M2_2_INA260_CURR", MB_SNR_P3V3_M2_2_INA260_CURR, read_ina260_sensor, false, {0, 0, 0, 0, 0, 0, 0, 0}, CURR}, //0x97
  {"MB_P3V3_M2_2_INA260_POWER", MB_SNR_P3V3_M2_2_INA260_POWER, read_ina260_sensor, false, {0, 0, 0, 0, 0, 0, 0, 0}, POWER}, //0x98
  {"MB_P3V3_M2_3_INA260_VOL", MB_SNR_P3V3_M2_3_INA260_VOL, read_ina260_sensor, false, {3.465, 0, 0, 3.135, 0, 0, 0, 0}, VOLT}, //0x99
  {"MB_P3V3_M2_3_INA260_CURR", MB_SNR_P3V3_M2_3_INA260_CURR, read_ina260_sensor, false, {0, 0, 0, 0, 0, 0, 0, 0}, CURR}, //0x9A
  {"MB_P3V3_M2_3_INA260_POWER", MB_SNR_P3V3_M2_3_INA260_POWER, read_ina260_sensor, false, {0, 0, 0, 0, 0, 0, 0, 0}, POWER}, //0x9B
  {NULL, 0, NULL, 0, {0, 0, 0, 0, 0, 0, 0, 0}, 0}, //0x9C
  {NULL, 0, NULL, 0, {0, 0, 0, 0, 0, 0, 0, 0}, 0}, //0x9D
  {NULL, 0, NULL, 0, {0, 0, 0, 0, 0, 0, 0, 0}, 0}, //0x9E
  {NULL, 0, NULL, 0, {0, 0, 0, 0, 0, 0, 0, 0}, 0}, //0x9F

  {"MB_INLET_TEMP",    TEMP_INLET,    read_sensor, true, {40, 0, 0, 10, 0, 0, 0, 0}, TEMP}, //0xA0
  {"MB_OUTLET_TEMP_R", TEMP_OUTLET_R, read_sensor, true, {80, 0, 0, 10, 0, 0, 0, 0}, TEMP}, //0xA1
  {"MB_OUTLET_TEMP_L", TEMP_OUTLET_L, read_sensor, true, {80, 0, 0, 10, 0, 0, 0, 0}, TEMP}, //0xA2
  {"MB_INLET_REMOTE_TEMP",    TEMP_REMOTE_INLET,    read_sensor, true, {40, 0, 0, 10, 0, 0, 0, 0}, TEMP}, //0xA3
  {"MB_OUTLET_REMOTE_TEMP_R", TEMP_REMOTE_OUTLET_R, read_sensor, true, {80, 0, 0, 10, 0, 0, 0, 0}, TEMP}, //0xA4
  {"MB_OUTLET_REMOTE_TEMP_L", TEMP_REMOTE_OUTLET_L, read_sensor, true, {80, 0, 0, 10, 0, 0, 0, 0}, TEMP}, //0xA5
  {NULL, 0, NULL, 0, {0, 0, 0, 0, 0, 0, 0, 0}, 0}, //0xA6
  {NULL, 0, NULL, 0, {0, 0, 0, 0, 0, 0, 0, 0}, 0}, //0xA7
  {NULL, 0, NULL, 0, {0, 0, 0, 0, 0, 0, 0, 0}, 0}, //0xA8
  {NULL, 0, NULL, 0, {0, 0, 0, 0, 0, 0, 0, 0}, 0}, //0xA9
  {"MB_CPU0_TEMP", CPU_ID0, read_cpu_temp, false, {80, 0, 0, 5, 0, 0, 0, 0}, TEMP}, //0xAA
  {"MB_CPU1_TEMP", CPU_ID1, read_cpu_temp, false, {80, 0, 0, 5, 0, 0, 0, 0}, TEMP}, //0xAB
  {NULL, 0, NULL, 0, {0, 0, 0, 0, 0, 0, 0, 0}, 0}, //0xAC
  {NULL, 0, NULL, 0, {0, 0, 0, 0, 0, 0, 0, 0}, 0}, //0xAD
  {NULL, 0, NULL, 0, {0, 0, 0, 0, 0, 0, 0, 0}, 0}, //0xAE
  {NULL, 0, NULL, 0, {0, 0, 0, 0, 0, 0, 0, 0}, 0}, //0xAF

  {"MB_VR_CPU0_VCCIN_VOUT", VR_ID0, read_vr_vout, false, {2.1, 0, 0, 1.35, 0, 0, 0, 0}, VOLT}, //0xB0
  {"MB_VR_CPU0_VCCIN_TEMP", VR_ID0, read_vr_temp, false, {100, 0, 0, 10, 0, 0, 0, 0}, TEMP}, //0xB1
  {"MB_VR_CPU0_VCCIN_IOUT", VR_ID0, read_vr_iout, false, {0, 0, 0, 0, 0, 0, 0, 0}, CURR}, //0XB2
  {"MB_VR_CPU0_VCCIN_POUT", VR_ID0, read_vr_pout, false, {0, 0, 0, 0, 0, 0, 0, 0}, POWER}, //0xB3
  {"MB_VR_CPU0_VCCSA_VOUT", VR_ID1, read_vr_vout, false, {1.25, 0, 0, 0.4, 0, 0, 0, 0}, VOLT}, //0xB4
  {"MB_VR_CPU0_VCCSA_TEMP", VR_ID1, read_vr_temp, false, {100, 0, 0, 10, 0, 0, 0, 0}, TEMP}, //0xB5
  {"MB_VR_CPU0_VCCSA_IOUT", VR_ID1, read_vr_iout, false, {0, 0, 0, 0, 0, 0, 0, 0}, CURR}, //0xB6
  {"MB_VR_CPU0_VCCSA_POUT", VR_ID1, read_vr_pout, false, {0, 0, 0, 0, 0, 0, 0, 0}, POWER}, //0xB7
  {"MB_VR_CPU0_VCCIO_VOUT", VR_ID2, read_vr_vout, false, {1.25, 0, 0, 0.8, 0, 0, 0, 0}, VOLT}, //0xB8
  {"MB_VR_CPU0_VCCIO_TEMP", VR_ID2, read_vr_temp, false, {0, 0, 0, 10, 0, 0, 0, 0}, TEMP}, //0xB9
  {"MB_VR_CPU0_VCCIO_IOUT", VR_ID2, read_vr_iout, false, {0, 0, 0, 0, 0, 0, 0, 0}, CURR}, //0xBA
  {"MB_VR_CPU0_VCCIO_POUT", VR_ID2, read_vr_pout, false, {0, 0, 0, 0, 0, 0, 0, 0}, POWER}, //0xBB
  {"MB_VR_CPU0_VDDQ_ABC_VOUT", VR_ID3, read_vr_vout, false, {1.4, 0, 0, 1.09, 0, 0, 0, 0}, VOLT}, //0xBC
  {"MB_VR_CPU0_VDDQ_ABC_TEMP", VR_ID3, read_vr_temp, false, {100, 0, 0, 10, 0, 0, 0, 0}, TEMP}, //0xBD
  {"MB_VR_CPU0_VDDQ_ABC_IOUT", VR_ID3, read_vr_iout, false, {0, 0, 0, 0, 0, 0, 0, 0}, CURR}, //0xBE
  {"MB_VR_CPU0_VDDQ_ABC_POUT", VR_ID3, read_vr_pout, false, {0, 0, 0, 0, 0, 0, 0, 0}, POWER}, //0xBF

  {"MB_VR_CPU0_VDDQ_DEF_VOUT", VR_ID4, read_vr_vout, false, {1.4, 0, 0, 1.09, 0, 0, 0, 0}, VOLT}, //0xC0
  {"MB_VR_CPU0_VDDQ_DEF_TEMP", VR_ID4, read_vr_temp, false, {100, 0, 0, 10, 0, 0, 0, 0}, TEMP}, //0xC1
  {"MB_VR_CPU0_VDDQ_DEF_IOUT", VR_ID4, read_vr_iout, false, {0, 0, 0, 0, 0, 0, 0, 0}, CURR}, //0xC2
  {"MB_VR_CPU0_VDDQ_DEF_POUT", VR_ID4, read_vr_pout, false, {0, 0, 0, 0, 0, 0, 0, 0}, POWER}, //0xC3
  {"MB_VR_PCH_P1V05_VOLT", VR_ID10, read_vr_vout, true, {1.2, 0, 0, 1.0085, 0, 0, 0, 0}, VOLT}, //0xC4
  {"MB_VR_PCH_P1V05_TEMP", VR_ID10, read_vr_temp, true, {115, 0, 0, 10, 0, 0, 0, 0}, TEMP}, //0xC5
  {"MB_VR_PCH_P1V05_IOUT", VR_ID10, read_vr_iout, true, {0, 0, 0, 0, 0, 0, 0, 0}, CURR}, //0xC6
  {"MB_VR_PCH_P1V05_POUT", VR_ID10, read_vr_pout, true, {0, 0, 0, 0, 0, 0, 0, 0}, POWER}, //0xC7
  {"MB_VR_PCH_PVNN_VOLT", VR_ID11, read_vr_vout, true, {1.2, 0, 0, 0.80175, 0, 0, 0, 0}, VOLT}, //0xC8
  {"MB_VR_PCH_PVNN_TEMP", VR_ID11, read_vr_temp, true, {115, 0, 0, 10, 0, 0, 0, 0}, TEMP}, //0xC9
  {"MB_VR_PCH_PVNN_IOUT", VR_ID11, read_vr_iout, true, {0, 0, 0, 0, 0, 0, 0, 0}, CURR}, //0xCA
  {"MB_VR_PCH_PVNN_POUT", VR_ID11, read_vr_pout, true, {0, 0, 0, 0, 0, 0, 0, 0}, POWER}, //0xCB
  {NULL, 0, NULL, 0, {0, 0, 0, 0, 0, 0, 0, 0}, 0}, //0xCC
  {NULL, 0, NULL, 0, {0, 0, 0, 0, 0, 0, 0, 0}, 0}, //0xCD
  {NULL, 0, NULL, 0, {0, 0, 0, 0, 0, 0, 0, 0}, 0}, //0xCE
  {NULL, 0, NULL, 0, {0, 0, 0, 0, 0, 0, 0, 0}, 0}, //0xCF

  {"MB_P5V",       ADC0, read_adc_val, false, {5.25, 0, 0, 4.75, 0, 0, 0, 0}, VOLT}, //0xD0
  {"MB_P5V_STBY",  ADC1, read_adc_val, true,  {5.25, 0, 0, 4.75, 0, 0, 0, 0}, VOLT}, //0xD1
  {"MB_P3V3_STBY", ADC2, read_adc_val, true,  {3.465, 0, 0, 3.135, 0, 0, 0, 0}, VOLT}, //0xD2
  {"MB_P3V3",      ADC3, read_adc_val, false, {3.465, 0, 0, 3.135, 0, 0, 0, 0}, VOLT}, //0xD3
  {"MB_P3V_BAT",   ADC4, read_battery_val, false, {3.4, 0, 0, 2.6, 0, 0, 0, 0}, VOLT}, //0xD4
  {"MB_CPU_1V8",   ADC5, read_adc_val, false, {0, 0, 0, 0, 0, 0, 0, 0}, VOLT}, //0xD5
  {"MB_PCH_1V8",   ADC6, read_adc_val, false, {0, 0, 0, 0, 0, 0, 0, 0}, VOLT}, //0xD6
  {"MB_CPU0_PVPP_ABC", ADC7,  read_adc_val, false, {0, 0, 0, 0, 0, 0, 0, 0}, VOLT}, //0xD7
  {"MB_CPU1_PVPP_ABC", ADC8,  read_adc_val, false, {0, 0, 0, 0, 0, 0, 0, 0}, VOLT}, //0xD8
  {"MB_CPU0_PVPP_DEF", ADC9,  read_adc_val, false, {0, 0, 0, 0, 0, 0, 0, 0}, VOLT}, //0xD9
  {"MB_CPU1_PVPP_DEF", ADC10, read_adc_val, false, {0, 0, 0, 0, 0, 0, 0, 0}, VOLT}, //0xDA
  {"MB_CPU0_PVTT_ABC", ADC11, read_adc_val, false, {0, 0, 0, 0, 0, 0, 0, 0}, VOLT}, //0xDB
  {"MB_CPU1_PVTT_ABC", ADC12, read_adc_val, false, {0, 0, 0, 0, 0, 0, 0, 0}, VOLT}, //0xDC
  {"MB_CPU0_PVTT_DEF", ADC13, read_adc_val, false, {0, 0, 0, 0, 0, 0, 0, 0}, VOLT}, //0xDD
  {"MB_CPU1_PVTT_DEF", ADC14, read_adc_val, false, {0, 0, 0, 0, 0, 0, 0, 0}, VOLT}, //0xDE
  {"MB_P12V_OCP_IOUT", ADC15, read_adc_val, true, {0, 0, 0, 0, 0, 0, 0, 0}, CURR}, //0xDF

  {"MB_VR_CPU1_VCCIN_VOUT", VR_ID5, read_vr_vout, false, {2.1, 0, 0, 1.35, 0, 0, 0, 0}, VOLT}, //0xE0
  {"MB_VR_CPU1_VCCIN_TEMP", VR_ID5, read_vr_temp, false, {100, 0, 0, 10, 0, 0, 0, 0}, TEMP}, //0xE1
  {"MB_VR_CPU1_VCCIN_IOUT", VR_ID5, read_vr_iout, false, {0, 0, 0, 0, 0, 0, 0, 0}, CURR}, //0xE2
  {"MB_VR_CPU1_VCCIN_POUT", VR_ID5, read_vr_pout, false, {0, 0, 0, 0, 0, 0, 0, 0}, POWER}, //0xE3
  {"MB_VR_CPU1_VCCSA_VOUT", VR_ID6, read_vr_vout, false, {1.25, 0, 0, 0.4, 0, 0, 0, 0}, VOLT}, //0xE4
  {"MB_VR_CPU1_VCCSA_TEMP", VR_ID6, read_vr_temp, false, {100, 0, 0, 10, 0, 0, 0, 0}, TEMP}, //0xE5
  {"MB_VR_CPU1_VCCSA_IOUT", VR_ID6, read_vr_iout, false, {0, 0, 0, 0, 0, 0, 0, 0}, CURR}, //0xE6
  {"MB_VR_CPU1_VCCSA_POUT", VR_ID6, read_vr_pout, false, {0, 0, 0, 0, 0, 0, 0, 0}, POWER}, //0xE7
  {"MB_VR_CPU1_VCCIO_VOUT", VR_ID7, read_vr_vout, false, {1.25, 0, 0, 0.8, 0, 0, 0, 0}, VOLT}, //0xE8
  {"MB_VR_CPU1_VCCIO_TEMP", VR_ID7, read_vr_temp, false, {0, 0, 0, 10, 0, 0, 0, 0}, TEMP}, //0xE9
  {"MB_VR_CPU1_VCCIO_IOUT", VR_ID7, read_vr_iout, false, {0, 0, 0, 0, 0, 0, 0, 0}, CURR}, //0xEA
  {"MB_VR_CPU1_VCCIO_POUT", VR_ID7, read_vr_pout, false, {0, 0, 0, 0, 0, 0, 0, 0}, POWER}, //0xEB
  {"MB_VR_CPU1_VDDQ_ABC_VOUT", VR_ID8, read_vr_vout, false, {1.4, 0, 0, 1.09, 0, 0, 0, 0}, VOLT}, //0xEC
  {"MB_VR_CPU1_VDDQ_ABC_TEMP", VR_ID8, read_vr_temp, false, {100, 0, 0, 10, 0, 0, 0, 0}, TEMP}, //0xED
  {"MB_VR_CPU1_VDDQ_ABC_IOUT", VR_ID8, read_vr_iout, false, {0, 0, 0, 0, 0, 0, 0, 0}, CURR}, //0xEE
  {"MB_VR_CPU1_VDDQ_ABC_POUT", VR_ID8, read_vr_pout, false, {0, 0, 0, 0, 0, 0, 0, 0}, POWER}, //0xEF

  {"MB_VR_CPU1_VDDQ_DEF_VOUT", VR_ID9, read_vr_vout, false, {1.4, 0, 0, 1.09, 0, 0, 0, 0}, VOLT}, //0xF0
  {"MB_VR_CPU1_VDDQ_DEF_TEMP", VR_ID9, read_vr_temp, false, {100, 0, 0, 10, 0, 0, 0, 0}, TEMP}, //0xF1
  {"MB_VR_CPU1_VDDQ_DEF_IOUT", VR_ID9, read_vr_iout, false, {0, 0, 0, 0, 0, 0, 0, 0}, CURR}, //0xF2
  {"MB_VR_CPU1_VDDQ_DEF_POUT", VR_ID9, read_vr_pout, false, {0, 0, 0, 0, 0, 0, 0, 0}, POWER}, //0xF3
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

size_t mb_sensor_cnt = sizeof(mb_sensor_list)/sizeof(uint8_t);
size_t nic0_sensor_cnt = sizeof(nic0_sensor_list)/sizeof(uint8_t);
size_t nic1_sensor_cnt = sizeof(nic1_sensor_list)/sizeof(uint8_t);
size_t mb_discrete_sensor_cnt = sizeof(mb_discrete_sensor_list)/sizeof(uint8_t);
size_t fcb_sensor_cnt = sizeof(fcb_sensor_list)/sizeof(uint8_t);
size_t riser1_sensor_cnt = sizeof(riser1_sensor_list)/sizeof(uint8_t);
size_t riser2_sensor_cnt = sizeof(riser2_sensor_list)/sizeof(uint8_t);

int
pal_get_fru_sensor_list(uint8_t fru, uint8_t **sensor_list, int *cnt) {
  switch(fru) {
  case FRU_MB:
    *sensor_list = (uint8_t *) mb_sensor_list;
    *cnt = mb_sensor_cnt;
    break;
  case FRU_NIC0:
    *sensor_list = (uint8_t *) nic0_sensor_list;
    *cnt = nic0_sensor_cnt;
    break;
  case FRU_NIC1:
    *sensor_list = (uint8_t *) nic1_sensor_list;
    *cnt = nic1_sensor_cnt;
    break;
  case FRU_FCB:
    *sensor_list = (uint8_t *) fcb_sensor_list;
    *cnt = fcb_sensor_cnt;
    break;
  case FRU_RISER1:
    *sensor_list = (uint8_t *) riser1_sensor_list;
    *cnt = riser1_sensor_cnt;
    break;
  case FRU_RISER2:
    *sensor_list = (uint8_t *) riser2_sensor_list;
    *cnt = riser2_sensor_cnt;
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
  case FRU_MB:
    *sensor_list = (uint8_t *) mb_discrete_sensor_list;
    *cnt = mb_discrete_sensor_cnt;
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
pal_set_fan_led_state(uint8_t fan_id, char* state) {
  char key[64] = {0};
  int completion_code = CC_UNSPECIFIED_ERROR;

  snprintf(key, sizeof(key), FAN_LED_STATE, fan_id);

  if (kv_set(key, state, 0, KV_FPERSIST) != 0)
    return completion_code;

  completion_code = CC_SUCCESS;

  return completion_code;
}

int
pal_get_fan_led_state(uint8_t fan_id, char* state) {
  char key[64] = {0};
  int completion_code = CC_UNSPECIFIED_ERROR;

  snprintf(key, sizeof(key), FAN_LED_STATE, fan_id);

  if(kv_get(key, state, NULL, KV_FPERSIST) < 0)
    return completion_code;

  completion_code = CC_SUCCESS;

  return completion_code;
}

static int
get_snr_state(int retry) {
  int ret = 0;

  // Check the count of retry to decide return code.
  if (retry <= MAX_SNR_READ_RETRY) {
    ret = READING_SKIP;
  } else {
    ret = READING_NA;
  }

  return ret;
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
    "MB_CPU_1V8",
    "MB_PCH_1V8",
    "MB_CPU0_PVPP_ABC",
    "MB_CPU1_PVPP_ABC",
    "MB_CPU0_PVPP_DEF",
    "MB_CPU1_PVPP_DEF",
    "MB_CPU0_PVTT_ABC",
    "MB_CPU1_PVTT_ABC",
    "MB_CPU0_PVTT_DEF",
    "MB_CPU1_PVTT_DEF",
    "MB_P12V_OCP_IOUT",
  };
  if (adc_id >= ARRAY_SIZE(adc_label)) {
    return -1;
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

int
pal_get_peci_val(struct peci_xfer_msg *msg) {
  int fd, retry;

  if ((fd = open(PECI_DEVICE, O_RDWR)) < 0) {
#ifdef DEBUG
    syslog(LOG_WARNING, "Failed to open %s\n", PECI_DEVICE);
#endif
    return -1;
  }

  retry = 0;
  do {
    if (peci_cmd_xfer_fd(fd, msg) < 0) {
#ifdef DEBUG
      syslog(LOG_WARNING, "peci_cmd_xfer_fd");
#endif
      close(fd);
      return -1;
    }

    if (PECI_RETRY_TIMES) {
      switch (msg->rx_buf[0]) {
      case 0x80:  // response timeout, Data not ready
      case 0x81:  // response timeout, not able to allocate resource
        if (retry < PECI_RETRY_TIMES) {
          #ifdef DEBUG
            syslog(LOG_DEBUG, "CC: %02Xh, retry...\n", msg->rx_buf[0]);
          #endif
          retry++;
          usleep(250*1000);
          continue;
        }
        break;
      case 0x40:  // command passed
      case 0x82:  // low power state
      case 0x90:  // unknown/invalid/illegal request
      case 0x91:  // error
      default:
        break;
      }
    }
    break;
  } while (retry <= PECI_RETRY_TIMES);

#ifdef DEBUG
{
  int i;
  for (i = 0; i < msg->rx_len; i++) {
    syslog(LOG_DEBUG, "rx_buf=%02X ", msg->rx_buf[i]);
  }
  syslog(LOG_DEBUG, "\n");
}
#endif
  close(fd);
  return 0;
}

static int
cmd_peci_get_temp(uint8_t cpu_addr, float *dts) {
  struct peci_xfer_msg msg;
  int ret;
  PAL_S10_6_FORMAT margin;
  uint16_t tmp;

  msg.addr = cpu_addr;
  msg.tx_len = 0x01;
  msg.rx_len = 0x02;
  msg.tx_buf[0] = 0x01;

  ret = pal_get_peci_val(&msg);
  if(ret != 0) {
    return -1;
  }

  tmp = (msg.rx_buf[1] << 8 | msg.rx_buf[0]);
  margin.fract = (tmp & 0x003F) * 0.016;
  margin.integer = tmp >> 6;

  if((0x80 & msg.rx_buf[1]) == 0) {
   *dts = (float) (margin.integer + margin.fract);
  } else {
   *dts = (float) (margin.integer - margin.fract);
  }

#ifdef DEBUG
  syslog(LOG_DEBUG, "%s value=%f\n", __func__, *dts);
#endif

  return 0;
}

static int
cmd_peci_rdpkgconfig(PECI_RD_PKG_CONFIG_INFO* info, uint8_t* rx_buf, uint8_t rx_len) {
  struct peci_xfer_msg msg;
  int ret;

  msg.addr = info->cpu_addr;
  msg.tx_len = 0x05;
  msg.rx_len = rx_len;
  msg.tx_buf[0] = PECI_CMD_RD_PKG_CONFIG;
  msg.tx_buf[1] = info->dev_info;
  msg.tx_buf[2] = info->index;
  msg.tx_buf[3] = info->para_l;
  msg.tx_buf[4] = info->para_h;

  ret = pal_get_peci_val(&msg);
  if(ret != 0) {
    syslog(LOG_DEBUG, "peci rdpkg error index=%x\n", info->index);
    return -1;
  }
  memcpy(rx_buf, msg.rx_buf, msg.rx_len);
  return 0;
}

int
cmd_peci_get_thermal_margin(uint8_t cpu_addr, float* value) {
  PECI_RD_PKG_CONFIG_INFO info;
  int ret;
  uint8_t rx_len=5;
  uint8_t rx_buf[rx_len];
  PAL_S10_6_FORMAT margin;
  uint16_t tmp;

  info.cpu_addr= cpu_addr;
  info.dev_info = 0x00;
  info.index = PECI_INDEX_THERMAL_MARGIN;
  info.para_l = 0x00;
  info.para_h = 0x00;

  ret = cmd_peci_rdpkgconfig(&info, rx_buf, rx_len);
  if(ret != 0) {
    return -1;
  }

  tmp = rx_buf[1] | (rx_buf[2] << 8);

  margin.integer = (int)(tmp >> 6);
  margin.fract = (0x003F & tmp) * 0.016;

#ifdef DEBUG
  syslog(LOG_DEBUG, "%s rxbuf[0]=%x, rxbuf[1]=%x tmp=%x\n", __func__, rx_buf[1], rx_buf[2], tmp);
#endif
  if((0x80 & rx_buf[1]) == 0) {
   *value = (float) (margin.integer + margin.fract);
  } else {
   *value = (float) (margin.integer - margin.fract);
  }
#ifdef DEBUG
  syslog(LOG_DEBUG, "%s value=%f\n", __func__, *value);
#endif
  return 0;
}

static int
cmd_peci_get_tjmax(uint8_t cpu_addr, int* tjmax) {
  PECI_RD_PKG_CONFIG_INFO info;
  int ret;
  uint8_t rx_len=5;
  uint8_t rx_buf[rx_len];
  static uint8_t cached=0;
  static int tjmax_cached=0;

  if (!cached) {
    info.cpu_addr= cpu_addr;
    info.dev_info = 0x00;
    info.index = PECI_INDEX_TEMP_TARGET;
    info.para_l = 0x00;
    info.para_h = 0x00;

    ret = cmd_peci_rdpkgconfig(&info, rx_buf, rx_len);
    if(ret != 0) {
      return -1;
    }
    tjmax_cached  = rx_buf[3];
    cached = 1;
  }

  *tjmax = tjmax_cached;
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
  if(ret != 0) {
    return -1;
  }

  *temp = rx_buf[PECI_THERMAL_DIMM0_BYTE];
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
  if(ret != 0) {
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
  if(ret != 0) {
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
  if(ret != 0) {
    return -1;
  }

//CATERR ERROR
  if( is_caterr ) {
    if((rx_buf[3] & BOTH_CPU_ERROR_MASK) == BOTH_CPU_ERROR_MASK)
      cpu_num = 2; //Both
    else if((rx_buf[3] & EXTERNAL_CPU_ERROR_MASK) > 0)
      cpu_num = 1; //CPU1
    else if((rx_buf[3] & INTERNAL_CPU_ERROR_MASK) > 0)
      cpu_num = 0; //CPU0
  } else {
//MSMI
    if(((rx_buf[2] & BOTH_CPU_ERROR_MASK) == BOTH_CPU_ERROR_MASK))
      cpu_num = 2; //Both
    else if((rx_buf[2] & EXTERNAL_CPU_ERROR_MASK) > 0)
      cpu_num = 1; //CPU1
    else if((rx_buf[2] & INTERNAL_CPU_ERROR_MASK) > 0)
      cpu_num = 0; //CPU0
  }
  *num = cpu_num;

  syslog(LOG_DEBUG,"%s cpu error number=%x\n",__func__, *num);

  return 0;
}

static int
read_cpu_tjmax(uint8_t cpu_id, float *value) {
  uint8_t cpu_addr = cpu_info_list[cpu_id].cpu_addr;
  int ret=0;
  int tjmax=0;
  static int retry = 0;

  ret =  cmd_peci_get_tjmax(cpu_addr, &tjmax);
  if (ret != 0) {
    retry++;
    return get_snr_state(retry);
  } else {
    retry = 0;
  }
  *value = (float)tjmax;
  return 0;
}

static int
read_cpu_thermal_margin(uint8_t cpu_id, float *value) {
  uint8_t cpu_addr = cpu_info_list[cpu_id].cpu_addr;
  int ret=0;
  float margin=0;
  static int retry = 0;

  ret = cmd_peci_get_temp(cpu_addr, &margin);
  if (ret != 0) {
    retry++;
    return get_snr_state(retry);
  } else {
    retry = 0;
  }
  *value = margin;
  return 0;
}

static int
read_cpu_pkg_pwr(uint8_t cpu_id, float *value) {
  // Energy units: Intel Doc#554767, p37, 2^(-ENERGY UNIT) J, ENERGY UNIT defalut is 14
  // Run Time units: Intel Doc#554767, p33, msec
  float unit = 0.06103515625f; // 2^(-14)*1000 = 0.06103515625
  uint32_t pkg_energy=0, run_time=0, diff_energy=0, diff_time=0;
  uint8_t cpu_addr;
  static uint32_t last_pkg_energy[2] = {0}, last_run_time[2] = {0};
  static uint8_t retry[2] = {0x00}; // CPU0 and CPU1
  int ret = READING_NA;
  cpu_addr = cpu_info_list[cpu_id].cpu_addr;

  ret = cmd_peci_accumulated_energy(cpu_addr, &pkg_energy);
  if( ret != 0) {
    goto error_exit;
  }

  ret =  cmd_peci_total_time(cpu_addr, &run_time);
  if ( ret != 0) {
    goto error_exit;
  }

  // need at least 2 entries to calculate
  if (last_pkg_energy[cpu_id] == 0 && last_run_time[cpu_id] == 0) {
    if (ret == 0) {
      last_pkg_energy[cpu_id] = pkg_energy;
      last_run_time[cpu_id] = run_time;
    }
    ret = READING_NA;
  }

  if(!ret) {
    if(pkg_energy >= last_pkg_energy[cpu_id]) {
      diff_energy = pkg_energy - last_pkg_energy[cpu_id];
    } else {
      diff_energy = pkg_energy + (0xffffffff - last_pkg_energy[cpu_id] + 1);
    }
    last_pkg_energy[cpu_id] = pkg_energy;

    if(run_time >= last_run_time[cpu_id]) {
      diff_time = run_time - last_run_time[cpu_id];
    } else {
      diff_time = run_time + (0xffffffff - last_run_time[cpu_id] + 1);
    }
    last_run_time[cpu_id] = run_time;

    if(diff_time == 0) {
      ret = READING_NA;
    } else {
      *value = ((float)diff_energy / (float)diff_time * unit);
    }
  }

  error_exit:
    if (ret != 0) {
      retry[cpu_id]++;
      if (retry[cpu_id] <= MAX_SNR_READ_RETRY) {
        ret = READING_SKIP;
        return ret;
      }
    } else {
      retry[cpu_id] = 0;
    }
  return ret;
}

static int
read_cpu_temp(uint8_t cpu_id, float *value) {
  int ret, tjmax;
  float dts;
  uint8_t cpu_addr;
  static int retry = 0;

  cpu_addr = cpu_info_list[cpu_id].cpu_addr;

  ret = cmd_peci_get_temp(cpu_addr, &dts);
  if (ret != 0) {
    retry++;
    return get_snr_state(retry);
  } else {
    retry = 0;
  }

  ret = cmd_peci_get_tjmax(cpu_addr, &tjmax);
  if (ret != 0) {
    retry++;
    return get_snr_state(retry);
  } else {
    retry = 0;
  }

  *value = (float)tjmax + dts;
  #ifdef DEBUG
    syslog(LOG_DEBUG, "%s cpu_id=%d value=%f\n", __func__, cpu_id, *value);
  #endif

  return 0;
}


static int
read_cpu0_dimm_temp(uint8_t dimm_id, float *value) {
  int ret;
  uint8_t temp;
  static int retry = 0;

  if(!pal_is_dimm_present(0, dimm_id)) {
    ret = READING_NA;
    return ret;
  }

  ret = cmd_peci_dimm_thermal_reading(PECI_CPU0_ADDR, dimm_id, &temp);
  if (ret != 0) {
    retry++;
    return get_snr_state(retry);
  } else {
    retry = 0;
  }

  *value = (float)temp;
#ifdef DEBUG
  syslog(LOG_DEBUG, "%s CPU0 DIMM Temp=%f id=%d\n", __func__, *value, dimm_id);
#endif
  return 0;
}

static int
read_cpu1_dimm_temp(uint8_t dimm_id, float *value) {
  int ret;
  uint8_t temp;
  static int retry = 0;

  if(!pal_is_dimm_present(1, dimm_id)) {
    ret = READING_NA;
    return ret;
  }

  ret = cmd_peci_dimm_thermal_reading(PECI_CPU1_ADDR, dimm_id, &temp);
  if (ret != 0) {
    retry++;
    return get_snr_state(retry);
  } else {
    retry = 0 ;
  }

  *value = (float)temp;
#ifdef DEBUG
  syslog(LOG_DEBUG, "%s CPU1 Dimm Temp=%f id=%d\n", __func__, *value, dimm_id);
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

static void
get_adm1278_info(uint8_t hsc_id, uint8_t type, float* m, float* b, float* r) {

 *m = hsc_info_list[hsc_id].info[type].m;
 *b = hsc_info_list[hsc_id].info[type].b;
 *r = hsc_info_list[hsc_id].info[type].r;

 return;
}

//Sensor HSC
static int
set_NM_hsc_pmon_config(uint8_t hsc_id, uint8_t value_l, uint8_t value_h) {
  uint8_t data[2] = {0x00};
  uint8_t rbuf[20] = {0x00};
  uint8_t dev_addr = hsc_info_list[hsc_id].slv_addr;
  NM_RW_INFO info;
  int ret = 0;

  get_nm_rw_info(NM_ID0, &info.bus, &info.nm_addr, &info.bmc_addr);
  info.nm_cmd= PMBUS_PMON_CONFIG;

  ret = cmd_NM_pmbus_read_word(info, dev_addr, rbuf);
  if (ret != 0) {
    ret = READING_NA;
    return ret;
  }

  if (rbuf[6] == 0) {
    data[0] = rbuf[10];   //PMON data low byte
    data[1] = rbuf[11];   //PMON data high byte

    data[0] |= value_l;
    data[1] |= value_h;
  } else {
    ret = READING_NA;
    return ret;
  }

  ret = cmd_NM_pmbus_write_word(info, dev_addr, data);
  if (ret != 0) {
    return ret;
  }

  return 0;
}

static int
enable_hsc_temp_monitor(uint8_t hsc_id) {
  int ret=0;

  ret = set_NM_hsc_pmon_config(hsc_id, 0x08, 0x00);
  if (ret != 0) {
    return ret;
  }

  return 0;
}

static int
read_hsc_iout(uint8_t hsc_id, float *value) {
  uint8_t rbuf[32] = {0x00};
  uint8_t addr = hsc_info_list[hsc_id].slv_addr;
  float m, b, r;
  static int retry = 0;
  int ret = 0;
  NM_RW_INFO info;

  get_nm_rw_info(NM_ID0, &info.bus, &info.nm_addr, &info.bmc_addr);
  info.nm_cmd = PMBUS_READ_IOUT;

  ret = cmd_NM_pmbus_read_word(info, addr, rbuf);
  if (ret != 0) {
    retry++;
    return get_snr_state(retry);
  }

  get_adm1278_info(hsc_id, ADM1278_CURRENT, &m, &b, &r);

  if (rbuf[6] == 0) {
    *value = ((float)(rbuf[11] << 8 | rbuf[10]) * r - b) / m;
  #ifdef DEBUG
    syslog(LOG_DEBUG, "%s Iout value=%f\n", __func__, *value);
  #endif
    retry = 0;
  } else {
  #ifdef DEBUG
    syslog(LOG_DEBUG, "%s cc=%x\n", __func__, rbuf[6]);
  #endif
    ret = READING_NA;
  }

  return ret;
}

static int
read_hsc_vin(uint8_t hsc_id, float *value) {
  uint8_t rbuf[32] = {0x00};
  uint8_t addr = hsc_info_list[hsc_id].slv_addr;
  float m, b, r;
  static int retry = 0;
  int ret = 0;
  NM_RW_INFO info;

  get_nm_rw_info(NM_ID0, &info.bus, &info.nm_addr, &info.bmc_addr);
  info.nm_cmd = PMBUS_READ_VIN;

  ret = cmd_NM_pmbus_read_word(info, addr, rbuf);
  if (ret != 0) {
    retry++;
    return get_snr_state(retry);
  }


  get_adm1278_info(hsc_id, ADM1278_VOLTAGE, &m, &b, &r);
  if (rbuf[6] == 0) {
    *value = ((float)(rbuf[11] << 8 | rbuf[10]) * r - b) / m;
  #ifdef DEBUG
    syslog(LOG_DEBUG, "%s Vin value=%f\n", __func__, *value);
  #endif
    retry = 0;
  } else {
  #ifdef DEBUG
    syslog(LOG_DEBUG, "%s cc=%x\n", __func__, rbuf[6]);
  #endif
    ret = READING_NA;
  }

  return ret;
}

static int
read_hsc_pin(uint8_t hsc_id, float *value) {
  uint8_t rbuf[32] = {0x00};
  uint8_t addr = hsc_info_list[hsc_id].slv_addr;
  float m, b, r;
  static int retry = 0;
  int ret = 0;
  NM_RW_INFO info;

  get_nm_rw_info(NM_ID0, &info.bus, &info.nm_addr, &info.bmc_addr);
  info.nm_cmd = PMBUS_READ_PIN;

  ret = cmd_NM_pmbus_read_word(info, addr, rbuf);
  if (ret != 0) {
    retry++;
    return get_snr_state(retry);
  }


  get_adm1278_info(hsc_id, ADM1278_POWER, &m, &b, &r);

  if (rbuf[6] == 0) {
    *value = ((float)(rbuf[11] << 8 | rbuf[10]) * r - b) / m;
  #ifdef DEBUG
    syslog(LOG_DEBUG, "%s Pin value=%f\n", __func__, *value);
  #endif
    retry = 0;
  } else {
  #ifdef DEBUG
    syslog(LOG_DEBUG, "%s cc=%x\n", __func__, rbuf[6]);
  #endif
    ret = READING_NA;
  }

  return ret;
}

static int
read_hsc_temp(uint8_t hsc_id, float *value) {
  uint8_t rbuf[32] = {0x00};
  uint8_t addr = hsc_info_list[hsc_id].slv_addr;
  float m, b, r;
  static int retry = 0;
  int ret = 0;
  NM_RW_INFO info;

  enable_hsc_temp_monitor(hsc_id);

  get_nm_rw_info(NM_ID0, &info.bus, &info.nm_addr, &info.bmc_addr);
  info.nm_cmd = PMBUS_READ_TEMPERATURE_1;

  ret = cmd_NM_pmbus_read_word(info, addr, rbuf);
  if (ret != 0) {
    retry++;
    return get_snr_state(retry);
  }

  get_adm1278_info(hsc_id, ADM1278_TEMP, &m, &b, &r);

  if (rbuf[6] == 0) {
    *value = ((float)(rbuf[11] << 8 | rbuf[10]) * r - b) / m;
  #ifdef DEBUG
    syslog(LOG_DEBUG, "%s Temp value=%f\n", __func__, *value);
  #endif
    retry = 0;
  } else {
  #ifdef DEBUG
    syslog(LOG_DEBUG, "%s cc=%x\n", __func__, rbuf[6]);
  #endif
    ret = READING_NA;
  }

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
    if (retry <= MAX_SNR_READ_RETRY) {
      ret = READING_SKIP;
    }
  }
  return ret;
}

static void apply_inlet_correction(bool remove_sensor, float *value) {
  static float dt = 0;
  uint8_t pwm[2] = {0};
  uint8_t ict_cnt;
  int i;
  inlet_corr_t *ict;

  // Get PWM value
  if (pal_get_pwm_value(0, &pwm[0]) || pal_get_pwm_value(1, &pwm[1])) {
    // If error reading PWM value, use the previous deltaT
    *value -= dt;
    return;
  }
  pwm[0] = (pwm[0] + pwm[1]) / 2;

  if(remove_sensor) {
    ict = g_remote_ict;
    ict_cnt = g_remote_ict_count;
  } else {
    ict = g_ict;
    ict_cnt = g_ict_count;
  }

  // Scan through the correction table to get correction value for given PWM
  dt = ict[0].delta_t;
  for (i = 1; i < ict_cnt; i++) {
    if (pwm[0] >= ict[i].duty)
      dt = ict[i].delta_t;
    else
      break;
  }

  // Apply correction for the sensor
  *(float*)value -= dt;
}

/*==========================================
Read temperature sensor TMP421 value.
Interface: temp_id: temperature id
           *value: real temperature value
           return: error code
============================================*/
static int
read_sensor(uint8_t id, float *value) {
  int ret;
  struct {
    const char *chip;
    const char *label;
  } devs[] = {
    {"tmp421-i2c-19-4c", "MB_INLET_TEMP"},
    {"tmp421-i2c-19-4e", "MB_OUTLET_R_TEMP"},
    {"tmp421-i2c-19-4f", "MB_OUTLET_L_TEMP"},
    {"tmp421-i2c-19-4c", "MB_INLET_REMOTE_TEMP"},
    {"tmp421-i2c-19-4e", "MB_OUTLET_R_REMOTE_TEMP"},
    {"tmp421-i2c-19-4f", "MB_OUTLET_L_REMOTE_TEMP"},
  };
  if (id >= ARRAY_SIZE(devs)) {
    return -1;
  }

  ret = sensors_read(devs[id].chip, devs[id].label, value);
  if(strcmp(devs[id].label, "MB_INLET_TEMP") == 0) {
    apply_inlet_correction(false, (float *)value);
  } else if (strcmp(devs[id].label, "MB_INLET_REMOTE_TEMP") == 0) {
    apply_inlet_correction(true, (float *)value);
  }

  return ret;
}

static int
read_nic_temp(uint8_t nic_id, float *value) {
  int fd = 0, ret = -1;
  char fn[32];
  uint8_t retry = 3, tlen, rlen, addr, bus;
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

  while (ret < 0 && retry-- > 0 ) {
    ret = i2c_rdwr_msg_transfer(fd, addr, tbuf, tlen, rbuf, rlen);
  }

#ifdef DEBUG
  syslog(LOG_DEBUG, "%s Temp[%d]=%x bus=%x slavaddr=%x\n", __func__, nic_id, rbuf[0], bus, addr);
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

  while (ret < 0 && retry-- > 0 ) {
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

int pal_mapping_fan_chnnel_id(uint8_t fan) {
  int fan_id;
  if(fan < PAL_FAN_CNT)
    fan_id = fan + PAL_FAN_CNT;
  else
    fan_id = fan - PAL_FAN_CNT;

  return fan_id;
}

int pal_fan_snr_num_to_id(uint8_t snr_num) {

  if( snr_num >= MB_SNR_FAN4_TACH_IN) {
    snr_num = snr_num & 0x0F;
    if(snr_num - PAL_FAN_CNT != 0)
      snr_num = (snr_num - PAL_FAN_CNT) / 2;
    else
      snr_num = 0;
  } else {
    snr_num = snr_num & 0x0F;
    snr_num = (snr_num + PAL_FAN_CNT) / 2;
  }

  return snr_num;
}

int pal_set_fan_speed(uint8_t fan, uint8_t pwm)
{
  char label[32] = {0};

  if (fan >= pal_pwm_cnt ||
      snprintf(label, sizeof(label), "pwm%d", pal_pwm_cnt - fan) > sizeof(label)) {
    return -1;
  }
  return sensors_write_fan(label, (float)pwm);
}

int pal_get_fan_speed(uint8_t fan, int *rpm)
{
  char label[32] = {0};
  float value;
  int ret, fan_id;

  fan_id = pal_mapping_fan_chnnel_id(fan);

  if (fan_id >= pal_tach_cnt ||
      snprintf(label, sizeof(label), "fan%d", fan_id + 1) > sizeof(label)) {
    syslog(LOG_WARNING, "%s: invalid fan#:%d", __func__, fan_id);
    return -1;
  }
  ret = sensors_read_fan(label, &value);
  *rpm = (int)value;
  return ret;
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

int pal_get_pwm_value(uint8_t fan, uint8_t *pwm)
{
  char label[32] = {0};
  float value;
  int ret;

  if (fan >= pal_tach_cnt ||
      snprintf(label, sizeof(label), "pwm%d", pal_pwm_cnt - fan/8) > sizeof(label)) {
    syslog(LOG_WARNING, "%s: invalid fan#:%d", __func__, fan);
    return -1;
  }
  ret = sensors_read_fan(label, &value);
  if (ret == 0)
    *pwm = (int)value;
  return ret;
}

static int
read_ina260_sensor(uint8_t sensor_num, float *value) {
  int fd = 0, ret = -1;
  char fn[32];
  float scale, vol_scale = 0.00125, curr_scale = 0.00125, pwr_scale = 0.01;
  uint8_t retry = 3, tlen, rlen, addr, bus, cmd;
  uint8_t tbuf[16] = {0};
  uint8_t rbuf[16] = {0};
  uint16_t ina260_id, tmp;

  switch(sensor_num) {
    case  MB_SNR_P3V3_STBY_INA260_VOL:
    case  MB_SNR_P3V3_STBY_INA260_CURR:
    case  MB_SNR_P3V3_STBY_INA260_POWER:
      ina260_id = 0;
      break;
    case  MB_SNR_P3V3_M2_1_INA260_VOL:
    case  MB_SNR_P3V3_M2_1_INA260_CURR:
    case  MB_SNR_P3V3_M2_1_INA260_POWER:
      ina260_id = 1;
      break;
    case  MB_SNR_P3V3_M2_2_INA260_VOL:
    case  MB_SNR_P3V3_M2_2_INA260_CURR:
    case  MB_SNR_P3V3_M2_2_INA260_POWER:
      ina260_id = 2;
      break;
    case  MB_SNR_P3V3_M2_3_INA260_VOL:
    case  MB_SNR_P3V3_M2_3_INA260_CURR:
    case  MB_SNR_P3V3_M2_3_INA260_POWER:
      ina260_id = 3;
      break;
    default:
      return READING_NA;
  }

  bus = ina260_info_list[ina260_id].bus;
  addr = ina260_info_list[ina260_id].slv_addr;

#ifdef DEBUG
  syslog(LOG_DEBUG, "%s ina_id = %d, sensor_num = %d, name = %s\n", __func__, ina260_id, sensor_num, sensor_map[sensor_num].snr_name);
#endif

  if (strstr(sensor_map[sensor_num].snr_name, "CURR")) {
    cmd = INA260_CURRENT;
    scale = curr_scale;
  }
  else if (strstr(sensor_map[sensor_num].snr_name, "VOL")) {
    cmd = INA260_VOLTAGE;
    scale = vol_scale;
  }
  else if (strstr(sensor_map[sensor_num].snr_name, "POWER")) {
    cmd = INA260_POWER;
    scale = pwr_scale;
  }
  else {
    return READING_NA;
  }

#ifdef DEBUG
  syslog(LOG_DEBUG, "%s bus=%x cmd=%x slavaddr=%x\n", __func__, bus, cmd, addr);
#endif

  snprintf(fn, sizeof(fn), "/dev/i2c-%d", bus);
  fd = open(fn, O_RDWR);
  if (fd < 0) {
    goto err_exit;
  }

  tbuf[0] = cmd;
  tlen = 1;
  rlen = 2;

  while (ret < 0 && retry-- > 0 ) {
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
  uint8_t sku_id = 0xFF,  rev_id = 0xFF, chip_id = vr_id;
  static const char *labels[VR_ID_NUM + PCH_ID_NUM] = {
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

  pal_get_platform_id(BOARD_REV_ID, &rev_id);
  pal_get_platform_id(BOARD_SKU_ID, &sku_id);

  //EVT SKU_ID[2:1] = 00 (INFINEON), 01 (T1)
  //DVT SKU_ID[2:1] = 00 (TI), 01 (INFINEON), TODO: 10 (3rd Source)
  if (((rev_id == PLATFORM_EVT) && !(sku_id & 0x4)) ||
      ((rev_id == PLATFORM_DVT) && (sku_id & 0x2))) {
    chip_id += VR_ID_NUM;
  }

  //PCH VR
  if (vr_id >=  VR_ID10) {
    chip_id = 2*VR_ID_NUM;
  }

  return sensors_read(vr_chips[chip_id], labels[vr_id], value);
}

static int
read_vr_temp(uint8_t vr_id, float *value) {
  uint8_t sku_id = 0xFF,  rev_id = 0xFF, chip_id = vr_id;
  static const char *labels[VR_ID_NUM + PCH_ID_NUM] = {
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

  pal_get_platform_id(BOARD_REV_ID, &rev_id);
  pal_get_platform_id(BOARD_SKU_ID, &sku_id);

  //EVT SKU_ID[2:1] = 00 (INFINEON), 01 (T1)
  //DVT SKU_ID[2:1] = 00 (TI), 01 (INFINEON), TODO: 10 (3rd Source)
  if (((rev_id == PLATFORM_EVT) && !(sku_id & 0x4)) ||
      ((rev_id == PLATFORM_DVT) && (sku_id & 0x2))) {
    chip_id += VR_ID_NUM;
  }

  //PCH VR
  if (vr_id >=  VR_ID10) {
    chip_id = 2*VR_ID_NUM;
  }

  return sensors_read(vr_chips[chip_id], labels[vr_id], value);
}

static int
read_vr_iout(uint8_t vr_id, float *value) {
  uint8_t sku_id = 0xFF,  rev_id = 0xFF, chip_id = vr_id;
  static const char *labels[VR_ID_NUM + PCH_ID_NUM] = {
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

  pal_get_platform_id(BOARD_REV_ID, &rev_id);
  pal_get_platform_id(BOARD_SKU_ID, &sku_id);

  //EVT SKU_ID[2:1] = 00 (INFINEON), 01 (T1)
  //DVT SKU_ID[2:1] = 00 (TI), 01 (INFINEON), TODO: 10 (3rd Source)
  if (((rev_id == PLATFORM_EVT) && !(sku_id & 0x4)) ||
      ((rev_id == PLATFORM_DVT) && (sku_id & 0x2))) {
    chip_id += VR_ID_NUM;
  }

  //PCH VR
  if (vr_id >=  VR_ID10) {
    chip_id = 2*VR_ID_NUM;
  }

  return sensors_read(vr_chips[chip_id], labels[vr_id], value);
}

int
read_vr_iin(uint8_t vr_id, float *value) {
  return 0;
}

static int
read_vr_pout(uint8_t vr_id, float *value) {
  uint8_t sku_id = 0xFF,  rev_id = 0xFF, chip_id = vr_id;
  static const char *labels[VR_ID_NUM + PCH_ID_NUM] = {
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

  pal_get_platform_id(BOARD_REV_ID, &rev_id);
  pal_get_platform_id(BOARD_SKU_ID, &sku_id);

  //EVT SKU_ID[2:1] = 00 (INFINEON), 01 (T1)
  //DVT SKU_ID[2:1] = 00 (TI), 01 (INFINEON), TODO: 10 (3rd Source)
  if (((rev_id == PLATFORM_EVT) && !(sku_id & 0x4)) ||
      ((rev_id == PLATFORM_DVT) && (sku_id & 0x2))) {
    chip_id += VR_ID_NUM;
  }

  //PCH VR
  if (vr_id >=  VR_ID10) {
    chip_id = 2*VR_ID_NUM;
  }

  return sensors_read(vr_chips[chip_id], labels[vr_id], value);
}

int
read_vr_pin(uint8_t vr_id, float *value) {
  return 0;
}

bool
pal_is_fan_prsnt(uint8_t fan) {
  uint8_t fan_id = 0;
  fan_id = pal_mapping_fan_chnnel_id(fan);
  fan_id = (fan_id / 2);
  return !is_fan_present(fan_id);
}

static bool
is_fan_present(uint8_t fan_id) {
  char shadow_name[32];
  char led_state[32];
  gpio_value_t value;
  static bool state_flag = false;

  if(state_flag == false) {
    state_flag = true;
    for(int i = 0; i < PAL_FAN_CNT; i++) {
      pal_set_fan_led_state(fan_id, "OFF");
    }
  }

  sprintf(shadow_name, GPIO_FAN_BUF_PRESENT, fan_id);
  gpio_desc_t *desc = gpio_open_by_shadow(shadow_name);
  if (!desc) {
    return false;
  }
  gpio_get_value(desc, &value);
  gpio_close(desc);
  if (value == GPIO_VALUE_LOW) {
    pal_set_fan_led_state(fan_id, "OFF");
    return true;
  }
  else {
    pal_get_fan_led_state(fan_id, led_state);
    if((strcmp(led_state, "YELLOW") != 0)) {
      pal_set_fan_led_state(fan_id, "BLUE");
    }
    return false;
  }
}

static int
read_fan_volt(uint8_t fan_id, float *value) {
  struct {
    const char *chip;
    const char *label;
  } devs[] = {
    {"adc128d818-i2c-8-1d", "FAN0_VOLT"},
    {"adc128d818-i2c-8-1d", "FAN1_VOLT"},
    {"adc128d818-i2c-8-1d", "FAN2_VOLT"},
    {"adc128d818-i2c-8-1d", "FAN3_VOLT"},
    {"adc128d818-i2c-8-1f", "FAN4_VOLT"},
    {"adc128d818-i2c-8-1f", "FAN5_VOLT"},
    {"adc128d818-i2c-8-1f", "FAN6_VOLT"},
    {"adc128d818-i2c-8-1f", "FAN7_VOLT"},
  };
  int ret = 0;

  if (fan_id >= ARRAY_SIZE(devs)) {
    return -1;
  }
  if (is_fan_present(fan_id) == true) {
    return READING_NA;
  }
  ret = sensors_read(devs[fan_id].chip, devs[fan_id].label, value);
  // Real voltage(V) = ((R1(ohm) + R2(ohm)) * ADC voltage(V)) / R2(ohm)
  *value = ((ADC128_FAN_SCALED_R1 + ADC128_FAN_SCALED_R2) * (*value)) / ADC128_FAN_SCALED_R2;
  fan_volt[fan_id] = *value;
  return ret;
}

static int
read_fan_curr(uint8_t fan_id, float *value) {
  struct {
    const char *chip;
    const char *label;
  } devs[] = {
    {"adc128d818-i2c-8-1d", "FAN0_CURR"},
    {"adc128d818-i2c-8-1d", "FAN1_CURR"},
    {"adc128d818-i2c-8-1d", "FAN2_CURR"},
    {"adc128d818-i2c-8-1d", "FAN3_CURR"},
    {"adc128d818-i2c-8-1f", "FAN4_CURR"},
    {"adc128d818-i2c-8-1f", "FAN5_CURR"},
    {"adc128d818-i2c-8-1f", "FAN6_CURR"},
    {"adc128d818-i2c-8-1f", "FAN7_CURR"},
  };
  int ret = 0;

  if (fan_id >= ARRAY_SIZE(devs)) {
    return -1;
  }
  if (is_fan_present(fan_id) == true) {
    return READING_NA;
  }
  ret = sensors_read(devs[fan_id].chip, devs[fan_id].label, value);
  // I_OUT(A) = V_IMON(V) * 10^6 / G_IMON(uA/A) / R_IMON(ohm)
  *value = (*value) * 1000000 / ADC128_GIMON / ADC128_RIMON;
  fan_curr[fan_id] = *value;
  return ret;
}

static int
read_fan_pwr(uint8_t fan_id, float *value) {
  if (is_fan_present(fan_id) == true) {
    return READING_NA;
  }
  *value = fan_volt[fan_id] * fan_curr[fan_id];
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
pal_is_BIOS_completed(uint8_t fru)
{
  gpio_desc_t *desc;
  gpio_value_t value;
  bool ret = false;

  if ( FRU_MB != fru )
  {
    syslog(LOG_WARNING, "[%s]incorrect fru id: %d", __func__, fru);
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

void
pal_is_dimm_present_check(uint8_t fru, bool *dimm_sts_list)
{
  char key[MAX_KEY_LEN] = {0};
  char value[MAX_VALUE_LEN] = {0};
  int i;
  size_t ret;

  //check dimm info from /mnt/data/sys_config/
  for (i=0; i<DIMM_SLOT_CNT; i++)
  {
    sprintf(key, "sys_config/fru%d_dimm%d_location", fru, i);
    if(kv_get(key, value, &ret, KV_FPERSIST) != 0 || ret < 4)
    {
      syslog(LOG_WARNING,"[%s]Cannot get dimm_slot%d present info", __func__, i);
      return;
    }

    if ( 0xff == value[0] )
    {
      dimm_sts_list[i] = false;
    }
    else
    {
      dimm_sts_list[i] = true;
    }
  }
}

bool
pal_is_dimm_present(int cpu_id, uint8_t dimm_id)
{
  static bool is_check = false;
  static bool dimm_sts_list[DIMM_SLOT_CNT] = {0};
  uint8_t fru = FRU_MB;
  int list_index = 0;

  if ( false == pal_is_BIOS_completed(fru) )
  {
    return false;
  }

  if ( false == is_check )
  {
    is_check = true;
    pal_is_dimm_present_check(fru, dimm_sts_list);
  }

  list_index = ((DIMM_SLOT_CNT / 2) * cpu_id) + (2 * dimm_id);

  if(true == dimm_sts_list[list_index])
  {
    return true;
  }

  return false;
}

bool
pal_dimm_present_check(uint8_t snr_num) {
  int cpu_id = 0;
  uint8_t dimm_id = 0xFF;

  if(snr_num >= MB_SNR_CPU1_DIMM_GRPA_TEMP) {
    cpu_id = 1;
    dimm_id = snr_num - MB_SNR_CPU1_DIMM_GRPA_TEMP;
  } else {
    cpu_id = 0;
    dimm_id = snr_num - MB_SNR_CPU0_DIMM_GRPA_TEMP;
  }

  return !pal_is_dimm_present(cpu_id, dimm_id);
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
    if (pal_is_BIOS_completed(FRU_MB))
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

int
pal_set_fan_led_reg(uint8_t sel_addr, uint8_t led_func, uint8_t led_state) {
  int dev, ret = 0, len = 0;
  uint8_t wbuf[4] = {0};
  uint8_t rbuf[4] = {0};

  wbuf[0] = led_func;
  wbuf[1] = led_state;
  dev = open(I2C_DEV_FAN_LED, O_RDWR);
  if (dev < 0) {
    ret = dev;
    return ret;
  }

  ret = i2c_rdwr_msg_transfer(dev, sel_addr, wbuf, 2, rbuf, len);

  if (dev > 0) {
    close(dev);
  }
  return ret;
}

int
pal_set_fan_led(int fan_board) {
  int ret = 0;
  int start_index, end_index;
  uint8_t yellow_led = 0x00;
  uint8_t blue_led = 0x00;
  uint8_t sel_addr = 0x00;
  char led_state[32];

  if(fan_board == FCB_0) {
    start_index = (PAL_FAN_CNT/2) - 1;
    end_index = (PAL_FAN_CNT/2) - (PAL_FAN_CNT/2);
    sel_addr = FCB_0_ADDR;
  } else {
    start_index = PAL_FAN_CNT - 1;
    end_index = PAL_FAN_CNT - (PAL_FAN_CNT/2);
    sel_addr = FCB_1_ADDR;
  }

  for(int i = start_index; i >= end_index; i--) {
    ret = pal_get_fan_led_state(i, led_state);
    if(ret) {
      yellow_led = (yellow_led << 2) | 0x01;
      blue_led = (blue_led << 2) | 0x01;
    } else if(strcmp(led_state, "OFF") == 0) {
      yellow_led = (yellow_led << 2) | 0x01;
      blue_led = (blue_led << 2) | 0x01;
    } else if(strcmp(led_state, "YELLOW") == 0) {
      yellow_led = (yellow_led << 2) | 0x00;
      blue_led = (blue_led << 2) | 0x01;
    } else if(strcmp(led_state, "BLUE") == 0) {
      yellow_led = (yellow_led << 2) | 0x01;
      blue_led = (blue_led << 2) | 0x00;
    }
  }

  pal_set_fan_led_reg(sel_addr, BLUE_LED_FUNC, blue_led);
  pal_set_fan_led_reg(sel_addr, YELLOW_LED_FUNC, yellow_led);

  return ret;
}

int
pal_fan_led_control(void) {
  int ret = 0;

  pal_set_fan_led(FCB_0);
  pal_set_fan_led(FCB_1);

  return ret;
}

void
set_fan_led(uint8_t snr_num, bool assert) {
  int fan_channel_id;
  switch (snr_num) {
    case MB_SNR_FAN0_TACH_IN:
    case MB_SNR_FAN0_TACH_OUT:
    case MB_SNR_FAN1_TACH_IN:
    case MB_SNR_FAN1_TACH_OUT:
    case MB_SNR_FAN2_TACH_IN:
    case MB_SNR_FAN2_TACH_OUT:
    case MB_SNR_FAN3_TACH_IN:
    case MB_SNR_FAN3_TACH_OUT:
    case MB_SNR_FAN4_TACH_IN:
    case MB_SNR_FAN4_TACH_OUT:
    case MB_SNR_FAN5_TACH_IN:
    case MB_SNR_FAN5_TACH_OUT:
    case MB_SNR_FAN6_TACH_IN:
    case MB_SNR_FAN6_TACH_OUT:
    case MB_SNR_FAN7_TACH_IN:
    case MB_SNR_FAN7_TACH_OUT:
      if(assert) {
        fan_channel_id = pal_fan_snr_num_to_id(snr_num);
        pal_set_fan_led_state(fan_channel_id, "YELLOW");
      }
      else {
        fan_channel_id = pal_fan_snr_num_to_id(snr_num);
        pal_set_fan_led_state(fan_channel_id, "BLUE");
      }
      break;

    default:
      break;
  }
}

void
pal_sensor_assert_handle(uint8_t fru, uint8_t snr_num, float val, uint8_t thresh) {
  char cmd[128];
  char thresh_name[10];

  switch (thresh) {
    case UNR_THRESH:
        sprintf(thresh_name, "UNR");
      break;
    case UCR_THRESH:
        set_fan_led(snr_num, true);
        sprintf(thresh_name, "UCR");
      break;
    case UNC_THRESH:
        sprintf(thresh_name, "UNCR");
      break;
    case LNR_THRESH:
        sprintf(thresh_name, "LNR");
      break;
    case LCR_THRESH:
        set_fan_led(snr_num, true);
        sprintf(thresh_name, "LCR");
      break;
    case LNC_THRESH:
        sprintf(thresh_name, "LNCR");
      break;
    default:
      syslog(LOG_WARNING, "pal_sensor_assert_handle: wrong thresh enum value");
      exit(-1);
  }

  switch(snr_num) {
    case MB_SNR_CPU0_TEMP:
      sprintf(cmd, "P0 Temp %s %.0fC - Assert", thresh_name, val);
      break;
    case MB_SNR_CPU1_TEMP:
      sprintf(cmd, "P1 Temp %s %.0fC - Assert", thresh_name, val);
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

  switch (thresh) {
    case UNR_THRESH:
        sprintf(thresh_name, "UNR");
      break;
    case UCR_THRESH:
        set_fan_led(snr_num, false);
        sprintf(thresh_name, "UCR");
      break;
    case UNC_THRESH:
        sprintf(thresh_name, "UNCR");
      break;
    case LNR_THRESH:
        sprintf(thresh_name, "LNR");
      break;
    case LCR_THRESH:
        set_fan_led(snr_num, false);
        sprintf(thresh_name, "LCR");
      break;
    case LNC_THRESH:
        sprintf(thresh_name, "LNCR");
      break;
    default:
      syslog(LOG_WARNING, "pal_sensor_assert_handle: wrong thresh enum value");
      exit(-1);
  }

  switch(snr_num) {
    case MB_SNR_CPU0_TEMP:
      sprintf(cmd, "P0 Temp %s %3.0fC - Deassert", thresh_name, val);
      break;
    case MB_SNR_CPU1_TEMP:
      sprintf(cmd, "P1 Temp %s %3.0fC - Deassert", thresh_name, val);
    default:
      return;
  }
  pal_add_cri_sel(cmd);

}

static bool
is_ava_card_present(uint8_t riser_bus) {
  int fd = 0;
  char fn[32];
  bool ret = false;
  uint8_t tbuf[16] = {0};
  uint8_t rbuf[16] = {0};
  uint8_t tcount = 0, rcount = 0;
  int  val = 0;

  snprintf(fn, sizeof(fn), "/dev/i2c-%d", riser_bus);
  fd = open(fn, O_RDWR);
  if ( fd < 0 ) {
    return false;
  }

  //Send I2C to AVAII for FRU present check
  rcount = 1;
  tcount = 0;
  val = i2c_rdwr_msg_transfer(fd, AVA_FRUID_ADDR, tbuf, tcount, rbuf, rcount);
  if( val < 0 ) {
    ret = false;
  }
  else {
    ret = true;
  }
  close(fd);

  return ret;
}

static int
read_ava_temp(uint8_t sensor_num, float *value) {
  int ret = 0;

  switch(sensor_num) {
    case RISER1_SNR_AVAII_FTEMP:
    case RISER1_SNR_AVAII_RTEMP:
      if (is_ava_card_present(RISER1_BUS) == false) {
        return READING_NA;
      }
      break;
    case RISER2_SNR_AVAII_FTEMP:
    case RISER2_SNR_AVAII_RTEMP:
      if (is_ava_card_present(RISER2_BUS) == false) {
        return READING_NA;
      }
      break;
  }

  switch(sensor_num) {
    case RISER1_SNR_AVAII_FTEMP:
      ret = sensors_read("tmp75-i2c-2-49", "RISER1_AVAII_FRONT", value);
      break;
    case RISER1_SNR_AVAII_RTEMP:
      ret = sensors_read("tmp75-i2c-2-48", "RISER1_AVAII_REAR", value);
      break;
    case RISER2_SNR_AVAII_FTEMP:
      ret = sensors_read("tmp75-i2c-6-49", "RISER2_AVAII_FRONT", value);
      break;
    case RISER2_SNR_AVAII_RTEMP:
      ret = sensors_read("tmp75-i2c-6-48", "RISER2_AVAII_REAR", value);
      break;
  }

  return ret;
}

static int
read_nvme_temp(uint8_t sensor_num, float *value) {
  int ret = 0;
  int bus_id = 0;
  int fd = 0;
  int riser_pca9846_bus_base = 0;
  char fn[32];
  uint8_t tcount = 0, rcount = 0;
  uint8_t tbuf[16] = {0};
  uint8_t rbuf[16] = {0};

  switch(sensor_num) {
    case RISER1_SNR_SLOT0_NVME_TEMP:
    case RISER1_SNR_SLOT1_NVME_TEMP:
    case RISER1_SNR_SLOT2_NVME_TEMP:
    case RISER1_SNR_SLOT3_NVME_TEMP:
      if (is_ava_card_present(RISER1_BUS) == false) {
        return READING_NA;
      }
      break;
    case RISER2_SNR_SLOT0_NVME_TEMP:
    case RISER2_SNR_SLOT1_NVME_TEMP:
    case RISER2_SNR_SLOT2_NVME_TEMP:
    case RISER2_SNR_SLOT3_NVME_TEMP:
      if (is_ava_card_present(RISER2_BUS) == false) {
        return READING_NA;
      }
      break;
    default:
      return READING_NA;
  }

  if (sensor_num < RISER2_SNR_SLOT0_NVME_TEMP) {
    riser_pca9846_bus_base = RISER1_PCA9846_BUS_BASE;
  } else {
    riser_pca9846_bus_base = RISER2_PCA9846_BUS_BASE;
  }

  bus_id = (sensor_num % RISER_PCA9846_CH_MAX_NUM) + riser_pca9846_bus_base;
  snprintf(fn, sizeof(fn), "/dev/i2c-%d", bus_id);
  fd = open(fn, O_RDWR);
  if (fd < 0) {
    return READING_NA;
  }

  ret = 0;
  tbuf[0] = NVMe_GET_STATUS_CMD;
  tcount = 1;
  rcount = NVMe_GET_STATUS_LEN;
  ret = i2c_rdwr_msg_transfer(fd, NVMe_SMBUS_ADDR, tbuf, tcount, rbuf, rcount);
  if (ret < 0) {
    ret = READING_NA;
    goto error_exit;
  }
  *value = (float)(signed char)rbuf[NVMe_TEMP_REG];

error_exit:
  close(fd);

  return ret;
}

bool
is_cpu_sensor(uint8_t sensor_num, uint8_t* cpu_id) {
  if(sensor_num >= 0xB0 && sensor_num <= 0xC3) {
    *cpu_id = 0;
    return true;
  }
  else if(sensor_num >= 0xE0 && sensor_num <= 0xF3) {
    *cpu_id = 1;
    return true;
  }

  return false;
}

int
pal_sensor_read_raw(uint8_t fru, uint8_t sensor_num, void *value) {
  char key[MAX_KEY_LEN] = {0};
  char str[MAX_VALUE_LEN] = {0};
  char fru_name[32];
  int ret=0;
  static uint8_t poweron_10s_flag = 0;
  bool server_off;
  uint8_t id=0;
  uint8_t cpu_id;

  pal_get_fru_name(fru, fru_name);
  sprintf(key, "%s_sensor%d", fru_name, sensor_num);
  server_off = is_server_off();
  id = sensor_map[sensor_num].id;

  switch(fru) {
  case FRU_MB:
  case FRU_NIC0:
  case FRU_NIC1:
  case FRU_FCB:
  case FRU_RISER1:
  case FRU_RISER2:
    if (server_off) {
      poweron_10s_flag = 0;
      if (sensor_map[sensor_num].stby_read == true ) {
        ret = sensor_map[sensor_num].read_sensor(id, (float*) value);
      } else {
        ret = READING_NA;
      }
    } else {
      if((poweron_10s_flag < 5) && ((sensor_num == MB_SNR_HSC_VIN) ||
        (sensor_num == MB_SNR_HSC_IOUT) || (sensor_num == MB_SNR_HSC_PIN) ||
        (sensor_num == MB_SNR_HSC_TEMP) )) {

        poweron_10s_flag++;
        ret = READING_NA;
        break;
      }

      //Discrete sensor
      switch(sensor_num) {
        case MB_SENSOR_PROCESSOR_FAIL:
          ret = check_frb3(FRU_MB, sensor_num, (float*) value);
          return ret;
          break;
        default:
          break;
      }

      if(is_cpu_sensor(sensor_num, &cpu_id)) {
        if(is_cpu_present(fru, cpu_id)) {
          ret = READING_NA;
          break;
        }
      }

      ret = sensor_map[sensor_num].read_sensor(id, (float*) value);
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
  case FRU_MB:
  case FRU_NIC0:
  case FRU_NIC1:
  case FRU_FCB:
  case FRU_RISER1:
  case FRU_RISER2:
    //Discrete sensor
    if(sensor_num == 0xFC) {
      sprintf(name, "%s", "MB_PROCESSOR_FAIL");
      break;
    }

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
  case FRU_FCB:
  case FRU_RISER1:
  case FRU_RISER2:
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
    case FRU_FCB:
    case FRU_RISER1:
    case FRU_RISER2:
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

static int
read_fan_speed(uint8_t fan, float *rpm) {
  char fan_name[32] = {0};
  float value;
  int ret, fan_id;

  fan_id = pal_mapping_fan_chnnel_id(fan);

  if (fan_id >= pal_tach_cnt ||
      snprintf(fan_name, sizeof(fan_name), "fan%d", fan_id + 1) > sizeof(fan_name)) {
    syslog(LOG_WARNING, "%s: invalid fan#:%d", __func__, fan_id);
    return -1;
  }
  ret = sensors_read_fan(fan_name, &value);
  *rpm = value;
  return ret;
}

int
pal_get_sensor_poll_interval(uint8_t fru, uint8_t sensor_num, uint32_t *value) {
  //default poll interval
  *value = 2;

  switch(fru) {
    case FRU_MB:
      if (sensor_num == MB_SNR_P3V_BAT) {
        *value = 3600;
      }
      break;
    case FRU_NIC0:
    case FRU_NIC1:
    case FRU_FCB:
    case FRU_RISER1:
    case FRU_RISER2:
      break;
  }

  return PAL_EOK;
}

static pthread_t tid_dwr = -1;
static void *dwr_handler(void *arg) {
  syslog(LOG_WARNING, "Start Second/DWR Autodump");
  if (system("/usr/local/bin/autodump.sh --second &") != 0) {
    syslog(LOG_ERR, "Autodump.sh --second failed!\n");
  }

  tid_dwr = -1;
  pthread_exit(NULL);
}

void
pal_second_crashdump_chk(void) {
    int fd;
    size_t len;

    if (tid_dwr != -1)
      pthread_cancel(tid_dwr);

    if (pthread_create(&tid_dwr, NULL, dwr_handler, NULL) == 0) {
      memset(postcodes_last, 0, sizeof(postcodes_last));
      pal_get_80port_record(FRU_MB, postcodes_last, sizeof(postcodes_last), &len);

      fd =  creat("/tmp/DWR", 0644);
      if (fd)
        close(fd);
      pthread_detach(tid_dwr);
    }
    else
      tid_dwr = -1;
}
