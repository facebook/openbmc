#include <stdio.h>
#include <stdbool.h>
#include <stdint.h>
#include <errno.h>
#include <unistd.h>
#include <syslog.h>
#include <time.h>
#include <openbmc/misc-utils.h>
#include <openbmc/libgpio.h>
#include <openbmc/obmc-i2c.h>
#include <openbmc/obmc-sensors.h>
#include "pal.h"
#include "pal_calibration.h"

#define PAL_FAN_CNT 4
#define PAL_NIC_CNT 8
#define GPIO_BATTERY_DETECT "BATTERY_DETECT"
#define GPIO_NIC_PRSNT "OCP_V3_%d_PRSNTB_R_N"
#define SENSOR_SKIP_MAX (1)

#define IIO_DEV_DIR(device, bus, addr, index) \
  "/sys/bus/i2c/drivers/"#device"/"#bus"-00"#addr"/iio:device"#index"/%s"
#define IIO_AIN_NAME       "in_voltage%s_raw"

#define NIC_MAX11615_DIR     IIO_DEV_DIR(max1363, 23, 33, 1)
#define NIC_MAX11617_DIR     IIO_DEV_DIR(max1363, 23, 35, 2)

size_t pal_pwm_cnt = 4;
size_t pal_tach_cnt = 8;
const char pal_pwm_list[] = "0..3";
const char pal_tach_list[] = "0..7";
uint8_t g_board_id = 0xFF;

static int sensors_read_fan_pwm(uint8_t sensor_num, float *value);
static int read_inlet_sensor(uint8_t snr_id, float *value);
static int read_fan_volt(uint8_t fan_id, float *value);
static int read_fan_curr(uint8_t fan_id, float *value);
static int read_fan_pwr(uint8_t fan_id, float *value);
static int read_adc_value(uint8_t adc_id, float *value);
static int read_bat_value(uint8_t adc_id, float *value);
static int read_hsc_iout(uint8_t hsc_id, float *value);
static int read_hsc_vin(uint8_t hsc_id, float *value);
static int read_hsc_vout(uint8_t hsc_id, float *value);
static int read_hsc_pin(uint8_t hsc_id, float *value);
static void hsc_value_adjust(struct calibration_table *table, float *value);
static int read_pax_therm(uint8_t, float*);
static int read_nvme_temp(uint8_t sensor_num, float *value);
static int read_nvme_sensor(uint8_t sensor_num, float *value);
static int read_bay_temp(uint8_t sensor_num, float *value);
static int read_hsc_temp(uint8_t hsc_id, float *value);
static int read_nic_temp(uint8_t nic_id, float *value);
static int read_nic_volt(uint8_t nic_id, float *value);
static int read_nic_curr(uint8_t nic_id, float *value);
static int read_nic_pwr(uint8_t nic_id, float *value);
static int sensors_read_vr(uint8_t sensor_num, float *value);
static bool is_fan_present(uint8_t fan_id);
static int sensors_read_fan_speed(uint8_t, float*);
static int read_ina219_sensor(uint8_t sensor_num, float *value);
static int read_bay0_ssd_volt(uint8_t nic_id, float *value);
static int read_bay0_ssd_curr(uint8_t nic_id, float *value);
static int read_bay1_ssd_volt(uint8_t nic_id, float *value);
static int read_bay1_ssd_curr(uint8_t nic_id, float *value);
static int read_ina260_sensor(uint8_t sensor_num, float *value);
static int sensors_read_maxim(const char *dev, const char *channel, int *data);

static float fan_volt[PAL_FAN_CNT];
static float fan_curr[PAL_FAN_CNT];
static float nic_volt[PAL_NIC_CNT];
static float nic_curr[PAL_NIC_CNT];
static float vr_vdd_volt[vr_NUM];
static float vr_vdd_curr[vr_NUM];
static float vr_avd_volt[vr_NUM];
static float vr_avd_curr[vr_NUM];

const uint8_t mb_sensor_list[] = {
  MB_VR_P0V8_VDD0_TEMP,
  MB_VR_P0V8_VDD0_VOUT,
  MB_VR_P0V8_VDD0_CURR,
  MB_VR_P0V8_VDD0_POWER,
  MB_VR_P1V0_AVD0_TEMP,
  MB_VR_P1V0_AVD0_VOUT,
  MB_VR_P1V0_AVD0_CURR,
  MB_VR_P1V0_AVD0_POWER,
  MB_VR_P0V8_VDD1_TEMP,
  MB_VR_P0V8_VDD1_VOUT,
  MB_VR_P0V8_VDD1_CURR,
  MB_VR_P0V8_VDD1_POWER,
  MB_VR_P1V0_AVD1_TEMP,
  MB_VR_P1V0_AVD1_VOUT,
  MB_VR_P1V0_AVD1_CURR,
  MB_VR_P1V0_AVD1_POWER,
  MB_VR_P0V8_VDD2_TEMP,
  MB_VR_P0V8_VDD2_VOUT,
  MB_VR_P0V8_VDD2_CURR,
  MB_VR_P0V8_VDD2_POWER,
  MB_VR_P1V0_AVD2_TEMP,

  MB_VR_P1V0_AVD2_VOUT,
  MB_VR_P1V0_AVD2_CURR,
  MB_VR_P1V0_AVD2_POWER,
  MB_VR_P0V8_VDD3_TEMP,
  MB_VR_P0V8_VDD3_VOUT,
  MB_VR_P0V8_VDD3_CURR,
  MB_VR_P0V8_VDD3_POWER,
  MB_VR_P1V0_AVD3_TEMP,
  MB_VR_P1V0_AVD3_VOUT,
  MB_VR_P1V0_AVD3_CURR,
  MB_VR_P1V0_AVD3_POWER,
  MB_P12V_AUX,
  MB_P3V3_STBY,
  MB_P5V_STBY,
  MB_P3V3,
  MB_P3V3_PAX,
  MB_P3V_BAT,
  MB_P2V5_AUX,
  MB_P1V2_AUX,
  MB_P1V15_AUX,
  MB_PAX_0_TEMP,
  MB_PAX_1_TEMP,
  MB_PAX_2_TEMP,
  MB_PAX_3_TEMP,
  SYSTEM_INLET_TEMP,
  SYSTEM_INLET_REMOTE_TEMP,
  MB_INLET_TEMP_L,
  MB_INLET_REMOTE_TEMP_L,
  MB_INLET_TEMP_R,
  MB_INLET_REMOTE_TEMP_R,
  BAY_0_VOL,
  BAY_0_IOUT,
  BAY_0_POUT,
  BAY_1_VOL,
  BAY_1_IOUT,
  BAY_1_POUT,
};

const uint8_t mb_brcm_sensor_list[] = {
  VR_P0V9_PEX0_TEMP,
  VR_P0V9_PEX0_VOUT,
  VR_P0V9_PEX0_CURR,
  VR_P0V9_PEX1_TEMP,
  VR_P0V9_PEX1_VOUT,
  VR_P0V9_PEX1_CURR,
  VR_P0V9_PEX2_TEMP,
  VR_P0V9_PEX2_VOUT,
  VR_P0V9_PEX2_CURR,
  VR_P0V9_PEX3_TEMP,
  VR_P0V9_PEX3_VOUT,
  VR_P0V9_PEX3_CURR,
  VR_P1V8_PEX0_1_TEMP,
  VR_P1V8_PEX0_1_VOUT,
  VR_P1V8_PEX0_1_CURR,
  VR_P1V8_PEX2_3_TEMP,
  VR_P1V8_PEX2_3_VOUT,
  VR_P1V8_PEX2_3_CURR,

  MB_P12V_AUX,
  MB_P3V3_STBY,
  MB_P5V_STBY,
  MB_P3V3,
  MB_P3V_BAT,
  MB_P2V5_AUX,
  MB_P1V2_AUX,
  MB_P1V15_AUX,
  MB_PEX_0_TEMP,
  MB_PEX_1_TEMP,
  MB_PEX_2_TEMP,
  MB_PEX_3_TEMP,
  SYSTEM_INLET_TEMP,
  SYSTEM_INLET_REMOTE_TEMP,
  MB_INLET_TEMP_L,
  MB_INLET_REMOTE_TEMP_L,
  MB_INLET_TEMP_R,
  MB_INLET_REMOTE_TEMP_R,
  BAY_0_VOL,
  BAY_0_IOUT,
  BAY_0_POUT,
  BAY_1_VOL,
  BAY_1_IOUT,
  BAY_1_POUT,
};

const uint8_t pdb_sensor_list[] = {
  PDB_HSC_TEMP,
  PDB_HSC_VIN,
  PDB_HSC_VOUT,
  PDB_HSC_IOUT,
  PDB_HSC_PIN,
  PDB_FAN0_VOLT,
  PDB_FAN0_CURR,
  PDB_FAN0_PWR,
  PDB_FAN1_VOLT,
  PDB_FAN1_CURR,
  PDB_FAN1_PWR,
  PDB_FAN2_VOLT,
  PDB_FAN2_CURR,
  PDB_FAN2_PWR,
  PDB_FAN3_VOLT,
  PDB_FAN3_CURR,
  PDB_FAN3_PWR,
  PDB_INLET_TEMP_L,
  PDB_INLET_REMOTE_TEMP_L,
  PDB_INLET_TEMP_R,
  PDB_INLET_REMOTE_TEMP_R,
  PDB_FAN0_TACH_I,
  PDB_FAN0_TACH_O,
  PDB_FAN1_TACH_I,
  PDB_FAN1_TACH_O,
  PDB_FAN2_TACH_I,
  PDB_FAN2_TACH_O,
  PDB_FAN3_TACH_I,
  PDB_FAN3_TACH_O,
  PDB_FAN0_PWM,
  PDB_FAN1_PWM,
  PDB_FAN2_PWM,
  PDB_FAN3_PWM,
};

const uint8_t m2_1_sensor_list[] = {
  BAY_0_FTEMP,
  BAY_0_RTEMP,
  BAY_0_0_NVME_CTEMP,
  BAY_0_1_NVME_CTEMP,
  BAY_0_2_NVME_CTEMP,
  BAY_0_3_NVME_CTEMP,
  BAY_0_4_NVME_CTEMP,
  BAY_0_5_NVME_CTEMP,
  BAY_0_6_NVME_CTEMP,
  BAY_0_7_NVME_CTEMP,
  BAY_0_NVME_CTEMP,
  M2_0_SSD_0_VOLT,
  M2_0_SSD_0_CURR,
  M2_0_SSD_1_VOLT,
  M2_0_SSD_1_CURR,
  M2_0_SSD_2_VOLT,
  M2_0_SSD_2_CURR,
  M2_0_SSD_3_VOLT,
  M2_0_SSD_3_CURR,
  M2_0_SSD_4_VOLT,
  M2_0_SSD_4_CURR,
  M2_0_SSD_5_VOLT,
  M2_0_SSD_5_CURR,
  M2_0_SSD_6_VOLT,
  M2_0_SSD_6_CURR,
  M2_0_SSD_7_VOLT,
  M2_0_SSD_7_CURR,
  BAY0_INA219_VOLT,
  BAY0_INA219_CURR,
  BAY0_INA219_PWR,
};

const uint8_t m2_2_sensor_list[] = {
  BAY_1_FTEMP,
  BAY_1_RTEMP,
  BAY_1_0_NVME_CTEMP,
  BAY_1_1_NVME_CTEMP,
  BAY_1_2_NVME_CTEMP,
  BAY_1_3_NVME_CTEMP,
  BAY_1_4_NVME_CTEMP,
  BAY_1_5_NVME_CTEMP,
  BAY_1_6_NVME_CTEMP,
  BAY_1_7_NVME_CTEMP,
  BAY_1_NVME_CTEMP,
  M2_1_SSD_0_VOLT,
  M2_1_SSD_0_CURR,
  M2_1_SSD_1_VOLT,
  M2_1_SSD_1_CURR,
  M2_1_SSD_2_VOLT,
  M2_1_SSD_2_CURR,
  M2_1_SSD_3_VOLT,
  M2_1_SSD_3_CURR,
  M2_1_SSD_4_VOLT,
  M2_1_SSD_4_CURR,
  M2_1_SSD_5_VOLT,
  M2_1_SSD_5_CURR,
  M2_1_SSD_6_VOLT,
  M2_1_SSD_6_CURR,
  M2_1_SSD_7_VOLT,
  M2_1_SSD_7_CURR,
  BAY1_INA219_VOLT,
  BAY1_INA219_CURR,
  BAY1_INA219_PWR,
};

const uint8_t e1s_1_sensor_list[] = {
  BAY_0_FTEMP,
  BAY_0_0_NVME_CTEMP,
  BAY_0_1_NVME_CTEMP,
  BAY_0_2_NVME_CTEMP,
  BAY_0_3_NVME_CTEMP,
  BAY_0_4_NVME_CTEMP,
  BAY_0_5_NVME_CTEMP,
  BAY_0_6_NVME_CTEMP,
  BAY_0_7_NVME_CTEMP,
  BAY_0_NVME_CTEMP,
};

const uint8_t e1s_2_sensor_list[] = {
  BAY_1_FTEMP,
  BAY_1_0_NVME_CTEMP,
  BAY_1_1_NVME_CTEMP,
  BAY_1_2_NVME_CTEMP,
  BAY_1_3_NVME_CTEMP,
  BAY_1_4_NVME_CTEMP,
  BAY_1_5_NVME_CTEMP,
  BAY_1_6_NVME_CTEMP,
  BAY_1_7_NVME_CTEMP,
  BAY_1_NVME_CTEMP,
};

const uint8_t nic0_sensor_list[] = {
  MB_NIC_0_TEMP,
  MB_NIC_0_VOLT,
  MB_NIC_0_CURR,
  MB_NIC_0_POWER,
};

const uint8_t nic1_sensor_list[] = {
  MB_NIC_1_TEMP,
  MB_NIC_1_VOLT,
  MB_NIC_1_CURR,
  MB_NIC_1_POWER,
};

const uint8_t nic2_sensor_list[] = {
  MB_NIC_2_TEMP,
  MB_NIC_2_VOLT,
  MB_NIC_2_CURR,
  MB_NIC_2_POWER,
};

const uint8_t nic3_sensor_list[] = {
  MB_NIC_3_TEMP,
  MB_NIC_3_VOLT,
  MB_NIC_3_CURR,
  MB_NIC_3_POWER,
};

const uint8_t nic4_sensor_list[] = {
  MB_NIC_4_TEMP,
  MB_NIC_4_VOLT,
  MB_NIC_4_CURR,
  MB_NIC_4_POWER,
};

const uint8_t nic5_sensor_list[] = {
  MB_NIC_5_TEMP,
  MB_NIC_5_VOLT,
  MB_NIC_5_CURR,
  MB_NIC_5_POWER,
};

const uint8_t nic6_sensor_list[] = {
  MB_NIC_6_TEMP,
  MB_NIC_6_VOLT,
  MB_NIC_6_CURR,
  MB_NIC_6_POWER,
};

const uint8_t nic7_sensor_list[] = {
  MB_NIC_7_TEMP,
  MB_NIC_7_VOLT,
  MB_NIC_7_CURR,
  MB_NIC_7_POWER,
};

