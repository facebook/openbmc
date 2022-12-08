#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include <fcntl.h>
#include <errno.h>
#include <syslog.h>
#include <time.h>
#include <sys/mman.h>
#include <sys/file.h>
#include <string.h>
#include <ctype.h>
#include <openbmc/kv.h>
#include <openbmc/obmc-sensors.h>
#include <openbmc/libgpio.h>
#include <openbmc/obmc-i2c.h>
#include <openbmc/ipmb.h>
#include <openbmc/nvme-mi.h>
#include "pal.h"
#include "pal_sensors.h"

static int read_adc_val(uint8_t id, float *value);
static int read_battery_val(uint8_t id, float *value);
static int read_temp(uint8_t id, float *value);
static int read_peci(uint8_t id, float *value);
static int read_rpm(uint8_t id, float *value);
static int read_pmbus(uint8_t id, float *value);
static int read_hsc(uint8_t id, float *value);
static int read_cpld_adc(uint8_t id, float *value);
static int read_ina230_pwr(uint8_t id, float *value);
static int read_nvme_temp(uint8_t id, float *value);
static int read_nic_temp(uint8_t id, float *value);
static int retryDIMM = 0;
static int retryFAN[4] = {0, 0, 0, 0};

//{SensorName, ID, FUNCTION, RAEDING AVAILABLE, {UCR, UNC, UNR, LCR, LNC, LNR, Pos, Neg}, unit}
PAL_SENSOR_MAP server_sensor_map[] = {
  [SERVER_INLET_TEMP] =
  {"NETLAKE_INLET_TEMP", MB_INLET, read_temp, STBY_READING, {0, 0, 0, 0, 0, 0, 0, 0}, TEMP, NORMAL_POLL_INTERVAL},
  [SERVER_OUTLET_TEMP] =
  {"NETLAKE_OUTLET_TEMP", MB_OUTLET, read_temp, STBY_READING, {0, 0, 0, 0, 0, 0, 0, 0}, TEMP, NORMAL_POLL_INTERVAL},
  [SERVER_SOC_TEMP] =
  {"SOC_TEMP", SOC_TEMP, read_peci, POST_COMPLT_READING, {0, 0, 0, 0, 0, 0, 0, 0}, TEMP, NORMAL_POLL_INTERVAL},
  [SERVER_DIMMA_TEMP] =
  {"DIMMA_TEMP", DIMMA_TEMP, read_peci, POST_COMPLT_READING, {0, 0, 0, 0, 0, 0, 0, 0}, TEMP, NORMAL_POLL_INTERVAL},
  [SERVER_DIMMB_TEMP] =
  {"DIMMB_TEMP", DIMMB_TEMP, read_peci, POST_COMPLT_READING, {0, 0, 0, 0, 0, 0, 0, 0}, TEMP, NORMAL_POLL_INTERVAL},
  [SERVER_A_P12V_STBY_NETLAKE_VOL] =
  {"A_P12V_STBY_NETLAKE_VOL", CPLD_ADC_P12V_STBY, read_cpld_adc, STBY_READING, {13.3488, 13.2192, 13.4784, 10.7088, 10.8192, 10.5984, 0, 0}, VOLT, NORMAL_POLL_INTERVAL},
  [SERVER_A_P3V3_STBY_NETLAKE_VOL] =
  {"A_P3V3_STBY_NETLAKE_VOL", CPLD_ADC_P3V3_STBY, read_cpld_adc, STBY_READING, {3.56895, 3.5343, 3.6036, 3.04095, 3.0723, 3.0096, 0, 0}, VOLT, NORMAL_POLL_INTERVAL},
  [SERVER_A_P1V8_STBY_NETLAKE_VOL] =
  {"A_P1V8_STBY_NETLAKE_VOL", CPLD_ADC_P1V8_STBY, read_cpld_adc, STBY_READING, {1.91889, 1.90026, 1.93752, 1.67616, 1.69344, 1.65888, 0, 0}, VOLT, ONE_HOUR_INVERVAL},
  [SERVER_A_P5V_STBY_NETLAKE_VOL] =
  {"A_P5V_STBY_NETLAKE_VOL", CPLD_ADC_P5V_STBY, read_cpld_adc, STBY_READING, {5.4075, 5.355, 5.46, 4.6075, 4.655, 4.56, 0, 0}, VOLT, NORMAL_POLL_INTERVAL},
  [SERVER_A_P1V05_STBY_NETLAKE_VOL] =
  {"A_P1V05_STBY_VOL", CPLD_ADC_P1V05_STBY, read_cpld_adc, STBY_READING, {1.12167, 1.11078, 1.13256, 0.99813, 1.00842, 0.98784, 0, 0}, VOLT, NORMAL_POLL_INTERVAL},
  [SERVER_A_PVNN_PCH_VOL] =
  {"A_PVNN_PCH_VOL", CPLD_ADC_PVNN_PCH, read_cpld_adc, STBY_READING, {1.236, 1.224, 1.248, 0.582, 0.588, 0.576, 0, 0}, VOLT, NORMAL_POLL_INTERVAL},
  [SERVER_A_PVCCIN_VOL] =
  {"A_PVCCIN_VOL", CPLD_ADC_PVCCIN, read_cpld_adc, POWER_ON_READING, {2.06, 2.04, 2.08, 1.455, 1.47, 1.44, 0, 0}, VOLT, NORMAL_POLL_INTERVAL},
  [SERVER_A_PVCCANA_CPU_VOL] =
  {"A_PVCCANA_CPU_VOL", CPLD_ADC_PVCCANA, read_cpld_adc, POWER_ON_READING, {1.03515, 1.0251, 1.0452, 0.96515, 0.9751, 0.9552, 0, 0}, VOLT, NORMAL_POLL_INTERVAL},
  [SERVER_A_PVDDQ_ABC_CPU_VOL] =
  {"A_PVDDQ_ABC_CPU_VOL", CPLD_ADC_PVDDQ, read_cpld_adc, POWER_ON_READING, {1.2978, 1.2852, 1.3104, 1.1058, 1.1172, 1.0944, 0, 0}, VOLT, NORMAL_POLL_INTERVAL},
  [SERVER_A_PVPP_ABC_CPU_VOL] =
  {"A_PVPP_ABC_CPU_VOL", CPLD_ADC_PVPP_CPU, read_cpld_adc, POWER_ON_READING, {2.8325, 2.805, 2.86, 2.30375, 2.3275, 2.28, 0, 0}, VOLT, NORMAL_POLL_INTERVAL},
  [SERVER_A_P3V3_NETLAKE_VOL] =
  {"A_P3V3_NETLAKE_VOL", CPLD_ADC_P3V3, read_cpld_adc, POWER_ON_READING, {3.56895, 3.5343, 3.6036, 3.04095, 3.0723, 3.0096, 0, 0}, VOLT, NORMAL_POLL_INTERVAL},
  [SERVER_P1V8_STBY_VOL] =
  {"P1V8_STBY_NETLAKE_VOL", VR_P1V8_STBY_VOL, read_pmbus, STBY_READING, {1.91889, 1.90026, 1.93752, 1.67616, 1.69344, 1.65888, 0, 0}, VOLT, NORMAL_POLL_INTERVAL},
  [SERVER_P1V05_STBY_VOL] =
  {"P1V05_STBY_VOL", VR_P1V05_STBY_VOL, read_pmbus, STBY_READING, {1.12167, 1.11078, 1.13256, 0.99813, 1.00842, 0.98784, 0, 0}, VOLT, NORMAL_POLL_INTERVAL},
  [SERVER_PVNN_PCH_VOL] =
  {"PVNN_PCH_VOL", VR_PVNN_PCH_VOL, read_pmbus, STBY_READING, {1.236, 1.224, 1.248, 0.582, 0.588, 0.576, 0, 0}, VOLT, NORMAL_POLL_INTERVAL},
  [SERVER_PVCCIN_VOL] =
  {"PVCCIN_VOL", VR_PVCCIN_VOL, read_pmbus, POWER_ON_READING, {2.06, 2.04, 2.08, 1.455, 1.47, 1.44, 0, 0}, VOLT, NORMAL_POLL_INTERVAL},
  [SERVER_PVCCANCPU_VOL] =
  {"PVCCANCPU_VOL", VR_PVCCANCPU_VOL, read_pmbus, POWER_ON_READING, {1.03515, 1.0251, 1.0452, 0.96515, 0.9751, 0.9552, 0, 0}, VOLT, NORMAL_POLL_INTERVAL},
  [SERVER_PVDDQ_ABC_CPU_VOL] =
  {"PVDDQ_ABC_CPU_VOL", VR_PVDDQ_ABC_CPU_VOL, read_pmbus, POWER_ON_READING, {1.2978, 1.2852, 1.3104, 1.1058, 1.1172, 1.0944, 0, 0}, VOLT, NORMAL_POLL_INTERVAL},
  [SERVER_P1V8_STBY_CUR] =
  {"P1V8_STBY_NETLAKE_CUR", VR_P1V8_STBY_CUR, read_pmbus, STBY_READING, {2.163, 0, 2.184, 0, 0, 0, 0, 0}, CURR, NORMAL_POLL_INTERVAL},
  [SERVER_P1V05_STBY_CUR] =
  {"P1V05_STBY_CUR", VR_P1V05_STBY_CUR, read_pmbus, STBY_READING, {12.051, 0, 12.168, 0, 0, 0, 0, 0}, CURR, NORMAL_POLL_INTERVAL},
  [SERVER_PVNN_PCH_CUR] =
  {"PVNN_PCH_CUR", VR_PVNN_PCH_CUR, read_pmbus, STBY_READING, {7.21, 0, 7.28, 0, 0, 0, 0, 0}, CURR, NORMAL_POLL_INTERVAL},
  [SERVER_PVCCIN_CUR] =
  {"PVCCIN_CUR", VR_PVCCIN_CUR, read_pmbus, POWER_ON_READING, {63.9, 0, 71, 0, 0, 0, 0, 0}, CURR, NORMAL_POLL_INTERVAL},
  [SERVER_PVCCANCPU_CUR] =
  {"PVCCANCPU_CUR", VR_PVCCANCPU_CUR, read_pmbus, POWER_ON_READING, {1.545, 0, 1.56, 0, 0, 0, 0, 0}, CURR, NORMAL_POLL_INTERVAL},
  [SERVER_PVDDQ_ABC_CPU_CUR] =
  {"PVDDQ_ABC_CPU_CUR", VR_PVDDQ_ABC_CPU_CUR, read_pmbus, POWER_ON_READING, {17.0671, 0, 17.2328, 0, 0, 0, 0, 0}, CURR, NORMAL_POLL_INTERVAL},
  [SERVER_P12V_COME_PWR] =
  {"P12V_COME_PWR", P12V_COME_PWR, read_ina230_pwr, STBY_READING, {117.4645, 0, 119.7565, 0, 0, 0, 0, 0}, POWER, NORMAL_POLL_INTERVAL},
  [SERVER_P1V8_STBY_PWR] =
  {"P1V8_STBY_NETLAKE_PWR", VR_P1V8_STBY_PWR, read_pmbus, STBY_READING, {4.15056, 0, 4.23154, 0, 0, 0, 0, 0}, POWER, NORMAL_POLL_INTERVAL},
  [SERVER_P1V05_STBY_PWR] =
  {"P1V05_STBY_PWR", VR_P1V05_STBY_PWR, read_pmbus, STBY_READING, {13.0952, 0, 13.3507, 0, 0, 0, 0, 0}, POWER, NORMAL_POLL_INTERVAL},
  [SERVER_PVNN_PCH_PWR] =
  {"PVNN_PCH_PWR", VR_PVNN_PCH_PWR, read_pmbus, STBY_READING, {8.91156, 0, 9.08544, 0, 0, 0, 0, 0}, POWER, NORMAL_POLL_INTERVAL},
  [SERVER_PVCCIN_PWR] =
  {"PVCCIN_PWR", VR_PVCCIN_PWR, read_pmbus, POWER_ON_READING, {78.9804, 0, 88.608, 0, 0, 0, 0, 0}, POWER, NORMAL_POLL_INTERVAL},
  [SERVER_PVCCANCPU_PWR] =
  {"PVCCANCPU_PWR", VR_PVCCANCPU_PWR, read_pmbus, POWER_ON_READING, {1.59931, 0, 1.63051, 0, 0, 0, 0, 0}, POWER, NORMAL_POLL_INTERVAL},
  [SERVER_PVDDQ_ABC_CPU_PWR] =
  {"PVDDQ_ABC_CPU_PWR", VR_PVDDQ_ABC_CPU_PWR, read_pmbus, POWER_ON_READING, {22.1497, 0, 22.5819, 0, 0, 0, 0, 0}, POWER, NORMAL_POLL_INTERVAL},
};

PAL_SENSOR_MAP bmc_sensor_map[] = {
  [BMC_P12V_STBY_MTP_VOL] =
  {"P12V_STBY_MTP_VOL", ADC1, read_adc_val, STBY_READING, {13.3488, 13.2192, 14.333, 10.7088, 10.8192, 10.091, 0, 0}, VOLT, NORMAL_POLL_INTERVAL},
  [BMC_P12V_COME_VOL] =
  {"P12V_COME_MTP_VOL", ADC3, read_adc_val, STBY_READING, {13.3488, 13.2192, 14.333, 10.7088, 10.8192, 10.091, 0, 0}, VOLT, NORMAL_POLL_INTERVAL},
  [BMC_P3V_BAT_MTP_VOL] =
  {"P3V_BAT_MTP_VOL", ADC8, read_battery_val, STBY_READING, {3.502, 3.468, 3.993, 2.7645, 2.793, 2.31, 0, 0}, VOLT, ONE_HOUR_INVERVAL},
  [BMC_P3V3_STBY_MTP_VOL] =
  {"P3V3_STBY_MTP_VOL", ADC2, read_adc_val, STBY_READING, {3.56895, 3.5343, 3.993, 3.04095, 3.0723, 2.31, 0, 0}, VOLT, NORMAL_POLL_INTERVAL},
  [BMC_P5V_STBY_VOL] =
  {"P5V_STBY_VOL", ADC0, read_adc_val, STBY_READING, {5.4075, 5.355, 5.8, 4.6075, 4.655, 4, 0, 0}, VOLT, NORMAL_POLL_INTERVAL},
  [BMC_P12V_NIC_MTP_VOL] =
  {"P12V_NIC_MTP_VOL", ADC7, read_adc_val, STBY_READING, {13.3488, 13.2192, 14.333, 10.7088, 10.8192, 10.091, 0, 0}, VOLT, NORMAL_POLL_INTERVAL},
  [BMC_P3V3_NIC_MTP_VOL] =
  {"P3V3_NIC_MTP_VOL", ADC6, read_adc_val, STBY_READING, {3.56895, 3.5343, 3.993, 3.04095, 3.0723, 2.31, 0, 0}, VOLT, NORMAL_POLL_INTERVAL},
  [BMC_P12V_NIC_MTP_CURR] =
  {"P12V_NIC_MTP_CURR", ADC5, read_adc_val, STBY_READING, {3.9, 0, 0, 0, 0, 0, 0, 0}, CURR, NORMAL_POLL_INTERVAL},
  [BMC_P3V3_NIC_MTP_CURR] =
  {"P3V3_NIC_MTP_CURR", ADC4, read_adc_val, STBY_READING, {1, 0, 0, 0, 0, 0, 0, 0}, CURR, NORMAL_POLL_INTERVAL},
  [BMC_HSC_OUTPUT_VOL] =
  {"HSC_OUTPUT_VOL", HSC_VOUT, read_hsc, STBY_READING, {13.3488, 13.2192, 14.333, 10.7088, 10.8192, 10.091, 0, 0}, VOLT, NORMAL_POLL_INTERVAL},
  [BMC_HSC_OUTPUT_CUR] =
  {"HSC_OUTPUT_CUR", HSC_IOUT, read_hsc, STBY_READING, {20, 0, 0, 0, 0, 0, 0, 0}, CURR, NORMAL_POLL_INTERVAL},
  [BMC_HSC_INPUT_PWR] =
  {"HSC_INPUT_PWR", HSC_PIN, read_hsc, STBY_READING, {200, 0, 0, 0, 0, 0, 0, 0}, POWER, NORMAL_POLL_INTERVAL},
  [BMC_M2_A_TEMP] =
  {"M.2_A TEMP", NVME_A_TEMP, read_nvme_temp, POWER_ON_READING, {0, 0, 0, 0, 0, 0, 0, 0}, TEMP, NORMAL_POLL_INTERVAL},
  [BMC_M2_B_TEMP] =
  {"M.2_B_TEMP", NVME_B_TEMP, read_nvme_temp, POWER_ON_READING, {0, 0, 0, 0, 0, 0, 0, 0}, TEMP, NORMAL_POLL_INTERVAL},
  [BMC_M2_C_TEMP] =
  {"M.2_C_TEMP", NVME_C_TEMP, read_nvme_temp, POWER_ON_READING, {0, 0, 0, 0, 0, 0, 0, 0}, TEMP, NORMAL_POLL_INTERVAL},
};