PAL_I2C_BUS_INFO nic_info_list[] = {
  {MEZZ0, I2C_BUS_1 , 0x3E, 0},
  {MEZZ1, I2C_BUS_9 , 0x3E, 4},
  {MEZZ2, I2C_BUS_2 , 0x3E, 1},
  {MEZZ3, I2C_BUS_10, 0x3E, 5},
  {MEZZ4, I2C_BUS_4 , 0x3E, 2},
  {MEZZ5, I2C_BUS_11, 0x3E, 6},
  {MEZZ6, I2C_BUS_7 , 0x3E, 3},
  {MEZZ7, I2C_BUS_13, 0x3E, 7},
};

//ADM1278
PAL_ADM1278_INFO adm1278_info_list[] = {
  {ADM1278_VOLTAGE, 19599, 0, 100},
  {ADM1278_CURRENT, 842 * ADM1278_RSENSE, 20475, 10},
  {ADM1278_POWER, 6442 * ADM1278_RSENSE, 0, 100},
  {ADM1278_TEMP, 42, 31880, 10},
};

PAL_HSC_INFO hsc_info_list[] = {
  {HSC_ID0, ADM1278_SLAVE_ADDR, adm1278_info_list },
};

//INA219
PAL_I2C_BUS_INFO ina219_info_list[] = {
  {INA219_ID0, I2C_BUS_21, 0x8A},
  {INA219_ID1, I2C_BUS_22, 0x8A},
};