PAL_SENSOR_MAP fio_sensor_map[] = {
  [FIO_INLET_TEMP] =
  {"FIO_INLET_TEMP", FIO_INLET, read_temp, STBY_READING, {0, 0, 0, 0, 0, 0, 0, 0}, TEMP, NORMAL_POLL_INTERVAL},
};

PAL_SENSOR_MAP nic_sensor_map[] = {
  [BMC_OCP_NIC_TEMP] =
  {"OCP_NIC_TEMP", OCP_NIC_TEMP, read_nic_temp, POWER_ON_READING, {0, 0, 0, 0, 0, 0, 0, 0}, TEMP, NORMAL_POLL_INTERVAL},
};

const char *adc_label[] = {
  "ADC_P5V_STBY_VOL",
  "ADC_P12V_STBY_MTP_VOL",
  "ADC_P3V3_STBY_MTP_VOL",
  "ADC_P12V_COME_VOL",
  "ADC_P3V3_NIC_MTP_CURR",
  "ADC_P12V_NIC_MTP_CURR",
  "ADC_P3V3_NIC_MTP_VOL",
  "ADC_P12V_NIC_MTP_VOL",
  "ADC_P3V_BAT_MTP_VOL",
};

PAL_SENSOR_MAP pdb_sensor_map[] = {
  [FAN0_TACH] =
  {"FAN0_TACH", FAN0, read_rpm, STBY_READING, {0, 0, 0, 0, 0, 0, 0, 0}, FAN, FAN_POLL_INTERVAL},
  [FAN1_TACH] =
  {"FAN1_TACH", FAN1, read_rpm, STBY_READING, {0, 0, 0, 0, 0, 0, 0, 0}, FAN, FAN_POLL_INTERVAL},
  [FAN2_TACH] =
  {"FAN2_TACH", FAN2, read_rpm, STBY_READING, {0, 0, 0, 0, 0, 0, 0, 0}, FAN, FAN_POLL_INTERVAL},
  [FAN3_TACH] =
  {"FAN3_TACH", FAN3, read_rpm, STBY_READING, {0, 0, 0, 0, 0, 0, 0, 0}, FAN, FAN_POLL_INTERVAL},
};

const uint8_t server_sensor_list[] = {
  SERVER_INLET_TEMP,
  SERVER_OUTLET_TEMP,
  SERVER_SOC_TEMP,
  SERVER_DIMMA_TEMP,
  SERVER_DIMMB_TEMP,
  SERVER_A_P12V_STBY_NETLAKE_VOL,
  SERVER_A_P3V3_STBY_NETLAKE_VOL,
  SERVER_A_P1V8_STBY_NETLAKE_VOL,
  SERVER_A_P5V_STBY_NETLAKE_VOL,
  SERVER_A_P1V05_STBY_NETLAKE_VOL,
  SERVER_A_PVNN_PCH_VOL,
  SERVER_A_PVCCIN_VOL,
  SERVER_A_PVCCANA_CPU_VOL,
  SERVER_A_PVDDQ_ABC_CPU_VOL,
  SERVER_A_PVPP_ABC_CPU_VOL,
  SERVER_A_P3V3_NETLAKE_VOL,
  SERVER_P1V8_STBY_VOL,
  SERVER_P1V05_STBY_VOL,
  SERVER_PVNN_PCH_VOL,
  SERVER_PVCCIN_VOL,
  SERVER_PVCCANCPU_VOL,
  SERVER_PVDDQ_ABC_CPU_VOL,
  SERVER_P1V8_STBY_CUR,
  SERVER_P1V05_STBY_CUR,
  SERVER_PVNN_PCH_CUR,
  SERVER_PVCCIN_CUR,
  SERVER_PVCCANCPU_CUR,
  SERVER_PVDDQ_ABC_CPU_CUR,
  SERVER_P12V_COME_PWR,
  SERVER_P1V8_STBY_PWR,
  SERVER_P1V05_STBY_PWR,
  SERVER_PVNN_PCH_PWR,
  SERVER_PVCCIN_PWR,
  SERVER_PVCCANCPU_PWR,
  SERVER_PVDDQ_ABC_CPU_PWR,
};

const uint8_t bmc_sensor_list[] = {
  BMC_P12V_STBY_MTP_VOL,
  BMC_P12V_COME_VOL,
  BMC_P3V_BAT_MTP_VOL,
  BMC_P3V3_STBY_MTP_VOL,
  BMC_P5V_STBY_VOL,
  BMC_P12V_NIC_MTP_VOL,
  BMC_P3V3_NIC_MTP_VOL,
  BMC_P12V_NIC_MTP_CURR,
  BMC_P3V3_NIC_MTP_CURR,
  BMC_HSC_OUTPUT_VOL,
  BMC_HSC_OUTPUT_CUR,
  BMC_HSC_INPUT_PWR,
  BMC_M2_A_TEMP,
  BMC_M2_B_TEMP,
  BMC_M2_C_TEMP,
};

const uint8_t pdb_sensor_list[] = {
  FAN0_TACH,
  FAN1_TACH,
  FAN2_TACH,
  FAN3_TACH,
};

const uint8_t fio_sensor_list[] = {
  FIO_INLET_TEMP,
};

const uint8_t nic_sensor_list[] = {
  BMC_OCP_NIC_TEMP,
};

PAL_DEV_INFO temp_dev_list[] = {
  {"tmp75-i2c-4-48",  "MB_INLET_TEMP"},
  {"tmp75-i2c-4-4a",  "MB_OUTLET_TEMP"},
  {"tmp75-i2c-10-48",  "FIO_INLET_TEMP"},
  {"peci_cputemp.cpu0-isa-0000", "Die"},
  {"peci_dimmtemp.cpu0-isa-0000", "DIMM A1"},
  {"peci_dimmtemp.cpu0-isa-0000", "DIMM B1"},
};

PAL_PMBUS_INFO pmbus_dev_table[] = {
  [VR_P1V8_STBY_VOL] =
  {VR_BUS, VR_P1V8_STBY_ADDR, CMD_VOUT, {{XDPE15284D, PAGE1}, {MP2993GU, PAGE1}}},
  [VR_P1V05_STBY_VOL] =
  {VR_BUS, VR_P1V05_STBY_ADDR, CMD_VOUT, {{TDA38640, PAGE0}, {MP9941GMJT, PAGE0}}},
  [VR_PVNN_PCH_VOL] =
  {VR_BUS, VR_PVNN_PCH_ADDR, CMD_VOUT, {{TDA38640, PAGE0}, {MP9941GMJT, PAGE0}}},
  [VR_PVCCIN_VOL] =
  {VR_BUS, VR_PVCCIN_ADDR, CMD_VOUT, {{XDPE15284D, PAGE0}, {MP2993GU, PAGE0}}},
  [VR_PVCCANCPU_VOL] =
  {VR_BUS, VR_PVCCANCPU_ADDR, CMD_VOUT, {{TDA38640, PAGE0}, {MP9941GMJT, PAGE0}}},
  [VR_PVDDQ_ABC_CPU_VOL] =
  {VR_BUS, VR_PVDDQ_ABC_CPU_ADDR, CMD_VOUT, {{TDA38640, PAGE0}, {MP9941GMJT, PAGE0}}},
  [VR_P1V8_STBY_CUR] =
  {VR_BUS, VR_P1V8_STBY_ADDR, CMD_IOUT, {{XDPE15284D, PAGE1}, {MP2993GU, PAGE1}}},
  [VR_P1V05_STBY_CUR] =
  {VR_BUS, VR_P1V05_STBY_ADDR, CMD_IOUT, {{TDA38640, PAGE0}, {MP9941GMJT, PAGE0}}},
  [VR_PVNN_PCH_CUR] =
  {VR_BUS, VR_PVNN_PCH_ADDR, CMD_IOUT, {{TDA38640, PAGE0}, {MP9941GMJT, PAGE0}}},
  [VR_PVCCIN_CUR] =
  {VR_BUS, VR_PVCCIN_ADDR, CMD_IOUT, {{XDPE15284D, PAGE0}, {MP2993GU, PAGE0}}},
  [VR_PVCCANCPU_CUR] =
  {VR_BUS, VR_PVCCANCPU_ADDR, CMD_IOUT, {{TDA38640, PAGE0}, {MP9941GMJT, PAGE0}}},
  [VR_PVDDQ_ABC_CPU_CUR] =
  {VR_BUS, VR_PVDDQ_ABC_CPU_ADDR, CMD_IOUT, {{TDA38640, PAGE0}, {MP9941GMJT, PAGE0}}},
  [VR_P1V8_STBY_PWR] =
  {VR_BUS, VR_P1V8_STBY_ADDR, CMD_POUT, {{XDPE15284D, PAGE1}, {MP2993GU, PAGE1}}},
  [VR_P1V05_STBY_PWR] =
  {VR_BUS, VR_P1V05_STBY_ADDR, CMD_POUT, {{TDA38640, PAGE0}, {MP9941GMJT, PAGE0}}},
  [VR_PVNN_PCH_PWR] =
  {VR_BUS, VR_PVNN_PCH_ADDR, CMD_POUT, {{TDA38640, PAGE0}, {MP9941GMJT, PAGE0}}},
  [VR_PVCCIN_PWR] =
  {VR_BUS, VR_PVCCIN_ADDR, CMD_POUT, {{XDPE15284D, PAGE0}, {MP2993GU, PAGE0}}},
  [VR_PVCCANCPU_PWR] =
  {VR_BUS, VR_PVCCANCPU_ADDR, CMD_POUT, {{TDA38640, PAGE0}, {MP9941GMJT, PAGE0}}},
  [VR_PVDDQ_ABC_CPU_PWR] =
  {VR_BUS, VR_PVDDQ_ABC_CPU_ADDR, CMD_POUT, {{TDA38640, PAGE0}, {MP9941GMJT, PAGE0}}},
};

PAL_PMBUS_INFO hsc_dev_table[] = {
  [HSC_VOUT] = {MTP_HSC_BUS, MTP_HSC_ADDR, CMD_VOUT, {{ADM1278, HSC_NO_SET_PAGE}}},
  [HSC_IOUT] = {MTP_HSC_BUS, MTP_HSC_ADDR, CMD_IOUT, {{ADM1278, HSC_NO_SET_PAGE}}},
  [HSC_PIN] = {MTP_HSC_BUS, MTP_HSC_ADDR, CMD_PIN, {{ADM1278, HSC_NO_SET_PAGE}}},
};

/*List item: {CPLD-ADC reading low-byte offset, resistor ratio}
* High-byte offset of reading always be low-byte offset + 1
*/
PAL_CPLD_ADC_INFO cpld_adc_list[] = {
  {0x00, 3.6549},
  {0x04, 1},
  {0x06, 1},
  {0x02, 1.544},
  {0x0A, 1},
  {0x08, 1},
  {0x14, 1},
  {0x12, 1},
  {0x10, 1},
  {0x0E, 1},
  {0x0C, 1},
};

const uint8_t nvme_temp_list[] = {
  [NVME_A_TEMP] = NVME_A_BUS,
  [NVME_B_TEMP] = NVME_B_BUS,
  [NVME_C_TEMP] = NVME_C_BUS,
};

PAL_PMBUS_POWER_TABLE power_cal_list[] = {
  [VR_P1V8_STBY_PWR] = {VR_P1V8_STBY_VOL, VR_P1V8_STBY_CUR},
  [VR_P1V05_STBY_PWR] = {VR_P1V05_STBY_VOL, VR_P1V05_STBY_CUR},
  [VR_PVNN_PCH_PWR] = {VR_PVNN_PCH_VOL, VR_PVNN_PCH_CUR},
  [VR_PVCCIN_PWR] = {VR_PVCCIN_VOL, VR_PVCCIN_CUR},
  [VR_PVCCANCPU_PWR] = {VR_PVCCANCPU_VOL, VR_PVCCANCPU_CUR},
  [VR_PVDDQ_ABC_CPU_PWR] = {VR_PVDDQ_ABC_CPU_VOL, VR_PVDDQ_ABC_CPU_CUR},
};

size_t server_sensor_cnt = ARRAY_SIZE(server_sensor_list);
size_t bmc_sensor_cnt = ARRAY_SIZE(bmc_sensor_list);
size_t pdb_sensor_cnt = ARRAY_SIZE(pdb_sensor_list);
size_t fio_sensor_cnt = ARRAY_SIZE(fio_sensor_list);
size_t nic_sensor_cnt = ARRAY_SIZE(nic_sensor_list);

size_t pmbus_dev_cnt = ARRAY_SIZE(pmbus_dev_table);
size_t hsc_dev_cnt = ARRAY_SIZE(hsc_dev_table);