//{SensorName, ID, FUNCTION, PWR_STATUS, {UCR, UNR, UNC, LCR, LNR, LNC, Pos, Neg}
PAL_SENSOR_MAP sensor_map[] = {
  {"CC_MB_VR_P0V9_PEX0_TEMP" , VR_P0V9_PEX0_TEMP , sensors_read_vr, 0, {115, 0, 0, 10, 0, 0, 0, 0}, TEMP}, //0x00
  {"CC_MB_VR_P0V9_PEX0_VOUT" , VR_P0V9_PEX0_VOUT , sensors_read_vr, 0, {0.945, 0, 0, 0.855, 0, 0, 0}, VOLT}, //0x01
  {"CC_MB_VR_P0V9_PEX0_CURR" , VR_P0V9_PEX0_CURR , sensors_read_vr, 0, {0, 0, 0, 0, 0, 0, 0, 0}, CURR}, //0x02
  {"CC_MB_VR_P0V9_PEX1_TEMP" , VR_P0V9_PEX1_TEMP , sensors_read_vr, 0, {115, 0, 0, 10, 0, 0, 0, 0}, TEMP}, //0x03
  {"CC_MB_VR_P0V9_PEX1_VOUT" , VR_P0V9_PEX1_VOUT , sensors_read_vr, 0, {0.945, 0, 0, 0.855, 0, 0, 0, 0}, VOLT}, //0x04
  {"CC_MB_VR_P0V9_PEX1_CURR" , VR_P0V9_PEX1_CURR , sensors_read_vr, 0, {0, 0, 0, 0, 0, 0, 0, 0}, CURR}, //0x05
  {"CC_MB_VR_P0V9_PEX2_TEMP" , VR_P0V9_PEX2_TEMP , sensors_read_vr, 0, {115, 0, 0, 10, 0, 0, 0, 0}, TEMP}, //0x06
  {"CC_MB_VR_P0V9_PEX2_VOUT" , VR_P0V9_PEX2_VOUT , sensors_read_vr, 0, {0.945, 0, 0, 0.855, 0, 0, 0, 0}, VOLT}, //0x07
  {"CC_MB_VR_P0V9_PEX2_CURR" , VR_P0V9_PEX2_CURR , sensors_read_vr, 0, {0, 0, 0, 0, 0, 0, 0, 0}, CURR}, //0x08
  {"CC_MB_VR_P0V9_PEX3_TEMP" , VR_P0V9_PEX3_TEMP , sensors_read_vr, 0, {115, 0, 0, 10, 0, 0, 0, 0}, TEMP}, //0x09
  {"CC_MB_VR_P0V9_PEX3_VOUT" , VR_P0V9_PEX3_VOUT , sensors_read_vr, 0, {0.945, 0, 0, 0.855, 0, 0, 0, 0}, VOLT}, //0x0A
  {"CC_MB_VR_P0V9_PEX3_CURR" , VR_P0V9_PEX3_CURR , sensors_read_vr, 0, {0, 0, 0, 0, 0, 0, 0, 0}, CURR}, //0x0B
  {"CC_MB_VR_P1V8_PEX0_1_TEMP" , VR_P1V8_PEX0_1_TEMP , sensors_read_vr, 0, {115, 0, 0, 10, 0, 0, 0, 0}, TEMP}, //0x0C
  {"CC_MB_VR_P1V8_PEX0_1_VOUT" , VR_P1V8_PEX0_1_VOUT , sensors_read_vr, 0, {1.89, 0, 0, 1.71, 0, 0, 0}, VOLT}, //0x0D
  {"CC_MB_VR_P1V8_PEX0_1_CURR" , VR_P1V8_PEX0_1_CURR , sensors_read_vr, 0, {0, 0, 0, 0, 0, 0, 0, 0}, CURR}, //0x0E
  {NULL, 0, NULL, 0, {0, 0, 0, 0, 0, 0, 0, 0}, 0}, //0x0F

  {"CC_NIC_0_TEMP", NIC0, read_nic_temp, 0, {95, 0, 0, 10, 0, 0, 0, 0}, TEMP}, //0x10
  {"CC_NIC_0_VOLT", NIC0, read_nic_volt, 0, {12.96, 0, 0, 10.56, 0, 0, 0, 0}, VOLT}, //0x11
  {"CC_NIC_0_IOUT", NIC0, read_nic_curr, 0, {0, 0, 0, 0, 0, 0, 0, 0}, CURR}, //0x12
  {"CC_NIC_0_POUT", NIC0, read_nic_pwr , 0, {0, 0, 0, 0, 0, 0, 0, 0}, POWER}, //0x13
  {"CC_NIC_1_TEMP", NIC1, read_nic_temp, 0, {95, 0, 0, 10, 0, 0, 0, 0}, TEMP}, //0x14
  {"CC_NIC_1_VOLT", NIC1, read_nic_volt, 0, {12.96, 0, 0, 10.56, 0, 0, 0, 0}, VOLT}, //0x15
  {"CC_NIC_1_IOUT", NIC1, read_nic_curr, 0, {0, 0, 0, 0, 0, 0, 0, 0}, CURR}, //0x16
  {"CC_NIC_1_POUT", NIC1, read_nic_pwr , 0, {0, 0, 0, 0, 0, 0, 0, 0}, POWER}, //0x17
  {"CC_NIC_2_TEMP", NIC2, read_nic_temp, 0, {95, 0, 0, 10, 0, 0, 0, 0}, TEMP}, //0x18
  {"CC_NIC_2_VOLT", NIC2, read_nic_volt, 0, {12.96, 0, 0, 10.56, 0, 0, 0, 0}, VOLT}, //0x19
  {"CC_NIC_2_IOUT", NIC2, read_nic_curr, 0, {0, 0, 0, 0, 0, 0, 0, 0}, CURR}, //0x1A
  {"CC_NIC_2_POUT", NIC2, read_nic_pwr , 0, {0, 0, 0, 0, 0, 0, 0, 0}, POWER}, //0x1B
  {"CC_NIC_3_TEMP", NIC3, read_nic_temp, 0, {95, 0, 0, 10, 0, 0, 0, 0}, TEMP}, //0x1C
  {"CC_NIC_3_VOLT", NIC3, read_nic_volt, 0, {12.96, 0, 0, 10.56, 0, 0, 0, 0}, VOLT}, //0x1D
  {"CC_NIC_3_IOUT", NIC3, read_nic_curr, 0, {0, 0, 0, 0, 0, 0, 0, 0}, CURR}, //0x1E
  {"CC_NIC_3_POUT", NIC3, read_nic_pwr , 0, {0, 0, 0, 0, 0, 0, 0, 0}, POWER}, //0x1F

  {"CC_NIC_4_TEMP", NIC4, read_nic_temp, 0, {95, 0, 0, 10, 0, 0, 0, 0}, TEMP}, //0x20
  {"CC_NIC_4_VOLT", NIC4, read_nic_volt, 0, {12.96, 0, 0, 10.56, 0, 0, 0, 0}, VOLT}, //0x21
  {"CC_NIC_4_IOUT", NIC4, read_nic_curr, 0, {0, 0, 0, 0, 0, 0, 0, 0}, CURR}, //0x22
  {"CC_NIC_4_POUT", NIC4, read_nic_pwr , 0, {0, 0, 0, 0, 0, 0, 0, 0}, POWER}, //0x23
  {"CC_NIC_5_TEMP", NIC5, read_nic_temp, 0, {95, 0, 0, 10, 0, 0, 0, 0}, TEMP}, //0x24
  {"CC_NIC_5_VOLT", NIC5, read_nic_volt, 0, {12.96, 0, 0, 10.56, 0, 0, 0, 0}, VOLT}, //0x25
  {"CC_NIC_5_IOUT", NIC5, read_nic_curr, 0, {0, 0, 0, 0, 0, 0, 0, 0}, CURR}, //0x26
  {"CC_NIC_5_POUT", NIC5, read_nic_pwr , 0, {0, 0, 0, 0, 0, 0, 0, 0}, POWER}, //0x27
  {"CC_NIC_6_TEMP", NIC6, read_nic_temp, 0, {95, 0, 0, 10, 0, 0, 0, 0}, TEMP}, //0x28
  {"CC_NIC_6_VOLT", NIC6, read_nic_volt, 0, {12.96, 0, 0, 10.56, 0, 0, 0, 0}, VOLT}, //0x29
  {"CC_NIC_6_IOUT", NIC6, read_nic_curr, 0, {0, 0, 0, 0, 0, 0, 0, 0}, CURR}, //0x2A
  {"CC_NIC_6_POUT", NIC6, read_nic_pwr , 0, {0, 0, 0, 0, 0, 0, 0, 0}, POWER}, //0x2B
  {"CC_NIC_7_TEMP", NIC7, read_nic_temp, 0, {95, 0, 0, 10, 0, 0, 0, 0}, TEMP}, //0x2C
  {"CC_NIC_7_VOLT", NIC7, read_nic_volt, 0, {12.96, 0, 0, 10.56, 0, 0, 0, 0}, VOLT}, //0x2D
  {"CC_NIC_7_IOUT", NIC7, read_nic_curr, 0, {0, 0, 0, 0, 0, 0, 0, 0}, CURR}, //0x2E
  {"CC_NIC_7_POUT", NIC7, read_nic_pwr , 0, {0, 0, 0, 0, 0, 0, 0, 0}, POWER}, //0x2F

  {"CC_SSD_BAY_0_FTEMP", BAY0_FTEMP_ID, read_bay_temp, true, {50, 0, 0, 10, 0, 0, 0, 0}, TEMP}, //0x30
  {"CC_SSD_BAY_0_RTEMP", BAY0_RTEMP_ID, read_bay_temp, true, {70, 0, 0, 10, 0, 0, 0, 0}, TEMP}, //0x31
  {"CC_SSD_BAY_0_NVME_CTEMP", BAY_0_NVME_CTEMP, read_nvme_temp, 0, {69, 0, 0, 10, 0, 0, 0, 0}, TEMP}, //0x32
  {"CC_SSD_BAY_0_0_NVME_CTEMP", BAY_0_0_NVME_CTEMP, read_nvme_temp, 0, {69, 0, 0, 10, 0, 0, 0, 0}, TEMP}, //0x33
  {"CC_SSD_BAY_0_1_NVME_CTEMP", BAY_0_1_NVME_CTEMP, read_nvme_temp, 0, {69, 0, 0, 10, 0, 0, 0, 0}, TEMP}, //0x34
  {"CC_SSD_BAY_0_2_NVME_CTEMP", BAY_0_2_NVME_CTEMP, read_nvme_temp, 0, {69, 0, 0, 10, 0, 0, 0, 0}, TEMP}, //0x35
  {"CC_SSD_BAY_0_3_NVME_CTEMP", BAY_0_3_NVME_CTEMP, read_nvme_temp, 0, {69, 0, 0, 10, 0, 0, 0, 0}, TEMP}, //0x36
  {"CC_SSD_BAY_0_4_NVME_CTEMP", BAY_0_4_NVME_CTEMP, read_nvme_temp, 0, {69, 0, 0, 10, 0, 0, 0, 0}, TEMP}, //0x37
  {"CC_SSD_BAY_0_5_NVME_CTEMP", BAY_0_5_NVME_CTEMP, read_nvme_temp, 0, {69, 0, 0, 10, 0, 0, 0, 0}, TEMP}, //0x38
  {"CC_SSD_BAY_0_6_NVME_CTEMP", BAY_0_6_NVME_CTEMP, read_nvme_temp, 0, {69, 0, 0, 10, 0, 0, 0, 0}, TEMP}, //0x39
  {"CC_SSD_BAY_0_7_NVME_CTEMP", BAY_0_7_NVME_CTEMP, read_nvme_temp, 0, {69, 0, 0, 10, 0, 0, 0, 0}, TEMP}, //0x3A
  {"CC_MB_BAY_0_VOLT", BAY_0_VOL , read_nvme_sensor, 0, {12.96, 0, 0, 10.56, 0, 0, 0, 0}, VOLT}, //0x3B
  {"CC_MB_BAY_0_IOUT", BAY_0_IOUT, read_nvme_sensor, 0, {0, 0, 0, 0, 0, 0, 0, 0}, CURR}, //0x3C
  {"CC_MB_BAY_0_POUT", BAY_0_POUT, read_nvme_sensor, 0, {0, 0, 0, 0, 0, 0, 0, 0}, POWER}, //0x3D
  {"CC_MB_VR_P1V8_PEX2_3_TEMP" , VR_P1V8_PEX2_3_TEMP , sensors_read_vr, 0, {115, 0, 0, 10, 0, 0, 0, 0}, TEMP}, //0x3E
  {"CC_MB_VR_P1V8_PEX2_3_VOUT" , VR_P1V8_PEX2_3_VOUT , sensors_read_vr, 0, {1.89, 0, 0, 1.71, 0, 0, 0}, VOLT}, //0x3F

  {"CC_SSD_BAY_1_FTEMP", BAY1_FTEMP_ID, read_bay_temp, true, {50, 0, 0, 10, 0, 0, 0, 0}, TEMP}, //0x40
  {"CC_SSD_BAY_1_RTEMP", BAY1_RTEMP_ID, read_bay_temp, true, {70, 0, 0, 10, 0, 0, 0, 0}, TEMP}, //0x41
  {"CC_SSD_BAY_1_NVME_CTEMP", BAY_1_NVME_CTEMP, read_nvme_temp, 0, {69, 0, 0, 10, 0, 0, 0, 0}, TEMP}, //0x42
  {"CC_SSD_BAY_1_0_NVME_CTEMP", BAY_1_0_NVME_CTEMP, read_nvme_temp, 0, {69, 0, 0, 10, 0, 0, 0, 0}, TEMP}, //0x43
  {"CC_SSD_BAY_1_1_NVME_CTEMP", BAY_1_1_NVME_CTEMP, read_nvme_temp, 0, {69, 0, 0, 10, 0, 0, 0, 0}, TEMP}, //0x44
  {"CC_SSD_BAY_1_2_NVME_CTEMP", BAY_1_2_NVME_CTEMP, read_nvme_temp, 0, {69, 0, 0, 10, 0, 0, 0, 0}, TEMP}, //0x45
  {"CC_SSD_BAY_1_3_NVME_CTEMP", BAY_1_3_NVME_CTEMP, read_nvme_temp, 0, {69, 0, 0, 10, 0, 0, 0, 0}, TEMP}, //0x46
  {"CC_SSD_BAY_1_4_NVME_CTEMP", BAY_1_4_NVME_CTEMP, read_nvme_temp, 0, {69, 0, 0, 10, 0, 0, 0, 0}, TEMP}, //0x47
  {"CC_SSD_BAY_1_5_NVME_CTEMP", BAY_1_5_NVME_CTEMP, read_nvme_temp, 0, {69, 0, 0, 10, 0, 0, 0, 0}, TEMP}, //0x48
  {"CC_SSD_BAY_1_6_NVME_CTEMP", BAY_1_6_NVME_CTEMP, read_nvme_temp, 0, {69, 0, 0, 10, 0, 0, 0, 0}, TEMP}, //0x49
  {"CC_SSD_BAY_1_7_NVME_CTEMP", BAY_1_7_NVME_CTEMP, read_nvme_temp, 0, {69, 0, 0, 10, 0, 0, 0, 0}, TEMP}, //0x4A
  {"CC_MB_BAY_1_VOLT", BAY_1_VOL , read_nvme_sensor, 0, {12.96, 0, 0, 10.56, 0, 0, 0, 0}, VOLT}, //0x4B
  {"CC_MB_BAY_1_IOUT", BAY_1_IOUT, read_nvme_sensor, 0, {0, 0, 0, 0, 0, 0, 0, 0}, CURR}, //0x4C
  {"CC_MB_BAY_1_POUT", BAY_1_POUT, read_nvme_sensor, 0, {0, 0, 0, 0, 0, 0, 0, 0}, POWER}, //0x4D
  {"CC_MB_VR_P1V8_PEX2_3_CURR" , VR_P1V8_PEX2_3_CURR , sensors_read_vr, 0, {0, 0, 0, 0, 0, 0, 0, 0}, CURR}, //0x4E
  {NULL, 0, NULL, 0, {0, 0, 0, 0, 0, 0, 0, 0}, 0}, //0x4F

  {"CC_PDB_HSC_TEMP", HSC_ID0, read_hsc_temp, true, {75, 0, 0, 10, 0, 0, 0, 0}, TEMP}, //0x50
  {"CC_PDB_HSC_VIN" , HSC_ID0, read_hsc_vin , true, {13.2, 0, 0, 10.8, 0, 0, 0, 0}, VOLT}, //0x51
  {"CC_PDB_HSC_IOUT", HSC_ID0, read_hsc_iout, true, {0, 0, 0, 0, 0, 0, 0, 0}, CURR}, //0x52
  {"CC_PDB_HSC_PIN" , HSC_ID0, read_hsc_pin , true, {0, 0, 0, 0, 0, 0, 0, 0}, POWER}, //0x53
  {"CC_BB_P12V_AUX" , ADC0, read_adc_value, true, {13.2, 0, 0, 10.8, 0, 0, 0, 0}    , VOLT}, //0x54
  {"CC_BB_P3V3_STBY", ADC1, read_adc_value, true, {3.47, 0, 0, 3.13, 0, 0, 0, 0}  , VOLT}, //0x55
  {"CC_BB_P5V_STBY" , ADC2, read_adc_value, true, {5.25, 0, 0, 4.75, 0, 0, 0, 0}    , VOLT}, //0x56
  {"CC_BB_P3V3"     , ADC3, read_adc_value, 0, {3.47, 0, 0, 3.13, 0, 0, 0, 0}  , VOLT}, //0x57
  {"CC_BB_P3V3_PAX" , ADC4, read_adc_value, 0, {3.47, 0, 0, 3.13, 0, 0, 0, 0}  , VOLT}, //0x58
  {"CC_BB_P3V_BAT"  , ADC5, read_bat_value, true, {3.4, 0, 0, 2.85, 0, 0, 0, 0}    , VOLT}, //0x59
  {"CC_BB_P2V5_AUX" , ADC6, read_adc_value, true, {2.63, 0, 0, 2.37, 0, 0, 0, 0}  , VOLT}, //0x5A
  {"CC_BB_P1V2_AUX" , ADC7, read_adc_value, true, {1.26, 0, 0, 1.14, 0, 0, 0, 0}    , VOLT}, //0x5B
  {"CC_BB_P1V15_AUX", ADC8, read_adc_value, true, {1.21, 0, 0, 1.09, 0, 0, 0, 0}, VOLT}, //0x5C
  {"CC_PDB_HSC_VOUT", HSC_ID0, read_hsc_vout, true, {13.4, 0, 0, 11.4, 0, 0, 0, 0}, VOLT}, //0x5D
  {"CC_BB_PEX_3_TEMP", PAX_ID3, read_pax_therm, 0, {95, 0, 0, 10, 0, 0, 0, 0}, TEMP}, //0x5E
  {NULL, 0, NULL, 0, {0, 0, 0, 0, 0, 0, 0, 0}, 0}, //0x5F

  {"CC_P0V8_VDD0_TEMP", MB_VR_P0V8_VDD0_TEMP , sensors_read_vr, 0, {115, 0, 0, 10, 0, 0, 0, 0}, TEMP}, //0x60
  {"CC_P0V8_VDD0_VOLT" , MB_VR_P0V8_VDD0_VOUT , sensors_read_vr, 0, {0.87, 0, 0, 0.81, 0, 0, 0}, VOLT}, //0x61
  {"CC_P0V8_VDD0_IOUT", MB_VR_P0V8_VDD0_CURR , sensors_read_vr, 0, {0, 0, 0, 0, 0, 0, 0, 0}, CURR}, //0x62
  {"CC_P0V8_VDD0_POUT", MB_VR_P0V8_VDD0_POWER, sensors_read_vr, 0, {0, 0, 0, 0, 0, 0, 0, 0}, POWER}, //0x63
  {"CC_P0V8_AVD_PCIE0_TEMP", MB_VR_P1V0_AVD0_TEMP , sensors_read_vr, 0, {115, 0, 0, 10, 0, 0, 0, 0}, TEMP}, //0x64
  {"CC_P0V8_AVD_PCIE0_VOLT", MB_VR_P1V0_AVD0_VOUT , sensors_read_vr, 0, {0.87, 0, 0, 0.81, 0, 0, 0, 0}, VOLT}, //0x65
  {"CC_P0V8_AVD_PCIE0_IOUT", MB_VR_P1V0_AVD0_CURR , sensors_read_vr, 0, {0, 0, 0, 0, 0, 0, 0, 0}, CURR}, //0x66
  {"CC_P0V8_AVD_PCIE0_POUT", MB_VR_P1V0_AVD0_POWER, sensors_read_vr, 0, {0, 0, 0, 0, 0, 0, 0, 0}, POWER}, //0x67
  {"CC_P0V8_VDD1_TEMP", MB_VR_P0V8_VDD1_TEMP , sensors_read_vr, 0, {115, 0, 0, 10, 0, 0, 0, 0}, TEMP}, //0x68
  {"CC_P0V8_VDD1_VOLT", MB_VR_P0V8_VDD1_VOUT , sensors_read_vr, 0, {0.87, 0, 0, 0.81, 0, 0, 0, 0}, VOLT}, //0x69
  {"CC_P0V8_VDD1_IOUT", MB_VR_P0V8_VDD1_CURR , sensors_read_vr, 0, {0, 0, 0, 0, 0, 0, 0, 0}, CURR}, //0x6A
  {"CC_P0V8_VDD1_POUT", MB_VR_P0V8_VDD1_POWER, sensors_read_vr, 0, {0, 0, 0, 0, 0, 0, 0, 0}, POWER}, //0x6B
  {"CC_P0V8_AVD_PCIE1_TEMP", MB_VR_P1V0_AVD1_TEMP , sensors_read_vr, 0, {115, 0, 0, 10, 0, 0, 0, 0}, TEMP}, //0x6C
  {"CC_P0V8_AVD_PCIE1_VOLT" , MB_VR_P1V0_AVD1_VOUT , sensors_read_vr, 0, {0.87, 0, 0, 0.81, 0, 0, 0, 0}, VOLT}, //0x6D
  {"CC_P0V8_AVD_PCIE1_IOUT", MB_VR_P1V0_AVD1_CURR , sensors_read_vr, 0, {0, 0, 0, 0, 0, 0, 0, 0}, CURR}, //0x6E
  {"CC_P0V8_AVD_PCIE1_POUT", MB_VR_P1V0_AVD1_POWER, sensors_read_vr, 0, {0, 0, 0, 0, 0, 0, 0, 0}, POWER}, //0x6F

  {"CC_P0V8_VDD2_TEMP", MB_VR_P0V8_VDD2_TEMP, sensors_read_vr, 0, {115, 0, 0, 10, 0, 0, 0, 0}, TEMP}, //0x70
  {"CC_P0V8_VDD2_VOLT" , MB_VR_P0V8_VDD2_VOUT, sensors_read_vr, 0, {0.87, 0, 0, 0.81, 0, 0, 0, 0}, VOLT}, //0x71
  {"CC_P0V8_VDD2_IOUT", MB_VR_P0V8_VDD2_CURR, sensors_read_vr, 0, {0, 0, 0, 0, 0, 0, 0, 0}, CURR}, //0x72
  {"CC_P0V8_VDD2_POUT", MB_VR_P0V8_VDD2_POWER, sensors_read_vr, 0, {0, 0, 0, 0, 0, 0, 0, 0}, POWER}, //0x73
  {"CC_P0V8_AVD_PCIE2_TEMP", MB_VR_P1V0_AVD2_TEMP, sensors_read_vr, 0, {115, 0, 0, 10, 0, 0, 0, 0}, TEMP}, //0x74
  {"CC_P0V8_AVD_PCIE2_VOLT", MB_VR_P1V0_AVD2_VOUT, sensors_read_vr, 0, {0.87, 0, 0, 0.81, 0, 0, 0, 0}, VOLT}, //0x75
  {"CC_P0V8_AVD_PCIE2_IOUT", MB_VR_P1V0_AVD2_CURR, sensors_read_vr, 0, {0, 0, 0, 0, 0, 0, 0, 0}, CURR}, //0x76
  {"CC_P0V8_AVD_PCIE2_POUT", MB_VR_P1V0_AVD2_POWER, sensors_read_vr, 0, {0, 0, 0, 0, 0, 0, 0, 0}, POWER}, //0x77
  {"CC_P0V8_VDD3_TEMP", MB_VR_P0V8_VDD3_TEMP, sensors_read_vr, 0, {115, 0, 0, 10, 0, 0, 0, 0}, TEMP}, //0x78
  {"CC_P0V8_VDD3_VOLT", MB_VR_P0V8_VDD3_VOUT, sensors_read_vr, 0, {0.87, 0, 0, 0.81, 0, 0, 0, 0}, VOLT}, //0x79
  {"CC_P0V8_VDD3_IOUT", MB_VR_P0V8_VDD3_CURR, sensors_read_vr, 0, {0, 0, 0, 0, 0, 0, 0, 0}, CURR}, //0x7A
  {"CC_P0V8_VDD3_POUT", MB_VR_P0V8_VDD3_POWER, sensors_read_vr, 0, {0, 0, 0, 0, 0, 0, 0, 0}, POWER}, //0x7B
  {"CC_P0V8_AVD_PCIE3_TEMP", MB_VR_P1V0_AVD3_TEMP, sensors_read_vr, 0, {115, 0, 0, 10, 0, 0, 0, 0}, TEMP}, //0x7C
  {"CC_P0V8_AVD_PCIE3_VOLT", MB_VR_P1V0_AVD3_VOUT, sensors_read_vr, 0, {0.87, 0, 0, 0.81, 0, 0, 0, 0}, VOLT}, //0x7D
  {"CC_P0V8_AVD_PCIE3_IOUT", MB_VR_P1V0_AVD3_CURR, sensors_read_vr, 0, {0, 0, 0, 0, 0, 0, 0, 0}, CURR}, //0x7E
  {"CC_P0V8_AVD_PCIE3_POUT", MB_VR_P1V0_AVD3_POWER, sensors_read_vr, 0, {0, 0, 0, 0, 0, 0, 0, 0}, POWER}, //0x7F

  {"CC_BB_PAX_0_TEMP", PAX_ID0, read_pax_therm, 0, {95, 0, 0, 10, 0, 0, 0, 0}, TEMP}, //0x80
  {"CC_BB_PAX_1_TEMP", PAX_ID1, read_pax_therm, 0, {95, 0, 0, 10, 0, 0, 0, 0}, TEMP}, //0x81
  {"CC_BB_PAX_2_TEMP", PAX_ID2, read_pax_therm, 0, {95, 0, 0, 10, 0, 0, 0, 0}, TEMP}, //0x82
  {"CC_BB_PAX_3_TEMP", PAX_ID3, read_pax_therm, 0, {95, 0, 0, 10, 0, 0, 0, 0}, TEMP}, //0x83
  {"CC_FAN_0_VOLT" , FAN_ID0, read_fan_volt, true, {13.2, 0, 0, 10.8, 0, 0, 0, 0}, VOLT}, //0x84
  {"CC_FAN_0_CURR", FAN_ID0, read_fan_curr, true, {0, 0, 0, 0, 0, 0, 0, 0}, CURR}, //0x85
  {"CC_FAN_0_PWR" , FAN_ID0, read_fan_pwr , true, {0, 0, 0, 0, 0, 0, 0, 0}, POWER}, //0x86
  {"CC_FAN_1_VOLT" , FAN_ID1, read_fan_volt, true, {13.2, 0, 0, 10.8, 0, 0, 0, 0}, VOLT}, //0x87
  {"CC_FAN_1_CURR", FAN_ID1, read_fan_curr, true, {0, 0, 0, 0, 0, 0, 0, 0}, CURR}, //0x88
  {"CC_FAN_1_PWR" , FAN_ID1, read_fan_pwr , true, {0, 0, 0, 0, 0, 0, 0, 0}, POWER}, //0x89
  {"CC_FAN_2_VOLT" , FAN_ID2, read_fan_volt, true, {13.2, 0, 0, 10.8, 0, 0, 0, 0}, VOLT}, //0x8A
  {"CC_FAN_2_CURR", FAN_ID2, read_fan_curr, true, {0, 0, 0, 0, 0, 0, 0, 0}, CURR}, //0x8B
  {"CC_FAN_2_PWR" , FAN_ID2, read_fan_pwr , true, {0, 0, 0, 0, 0, 0, 0, 0}, POWER}, //0x8C
  {"CC_FAN_3_VOLT" , FAN_ID3, read_fan_volt, true, {13.2, 0, 0, 10.8, 0, 0, 0, 0}, VOLT}, //0x8D
  {"CC_FAN_3_CURR", FAN_ID3, read_fan_curr, true, {0, 0, 0, 0, 0, 0, 0, 0}, CURR}, //0x8E
  {"CC_FAN_3_PWR" , FAN_ID3, read_fan_pwr , true, {0, 0, 0, 0, 0, 0, 0, 0}, POWER}, //0x8F

  {"CC_SYSTEM_INLET_TEMP"       , SYS_TEMP       , read_inlet_sensor, true, {50, 0, 0, 10, 0, 0, 0, 0}, TEMP}, //0x90
  {"CC_SYSTEM_INLET_REMOTE_TEMP", SYS_REMOTE_TEMP, read_inlet_sensor, true, {50, 0, 0, 10, 0, 0, 0, 0}, TEMP}, //0x91
  {"CC_BB_INLET_TEMP_L"       , INLET_TEMP_L       , read_inlet_sensor, true, {65, 0, 0, 10, 0, 0, 0, 0}, TEMP}, //0x92
  {"CC_BB_INLET_REMOTE_TEMP_L", INLET_REMOTE_TEMP_L, read_inlet_sensor, true, {65, 0, 0, 10, 0, 0, 0, 0}, TEMP}, //0x93
  {"CC_BB_INLET_TEMP_R"       , INLET_TEMP_R       , read_inlet_sensor, true, {65, 0, 0, 10, 0, 0, 0, 0}, TEMP}, //0x94
  {"CC_BB_INLET_REMOTE_TEMP_R", INLET_REMOTE_TEMP_R, read_inlet_sensor, true, {65, 0, 0, 10, 0, 0, 0, 0}, TEMP}, //0x95
  {"CC_PDB_INLET_TEMP_L"       , PDB_INLET_TEMP_L_ID       , read_inlet_sensor, true, {65, 0, 0, 10, 0, 0, 0, 0}, TEMP}, //0x96
  {"CC_PDB_INLET_REMOTE_TEMP_L", PDB_INLET_REMOTE_TEMP_L_ID, read_inlet_sensor, true, {65, 0, 0, 10, 0, 0, 0, 0}, TEMP}, //0x97
  {"CC_PDB_INLET_TEMP_R"       , PDB_INLET_TEMP_R_ID       , read_inlet_sensor, true, {65, 0, 0, 10, 0, 0, 0, 0}, TEMP}, //0x98
  {"CC_PDB_INLET_REMOTE_TEMP_R", PDB_INLET_REMOTE_TEMP_R_ID, read_inlet_sensor, true, {65, 0, 0, 10, 0, 0, 0, 0}, TEMP}, //0x99
  {"CC_BAY0_INA219_VOLT", BAY0_INA219_VOLT, read_ina219_sensor, true, {0, 0, 0, 0, 0, 0, 0, 0}, VOLT},  //0x9A
  {"CC_BAY0_INA219_CURR", BAY0_INA219_CURR, read_ina219_sensor, true, {0, 0, 0, 0, 0, 0, 0, 0}, CURR},  //0x9B
  {"CC_BAY0_INA219_PWR" , BAY0_INA219_PWR , read_ina219_sensor, true, {0, 0, 0, 0, 0, 0, 0, 0}, POWER}, //0x9C
  {"CC_BAY1_INA219_VOLT", BAY1_INA219_VOLT, read_ina219_sensor, true, {0, 0, 0, 0, 0, 0, 0, 0}, VOLT},  //0x9D
  {"CC_BAY1_INA219_CURR", BAY1_INA219_CURR, read_ina219_sensor, true, {0, 0, 0, 0, 0, 0, 0, 0}, CURR},  //0x9E
  {"CC_BAY1_INA219_PWR" , BAY1_INA219_PWR , read_ina219_sensor, true, {0, 0, 0, 0, 0, 0, 0, 0}, POWER}, //0x9F

  {"CC_PDB_FAN0_TACH_I", PDB_FAN0_TACH_I, sensors_read_fan_speed, true, {13500, 0, 0, 600, 0, 0, 0, 0}, FAN}, //0xA0
  {"CC_PDB_FAN0_TACH_O", PDB_FAN0_TACH_O, sensors_read_fan_speed, true, {12500, 0, 0, 600, 0, 0, 0, 0}, FAN}, //0xA1
  {"CC_PDB_FAN1_TACH_I", PDB_FAN1_TACH_I, sensors_read_fan_speed, true, {13500, 0, 0, 600, 0, 0, 0, 0}, FAN}, //0xA2
  {"CC_PDB_FAN1_TACH_O", PDB_FAN1_TACH_O, sensors_read_fan_speed, true, {12500, 0, 0, 600, 0, 0, 0, 0}, FAN}, //0xA3
  {"CC_PDB_FAN2_TACH_I", PDB_FAN2_TACH_I, sensors_read_fan_speed, true, {13500, 0, 0, 600, 0, 0, 0, 0}, FAN}, //0xA4
  {"CC_PDB_FAN2_TACH_O", PDB_FAN2_TACH_O, sensors_read_fan_speed, true, {12500, 0, 0, 600, 0, 0, 0, 0}, FAN}, //0xA5
  {"CC_PDB_FAN3_TACH_I", PDB_FAN3_TACH_I, sensors_read_fan_speed, true, {13500, 0, 0, 600, 0, 0, 0, 0}, FAN}, //0xA6
  {"CC_PDB_FAN3_TACH_O", PDB_FAN3_TACH_O, sensors_read_fan_speed, true, {12500, 0, 0, 600, 0, 0, 0, 0}, FAN}, //0xA7
  {"CC_FAN_0_PWM", PDB_FAN0_PWM, sensors_read_fan_pwm, true, {0, 0, 0, 0, 0, 0, 0, 0}, PWM}, //0xA8
  {"CC_FAN_1_PWM", PDB_FAN0_PWM, sensors_read_fan_pwm, true, {0, 0, 0, 0, 0, 0, 0, 0}, PWM}, //0xA9
  {"CC_FAN_2_PWM", PDB_FAN0_PWM, sensors_read_fan_pwm, true, {0, 0, 0, 0, 0, 0, 0, 0}, PWM}, //0xAA
  {"CC_FAN_3_PWM", PDB_FAN0_PWM, sensors_read_fan_pwm, true, {0, 0, 0, 0, 0, 0, 0, 0}, PWM}, //0xAB
  {"CC_BB_PEX_0_TEMP", PAX_ID0, read_pax_therm, 0, {95, 0, 0, 10, 0, 0, 0, 0}, TEMP}, //0xAC
  {"CC_BB_PEX_1_TEMP", PAX_ID1, read_pax_therm, 0, {95, 0, 0, 10, 0, 0, 0, 0}, TEMP}, //0xAD
  {"CC_BB_PEX_2_TEMP", PAX_ID2, read_pax_therm, 0, {95, 0, 0, 10, 0, 0, 0, 0}, TEMP}, //0xAE
  {"CC_E1S_1_NVME_7_PWR" , E1S_1_7_NVME_PWR , read_ina260_sensor, 0, {0, 0, 0, 0, 0, 0, 0, 0}, POWER}, //0xAF

  {"CC_M2_0_NVME_0_VOLT", NVME_ID0, read_bay0_ssd_volt, 0, {3.47, 0, 0, 3.13, 0, 0, 0, 0}, VOLT}, //0xB0
  {"CC_M2_0_NVME_0_CURR", NVME_ID0, read_bay0_ssd_curr, 0, {0, 0, 0, 0, 0, 0, 0, 0}, CURR}, //0xB1
  {"CC_M2_0_NVME_1_VOLT", NVME_ID1, read_bay0_ssd_volt, 0, {3.47, 0, 0, 3.13, 0, 0, 0, 0}, VOLT}, //0xB2
  {"CC_M2_0_NVME_1_CURR", NVME_ID1, read_bay0_ssd_curr, 0, {0, 0, 0, 0, 0, 0, 0, 0}, CURR}, //0xB3
  {"CC_M2_0_NVME_2_VOLT", NVME_ID2, read_bay0_ssd_volt, 0, {3.47, 0, 0, 3.13, 0, 0, 0, 0}, VOLT}, //0xB4
  {"CC_M2_0_NVME_2_CURR", NVME_ID2, read_bay0_ssd_curr, 0, {0, 0, 0, 0, 0, 0, 0, 0}, CURR}, //0xB5
  {"CC_M2_0_NVME_3_VOLT", NVME_ID3, read_bay0_ssd_volt, 0, {3.47, 0, 0, 3.13, 0, 0, 0, 0}, VOLT}, //0xB6
  {"CC_M2_0_NVME_3_CURR", NVME_ID3, read_bay0_ssd_curr, 0, {0, 0, 0, 0, 0, 0, 0, 0}, CURR}, //0xB7
  {"CC_M2_0_NVME_4_VOLT", NVME_ID4, read_bay0_ssd_volt, 0, {3.47, 0, 0, 3.13, 0, 0, 0, 0}, VOLT}, //0xB8
  {"CC_M2_0_NVME_4_CURR", NVME_ID4, read_bay0_ssd_curr, 0, {0, 0, 0, 0, 0, 0, 0, 0}, CURR}, //0xB9
  {"CC_M2_0_NVME_5_VOLT", NVME_ID5, read_bay0_ssd_volt, 0, {3.47, 0, 0, 3.13, 0, 0, 0, 0}, VOLT}, //0xBA
  {"CC_M2_0_NVME_5_CURR", NVME_ID5, read_bay0_ssd_curr, 0, {0, 0, 0, 0, 0, 0, 0, 0}, CURR}, //0xBB
  {"CC_M2_0_NVME_6_VOLT", NVME_ID6, read_bay0_ssd_volt, 0, {3.47, 0, 0, 3.13, 0, 0, 0, 0}, VOLT}, //0xBC
  {"CC_M2_0_NVME_6_CURR", NVME_ID6, read_bay0_ssd_curr, 0, {0, 0, 0, 0, 0, 0, 0, 0}, CURR}, //0xBD
  {"CC_M2_0_NVME_7_VOLT", NVME_ID7, read_bay0_ssd_volt, 0, {3.47, 0, 0, 3.13, 0, 0, 0, 0}, VOLT}, //0xBE
  {"CC_M2_0_NVME_7_CURR", NVME_ID7, read_bay0_ssd_curr, 0, {0, 0, 0, 0, 0, 0, 0, 0}, CURR}, //0xBF

  {"CC_M2_1_NVME_0_VOLT", NVME_ID0, read_bay1_ssd_volt, 0, {3.47, 0, 0, 3.13, 0, 0, 0, 0}, VOLT}, //0xC0
  {"CC_M2_1_NVME_0_CURR", NVME_ID0, read_bay1_ssd_curr, 0, {0, 0, 0, 0, 0, 0, 0, 0}, CURR}, //0xC1
  {"CC_M2_1_NVME_1_VOLT", NVME_ID1, read_bay1_ssd_volt, 0, {3.47, 0, 0, 3.13, 0, 0, 0, 0}, VOLT}, //0xC2
  {"CC_M2_1_NVME_1_CURR", NVME_ID1, read_bay1_ssd_curr, 0, {0, 0, 0, 0, 0, 0, 0, 0}, CURR}, //0xC3
  {"CC_M2_1_NVME_2_VOLT", NVME_ID2, read_bay1_ssd_volt, 0, {3.47, 0, 0, 3.13, 0, 0, 0, 0}, VOLT}, //0xC4
  {"CC_M2_1_NVME_2_CURR", NVME_ID2, read_bay1_ssd_curr, 0, {0, 0, 0, 0, 0, 0, 0, 0}, CURR}, //0xC5
  {"CC_M2_1_NVME_3_VOLT", NVME_ID3, read_bay1_ssd_volt, 0, {3.47, 0, 0, 3.13, 0, 0, 0, 0}, VOLT}, //0xC6
  {"CC_M2_1_NVME_3_CURR", NVME_ID3, read_bay1_ssd_curr, 0, {0, 0, 0, 0, 0, 0, 0, 0}, CURR}, //0xC7
  {"CC_M2_1_NVME_4_VOLT", NVME_ID4, read_bay1_ssd_volt, 0, {3.47, 0, 0, 3.13, 0, 0, 0, 0}, VOLT}, //0xC8
  {"CC_M2_1_NVME_4_CURR", NVME_ID4, read_bay1_ssd_curr, 0, {0, 0, 0, 0, 0, 0, 0, 0}, CURR}, //0xC9
  {"CC_M2_1_NVME_5_VOLT", NVME_ID5, read_bay1_ssd_volt, 0, {3.47, 0, 0, 3.13, 0, 0, 0, 0}, VOLT}, //0xCA
  {"CC_M2_1_NVME_5_CURR", NVME_ID5, read_bay1_ssd_curr, 0, {0, 0, 0, 0, 0, 0, 0, 0}, CURR}, //0xCB
  {"CC_M2_1_NVME_6_VOLT", NVME_ID6, read_bay1_ssd_volt, 0, {3.47, 0, 0, 3.13, 0, 0, 0, 0}, VOLT}, //0xCC
  {"CC_M2_1_NVME_6_CURR", NVME_ID6, read_bay1_ssd_curr, 0, {0, 0, 0, 0, 0, 0, 0, 0}, CURR}, //0xCD
  {"CC_M2_1_NVME_7_VOLT", NVME_ID7, read_bay1_ssd_volt, 0, {3.47, 0, 0, 3.13, 0, 0, 0, 0}, VOLT}, //0xCE
  {"CC_M2_1_NVME_7_CURR", NVME_ID7, read_bay1_ssd_curr, 0, {0, 0, 0, 0, 0, 0, 0, 0}, CURR}, //0xCF

  {"CC_E1S_0_NVME_0_VOLT", E1S_0_0_NVME_VOLT, read_ina260_sensor, 0, {13.2, 0, 0, 10.8, 0, 0, 0, 0}, VOLT},  //0xD0
  {"CC_E1S_0_NVME_0_CURR", E1S_0_0_NVME_CURR, read_ina260_sensor, 0, {0, 0, 0, 0, 0, 0, 0, 0}, CURR},  //0xD1
  {"CC_E1S_0_NVME_0_PWR" , E1S_0_0_NVME_PWR , read_ina260_sensor, 0, {0, 0, 0, 0, 0, 0, 0, 0}, POWER}, //0xD2
  {"CC_E1S_0_NVME_1_VOLT", E1S_0_1_NVME_VOLT, read_ina260_sensor, 0, {13.2, 0, 0, 10.8, 0, 0, 0, 0}, VOLT},  //0xD3
  {"CC_E1S_0_NVME_1_CURR", E1S_0_1_NVME_CURR, read_ina260_sensor, 0, {0, 0, 0, 0, 0, 0, 0, 0}, CURR},  //0xD4
  {"CC_E1S_0_NVME_1_PWR" , E1S_0_1_NVME_PWR , read_ina260_sensor, 0, {0, 0, 0, 0, 0, 0, 0, 0}, POWER}, //0xD5
  {"CC_E1S_0_NVME_2_VOLT", E1S_0_2_NVME_VOLT, read_ina260_sensor, 0, {13.2, 0, 0, 10.8, 0, 0, 0, 0}, VOLT},  //0xD6
  {"CC_E1S_0_NVME_2_CURR", E1S_0_2_NVME_CURR, read_ina260_sensor, 0, {0, 0, 0, 0, 0, 0, 0, 0}, CURR},  //0xD7
  {"CC_E1S_0_NVME_2_PWR" , E1S_0_2_NVME_PWR , read_ina260_sensor, 0, {0, 0, 0, 0, 0, 0, 0, 0}, POWER}, //0xD8
  {"CC_E1S_0_NVME_3_VOLT", E1S_0_3_NVME_VOLT, read_ina260_sensor, 0, {13.2, 0, 0, 10.8, 0, 0, 0, 0}, VOLT},  //0xD9
  {"CC_E1S_0_NVME_3_CURR", E1S_0_3_NVME_CURR, read_ina260_sensor, 0, {0, 0, 0, 0, 0, 0, 0, 0}, CURR},  //0xDA
  {"CC_E1S_0_NVME_3_PWR" , E1S_0_3_NVME_PWR , read_ina260_sensor, 0, {0, 0, 0, 0, 0, 0, 0, 0}, POWER}, //0xDB
  {"CC_E1S_0_NVME_4_VOLT", E1S_0_4_NVME_VOLT, read_ina260_sensor, 0, {13.2, 0, 0, 10.8, 0, 0, 0, 0}, VOLT},  //0xDC
  {"CC_E1S_0_NVME_4_CURR", E1S_0_4_NVME_CURR, read_ina260_sensor, 0, {0, 0, 0, 0, 0, 0, 0, 0}, CURR},  //0xDD
  {"CC_E1S_0_NVME_4_PWR" , E1S_0_4_NVME_PWR , read_ina260_sensor, 0, {0, 0, 0, 0, 0, 0, 0, 0}, POWER}, //0xDE
  {"CC_E1S_0_NVME_5_VOLT", E1S_0_5_NVME_VOLT, read_ina260_sensor, 0, {13.2, 0, 0, 10.8, 0, 0, 0, 0}, VOLT},  //0xDF

  {"CC_E1S_0_NVME_5_CURR", E1S_0_5_NVME_CURR, read_ina260_sensor, 0, {0, 0, 0, 0, 0, 0, 0, 0}, CURR},  //0xE0
  {"CC_E1S_0_NVME_5_PWR" , E1S_0_5_NVME_PWR , read_ina260_sensor, 0, {0, 0, 0, 0, 0, 0, 0, 0}, POWER}, //0xE1
  {"CC_E1S_0_NVME_6_VOLT", E1S_0_6_NVME_VOLT, read_ina260_sensor, 0, {13.2, 0, 0, 10.8, 0, 0, 0, 0}, VOLT},  //0xE2
  {"CC_E1S_0_NVME_6_CURR", E1S_0_6_NVME_CURR, read_ina260_sensor, 0, {0, 0, 0, 0, 0, 0, 0, 0}, CURR},  //0xE3
  {"CC_E1S_0_NVME_6_PWR" , E1S_0_6_NVME_PWR , read_ina260_sensor, 0, {0, 0, 0, 0, 0, 0, 0, 0}, POWER}, //0xE4
  {"CC_E1S_0_NVME_7_VOLT", E1S_0_7_NVME_VOLT, read_ina260_sensor, 0, {13.2, 0, 0, 10.8, 0, 0, 0, 0}, VOLT},  //0xE5
  {"CC_E1S_0_NVME_7_CURR", E1S_0_7_NVME_CURR, read_ina260_sensor, 0, {0, 0, 0, 0, 0, 0, 0, 0}, CURR},  //0xE6
  {"CC_E1S_0_NVME_7_PWR" , E1S_0_7_NVME_PWR , read_ina260_sensor, 0, {0, 0, 0, 0, 0, 0, 0, 0}, POWER}, //0xE7
  {"CC_E1S_1_NVME_0_VOLT", E1S_1_0_NVME_VOLT, read_ina260_sensor, 0, {13.2, 0, 0, 10.8, 0, 0, 0, 0}, VOLT},  //0xE8
  {"CC_E1S_1_NVME_0_CURR", E1S_1_0_NVME_CURR, read_ina260_sensor, 0, {0, 0, 0, 0, 0, 0, 0, 0}, CURR},  //0xE9
  {"CC_E1S_1_NVME_0_PWR" , E1S_1_0_NVME_PWR , read_ina260_sensor, 0, {0, 0, 0, 0, 0, 0, 0, 0}, POWER}, //0xEA
  {"CC_E1S_1_NVME_1_VOLT", E1S_1_1_NVME_VOLT, read_ina260_sensor, 0, {13.2, 0, 0, 10.8, 0, 0, 0, 0}, VOLT},  //0xEB
  {"CC_E1S_1_NVME_1_CURR", E1S_1_1_NVME_CURR, read_ina260_sensor, 0, {0, 0, 0, 0, 0, 0, 0, 0}, CURR},  //0xEC
  {"CC_E1S_1_NVME_1_PWR" , E1S_1_1_NVME_PWR , read_ina260_sensor, 0, {0, 0, 0, 0, 0, 0, 0, 0}, POWER}, //0xED
  {"CC_E1S_1_NVME_2_VOLT", E1S_1_2_NVME_VOLT, read_ina260_sensor, 0, {13.2, 0, 0, 10.8, 0, 0, 0, 0}, VOLT},  //0xEE
  {"CC_E1S_1_NVME_2_CURR", E1S_1_2_NVME_CURR, read_ina260_sensor, 0, {0, 0, 0, 0, 0, 0, 0, 0}, CURR},  //0xEF

  {"CC_E1S_1_NVME_2_PWR" , E1S_1_2_NVME_PWR , read_ina260_sensor, 0, {0, 0, 0, 0, 0, 0, 0, 0}, POWER}, //0xF0
  {"CC_E1S_1_NVME_3_VOLT", E1S_1_3_NVME_VOLT, read_ina260_sensor, 0, {13.2, 0, 0, 10.8, 0, 0, 0, 0}, VOLT},  //0xF1
  {"CC_E1S_1_NVME_3_CURR", E1S_1_3_NVME_CURR, read_ina260_sensor, 0, {0, 0, 0, 0, 0, 0, 0, 0}, CURR},  //0xF2
  {"CC_E1S_1_NVME_3_PWR" , E1S_1_3_NVME_PWR , read_ina260_sensor, 0, {0, 0, 0, 0, 0, 0, 0, 0}, POWER}, //0xF3
  {"CC_E1S_1_NVME_4_VOLT", E1S_1_4_NVME_VOLT, read_ina260_sensor, 0, {13.2, 0, 0, 10.8, 0, 0, 0, 0}, VOLT},  //0xF4
  {"CC_E1S_1_NVME_4_CURR", E1S_1_4_NVME_CURR, read_ina260_sensor, 0, {0, 0, 0, 0, 0, 0, 0, 0}, CURR},  //0xF5
  {"CC_E1S_1_NVME_4_PWR" , E1S_1_4_NVME_PWR , read_ina260_sensor, 0, {0, 0, 0, 0, 0, 0, 0, 0}, POWER}, //0xF6
  {"CC_E1S_1_NVME_5_VOLT", E1S_1_5_NVME_VOLT, read_ina260_sensor, 0, {13.2, 0, 0, 10.8, 0, 0, 0, 0}, VOLT},  //0xF7
  {"CC_E1S_1_NVME_5_CURR", E1S_1_5_NVME_CURR, read_ina260_sensor, 0, {0, 0, 0, 0, 0, 0, 0, 0}, CURR},  //0xF8
  {"CC_E1S_1_NVME_5_PWR" , E1S_1_5_NVME_PWR , read_ina260_sensor, 0, {0, 0, 0, 0, 0, 0, 0, 0}, POWER}, //0xF9
  {"CC_E1S_1_NVME_6_VOLT", E1S_1_6_NVME_VOLT, read_ina260_sensor, 0, {13.2, 0, 0, 10.8, 0, 0, 0, 0}, VOLT},  //0xFA
  {"CC_E1S_1_NVME_6_CURR", E1S_1_6_NVME_CURR, read_ina260_sensor, 0, {0, 0, 0, 0, 0, 0, 0, 0}, CURR},  //0xFB
  {"CC_E1S_1_NVME_6_PWR" , E1S_1_6_NVME_PWR , read_ina260_sensor, 0, {0, 0, 0, 0, 0, 0, 0, 0}, POWER}, //0xFC
  {"CC_E1S_1_NVME_7_VOLT", E1S_1_7_NVME_VOLT, read_ina260_sensor, 0, {13.2, 0, 0, 10.8, 0, 0, 0, 0}, VOLT},  //0xFD
  {"CC_E1S_1_NVME_7_CURR", E1S_1_7_NVME_CURR, read_ina260_sensor, 0, {0, 0, 0, 0, 0, 0, 0, 0}, CURR},  //0xFE
  {NULL, 0, NULL, 0, {0, 0, 0, 0, 0, 0, 0, 0}, 0} //0xFF
};