/**
*  @brief Function of getting sensor list and sensor count from FRU ID
*
*  @param fru: FRU ID
*  @param **sensor_list: return value of sensor list
*  @param *cnt: return value of sensor count
*
*  @return Status of getting FRU ID
*  0: Success
*  ERR_UNKNOWN_FRU: Unknown FRU error
**/
int
pal_get_fru_sensor_list(uint8_t fru, uint8_t **sensor_list, int *cnt) {

  switch(fru) {
  case FRU_SERVER:
    *sensor_list = (uint8_t *) server_sensor_list;
    *cnt = server_sensor_cnt;
    break;
  case FRU_BMC:
    *sensor_list = (uint8_t *) bmc_sensor_list;
    *cnt = bmc_sensor_cnt;
    break;
  case FRU_PDB:
    *sensor_list = (uint8_t *) pdb_sensor_list;
    *cnt = pdb_sensor_cnt;
    break;
  case FRU_FIO:
    *sensor_list = (uint8_t *) fio_sensor_list;
    *cnt = fio_sensor_cnt;
    break;
  case FRU_NIC:
    *sensor_list = (uint8_t *) nic_sensor_list;
    *cnt = nic_sensor_cnt;
    break;
  default:
    if (fru > MAX_NUM_FRUS) {
      return ERR_UNKNOWN_FRU;
    }
    // Nothing to read yet.
    *sensor_list = NULL;
    *cnt = 0;
    break;
  }

  return 0;
}

/**
*  @brief Function of reading tmp75 sensor
*
*  @param id: i2c device id
*  @param *value: return value of sensor reading
*
*  @return Status of reading sensor
*  0: Success
*  -1: Failed
*  ERR_SENSOR_NA: Sensor NA error
**/
static int
read_temp(uint8_t id, float *value) {
  if (id >= ARRAY_SIZE(temp_dev_list)) {
    return ERR_SENSOR_NA;
  }

  return sensors_read(temp_dev_list[id].chip, temp_dev_list[id].label, value);
}

static int
read_peci(uint8_t id, float *value) {
  if (value == NULL) {
    syslog(LOG_ERR, "%s: invalid parameter: value pointer is NULL", __func__);
    return -1;
  }

  if (id >= ARRAY_SIZE(temp_dev_list)) {
    return ERR_SENSOR_NA;
  }

  int ret = 0;
  ret = sensors_read(temp_dev_list[id].chip, temp_dev_list[id].label, value);
  if ((ret < 0) && (retryDIMM < SENSOR_RETRY_TIME)) {
    sensors_reinit();
    sleep(POWER_ON_SENSOR_RETRY_SEC);
    ret = sensors_read(temp_dev_list[id].chip, temp_dev_list[id].label, value);
    if (ret == 0) {
      retryDIMM = SENSOR_RETRY_TIME;
    } else {
      retryDIMM++;
    }
  }

  return ret;
}

static int
read_adc_val(uint8_t id, float *value) {
  int ret = 0;

  if (id >= ARRAY_SIZE(adc_label)) {
    return ERR_SENSOR_NA;
  }

  ret = sensors_read_adc(adc_label[id], value);

  return ret;
}

static int
read_battery_val(uint8_t id, float *value) {
  int ret = -1;

  gpio_desc_t *gpio_bat_sense_en = gpio_open_by_shadow("FM_BATTERY_SENSE_EN");
  if (!gpio_bat_sense_en) {
    return -1;
  }
  if (gpio_set_value(gpio_bat_sense_en, GPIO_VALUE_HIGH)) {
    syslog(LOG_WARNING, "%s() Fail to set gpio for reading sensor_id:%x", __func__, id);
    goto fail;
  }

  msleep(20);
  ret = read_adc_val(id, value);
  if (gpio_set_value(gpio_bat_sense_en, GPIO_VALUE_LOW)) {
    syslog(LOG_WARNING, "%s() Fail to recover gpio for reading sensor_id:%x", __func__, id);
    goto fail;
  }

fail:
  gpio_close(gpio_bat_sense_en);
  return ret;
}

static int
read_pmbus(uint8_t id, float *value) {
  if (id >= ARRAY_SIZE(pmbus_dev_table)) {
    return ERR_SENSOR_NA;
  }

  int fd = 0, ret = 0;
  char key[MAX_KEY_LEN] = {0};

  if (pmbus_dev_table[id].offset == CMD_POUT) {
    float voltage, current;

    ret = read_pmbus(power_cal_list[id].vol_sensor_id, &voltage);
    if (ret < 0) {
      syslog(LOG_ERR, "%s() Failed to get voltage value to calculate power", __func__);
      return -1;
    }

    ret = read_pmbus(power_cal_list[id].cur_sensor_id, &current);
    if (ret < 0) {
      syslog(LOG_ERR, "%s() Failed to get current value to calculate power", __func__);
      return -1;
    }

    *value = voltage * current;

    return 0;
  } else {
    uint8_t retry = SENSOR_RETRY_TIME;
    uint8_t bus = pmbus_dev_table[id].bus;
    uint8_t addr = pmbus_dev_table[id].slv_addr;
    char val[MAX_VALUE_LEN] = {0};
    //kvbuf[0]: pmbus page, kvbuf[1]: read byte, kvbuf[2]: read format
    uint8_t kvbuf[3] = {0};
    char* saveptr;
    int bufcnt = 0;

    snprintf(key, MAX_FRU_CMD_STR, "pmbus-sensor%02x%c", id, '\0');
    ret = kv_get(key, val, NULL, 0);
    if (ret < 0) {
      syslog(LOG_ERR, "%s() Failed to get pmbus info, errno=%d", __func__, errno);
      return -1;
    }

    char *kv_value;
    kv_value = strtok_r(val, "-", &saveptr);
    while( kv_value != NULL )
    {
      kvbuf[bufcnt] = atoi(kv_value);
      bufcnt++;
      kv_value = strtok_r(NULL, "-", &saveptr);
    }

    pmbus_i2c_info pmbus_i2c_data;
    memcpy(&pmbus_i2c_data, &kvbuf, sizeof(pmbus_i2c_data));

    //Set pmbus page
    uint8_t setpage_data[2];
    setpage_data[0] = CMD_PAGE;
    setpage_data[1] = pmbus_i2c_data.page;

    fd = i2c_cdev_slave_open(bus, addr >> 1, I2C_SLAVE_FORCE_CLAIM);
    if (fd < 0) {
      syslog(LOG_ERR, "%s() Failed to open I2C bus %d\n", __func__, bus);
      return -1;
    }

    retry = SENSOR_RETRY_TIME;
    do {
      ret = i2c_rdwr_msg_transfer(fd, addr, setpage_data, sizeof(setpage_data),
                                  NULL, 0);
      usleep(SENSOR_RETRY_INTERVAL_USEC);
    } while ((ret < 0) && ((retry--) > 0));

    if (ret < 0) {
      close(fd);
      syslog(LOG_ERR, "%s() Failed to set page %d-%d\n", __func__, bus, addr);
      return -1;
    }

    //Get pmbus reading
    uint8_t getreading_data = pmbus_dev_table[id].offset;
    uint8_t rlen_reading = pmbus_i2c_data.read_byte;
    uint8_t rbuf_reading_raw[rlen_reading];
    uint8_t vout_mode_data = VR_VOUT_MODE_REG;
    uint8_t rlen_vout_mode = 1;
    uint8_t rbuf_vout_mode;
    float* rbuf_reading = 0;

    retry = SENSOR_RETRY_TIME;
    do {
      ret = i2c_rdwr_msg_transfer(fd, addr, &getreading_data,
                                  sizeof(getreading_data),
                                  rbuf_reading_raw, rlen_reading);
      usleep(SENSOR_RETRY_INTERVAL_USEC);
    } while ((ret < 0) && ((retry--) > 0));

    if (ret < 0) {
      close(fd);
      syslog(LOG_ERR, "%s() Failed to get reading %d-%d\n", __func__, bus, addr);
      return -1;
    }

    uint8_t read_format = pmbus_i2c_data.read_type;
    switch(read_format) {
    case LINEAR11:
      ret = netlakemtp_common_linear11_convert(rbuf_reading_raw, value);
      if (ret < 0) {
          *value = 0;
          syslog(LOG_ERR, "%s() Failed to get reading by linear-11 format %d-%d\n",
              __func__, bus, addr);
        }
      break;
    case VOUT_MODE:
      ret = i2c_rdwr_msg_transfer(fd, addr, &vout_mode_data,
                                  sizeof(vout_mode_data), &rbuf_vout_mode, rlen_vout_mode);
      if (ret < 0) {
        close(fd);
        syslog(LOG_ERR, "%s() Failed to get VOUT_MODE for device %d-%d\n",
              __func__, bus, addr);
        return -1;
      }
      // VOUT_MODE bit[7-5] is used for deciding VOUT format
      if ((rbuf_vout_mode & 0xe0) == 0) {
        ret = netlakemtp_common_linear16_convert(rbuf_reading_raw, rbuf_vout_mode, value);
        if (ret < 0) {
          *value = 0;
          syslog(LOG_ERR, "%s() Failed to get reading by linear-16 format %d-%d\n",
              __func__, bus, addr);
        }
      } else {
        *value = 0;
        syslog(LOG_ERR, "%s() VOUT_MODE is not supported for device %d-%d\n",
              __func__, bus, addr);
      }
      break;
    default:
      *value = 0;
      syslog(LOG_ERR, "%s() Now only support linear-11 and linear-16 format\n",
            __func__);
      break;
    }

    close(fd);
    return 0;
  }
}