static int retry_skip_handle(uint8_t retry_curr, uint8_t retry_max) {

  if( retry_curr <= retry_max) {
    return READING_SKIP;
  }
  return 0;
}

size_t mb_sensor_cnt = sizeof(mb_sensor_list)/sizeof(uint8_t);
size_t mb_brcm_sensor_cnt = sizeof(mb_brcm_sensor_list)/sizeof(uint8_t);
size_t pdb_sensor_cnt = sizeof(pdb_sensor_list)/sizeof(uint8_t);
size_t m2_1_sensor_cnt = sizeof(m2_1_sensor_list)/sizeof(uint8_t);
size_t m2_2_sensor_cnt = sizeof(m2_2_sensor_list)/sizeof(uint8_t);
size_t e1s_1_sensor_cnt = sizeof(e1s_1_sensor_list)/sizeof(uint8_t);
size_t e1s_2_sensor_cnt = sizeof(e1s_2_sensor_list)/sizeof(uint8_t);
size_t nic0_sensor_cnt = sizeof(nic0_sensor_list)/sizeof(uint8_t);
size_t nic1_sensor_cnt = sizeof(nic1_sensor_list)/sizeof(uint8_t);
size_t nic2_sensor_cnt = sizeof(nic2_sensor_list)/sizeof(uint8_t);
size_t nic3_sensor_cnt = sizeof(nic3_sensor_list)/sizeof(uint8_t);
size_t nic4_sensor_cnt = sizeof(nic4_sensor_list)/sizeof(uint8_t);
size_t nic5_sensor_cnt = sizeof(nic5_sensor_list)/sizeof(uint8_t);
size_t nic6_sensor_cnt = sizeof(nic6_sensor_list)/sizeof(uint8_t);
size_t nic7_sensor_cnt = sizeof(nic7_sensor_list)/sizeof(uint8_t);

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