static int
read_hsc(uint8_t id, float *value) {
  if (id >= ARRAY_SIZE(hsc_dev_table)) {
    return ERR_SENSOR_NA;
  }

  int fd = 0, ret = 0;
  uint8_t retry = SENSOR_RETRY_TIME;
  uint8_t bus = hsc_dev_table[id].bus;
  uint8_t addr = hsc_dev_table[id].slv_addr;
  char key[MAX_KEY_LEN] = {0};
  char val[MAX_VALUE_LEN] = {0};
  uint8_t kvbuf[2] = {0};
  char* saveptr;
  int bufcnt = 0;

  snprintf(key, MAX_FRU_CMD_STR, "hsc-sensor%02x%c", id, '\0');
  ret = kv_get(key, val, NULL, 0);
  if (ret < 0) {
    syslog(LOG_ERR, "%s() Failed to get HSC info, errno=%d", __func__, errno);
    return -1;
  }

  char *kv_value;
  kv_value = strtok_r(val, "-", &saveptr);
  while( kv_value != NULL )
  {
    kvbuf[bufcnt] = atoi(kv_value);
    bufcnt++;
    kv_value = strtok_r(NULL, "-", &saveptr);
  }

  hsc_i2c_info hsc_i2c_data;
  memcpy(&hsc_i2c_data, &kvbuf, sizeof(hsc_i2c_data));

  fd = i2c_cdev_slave_open(bus, addr >> 1, I2C_SLAVE_FORCE_CLAIM);
  if (fd < 0) {
    syslog(LOG_ERR, "%s() Failed to open I2C bus %d\n", __func__, bus);
    return -1;
  }

  //Get HSC reading
  uint8_t getreading_data = hsc_dev_table[id].offset;
  uint8_t rlen = hsc_dev_list[hsc_i2c_data.hsc_type_cnt].hsc_cmd_list[hsc_i2c_data.hsc_cmd_cnt].read_byte;
  uint8_t rbuf[rlen];

  retry = SENSOR_RETRY_TIME;
  do {
    ret = i2c_rdwr_msg_transfer(fd, addr, (uint8_t *)&getreading_data, sizeof(getreading_data), rbuf, rlen);
    usleep(SENSOR_RETRY_INTERVAL_USEC);
  } while ((ret < 0) && ((retry--) > 0));

  if (ret < 0) {
    close(fd);
    syslog(LOG_ERR, "%s() Failed to get reading %d-%d\n", __func__, bus, addr);
    return -1;
  }

  float m = hsc_dev_list[hsc_i2c_data.hsc_type_cnt].hsc_cmd_list[hsc_i2c_data.hsc_cmd_cnt].m;
  float b = hsc_dev_list[hsc_i2c_data.hsc_type_cnt].hsc_cmd_list[hsc_i2c_data.hsc_cmd_cnt].b;
  float r = hsc_dev_list[hsc_i2c_data.hsc_type_cnt].hsc_cmd_list[hsc_i2c_data.hsc_cmd_cnt].r;
  *value = ((float)(rbuf[1] << 8 | rbuf[0]) * r - b) / m;

  close(fd);
  return 0;
}

int
read_cpld_adc(uint8_t id, float *value) {
  if (value == NULL) {
    syslog(LOG_ERR, "%s: invalid parameter: value pointer is NULL", __func__);
    return -1;
  }

  if (id >= ARRAY_SIZE(cpld_adc_list)) {
    return ERR_SENSOR_NA;
  }

  int ret = 0, fd = 0;
  uint8_t retry = SENSOR_RETRY_TIME;
  uint8_t tlen = 1, rlen = 1;
  uint8_t tbuf = cpld_adc_list[id].lowbyte_cmd;
  uint8_t rbuf = 0;
  uint16_t sensor_read_raw = 0x0000;

  fd = i2c_cdev_slave_open(CPLD_ADC_REG_BUS, CPLD_ADC_REG_ADDR >> 1,
                           I2C_SLAVE_FORCE_CLAIM);
  if (fd < 0) {
    syslog(LOG_ERR, "Failed to open CPLD 0x%x\n", CPLD_ADC_REG_ADDR);
    return -1;
  }

  //Get low byte
  retry = SENSOR_RETRY_TIME;
  do {
    ret = i2c_rdwr_msg_transfer(fd, CPLD_ADC_REG_ADDR,
                                &tbuf, tlen, &rbuf, rlen);
    if (ret != 0) {
      usleep(SENSOR_RETRY_INTERVAL_USEC);
    }
  } while ((ret < 0) && ((retry--) > 0));

  if (ret < 0) {
    syslog(LOG_ERR, "%s() Failed to get lowbyte reading %x-%x", __func__,
           CPLD_ADC_REG_BUS, CPLD_ADC_REG_ADDR);
    close(fd);
    return -1;
  } else {
    //Mask for getting bit[0-3]
    sensor_read_raw = rbuf & 0xF;
  }

  //Get high byte
  tbuf += 1;
  retry = SENSOR_RETRY_TIME;
  do {
    ret = i2c_rdwr_msg_transfer(fd, CPLD_ADC_REG_ADDR,
                                &tbuf, tlen, &rbuf, rlen);
    if (ret != 0) {
      usleep(SENSOR_RETRY_INTERVAL_USEC);
    }
  } while ((ret < 0) && ((retry--) > 0));
  close(fd);

  if (ret < 0) {
    syslog(LOG_ERR, "%s() Failed to get highbyte reading %x-%x", __func__,
           CPLD_ADC_REG_BUS, CPLD_ADC_REG_ADDR);
    return -1;
  } else {
    //Mask for getting bit[4-11]
    sensor_read_raw += (rbuf << 4) & 0xFF0;
  }

  //Calculate from divided voltage
  *value = ((float)sensor_read_raw / 4096) * 3.3 * cpld_adc_list[id].resistor_ratio;

  return ret;
}

static int
read_ina230_pwr(uint8_t id, float *value) {
  if (value == NULL) {
    syslog(LOG_ERR, "%s: invalid parameter: value pointer is NULL", __func__);
    return -1;
  }

  int ret = 0;
  /* byte 1-3: Intel Manufacturer ID, byte 4: Mode
     byte 5: Domain ID, byte 6: Policy ID */
  uint8_t tbuf[] = {0x57, 0x1, 0x0, 0x1, 0x0, 0x0};
  uint8_t rbuf[NM_GLOBAL_POWER_STATISTICS_LENGTH];
  uint8_t rlen = 0;
  uint8_t retry;

  for (retry = SENSOR_RETRY_TIME; retry > 0; retry--) {
    ret = netlakemtp_common_me_ipmb_wrapper(NETFN_NM_REQ,
                                        CMD_NM_GET_NODE_MANAGER_STATISTICS,
                                        tbuf, sizeof(tbuf), rbuf, &rlen);
    if (ret >= 0) {
      break;
    }

    usleep(SENSOR_RETRY_INTERVAL_USEC);
  }

  if (ret < 0) {
    syslog(LOG_ERR, "%s() Failed to get ina230 reading from NM", __func__);
    return -1;
  } else {
    //2 bytes for Power Statistics Current Value
    *value = (float)(rbuf[4] << 8 | rbuf[3]);
  }

  return 0;
}

static int
read_nvme_temp(uint8_t id, float *value) {

  if (value == NULL) {
    syslog(LOG_ERR, "%s: invalid parameter: value pointer is NULL", __func__);
    return -1;
  }

  if (id >= ARRAY_SIZE(nvme_temp_list)) {
    return ERR_SENSOR_NA;
  }

  int ret = 0, fd = 0;
  uint8_t bus = nvme_temp_list[id];
  uint8_t addr = NVME_ADDR;
  uint8_t tbuf = NVME_GET_STATUS_CMD;
  uint8_t tlen = sizeof(tbuf);
  uint8_t rlen = NVME_GET_STATUS_LEN;
  uint8_t retry;
  uint8_t rbuf[NVME_GET_STATUS_LEN] = {0};

  fd = i2c_cdev_slave_open(bus, addr >> 1, I2C_SLAVE_FORCE_CLAIM);
  if (fd < 0) {
    syslog(LOG_WARNING, "%s() Failed to open I2C bus %d\n", __func__, bus);
    return ERR_SENSOR_NA;
  }

  for (retry = SENSOR_RETRY_TIME; retry > 0; retry--) {
    ret = i2c_rdwr_msg_transfer(fd, addr, &tbuf, tlen, rbuf, rlen);
    if (ret >= 0) {
      break;
    }

    usleep(SENSOR_RETRY_INTERVAL_USEC);
  }

  close(fd);

  if (ret < 0) {
    return ERR_SENSOR_NA;
  }

  // valid temperature range: -60C(0xC4) ~ +127C(0x7F)
  // C4h-FFh is two's complement, means -60 to -1
  ret = nvme_temp_value_check((int)rbuf[NVME_TEMP_REG], value);
  if (ret == SNR_READING_SKIP) {
    ret = SENSOR_NA;
  }
  else if (ret != 0) {
    ret = ERR_SENSOR_NA;
  }

  return ret;
}

/* Check the valid range of NIC Temperature. */
static int
nic_temp_value_check(uint8_t value, float *value_check) {

  if (value_check == NULL) {
    syslog(LOG_ERR, "%s(): fail to check NIC temperature due to the NULL parameter.\n", __func__);
    return -1;
  }

  if (value <= MAX_NIC_TEMPERATURE) {
    *value_check = (float)value;
  } else {
    return -1;
  }

  return 0;
}

static int
read_nic_temp(uint8_t id, float *value) {

  if (value == NULL) {
    syslog(LOG_ERR, "%s: invalid parameter: value pointer is NULL", __func__);
    return -1;
  }

  int fd = 0, ret = -1;
  uint8_t bus = NIC_BUS;
  uint8_t addr = NIC_ADDR;
  uint8_t retry = SENSOR_RETRY_TIME;
  uint8_t tbuf = NIC_INFO_TEMP_CMD;
  uint8_t tlen = sizeof(tbuf);
  uint8_t rlen = NIC_TEMP_LEN;
  uint8_t rbuf = 0;
  uint8_t res_temp = 0;
  static int nic_temp_retry = 0;

  fd = i2c_cdev_slave_open(bus, addr >> 1, I2C_SLAVE_FORCE_CLAIM);
  if (fd < 0) {
    syslog(LOG_WARNING, "%s() Failed to open I2C bus %d\n", __func__, bus);
    return ERR_SENSOR_NA;
  }

  for (retry = SENSOR_RETRY_TIME; retry > 0; retry--) {
    ret = i2c_rdwr_msg_transfer(fd, addr, &tbuf, tlen, &rbuf, rlen);
    if (ret == 0) {
      break;
    }

    usleep(SENSOR_RETRY_INTERVAL_USEC);
  }

  if (ret < 0) {
    return ERR_SENSOR_NA;
  }

  ret = nic_temp_value_check(rbuf, value);

  // Temperature within valid range
  if (ret == 0) {
    nic_temp_retry = 0;
  } else {
    if (nic_temp_retry <= NIC_TEMP_RETRY_TIME) {
      ret = READING_SKIP;
      nic_temp_retry++;
    } else {
      ret = ERR_SENSOR_NA;
    }
  }

  close(fd);

  if (ret < 0) {
    syslog(LOG_ERR, "%s() Failed to get reading %x-%x", __func__,
           NIC_BUS, NIC_ADDR);
    return -1;
  } else {
    *value = (float)rbuf;
  }

  return ret;
}