int pal_sensor_read_raw(uint8_t fru, uint8_t sensor_num, void *value)
{
  char key[MAX_KEY_LEN] = {0};
  char str[MAX_VALUE_LEN] = {0};
  char fru_name[32];
  int ret=0;
  static uint8_t retry[256] = {0};
  uint8_t id=0;
  bool server_off;

  if (g_board_id == 0xFF) {
    pal_get_platform_id(&g_board_id);
  }

  pal_get_fru_name(fru, fru_name);
  sprintf(key, "%s_sensor%d", fru_name, sensor_num);
  id = sensor_map[sensor_num].id;

  pal_get_platform_id(&g_board_id);

  switch(fru) {
    case FRU_MB:
    case FRU_PDB:
    case FRU_CARRIER1:
    case FRU_CARRIER2:
    case FRU_NIC0:
    case FRU_NIC1:
    case FRU_NIC2:
    case FRU_NIC3:
    case FRU_NIC4:
    case FRU_NIC5:
    case FRU_NIC6:
    case FRU_NIC7:
      server_off = pal_is_server_off();
      id = sensor_map[sensor_num].id;
      if (server_off) {
        if (sensor_map[sensor_num].stby_read == true) {
	      ret = sensor_map[sensor_num].read_sensor(id, (float*) value);
        } else {
          ret = READING_NA;
        }
      } else {
        if (check_pwron_time(1) ) {
          ret = sensor_map[sensor_num].read_sensor(id, (float*) value);
        } else {
          ret = READING_NA;
        }
      }
      break;
    default:
      return -1;
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

  if (ret) {
    if (ret == READING_NA || ret == -1) {
      strcpy(str, "NA");
    } else {
      return ret;
    }
  } else {
    switch(sensor_num){
      case PDB_HSC_IOUT:
        hsc_value_adjust(current_table, value);
       break;
      case PDB_HSC_PIN:
        hsc_value_adjust(power_table, value);
      break;
    }
    sprintf(str, "%.2f",*((float*)value));
  }

  if(ret == READING_SKIP) {
    return ret;
  }

  if(kv_set(key, str, 0, 0) < 0) {
    syslog(LOG_WARNING, "pal_sensor_read_raw: cache_set key = %s, str = %s failed.", key, str);
    return -1;
  } else {
    return ret;
  }

  return 0;
}

static int sensors_read_fan_pwm(uint8_t sensor_num, float *value) {
  int ret = 0;
  uint8_t pwm = 0;

  switch (sensor_num) {
    case PDB_FAN0_PWM:
      ret = pal_get_pwm_value(0, &pwm);
      break;
    case PDB_FAN1_PWM:
      ret = pal_get_pwm_value(2, &pwm);
      break;
    case PDB_FAN2_PWM:
      ret = pal_get_pwm_value(4, &pwm);
      break;
    case PDB_FAN3_PWM:
      ret = pal_get_pwm_value(6, &pwm);
      break;
    default:
      return ERR_SENSOR_NA;
  }
  *value = (float)pwm;
  return ret;
}

int
pal_get_sensor_name(uint8_t fru, uint8_t sensor_num, char *name) {
  switch(fru) {
  case FRU_MB:
  case FRU_PDB:
  case FRU_CARRIER1:
  case FRU_CARRIER2:
  case FRU_NIC0:
  case FRU_NIC1:
  case FRU_NIC2:
  case FRU_NIC3:
  case FRU_NIC4:
  case FRU_NIC5:
  case FRU_NIC6:
  case FRU_NIC7:
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
  case FRU_PDB:
  case FRU_CARRIER1:
  case FRU_CARRIER2:
  case FRU_NIC0:
  case FRU_NIC1:
  case FRU_NIC2:
  case FRU_NIC3:
  case FRU_NIC4:
  case FRU_NIC5:
  case FRU_NIC6:
  case FRU_NIC7:
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
    case FRU_PDB:
    case FRU_CARRIER1:
    case FRU_CARRIER2:
    case FRU_NIC0:
    case FRU_NIC1:
    case FRU_NIC2:
    case FRU_NIC3:
    case FRU_NIC4:
    case FRU_NIC5:
    case FRU_NIC6:
    case FRU_NIC7:
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
        case PWM:
          sprintf(units, "PCT");
        default:
          return -1;
      }
      break;
    default:
      return -1;
  }
  return 0;
}

int pal_get_fru_sensor_list(uint8_t fru, uint8_t **sensor_list, int *cnt)
{
  char key[MAX_VALUE_LEN] = {0};
  char value[MAX_VALUE_LEN] = {0};
  int ret = 0;

  *sensor_list = NULL;
  *cnt = 0;

  pal_get_platform_id(&g_board_id);

  if(fru == FRU_CARRIER1) {
    sprintf(key, "carrier_0");
    ret = kv_get(key, value, NULL, 0);
  } else if (fru == FRU_CARRIER2) {
    sprintf(key, "carrier_1");
    ret = kv_get(key, value, NULL, 0);
  }

  if(ret) {
    syslog(LOG_WARNING, "%s kv_get fail! ret = %d", __FUNCTION__, ret);
  }

  switch (fru) {
    case FRU_MB:
      if((g_board_id & 0x07) == 0x3) {
        *sensor_list = (uint8_t *) mb_brcm_sensor_list;
        *cnt = mb_brcm_sensor_cnt;
      }
      else {
        *sensor_list = (uint8_t *) mb_sensor_list;
        *cnt = mb_sensor_cnt;
      }
      break;
    case FRU_PDB:
      *sensor_list = (uint8_t *) pdb_sensor_list;
      *cnt = pdb_sensor_cnt;
      break;
    case FRU_CARRIER1:
      if(!strcmp(value, "m.2")) {
          *sensor_list = (uint8_t *) m2_1_sensor_list;
          *cnt = m2_1_sensor_cnt;
      } else if( (!strcmp(value, "e1.s")) ||  (!strcmp(value, "e1.s_v2")) ) {
          *sensor_list = (uint8_t *) e1s_1_sensor_list;
          *cnt = e1s_1_sensor_cnt;
      }
      break;
    case FRU_CARRIER2:
      if(!strcmp(value, "m.2")) {
          *sensor_list = (uint8_t *) m2_2_sensor_list;
          *cnt = m2_2_sensor_cnt;
      } else if( (!strcmp(value, "e1.s")) ||  (!strcmp(value, "e1.s_v2")) ) {
          *sensor_list = (uint8_t *) e1s_2_sensor_list;
          *cnt = e1s_2_sensor_cnt;
      }
      break;
    case FRU_BSM:
      if(!strcmp(value, "bsm")) {
          *sensor_list = NULL;
          *cnt = 0;
      }
      break;
    case FRU_FIO:
      if(!strcmp(value, "fio")) {
          *sensor_list = NULL;
          *cnt = 0;
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
    case FRU_NIC2:
      *sensor_list = (uint8_t *) nic2_sensor_list;
      *cnt = nic2_sensor_cnt;
      break;
    case FRU_NIC3:
      *sensor_list = (uint8_t *) nic3_sensor_list;
      *cnt = nic3_sensor_cnt;
      break;
    case FRU_NIC4:
      *sensor_list = (uint8_t *) nic4_sensor_list;
      *cnt = nic4_sensor_cnt;
      break;
    case FRU_NIC5:
      *sensor_list = (uint8_t *) nic5_sensor_list;
      *cnt = nic5_sensor_cnt;
      break;
    case FRU_NIC6:
      *sensor_list = (uint8_t *) nic6_sensor_list;
      *cnt = nic6_sensor_cnt;
      break;
    case FRU_NIC7:
      *sensor_list = (uint8_t *) nic7_sensor_list;
      *cnt = nic7_sensor_cnt;
      break;
    default:
      syslog(LOG_WARNING, "%s, get key fail", __FUNCTION__);
      return -1;
    }

  return 0;
}

int pal_set_fan_speed(uint8_t fan, uint8_t pwm)
{
  char label[32] = {0};

  if (fan >= pal_pwm_cnt ||
      snprintf(label, sizeof(label), "pwm%d", fan + 1) > sizeof(label)) {
    return -1;
  }
  return sensors_write_fan(label, (float)pwm);
}

int pal_get_fan_speed(uint8_t fan, int *rpm)
{
  char label[32] = {0};
  float value;
  int ret;

  if (fan >= pal_tach_cnt ||
      snprintf(label, sizeof(label), "fan%d", fan + 1) > sizeof(label)) {
    syslog(LOG_WARNING, "%s: invalid fan#:%d", __func__, fan);
    return -1;
  }
  ret = sensors_read_fan(label, &value);
  *rpm = ret < 0? 0: (int)value;
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

static int pal_set_fan_led(uint8_t led_ctrl)
{
  int fd = 0, ret = -1;
  char fn[32];
  uint8_t retry = 3, tlen, rlen, addr, bus;
  uint8_t tbuf[16] = {0};
  uint8_t rbuf[16] = {0};

  bus = I2C_BUS_5;
  addr = 0xEE;

  snprintf(fn, sizeof(fn), "/dev/i2c-%d", bus);
  fd = open(fn, O_RDWR);
  if (fd < 0) {
    goto err_exit;
  }

  tbuf[0] = 0x02;
  tbuf[1] = led_ctrl;
  tlen = 2;
  rlen = 1;

  while (ret < 0 && retry-- > 0 ) {
    ret = i2c_rdwr_msg_transfer(fd, addr, tbuf, tlen, rbuf, rlen);
  }

  if (ret < 0) {
    syslog(LOG_WARNING, "%s: ret < 0 = %d", __func__, ret);
    goto err_exit;
  }

  err_exit:
  if (fd > 0) {
    close(fd);
  }

  return 0;
}

static int sensors_read_fan_speed(uint8_t sensor_num, float *value)
{
  static uint8_t fan_status = 0xff, last_fan_status = 0;
  uint8_t led_ctrl = 0, fan_mask;
  int ret, fan_id;
  static int retry[8] = {0};
  *value = 0.0;

  fan_id = sensor_num - PDB_FAN0_TACH_I;
  if (is_fan_present(fan_id/2) == true) {
    return READING_NA;
  }

  switch (sensor_num) {
    case PDB_FAN0_TACH_I:
      ret = sensors_read_fan("fan1", (float *)value);
      break;
    case PDB_FAN0_TACH_O:
      ret = sensors_read_fan("fan2", (float *)value);
      break;
    case PDB_FAN1_TACH_I:
      ret = sensors_read_fan("fan3", (float *)value);
      break;
    case PDB_FAN1_TACH_O:
      ret = sensors_read_fan("fan4", (float *)value);
      break;
    case PDB_FAN2_TACH_I:
      ret = sensors_read_fan("fan5", (float *)value);
      break;
    case PDB_FAN2_TACH_O:
      ret = sensors_read_fan("fan6", (float *)value);
      break;
    case PDB_FAN3_TACH_I:
      ret = sensors_read_fan("fan7", (float *)value);
      break;
    case PDB_FAN3_TACH_O:
      ret = sensors_read_fan("fan8", (float *)value);
      break;
    default:
      return ERR_SENSOR_NA;
  }

  if (ret || *value >= sensor_map[sensor_num].snr_thresh.ucr_thresh ||
             *value <= sensor_map[sensor_num].snr_thresh.lcr_thresh ) {
    if (retry[fan_id] >= 2) {
      fan_status = CLEARBIT(fan_status, fan_id);
    }
    else {
      retry[fan_id] += 1;
    }
  }
  else {
    fan_status = SETBIT(fan_status, fan_id);
    retry[fan_id] = 0;
  }

  if (fan_status != last_fan_status) {
    for(int i=0; i < pal_tach_cnt; i=i+2) {
      fan_mask = GETBIT(fan_status, i) & GETBIT(fan_status, (i+1));
      led_ctrl = fan_mask ? SETBIT(led_ctrl, i/2) : CLEARBIT(led_ctrl, i/2);
    }
    led_ctrl |= ~led_ctrl << 4;
    pal_set_fan_led(led_ctrl);
    last_fan_status = fan_status;
  }

  return 0;
}

int pal_get_pwm_value(uint8_t fan, uint8_t *pwm)
{
  char label[32] = {0};
  float value;
  int ret;

  if (fan >= pal_tach_cnt ||
      snprintf(label, sizeof(label), "pwm%d", fan/2 + 1) > sizeof(label)) {
    syslog(LOG_WARNING, "%s: invalid fan#:%d", __func__, fan);
    return -1;
  }
  ret = sensors_read_fan(label, &value);
  if (ret == 0)
    *pwm = (int)value;
  return ret;
}

static int
read_inlet_sensor(uint8_t id, float *value) {
  struct {
    const char *chip;
    const char *label;
  } devs[] = {
    {"tmp421-i2c-6-4f", "INLET_TEMP_L"},
    {"tmp421-i2c-6-4f", "INLET_REMOTE_TEMP_L"},
    {"tmp421-i2c-6-4c", "INLET_TEMP_R"},
    {"tmp421-i2c-6-4c", "INLET_REMOTE_TEMP_R"},
    {"tmp421-i2c-8-4f", "SYSTEM_INLET_TEMP"},
    {"tmp421-i2c-8-4f", "SYSTEM_INLET_REMOTE_TEMP"},
    {"tmp421-i2c-5-4c", "PDB_INLET_TEMP_L_ID"},
    {"tmp421-i2c-5-4c", "PDB_INLET_REMOTE_TEMP_L_ID"},
    {"tmp421-i2c-5-4d", "PDB_INLET_TEMP_R_ID"},
    {"tmp421-i2c-5-4d", "PDB_INLET_REMOTE_TEMP_R_ID"},
  };
  if (id >= ARRAY_SIZE(devs)) {
    return -1;
  }

  return sensors_read(devs[id].chip, devs[id].label, value);
}

static bool
is_fan_present(uint8_t fan_id) {
  static uint8_t fan_prsnt, last_fan_prsnt;
  int fd = 0, ret = -1;
  char fn[32];
  bool value = false;
  uint8_t retry = 3, tlen, rlen, addr, bus;
  uint8_t tbuf[16] = {0};
  uint8_t rbuf[16] = {0};
  bus = 5;
  addr = 0xEE;

  snprintf(fn, sizeof(fn), "/dev/i2c-%d", bus);
  fd = open(fn, O_RDWR);
  if (fd < 0) {
    goto err_exit;
  }

  tbuf[0] = 0x01;
  tlen = 1;
  rlen = 1;

  while (ret < 0 && retry-- > 0 ) {
    ret = i2c_rdwr_msg_transfer(fd, addr, tbuf, tlen, rbuf, rlen);
  }

  if (ret < 0) {
    syslog(LOG_WARNING, "%s: ret < 0 = %d", __func__, ret);
    goto err_exit;
  }

  value = rbuf[0] & (0x1 << fan_id);
  fan_prsnt = rbuf[0] & 0x0F;

  if (fan_prsnt != last_fan_prsnt) {
    for (int i=0; i<4; i++) {
      if (GETBIT(fan_prsnt, i) != GETBIT(last_fan_prsnt, i)) {
        syslog(LOG_CRIT, "%s: fan%d present- FAN_%d_PRSNT_EXP_N\n",
           GETBIT(fan_prsnt, i)? "OFF": "ON", i , i);
      }
    }
    last_fan_prsnt = fan_prsnt;
  }

  err_exit:
  if (fd > 0) {
    close(fd);
  }
  return value;
}

bool pal_is_fan_prsnt(uint8_t fan_id)
{
  return !is_fan_present(fan_id/2);
}

static int
read_fan_volt(uint8_t fan_id, float *value) {
  struct {
    const char *chip;
    const char *label;
  } devs[] = {
    {"adc128d818-i2c-5-1d", "FAN0_VOLT"},
    {"adc128d818-i2c-5-1d", "FAN1_VOLT"},
    {"adc128d818-i2c-5-1d", "FAN2_VOLT"},
    {"adc128d818-i2c-5-1d", "FAN3_VOLT"},
  };
  int ret = 0;

  if (fan_id >= ARRAY_SIZE(devs)) {
    return -1;
  }
  if (is_fan_present(fan_id) == true) {
    return READING_NA;
  }
  ret = sensors_read(devs[fan_id].chip, devs[fan_id].label, value);

  fan_volt[fan_id] = *value;
  return ret;
}

static int
read_fan_curr(uint8_t fan_id, float *value) {
  struct {
    const char *chip;
    const char *label;
  } devs[] = {
    {"adc128d818-i2c-5-1d", "FAN0_CURR"},
    {"adc128d818-i2c-5-1d", "FAN1_CURR"},
    {"adc128d818-i2c-5-1d", "FAN2_CURR"},
    {"adc128d818-i2c-5-1d", "FAN3_CURR"},
  };
  int ret = 0;

  if (fan_id >= ARRAY_SIZE(devs)) {
    return -1;
  }
  if (is_fan_present(fan_id) == true) {
    return READING_NA;
  }
  ret = sensors_read(devs[fan_id].chip, devs[fan_id].label, value);

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

static int
read_adc_value(uint8_t adc_id, float *value) {
  const char *adc_label[] = {
    "MB_P12V_AUX",
    "MB_P3V3_STBY",
    "MB_P5V_STBY",
    "MB_P3V3",
    "MB_P3V3_PAX",
    "MB_P3V_BAT",
    "MB_P2V5_AUX",
    "MB_P1V2_AUX",
    "MB_P1V15_AUX",
  };
  if (adc_id >= ARRAY_SIZE(adc_label)) {
    return -1;
  }

  return sensors_read_adc(adc_label[adc_id], value);
}

static int
read_bat_value(uint8_t adc_id, float *value) {
  int ret = -1;
  gpio_desc_t *gp_batt = gpio_open_by_shadow(GPIO_BATTERY_DETECT);
  if (!gp_batt) {
    return -1;
  }
  if (gpio_set_value(gp_batt, GPIO_VALUE_HIGH)) {
    goto exit;
  }

#ifdef DEBUG
  syslog(LOG_DEBUG, "%s %s\n", __func__, path);
#endif
  msleep(10);

  ret = read_adc_value(adc_id, value);
  if (gpio_set_value(gp_batt, GPIO_VALUE_LOW)) {
    goto exit;
  }

exit:
  gpio_close(gp_batt);
  return ret;
}

static bool
is_nic_present(uint8_t nic_id) {
  gpio_desc_t *gdesc = NULL;
  gpio_value_t val;
  char gpio_name[32];

  sprintf(gpio_name, GPIO_NIC_PRSNT, nic_info_list[nic_id].gpio_netname);

  if ((gdesc = gpio_open_by_shadow(gpio_name))) {
    if (!gpio_get_value(gdesc, &val)) {
      gpio_close(gdesc);
      return val;
    }
    gpio_close(gdesc);
  }

  return 0;
}

static int
read_nic_temp(uint8_t nic_id, float *value) {
  int fd = 0, ret = -1;
  char fn[32];
  static uint8_t retry=0;
  uint8_t tbuf[16] = {0};
  uint8_t rbuf[16] = {0};
  uint8_t tlen, rlen, addr, bus;

  if (is_nic_present(nic_id) == true) {
    return READING_NA;
  }

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
get_hsc_reading(uint8_t hsc_id, uint8_t type, uint8_t cmd, float *value, uint8_t *raw_data) {
  const uint8_t adm1278_bus = 5;
  uint8_t addr = hsc_info_list[hsc_id].slv_addr;
  static int fd = -1;

  if ( fd < 0 ) {
    fd = i2c_cdev_slave_open(adm1278_bus, addr >> 1, I2C_SLAVE_FORCE_CLAIM);
    if ( fd < 0 ) {
      syslog(LOG_WARNING, "Failed to open bus %d", adm1278_bus);
      return READING_NA;
    }
  }

  uint8_t rbuf[2] = {0x00};
  uint8_t rlen = 2;
  int retry = MAX_SNR_READ_RETRY;
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

  float m = hsc_info_list[hsc_id].info[type].m;
  float b = hsc_info_list[hsc_id].info[type].b;
  float r = hsc_info_list[hsc_id].info[type].r;
  *value = ((float)(rbuf[1] << 8 | rbuf[0]) * r - b) / m;

#ifdef DEBUG
  syslog(LOG_WARNING, "cmd %d, rbuf[0] = %x, rbuf[1] %x, value = %f", cmd, rbuf[0], rbuf[1], *value);
#endif

  return PAL_EOK;
}

static int
read_hsc_temp(uint8_t hsc_id, float *value) {
  if ( get_hsc_reading(hsc_id, HSC_TEMP, PMBUS_READ_TEMP1, value, NULL) < 0 ) return READING_NA;
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
read_hsc_vin(uint8_t hsc_id, float *value) {
  if ( get_hsc_reading(hsc_id, HSC_VOLTAGE, PMBUS_READ_VIN, value, NULL) < 0 ) return READING_NA;
  return PAL_EOK;
}

static int
read_hsc_vout(uint8_t hsc_id, float *value) {
  if (get_hsc_reading(hsc_id, HSC_VOLTAGE, PMBUS_READ_VOUT, value, NULL) < 0 ) return READING_NA;
  return PAL_EOK;
}

static void
hsc_value_adjust(struct calibration_table *table, float *value) {
  float x0, x1, y0, y1, x;
  int i;
  x = *value;
  x0 = table[0].ein;
  y0 = table[0].coeff;
  if (x0 > *value) {
    *value = x * y0;
    return;
  }

  for (i = 0; table[i].ein > 0.0; i++) {
    if (*value < table[i].ein)
      break;
    x0 = table[i].ein;
    y0 = table[i].coeff;
  }

  if (table[i].ein <= 0.0) {
    *value = x * y0;
    return;
  }

  //if value is bwtween x0 and x1, use linear interpolation method.
  x1 = table[i].ein;
  y1 = table[i].coeff;
  *value = (y0 + (((y1 - y0)/(x1 - x0)) * (x - x0))) * x;

  return;
}

static int
read_pax_therm(uint8_t pax_id, float *value) {
  int ret = 0;

  if (!is_device_ready())
    return ERR_SENSOR_NA;

  struct {
    const char *chip;
    const char *label;
  } devs[] = {
    {"tmp422-i2c-6-4d", "PAX0_THERM_REMOTE"},
    {"tmp422-i2c-6-4d", "PAX1_THERM_REMOTE"},
    {"tmp422-i2c-6-4e", "PAX2_THERM_REMOTE"},
    {"tmp422-i2c-6-4e", "PAX3_THERM_REMOTE"},
  };

  if ((g_board_id & 0x7) == 0x3) {
    ret = pal_get_pex_therm(pax_id, value);
  }
  else {
    ret = sensors_read(devs[pax_id].chip, devs[pax_id].label, value);
  }

  return ret;
}

static int
read_bay_temp(uint8_t bay_id, float *value) {
  int ret = 0;
  struct {
    const char *chip;
    const char *label;
  } devs[] = {
    {"tmp75-i2c-21-48", "BAY0_FTEMP"},
    {"tmp75-i2c-21-49", "BAY0_RTEMP"},
    {"tmp75-i2c-22-48", "BAY1_FTEMP"},
    {"tmp75-i2c-22-49", "BAY1_RTEMP"},
  };
  ret = sensors_read(devs[bay_id].chip, devs[bay_id].label, value);

  return ret;
}

static float max_nvme_temp0 = -99, max_nvme_temp1 = -99;
static int
read_nvme_temp(uint8_t sensor_num, float *value) {
  int ret = 0;
  int bus_id = 0, nvme_id = 0;
  int fd = 0;
  char fn[32];
  uint8_t tcount = 0, rcount = 0;
  uint8_t tbuf[16] = {0};
  uint8_t rbuf[16] = {0};

  switch (sensor_num) {
    case BAY_0_NVME_CTEMP:
      if (max_nvme_temp0 == -99) {
        return READING_NA;
      } else {
        *value = max_nvme_temp0;
        if (pal_is_server_off()) {
          max_nvme_temp1 = -99;
        }
        return PAL_EOK;
      }
      break;
    case BAY_1_NVME_CTEMP:
      if (max_nvme_temp1 == -99) {
        return READING_NA;
      } else {
        *value = max_nvme_temp1;
        if (pal_is_server_off()) {
          max_nvme_temp1 = -99;
        }
        return PAL_EOK;
      }
      break;
    default:
      if (sensor_num >= BAY_0_0_NVME_CTEMP &&
          sensor_num <= BAY_0_7_NVME_CTEMP) {
        nvme_id = sensor_num - BAY_0_0_NVME_CTEMP;
        bus_id = I2C_BUS_21;
      }
      else if (sensor_num >= BAY_1_0_NVME_CTEMP &&
               sensor_num <= BAY_1_7_NVME_CTEMP) {
        nvme_id = sensor_num - BAY_1_0_NVME_CTEMP;
        bus_id = I2C_BUS_22;
      }
      else {
        syslog(LOG_ERR, "Unknown NVME sensor ID");
        return READING_NA;
      }
  }

  pal_control_mux_to_target_ch(1 << nvme_id, bus_id, 0xE6);

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

  if (sensor_num >= BAY_0_0_NVME_CTEMP &&
      sensor_num <= BAY_0_7_NVME_CTEMP) {
    if (*value > max_nvme_temp0) {
      max_nvme_temp0 = *value;
    }
  }
  else if (sensor_num >= BAY_1_0_NVME_CTEMP &&
           sensor_num <= BAY_1_7_NVME_CTEMP) {
    if (*value > max_nvme_temp1) {
      max_nvme_temp1 = *value;
    }
  }

error_exit:
  close(fd);

  return ret;
}

static int sensors_read_vr(uint8_t sensor_num, float *value)
{
  int ret = 0;

  if (pal_is_fw_update_ongoing(FRU_MB)) {
#ifdef DEBUG
    syslog(LOG_DEBUG,"%s, Skip reading VR sensor when FW update", __func__);
#endif
    return READING_SKIP;
  }

  switch (sensor_num) {
    case MB_VR_P0V8_VDD0_TEMP:
      ret = sensors_read("mpq8645p-i2c-5-30", "VR_P0V8_VDD0_TEMP", value);
      break;
    case MB_VR_P0V8_VDD0_VOUT:
      ret = sensors_read("mpq8645p-i2c-5-30", "VR_P0V8_VDD0_VOUT", value);
      vr_vdd_volt[VR_ID0] = *value;
      break;
    case MB_VR_P0V8_VDD0_CURR:
      ret = sensors_read("mpq8645p-i2c-5-30", "VR_P0V8_VDD0_CURR", value);
      vr_vdd_curr[VR_ID0] = *value;
      break;
    case MB_VR_P0V8_VDD0_POWER:
      *value = vr_vdd_volt[VR_ID0] * vr_vdd_curr[VR_ID0];
      break;
    case MB_VR_P0V8_VDD1_TEMP:
      ret = sensors_read("mpq8645p-i2c-5-31", "VR_P0V8_VDD1_TEMP", value);
      break;
    case MB_VR_P0V8_VDD1_VOUT:
      ret = sensors_read("mpq8645p-i2c-5-31", "VR_P0V8_VDD1_VOUT", value);
      vr_vdd_volt[VR_ID1] = *value;
      break;
    case MB_VR_P0V8_VDD1_CURR:
      ret = sensors_read("mpq8645p-i2c-5-31", "VR_P0V8_VDD1_CURR", value);
      vr_vdd_curr[VR_ID1] = *value;
      break;
    case MB_VR_P0V8_VDD1_POWER:
      *value = vr_vdd_volt[VR_ID1] * vr_vdd_curr[VR_ID1];
      break;
    case MB_VR_P0V8_VDD2_TEMP:
      ret = sensors_read("mpq8645p-i2c-5-32", "VR_P0V8_VDD2_TEMP", value);
      break;
    case MB_VR_P0V8_VDD2_VOUT:
      ret = sensors_read("mpq8645p-i2c-5-32", "VR_P0V8_VDD2_VOUT", value);
      vr_vdd_volt[VR_ID2] = *value;
      break;
    case MB_VR_P0V8_VDD2_CURR:
      ret = sensors_read("mpq8645p-i2c-5-32", "VR_P0V8_VDD2_CURR", value);
      vr_vdd_curr[VR_ID2] = *value;
      break;
    case MB_VR_P0V8_VDD2_POWER:
      *value = vr_vdd_volt[VR_ID2] * vr_vdd_curr[VR_ID2];
      break;
    case MB_VR_P0V8_VDD3_TEMP:
      ret = sensors_read("mpq8645p-i2c-5-33", "VR_P0V8_VDD3_TEMP", value);
      break;
    case MB_VR_P0V8_VDD3_VOUT:
      ret = sensors_read("mpq8645p-i2c-5-33", "VR_P0V8_VDD3_VOUT", value);
      vr_vdd_volt[VR_ID3] = *value;
      break;
    case MB_VR_P0V8_VDD3_CURR:
      ret = sensors_read("mpq8645p-i2c-5-33", "VR_P0V8_VDD3_CURR", value);
      vr_vdd_curr[VR_ID3] = *value;
      break;
    case MB_VR_P0V8_VDD3_POWER:
      *value = vr_vdd_volt[VR_ID3] * vr_vdd_curr[VR_ID3];
      break;
    case MB_VR_P1V0_AVD0_TEMP:
    case VR_P1V8_PEX0_1_TEMP:
      ret = sensors_read("mpq8645p-i2c-5-34", "VR_P1V0_AVD0_TEMP", value);
      break;
    case MB_VR_P1V0_AVD0_VOUT:
    case VR_P1V8_PEX0_1_VOUT:
      ret = sensors_read("mpq8645p-i2c-5-34", "VR_P1V0_AVD0_VOUT", value);
      vr_avd_volt[VR_ID0] = *value;
      break;
    case MB_VR_P1V0_AVD0_CURR:
    case VR_P1V8_PEX0_1_CURR:
      ret = sensors_read("mpq8645p-i2c-5-34", "VR_P1V0_AVD0_CURR", value);
      vr_avd_curr[VR_ID0] = *value;
      break;
    case MB_VR_P1V0_AVD0_POWER:
      *value = vr_avd_volt[VR_ID0] * vr_avd_curr[VR_ID0];
      break;
    case MB_VR_P1V0_AVD1_TEMP:
      ret = sensors_read("mpq8645p-i2c-5-35", "VR_P1V0_AVD1_TEMP", value);
      break;
    case MB_VR_P1V0_AVD1_VOUT:
      ret = sensors_read("mpq8645p-i2c-5-35", "VR_P1V0_AVD1_VOUT", value);
      vr_avd_volt[VR_ID1] = *value;
      break;
    case MB_VR_P1V0_AVD1_CURR:
      ret = sensors_read("mpq8645p-i2c-5-35", "VR_P1V0_AVD1_CURR", value);
      vr_avd_curr[VR_ID1] = *value;
      break;
    case MB_VR_P1V0_AVD1_POWER:
      *value = vr_avd_volt[VR_ID1] * vr_avd_curr[VR_ID1];
      break;
    case MB_VR_P1V0_AVD2_TEMP:
    case VR_P1V8_PEX2_3_TEMP:
      ret = sensors_read("mpq8645p-i2c-5-36", "VR_P1V0_AVD2_TEMP", value);
      break;
    case MB_VR_P1V0_AVD2_VOUT:
    case VR_P1V8_PEX2_3_VOUT:
      ret = sensors_read("mpq8645p-i2c-5-36", "VR_P1V0_AVD2_VOUT", value);
      vr_avd_volt[VR_ID2] = *value;
      break;
    case MB_VR_P1V0_AVD2_CURR:
    case VR_P1V8_PEX2_3_CURR:
      ret = sensors_read("mpq8645p-i2c-5-36", "VR_P1V0_AVD2_CURR", value);
      vr_avd_curr[VR_ID2] = *value;
      break;
    case MB_VR_P1V0_AVD2_POWER:
      *value = vr_avd_volt[VR_ID2] * vr_avd_curr[VR_ID2];
      break;
    case MB_VR_P1V0_AVD3_TEMP:
      ret = sensors_read("mpq8645p-i2c-5-3b", "VR_P1V0_AVD3_TEMP", value);
      break;
    case MB_VR_P1V0_AVD3_VOUT:
      ret = sensors_read("mpq8645p-i2c-5-3b", "VR_P1V0_AVD3_VOUT", value);
      vr_avd_volt[VR_ID3] = *value;
      break;
    case MB_VR_P1V0_AVD3_CURR:
      ret = sensors_read("mpq8645p-i2c-5-3b", "VR_P1V0_AVD3_CURR", value);
      vr_avd_curr[VR_ID3] = *value;
      break;
    case MB_VR_P1V0_AVD3_POWER:
      *value = vr_avd_volt[VR_ID3] * vr_avd_curr[VR_ID3];
      break;
    case VR_P0V9_PEX0_TEMP:
      ret = sensors_read("isl69260-i2c-5-60", "VR_P0V9_PEX0_TEMP", value);
      break;
    case VR_P0V9_PEX0_VOUT:
      ret = sensors_read("isl69260-i2c-5-60", "VR_P0V9_PEX0_VOUT", value);
      break;
    case VR_P0V9_PEX0_CURR:
      ret = sensors_read("isl69260-i2c-5-60", "VR_P0V9_PEX0_IOUT", value);
      break;
    case VR_P0V9_PEX1_TEMP:
      ret = sensors_read("isl69260-i2c-5-61", "VR_P0V9_PEX1_TEMP", value);
      break;
    case VR_P0V9_PEX1_VOUT:
      ret = sensors_read("isl69260-i2c-5-61", "VR_P0V9_PEX1_VOUT", value);
      break;
    case VR_P0V9_PEX1_CURR:
      ret = sensors_read("isl69260-i2c-5-61", "VR_P0V9_PEX1_IOUT", value);
      break;
    case VR_P0V9_PEX2_TEMP:
      ret = sensors_read("isl69260-i2c-5-72", "VR_P0V9_PEX2_TEMP", value);
      break;
    case VR_P0V9_PEX2_VOUT:
      ret = sensors_read("isl69260-i2c-5-72", "VR_P0V9_PEX2_VOUT", value);
      break;
    case VR_P0V9_PEX2_CURR:
      ret = sensors_read("isl69260-i2c-5-72", "VR_P0V9_PEX2_IOUT", value);
      break;
    case VR_P0V9_PEX3_TEMP:
      ret = sensors_read("isl69260-i2c-5-75", "VR_P0V9_PEX3_TEMP", value);
      break;
    case VR_P0V9_PEX3_VOUT:
      ret = sensors_read("isl69260-i2c-5-75", "VR_P0V9_PEX3_VOUT", value);
      break;
    case VR_P0V9_PEX3_CURR:
      ret = sensors_read("isl69260-i2c-5-75", "VR_P0V9_PEX3_IOUT", value);
      break;
    default:
      ret = ERR_SENSOR_NA;
  }

  return ret < 0? ERR_SENSOR_NA: 0;
}

static int
read_nic_volt(uint8_t nic_id, float *value) {
  struct devs{
    const char *chip;
    const char *label;
  };

  struct devs ti_adc[] = {
    {"adc128d818-i2c-23-1d", "NIC0_VOLT"},
    {"adc128d818-i2c-23-1d", "NIC1_VOLT"},
    {"adc128d818-i2c-23-1d", "NIC2_VOLT"},
    {"adc128d818-i2c-23-1d", "NIC3_VOLT"},
    {"adc128d818-i2c-23-1f", "NIC4_VOLT"},
    {"adc128d818-i2c-23-1f", "NIC5_VOLT"},
    {"adc128d818-i2c-23-1f", "NIC6_VOLT"},
    {"adc128d818-i2c-23-1f", "NIC7_VOLT"},
  };

  struct devs max_adc[] = {
    {NIC_MAX11615_DIR, "7"},
    {NIC_MAX11615_DIR, "6"},
    {NIC_MAX11615_DIR, "5"},
    {NIC_MAX11615_DIR, "4"},
    {NIC_MAX11617_DIR, "7"},
    {NIC_MAX11617_DIR, "6"},
    {NIC_MAX11617_DIR, "5"},
    {NIC_MAX11617_DIR, "4"},
  };

  int ret = 0, data = 0;

  if (is_nic_present(nic_id) == true) {
    return READING_NA;
  }

  if (GETBIT(g_board_id, 0) && !GETBIT(g_board_id, 1)) {
    ret = sensors_read_maxim(max_adc[nic_id].chip, max_adc[nic_id].label, &data);
    if (!ret) {
    //Convert MAXIM ADC, Volt = Read * 0.0005 * 9.08 / 1.21
      *value = (float) data * 0.0005 * 17.8 / 2;
    }
  }
  else {
    ret = sensors_read(ti_adc[nic_id].chip, ti_adc[nic_id].label, value);
  }

  nic_volt[nic_id] = *value;
  return ret;
}

static int
read_nic_curr(uint8_t nic_id, float *value) {
  struct devs{
    const char *chip;
    const char *label;
  };

  struct devs ti_adc[] = {
    {"adc128d818-i2c-23-1d", "NIC0_CURR"},
    {"adc128d818-i2c-23-1d", "NIC1_CURR"},
    {"adc128d818-i2c-23-1d", "NIC2_CURR"},
    {"adc128d818-i2c-23-1d", "NIC3_CURR"},
    {"adc128d818-i2c-23-1f", "NIC4_CURR"},
    {"adc128d818-i2c-23-1f", "NIC5_CURR"},
    {"adc128d818-i2c-23-1f", "NIC6_CURR"},
    {"adc128d818-i2c-23-1f", "NIC7_CURR"},
  };

  struct devs max_adc[] = {
    {NIC_MAX11615_DIR, "0"},
    {NIC_MAX11615_DIR, "1"},
    {NIC_MAX11615_DIR, "2"},
    {NIC_MAX11615_DIR, "3"},
    {NIC_MAX11617_DIR, "8"},
    {NIC_MAX11617_DIR, "9"},
    {NIC_MAX11617_DIR, "10"},
    {NIC_MAX11617_DIR, "11"},
  };

  int ret = 0, data = 0;

  if (is_nic_present(nic_id) == true) {
    return READING_NA;
  }

  if (GETBIT(g_board_id, 0) && !GETBIT(g_board_id, 1)) {
    ret = sensors_read_maxim(max_adc[nic_id].chip, max_adc[nic_id].label, &data);
    if (ret) {
      return ret;
    }
    //Convert MAXIM ADC, Current = Read / 0.185
    *value = (float) data * 0.0005 / 0.185;
  }
  else if (GETBIT(g_board_id, 1)) {
    ret = sensors_read(ti_adc[nic_id].chip, ti_adc[nic_id].label, value);
    if (ret) {
      return ret;
    }
    //Convert TI ADC + MPS efuse
    *value = *value * 1.576;
  }
  else {
    ret = sensors_read(ti_adc[nic_id].chip, ti_adc[nic_id].label, value);
  }

  nic_curr[nic_id] = *value;
  return ret;
}

static int
read_nic_pwr(uint8_t nic_id, float *value) {
  if (is_nic_present(nic_id) == true) {
    return READING_NA;
  }
  *value = nic_volt[nic_id] * nic_curr[nic_id];
  return 0;
}

static int
read_nvme_sensor(uint8_t sensor_num, float *value) {
  int fd = 0, ret = -1;
  char fn[32];
  float scale, volt_scale = 0.03125;
  float curr_scale = 0.0625, pwr_scale = 1;
  uint8_t retry = 3, tlen, rlen, addr=0, bus, cmd;
  uint8_t tbuf[16] = {0};
  uint8_t rbuf[16] = {0};
  uint16_t tmp;

  switch(sensor_num) {
    case BAY_0_VOL:
    case BAY_0_IOUT:
    case BAY_0_POUT:
      addr = 0x88;
      break;
    case BAY_1_VOL:
    case BAY_1_IOUT:
    case BAY_1_POUT:
      addr = 0x86;
      break;
    default:
      return READING_NA;
  }

  bus = I2C_BUS_5;

  switch(sensor_num) {
    case BAY_0_VOL:
    case BAY_1_VOL:
      cmd = MP5023_VOUT;
      scale = volt_scale;
      break;
    case BAY_0_IOUT:
    case BAY_1_IOUT:
      cmd = MP5023_IOUT;
      scale = curr_scale;
      break;
    case BAY_0_POUT:
    case BAY_1_POUT:
      cmd = MP5023_POUT;
      scale = pwr_scale;
      break;
    default:
      return READING_NA;
  }

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

  if (ret < 0) {
    syslog(LOG_DEBUG, "%s ret=%d", __FUNCTION__, ret);
    goto err_exit;
  }

  tmp = ((rbuf[1] & 0x03) << 8) | (rbuf[0]);
  *value = (float)tmp * scale;

err_exit:
  if (fd > 0) {
    close(fd);
  }

  return ret;
}

static int
read_ina219_sensor(uint8_t sensor_num, float *value) {
  int fd = 0, ret = -1;
  char fn[32];
  float scale, vol_scale = 0.004;
  float curr_scale = 0.0002, pwr_scale = 0.004;
  uint8_t retry = 3, tlen, rlen, addr, bus, cmd;
  uint8_t tbuf[16] = {0};
  uint8_t rbuf[16] = {0};
  uint16_t ina219_id, tmp;

  switch(sensor_num) {
    case  BAY0_INA219_VOLT:
    case  BAY0_INA219_CURR:
    case  BAY0_INA219_PWR:
      ina219_id = 0;
      break;
    case  BAY1_INA219_VOLT:
    case  BAY1_INA219_CURR:
    case  BAY1_INA219_PWR:
      ina219_id = 1;
      break;
    default:
      return READING_NA;
  }

  bus = ina219_info_list[ina219_id].bus;
  addr = ina219_info_list[ina219_id].slv_addr;

  if (strstr(sensor_map[sensor_num].snr_name, "CURR")) {
    cmd = INA219_CURRENT;
    scale = curr_scale;
  }
  else if (strstr(sensor_map[sensor_num].snr_name, "VOLT")) {
    cmd = INA219_VOLTAGE;
    scale = vol_scale;
  }
  else if (strstr(sensor_map[sensor_num].snr_name, "PWR")) {
    cmd = INA219_POWER;
    scale = pwr_scale;
  }
  else {
    return READING_NA;
  }

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

  if (ret < 0) {
    syslog(LOG_DEBUG, "ret=%d", ret);
    goto err_exit;
  }

  switch(sensor_num) {
    case BAY0_INA219_VOLT:
    case BAY1_INA219_VOLT:
      //only bit 15:3 represent Bus voltage
      tmp = ((rbuf[0] << 8) | (rbuf[1])) >> 3;
      *value = (float)tmp * scale;
      break;
    case BAY0_INA219_CURR:
    case BAY1_INA219_CURR:
    case BAY0_INA219_PWR:
    case BAY1_INA219_PWR:
      tmp = (rbuf[1] << 8) | (rbuf[0]);
      *value = (float)tmp * scale;
      break;
    default:
      return READING_NA;
  }

  err_exit:
  if (fd > 0) {
    close(fd);
  }
  return ret;
}

static int
read_bay0_ssd_curr(uint8_t ssd_id, float *value) {
  int ret = 0;
  struct {
    const char *chip;
    const char *label;
  } devs[] = {
    {"adc128d818-i2c-21-1f", "SSD1_CURR"},
    {"adc128d818-i2c-21-1f", "SSD0_CURR"},
    {"adc128d818-i2c-21-1f", "SSD3_CURR"},
    {"adc128d818-i2c-21-1f", "SSD2_CURR"},
    {"adc128d818-i2c-21-1d", "SSD5_CURR"},
    {"adc128d818-i2c-21-1d", "SSD4_CURR"},
    {"adc128d818-i2c-21-1d", "SSD7_CURR"},
    {"adc128d818-i2c-21-1d", "SSD6_CURR"},
  };

  ret = sensors_read(devs[ssd_id].chip, devs[ssd_id].label, value);

  return ret;
}

static int
read_bay0_ssd_volt(uint8_t ssd_id, float *value) {
  int ret = 0;
  struct {
    const char *chip;
    const char *label;
  } devs[] = {
    {"adc128d818-i2c-21-1f", "SSD1_VOLT"},
    {"adc128d818-i2c-21-1f", "SSD0_VOLT"},
    {"adc128d818-i2c-21-1f", "SSD3_VOLT"},
    {"adc128d818-i2c-21-1f", "SSD2_VOLT"},
    {"adc128d818-i2c-21-1d", "SSD5_VOLT"},
    {"adc128d818-i2c-21-1d", "SSD4_VOLT"},
    {"adc128d818-i2c-21-1d", "SSD7_VOLT"},
    {"adc128d818-i2c-21-1d", "SSD6_VOLT"},
  };

  ret = sensors_read(devs[ssd_id].chip, devs[ssd_id].label, value);

  return ret;
}

static int
read_bay1_ssd_curr(uint8_t ssd_id, float *value) {
  int ret = 0;
  struct {
    const char *chip;
    const char *label;
  } devs[] = {
    {"adc128d818-i2c-22-1f", "SSD1_CURR"},
    {"adc128d818-i2c-22-1f", "SSD0_CURR"},
    {"adc128d818-i2c-22-1f", "SSD3_CURR"},
    {"adc128d818-i2c-22-1f", "SSD2_CURR"},
    {"adc128d818-i2c-22-1d", "SSD5_CURR"},
    {"adc128d818-i2c-22-1d", "SSD4_CURR"},
    {"adc128d818-i2c-22-1d", "SSD7_CURR"},
    {"adc128d818-i2c-22-1d", "SSD6_CURR"},
  };

  ret = sensors_read(devs[ssd_id].chip, devs[ssd_id].label, value);

  return ret;
}

static int
read_bay1_ssd_volt(uint8_t ssd_id, float *value) {
  int ret = 0;
  struct {
    const char *chip;
    const char *label;
  } devs[] = {
    {"adc128d818-i2c-22-1f", "SSD1_VOLT"},
    {"adc128d818-i2c-22-1f", "SSD0_VOLT"},
    {"adc128d818-i2c-22-1f", "SSD3_VOLT"},
    {"adc128d818-i2c-22-1f", "SSD2_VOLT"},
    {"adc128d818-i2c-22-1d", "SSD5_VOLT"},
    {"adc128d818-i2c-22-1d", "SSD4_VOLT"},
    {"adc128d818-i2c-22-1d", "SSD7_VOLT"},
    {"adc128d818-i2c-22-1d", "SSD6_VOLT"},
  };

  ret = sensors_read(devs[ssd_id].chip, devs[ssd_id].label, value);

  return ret;
}

static int
read_ina260_sensor(uint8_t sensor_num, float *value) {
  int fd = 0, ret = -1;
  char fn[32];
  float scale, vol_scale = 0.00125, curr_scale = 0.00125, pwr_scale = 0.01;
  uint8_t retry = 3, tlen, rlen, addr = 0, bus, cmd;
  uint8_t tbuf[16] = {0};
  uint8_t rbuf[16] = {0};
  uint16_t tmp;

  if(sensor_num >= E1S_0_0_NVME_VOLT && sensor_num <= E1S_0_7_NVME_PWR) {
    bus = I2C_BUS_21;
    addr = ((sensor_num - E1S_0_0_NVME_VOLT)/3)*2 + 0x80;
  }
  else if(sensor_num >= E1S_1_0_NVME_VOLT && sensor_num < SENSOR_END){
    bus = I2C_BUS_22;
    addr = ((sensor_num - E1S_1_0_NVME_VOLT)/3)*2 + 0x80;
  }
  else if (sensor_num == E1S_1_7_NVME_PWR){
    bus = I2C_BUS_22;
    addr = 0x8e;
    cmd = INA260_POWER;
    scale = pwr_scale;
  }
  else {
    syslog(LOG_WARNING, "%s Unknown sensor id: %d", __FUNCTION__, sensor_num);
    return READING_NA;
  }

  if (strstr(sensor_map[sensor_num].snr_name, "CURR")) {
    cmd = INA260_CURRENT;
    scale = curr_scale;
  }
  else if (strstr(sensor_map[sensor_num].snr_name, "VOLT")) {
    cmd = INA260_VOLTAGE;
    scale = vol_scale;
  }
  else if (strstr(sensor_map[sensor_num].snr_name, "PWR")) {
    cmd = INA260_POWER;
    scale = pwr_scale;
  }
  else {
    return READING_NA;
  }

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

  if (ret < 0) {
    syslog(LOG_DEBUG, "%s ret=%d", __FUNCTION__, ret);
    goto err_exit;
  }

  tmp = (rbuf[0] << 8) | (rbuf[1]);
  *value = (float)tmp * scale;

  err_exit:
  if (fd > 0) {
    close(fd);
  }
  return ret;
}

int
read_device(const char *device, int *value) {
  FILE *fp = NULL;

  if (device == NULL || value == NULL) {
    syslog(LOG_ERR, "%s: Invalid parameter", __func__);
    return -1;
  }

  fp = fopen(device, "r");
  if (fp == NULL) {
    syslog(LOG_INFO, "%s failed to open device %s error: %s", __func__, device, strerror(errno));
    return -1;
  }

  if (fscanf(fp, "%d", value) != 1) {
    syslog(LOG_INFO, "%s failed to read device %s error: %s", __func__, device, strerror(errno));
    fclose(fp);
    return -1;
  }
  fclose(fp);

  return 0;
}

static int sensors_read_maxim(const char *dev, const char *channel, int *data)
{
  int val = 0;
  char ain_name[30] = {0};
  char dev_dir[LARGEST_DEVICE_NAME] = {0};

  sprintf(ain_name, IIO_AIN_NAME, channel);
  sprintf(dev_dir, dev, ain_name);

  if(access(dev_dir, F_OK)) {
    return ERR_SENSOR_NA;
  }

  if (read_device(dev_dir, &val) < 0) {
    syslog(LOG_ERR, "%s: dev_dir: %s read fail", __func__, dev_dir);
    return ERR_FAILURE;
  }

  *data = val;

  return 0;
}