/**
*  @brief Function of reading sensor
*
*  @param fru: FRU ID
*  @param sensor_num: sensor number
*  @param *value: return value of sensor reading
*
*  @return Status of reading sensor
*  0: Success
*  -1: Failed
*  ERR_SENSOR_NA: Sensor NA error
**/
int
pal_sensor_read_raw(uint8_t fru, uint8_t sensor_num, void *value) {
  char key[MAX_KEY_LEN] = {0};
  char str[MAX_VALUE_LEN] = {0};
  char fru_name[32] = {0};
  int ret = 0;
  uint8_t id = 0;
  uint8_t server_status = 0;
  PAL_SENSOR_MAP sensor;

  if (pal_get_fru_name(fru, fru_name)) {
    syslog(LOG_WARNING, "%s() Fail to get fru%d name\n", __func__, fru);
    return ERR_SENSOR_NA;
  }

  snprintf(key, sizeof(key), "%s_sensor%d", fru_name, sensor_num);

  switch(fru) {
  case FRU_SERVER:
    sensor = server_sensor_map[sensor_num];
    break;
  case FRU_BMC:
    sensor = bmc_sensor_map[sensor_num];
    break;
  case FRU_PDB:
    sensor = pdb_sensor_map[sensor_num];
    break;
  case FRU_FIO:
    sensor = fio_sensor_map[sensor_num];
    break;
  case FRU_NIC:
    sensor = nic_sensor_map[sensor_num];
    break;
  default:
    return ERR_SENSOR_NA;
  }

  // Check available sensor readings to avoid readings in wrong status
  if (sensor.reading_available == STBY_READING) {
    ret = sensor.read_sensor(sensor.id, (float * ) value);

  } else if (sensor.reading_available == POWER_ON_READING) {
    ret = kv_get(PWR_GOOD_KV_KEY, str, NULL, 0);
    if (ret < 0) {
      syslog(LOG_ERR, "%s: Failed to get gpio power good status in kv.", __func__);
      return ERR_SENSOR_NA;
    }

    if (strncmp(str, HIGH_STR, strlen(HIGH_STR)) == 0) {
      ret = sensor.read_sensor(sensor.id, (float * ) value);
    } else {
      ret = SENSOR_NA;
    }
  } else if (sensor.reading_available == POST_COMPLT_READING) {
    ret = kv_get(POST_CMPLT_KV_KEY, str, NULL, 0);
    if (ret < 0) {
      syslog(LOG_ERR, "%s: Failed to get post complete status in kv.", __func__);
      return ERR_SENSOR_NA;
    }

    if (strncmp(str, LOW_STR, strlen(LOW_STR)) == 0) {
      ret = sensor.read_sensor(sensor.id, (float * ) value);
    } else {
      ret = SENSOR_NA;
    }
  } else {
    ret = ERR_SENSOR_NA;
  }

  memset(str, 0, sizeof(str));

  if (ret != 0) {
    kv_get(key, str, 0, 0);

    if (strcmp(str, "NA") != 0) {
      switch (ret) {
      case ERR_SENSOR_NA:
          syslog(LOG_WARNING,
                 "%s() Read sensor %s failed because of sensor NA error.\n",
                 __func__, sensor.snr_name);
          break;
      case ERR_UNKNOWN_FRU:
          syslog(LOG_WARNING,
                 "%s() Read sensor %s failed because of unknown FRU error.\n",
                 __func__, sensor.snr_name);
          break;
      case SENSOR_NA:
          break;
      default:
          syslog(LOG_WARNING,
                 "%s() Read sensor %s failed because of unknwon issue.\n",
                  __func__, sensor.snr_name);
          break;
      }
    }

    strncpy(str, "NA", sizeof(str));
  } else {
    snprintf(str, sizeof(str), "%.2f", *((float * ) value));
  }

  if (kv_set(key, str, 0, 0) < 0) {
    syslog(LOG_WARNING, "%s() cache_set key = %s, str = %s failed.\n", __func__, key, str);
    return ERR_SENSOR_NA;
  }

  return ret;
}

/**
*  @brief Function of getting sensor name
*
*  @param fru: FRU ID
*  @param sensor_num: sensor number
*  @param *name: return value of sensor name
*
*  @return Status of reading sensor name
*  0: Success
*  ERR_UNKNOWN_FRU: Unknown FRU error
**/
int
pal_get_sensor_name(uint8_t fru, uint8_t sensor_num, char *name) {
  switch(fru) {
  case FRU_SERVER:
    snprintf(name, MAX_SENSOR_NAME_SIZE, "%s", server_sensor_map[sensor_num].snr_name);
    break;
  case FRU_BMC:
    snprintf(name, MAX_SENSOR_NAME_SIZE, "%s", bmc_sensor_map[sensor_num].snr_name);
    break;
  case FRU_PDB:
    snprintf(name, MAX_SENSOR_NAME_SIZE, "%s", pdb_sensor_map[sensor_num].snr_name);
    break;
  case FRU_FIO:
    snprintf(name, MAX_SENSOR_NAME_SIZE, "%s", fio_sensor_map[sensor_num].snr_name);
    break;
  case FRU_NIC:
    snprintf(name, MAX_SENSOR_NAME_SIZE, "%s", nic_sensor_map[sensor_num].snr_name);
    break;
  default:
    return ERR_UNKNOWN_FRU;
  }

  return 0;
}

/**
*  @brief Function of getting sensor threshold
*
*  @param fru: FRU ID
*  @param sensor_num: sensor number
*  @param thresh: sensor threshold type
*  @param *value: return value of sensor threshold
*
*  @return Status of reading sensor threshold
*  0: Success
*  -1: Threshold type error
*  ERR_UNKNOWN_FRU: Unknown FRU error
**/
int
pal_get_sensor_threshold(uint8_t fru, uint8_t sensor_num, uint8_t thresh, void *value) {
  float *val = (float*) value;
  PAL_SENSOR_MAP * sensor_map = NULL;

  switch (fru) {
  case FRU_SERVER:
    sensor_map = server_sensor_map;
    break;
  case FRU_BMC:
    sensor_map = bmc_sensor_map;
    break;
  case FRU_PDB:
    sensor_map = pdb_sensor_map;
    break;
  case FRU_FIO:
    sensor_map = fio_sensor_map;
    break;
  case FRU_NIC:
    sensor_map = nic_sensor_map;
    break;
  default:
    return ERR_UNKNOWN_FRU;
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
    syslog(LOG_WARNING, "%s() Threshold type error value=%d\n", __func__, thresh);
    return -1;
  }

  return 0;
}

/**
*  @brief Function of getting sensor units
*
*  @param fru: FRU ID
*  @param sensor_num: sensor number
*  @param *units: return value of sensor units
*
*  @return Status of reading sensor units
*  0: Success
*  ERR_UNKNOWN_FRU: Unknown FRU error
**/
int
pal_get_sensor_units(uint8_t fru, uint8_t sensor_num, char *units) {
  uint8_t sensor_units = 0;

  switch (fru) {
  case FRU_SERVER:
    sensor_units = server_sensor_map[sensor_num].units;
    break;
  case FRU_BMC:
    sensor_units = bmc_sensor_map[sensor_num].units;
    break;
  case FRU_PDB:
    sensor_units = pdb_sensor_map[sensor_num].units;
    break;
  case FRU_FIO:
    sensor_units = fio_sensor_map[sensor_num].units;
    break;
  case FRU_NIC:
    sensor_units = nic_sensor_map[sensor_num].units;
    break;
  default:
    return ERR_UNKNOWN_FRU;
  }

  switch(sensor_units) {
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
  case FLOW:
    sprintf(units, "CFM");
    break;
  default:
    syslog(LOG_WARNING, "%s() unit not found, sensor number: %x, unit: %u\n", __func__, sensor_num, sensor_units);
    break;
  }

  return 0;
}

int
pal_get_sensor_poll_interval(uint8_t fru, uint8_t sensor_num, uint32_t *value)
{
  switch (fru)
  {
    case FRU_SERVER:
      *value = server_sensor_map[sensor_num].poll_invernal;
      break;
    case FRU_BMC:
      *value = bmc_sensor_map[sensor_num].poll_invernal;
      break;
    case FRU_PDB:
      *value = pdb_sensor_map[sensor_num].poll_invernal;
      break;
    case FRU_NIC:
      *value = nic_sensor_map[sensor_num].poll_invernal;
      break;
    default:
      return ERR_UNKNOWN_FRU;
  }

  return PAL_EOK;
}

static
int read_rpm(uint8_t id, float *value) {
  int rpm = 0;
  int ret = 0;

  ret = pal_get_fan_speed(id, &rpm);
  if (ret == 0) {
    *value = rpm;
    return ret;
  }

  if (retryFAN[id] < SENSOR_RETRY_TIME) {
    retryFAN[id]++;
    return SENSOR_NA;
  }

  retryFAN[id] = 0;
  return ERR_SENSOR_NA;
}
