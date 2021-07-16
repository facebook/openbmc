/*
 *
 * Copyright 2019-present Facebook. All Rights Reserved.
 *
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */

#ifndef __PAL_H__
#define __PAL_H__

#include <openbmc/obmc-pal.h>

#ifdef __cplusplus
extern "C" {
#endif

#include <openbmc/kv.h>
#include <openbmc/ipmi.h>
#include <openbmc/obmc-pal.h>
#include <stdbool.h>
#include <math.h>

#define PLATFORM_NAME "agc032a"
#define READ_UNIT_SENSOR_TIMEOUT 5

#define SYS_PLATFROM_DIR(dev) "/sys/devices/platform/"#dev"/"
#define I2C_DRIVER_DIR(name, bus, addr) "/sys/bus/i2c/drivers/"#name"/"#bus"-00"#addr"/"
#define I2C_DEV_DIR(bus, addr) "/sys/bus/i2c/devices/i2c-"#bus"/"#bus"-00"#addr"/"
#define HW_MON_DIR "hwmon/hwmon*"

#define SWPLD1 "swpld1"
#define SWPLD2 "swpld2"
#define SWPLD3 "swpld3"

#define SWBD_ID "board_id"

#define SCM_CPLD       "scmcpld"
#define SMB_CPLD       "smbcpld"
#define FCM_CPLD       "fcmcpld"
#define PWR_CPLD       "pwrcpld"
#define DOM_FPGA1      "domfpga1"
#define DOM_FPGA2      "domfpga2"

#define GPIO_VAL "/sys/class/gpio/gpio%d/value"
#define GPIO_DIR "/sys/class/gpio/gpio%d/direction"
#define SCM_SYSFS        I2C_DEV_DIR(2, 3e)"%s"
#define SMB_SYSFS        I2C_DEV_DIR(12, 3e)"%s"
#define PWR_SYSFS        I2C_DEV_DIR(29, 3e)"%s"
#define FCM_SYSFS        I2C_DEV_DIR(30, 3e)"%s"
#define DOMFPGA1_SYSFS   I2C_DEV_DIR(13, 60)"%s"
#define DOMFPGA2_SYSFS   I2C_DEV_DIR(5, 60)"%s"

#define SWPLD1_SYSFS I2C_DEV_DIR(7, 32) "%s"
#define SWPLD2_SYSFS I2C_DEV_DIR(7, 34) "%s"
#define SWPLD3_SYSFS I2C_DEV_DIR(7, 35) "%s"

#define SENSORD_FILE_SMB "/tmp/cache_store/smb_sensor%d"
#define SENSORD_FILE_PSU "/tmp/cache_store/psu%d_sensor%d"
#define KV_PATH "/mnt/data/kv_store/%s"

#define WEDGE400_SDR_PATH "/tmp/sdr_%s.bin"

#define COM_PWR_BTN_N "come_pwr_btn_n"
#define SYS_LED_COLOR "sys_led_color"
#define SCM_COM_RST_BTN "iso_com_rst_n"

#define GB_POWER  "gb_turn_on"
#define TH3_POWER "th3_turn_on"
#define TH3_RESET_BTN "mac_reset_n"

#define SWPLD1_TH3_ROV "rov_avs%d"
#define SWPLD1_TH3_ROV_NUM 8

#define SMB_CPLD_TH3_ROV "th3_rov%d"
#define SMB_CPLD_TH3_ROV_NUM 8
#define SMB_CPLD_GB_ROV "gb_rov%d"
#define SMB_CPLD_GB_ROV_NUM SMB_CPLD_TH3_ROV_NUM

#define SCM_PRSNT_STATUS "scm_present_int_status"
#define SCM_SUS_S3_STATUS "iso_com_sus_s3_n"
#define SCM_SUS_S4_STATUS "iso_com_sus_s4_n"
#define SCM_SUS_S5_STATUS "iso_com_sus_s5_n"
#define SCM_SUS_STAT_STATUS "iso_com_sus_stat_n"
#define FAN_PRSNT_STATUS "FAN%d_PRESENT"
#define PSU_PRSNT_STATUS "psu%d_present"
#define FAN_PRSNT_GPIO_START_POS 776

#define CRASHDUMP_BIN    "/usr/local/bin/autodump.sh"

#define LAST_KEY "last_key"

// SCM Sensor Devices
#define SCM_HSC_DEVICE         I2C_DEV_DIR(14, 10)HW_MON_DIR
#define SCM_OUTLET_TEMP_DEVICE I2C_DEV_DIR(15, 4d)HW_MON_DIR
#define SCM_INLET_TEMP_DEVICE  I2C_DEV_DIR(15, 4c)HW_MON_DIR

// SMB Sensor Devices
#define SMB_PXE1211_DEVICE            I2C_DEV_DIR(1, 0e)HW_MON_DIR
#define SMB_SW_SERDES_PVDD_DEVICE     I2C_DEV_DIR(1, 4d)HW_MON_DIR
#define SMB_SW_SERDES_TRVDD_DEVICE    I2C_DEV_DIR(1, 47)HW_MON_DIR
#define SMB_IR_HMB_DEVICE             I2C_DEV_DIR(1, 43)HW_MON_DIR
#define SMB_ISL_DEVICE                I2C_DEV_DIR(1, 60)HW_MON_DIR
#define SMB_XPDE_DEVICE               I2C_DEV_DIR(1, 40)HW_MON_DIR
#define SMB_1220_DEVICE               I2C_DEV_DIR(1, 3a)HW_MON_DIR
#define SMB_LM75B_U28_DEVICE          I2C_DEV_DIR(3, 48)HW_MON_DIR
#define SMB_LM75B_U25_DEVICE          I2C_DEV_DIR(3, 49)HW_MON_DIR
#define SMB_LM75B_U56_DEVICE          I2C_DEV_DIR(3, 4a)HW_MON_DIR
#define SMB_LM75B_U55_DEVICE          I2C_DEV_DIR(3, 4b)HW_MON_DIR
#define SMB_TMP421_U62_DEVICE         I2C_DEV_DIR(3, 4c)HW_MON_DIR
#define SMB_TMP421_U63_DEVICE         I2C_DEV_DIR(3, 4e)HW_MON_DIR
#define SMB_SW_TEMP_DEVICE            I2C_DEV_DIR(3, 4f)HW_MON_DIR
#define SMB_GB_TEMP_DEVICE            I2C_DEV_DIR(3, 2a)HW_MON_DIR
#define SMB_DOM1_DEVICE               I2C_DEV_DIR(13, 60)
#define SMB_DOM2_DEVICE               I2C_DEV_DIR(5, 60)
#define SMB_FCM_TACH_DEVICE           I2C_DEV_DIR(30, 3e)
#define SMB_FCM_LM75B_U1_DEVICE       I2C_DEV_DIR(32, 48)HW_MON_DIR
#define SMB_FCM_LM75B_U2_DEVICE       I2C_DEV_DIR(32, 49)HW_MON_DIR
#define SMB_FCM_HSC_DEVICE            I2C_DEV_DIR(33, 10)HW_MON_DIR
#define SMB_TMP75_LF_DEVICE           I2C_DEV_DIR(4, 4b)HW_MON_DIR
#define SMB_TMP75_RF_DEVICE           I2C_DEV_DIR(4, 49)HW_MON_DIR
#define SMB_TMP75_UPPER_MAC_DEVICE    I2C_DEV_DIR(4, 4a)HW_MON_DIR
#define SMB_TMP75_LOWER_MAC_DEVICE    I2C_DEV_DIR(4, 4e)HW_MON_DIR
#define SMB_FAN_CONTROLLER1_DEVICE    I2C_DEV_DIR(3, 2e)
#define SMB_FAN_CONTROLLER2_DEVICE    I2C_DEV_DIR(3, 4c)
#define SMB_FAN_CONTROLLER3_DEVICE    I2C_DEV_DIR(3, 2d)

//BMC Sensor Devices
#define AST_ADC_DEVICE         SYS_PLATFROM_DIR("ast-adc-hwmon")HW_MON_DIR

// PSU Sensor Devices
#define PSU_DRIVER             "dps_driver"
#define PSU1_DEVICE            I2C_DEV_DIR(0, 58)
#define PSU2_DEVICE            I2C_DEV_DIR(1, 58)

#define TEMP(x)                "temp"#x"_input"
#define VOLT(x)                "in"#x"_input"
#define VOLT_SET(x)            "vo"#x"_input"
#define CURR(x)                "curr"#x"_input"
#define POWER(x)               "power"#x"_input"

#define GPIO_SMB_REV_ID_0   "/tmp/gpionames/BMC_CPLD_BOARD_REV_ID0/%s"
#define GPIO_SMB_REV_ID_1   "/tmp/gpionames/BMC_CPLD_BOARD_REV_ID1/%s"
#define GPIO_SMB_REV_ID_2   "/tmp/gpionames/BMC_CPLD_BOARD_REV_ID2/%s"
#define GPIO_BMC_BRD_TPYE   "/tmp/gpionames/BMC_CPLD_BOARD_TYPE/%s"
#define GPIO_POSTCODE_0     "/tmp/gpionames/GPIO_H0/%s"
#define GPIO_POSTCODE_1     "/tmp/gpionames/GPIO_H1/%s"
#define GPIO_POSTCODE_2     "/tmp/gpionames/GPIO_H2/%s"
#define GPIO_POSTCODE_3     "/tmp/gpionames/GPIO_H3/%s"
#define GPIO_POSTCODE_4     "/tmp/gpionames/GPIO_H4/%s"
#define GPIO_POSTCODE_5     "/tmp/gpionames/GPIO_H5/%s"
#define GPIO_POSTCODE_6     "/tmp/gpionames/GPIO_H6/%s"
#define GPIO_POSTCODE_7     "/tmp/gpionames/GPIO_H7/%s"
#define GPIO_DEBUG_PRSNT_N  "/tmp/gpionames/DEBUG_PRESENT_N/%s"
#define GPIO_COME_PWRGD     "/tmp/gpionames/PWRGD_PCH_PWROK/%s"
/* Add button function for Debug Card */
#define BMC_UART_SEL        I2C_DEV_DIR(7, 32)"console_sel"

#define MAX_NUM_SCM 1
#define MAX_NUM_FAN 6
#define MAX_RETRIES_SDR_INIT  30
#define MAX_READ_RETRY 10
#define DELAY_POWER_BTN 2
#define DELAY_POWER_CYCLE 5

#define MAX_POS_READING_MARGIN 127
#define LARGEST_DEVICE_NAME 128
#define WEDGE400_MAX_NUM_SLOTS 1

#define UNIT_DIV 1000

#define READING_SKIP 1
#define READING_NA -2

#define SCM_RSENSE 1

#define IPMB_BUS 0

#define BMC_READY_N   28
#define THERMAL_CONSTANT   256
#define ERR_NOT_READY   -2
#define MAX_NODES     1
// #define MAX_NUM_FRUS    6
#define MAX_SENSOR_NUM  0xFF
#define MAX_SENSOR_THRESHOLD  8
#define MAX_SDR_LEN     64
#define TH3_VOL_MAX 927
#define TH3_VOL_MIN 750

#define FRU_STATUS_GOOD   1
#define FRU_STATUS_BAD    0

#define LED_DEV "/dev/i2c-6"
#define I2C_ADDR_SIM_LED 0x20

#define FPGA_STS_LED_REG 0x82
#define FPGA_STS_CLR_BLUE 0x01
#define FPGA_STS_CLR_YELLOW 0x05

#define PWM_UNIT_MAX 255
extern const char pal_fru_list[];
extern size_t pal_pwm_cnt;
extern size_t pal_tach_cnt;

enum {
  BRD_TYPE_WEDGE400 = 0x00,
  BRD_TYPE_WEDGE400C = 0x01,
};

enum {
  BOARD_WEDGE400_EVT   = 0x00,
  BOARD_WEDGE400_EVT3  = 0x01,
  BOARD_WEDGE400_DVT   = 0x02,
  BOARD_WEDGE400_DVT2_PVT_PVT2  = 0x03,
  BOARD_WEDGE400_PVT3  = 0x04,
  BOARD_WEDGE400C_EVT  = 0x10,
  BOARD_WEDGE400C_EVT2 = 0x11,
  BOARD_WEDGE400C_DVT  = 0x12,
  BOARD_UNDEFINED      = 0xFF,
};

enum {
  TH3_POWER_ON,
  TH3_POWER_OFF,
  TH3_RESET,
};

//Move FRU_SCM to virtual FRU ID
enum {

  FRU_ALL  = 0,
  FRU_SMB  = 1,
  FRU_PSU1 = 2,
  FRU_PSU2 = 3,
  FRU_FAN1 = 4,
  FRU_FAN2 = 5,
  FRU_FAN3 = 6,
  FRU_FAN4 = 7,
  FRU_FAN5 = 8,
  FRU_FAN6 = 9,
  MAX_NUM_FRUS = 9,
  FRU_SCM,
  FRU_FCM,
  FRU_BMC,
  FRU_CPLD,
  FRU_FPGA,
};

enum {
  HSC_FCM = 0,
};

/* Sensors on SCM */
enum {
  SCM_SENSOR_OUTLET_TEMP = 0x02,
  SCM_SENSOR_INLET_TEMP = 0x04,
  SCM_SENSOR_HSC_VOLT = 0x0a,
  SCM_SENSOR_HSC_CURR = 0x0b
};

/* Sensors on SMB */
enum {
  SMB_SENSOR_1220_VMON1 = 0x01,
  SMB_SENSOR_1220_VMON2,
  SMB_SENSOR_1220_VMON3,
  SMB_SENSOR_1220_VMON4,
  SMB_SENSOR_1220_VMON5,
  SMB_SENSOR_1220_VMON6,
  SMB_SENSOR_1220_VMON7,
  SMB_SENSOR_1220_VMON8,
  SMB_SENSOR_1220_VMON9,
  SMB_SENSOR_1220_VMON10,
  SMB_SENSOR_1220_VMON11,
  SMB_SENSOR_1220_VMON12,
  SMB_SENSOR_1220_VCCA,
  SMB_SENSOR_1220_VCCINP,
  SMB_SENSOR_SW_SERDES_PVDD_IN_VOLT,
  SMB_SENSOR_SW_SERDES_PVDD_IN_CURR,
  SMB_SENSOR_SW_SERDES_PVDD_IN_POWER,
  SMB_SENSOR_SW_SERDES_PVDD_OUT_VOLT,
  SMB_SENSOR_SW_SERDES_PVDD_OUT_CURR,
  SMB_SENSOR_SW_SERDES_PVDD_OUT_POWER,
  SMB_SENSOR_SW_SERDES_PVDD_TEMP1,
  SMB_SENSOR_SW_SERDES_TRVDD_IN_VOLT,
  SMB_SENSOR_SW_SERDES_TRVDD_IN_CURR,
  SMB_SENSOR_SW_SERDES_TRVDD_IN_POWER,
  SMB_SENSOR_SW_SERDES_TRVDD_OUT_VOLT,
  SMB_SENSOR_SW_SERDES_TRVDD_OUT_CURR,
  SMB_SENSOR_SW_SERDES_TRVDD_OUT_POWER,
  SMB_SENSOR_SW_SERDES_TRVDD_TEMP1,
  SMB_SENSOR_IR3R3V_LEFT_IN_VOLT,
  SMB_SENSOR_IR3R3V_LEFT_IN_CURR,
  SMB_SENSOR_IR3R3V_LEFT_IN_POWER,
  SMB_SENSOR_IR3R3V_LEFT_OUT_VOLT,
  SMB_SENSOR_IR3R3V_LEFT_OUT_CURR,
  SMB_SENSOR_IR3R3V_LEFT_OUT_POWER,
  SMB_SENSOR_IR3R3V_LEFT_TEMP,
  SMB_SENSOR_IR3R3V_RIGHT_IN_VOLT,
  SMB_SENSOR_IR3R3V_RIGHT_IN_CURR,
  SMB_SENSOR_IR3R3V_RIGHT_IN_POWER,
  SMB_SENSOR_IR3R3V_RIGHT_OUT_VOLT,
  SMB_SENSOR_IR3R3V_RIGHT_OUT_CURR,
  SMB_SENSOR_IR3R3V_RIGHT_OUT_POWER,
  SMB_SENSOR_IR3R3V_RIGHT_TEMP,
  SMB_SENSOR_SW_CORE_VOLT,
  SMB_SENSOR_SW_CORE_CURR,
  SMB_SENSOR_SW_CORE_POWER,
  SMB_SENSOR_SW_CORE_TEMP1,
  SMB_SENSOR_LM75B_U28_TEMP,
  SMB_SENSOR_LM75B_U25_TEMP,
  SMB_SENSOR_LM75B_U56_TEMP,
  SMB_SENSOR_LM75B_U55_TEMP,
  SMB_SENSOR_TMP421_U62_TEMP,
  SMB_SENSOR_TMP421_Q9_TEMP,
  SMB_SENSOR_TMP421_U63_TEMP,
  SMB_SENSOR_TMP421_Q10_TEMP,
  SMB_SENSOR_TMP422_U20_TEMP,
  SMB_SENSOR_SW_DIE_TEMP1,
  SMB_SENSOR_SW_DIE_TEMP2,
  SMB_DOM1_MAX_TEMP,
  SMB_DOM2_MAX_TEMP,
  SMB_SENSOR_TMP75_LF_TEMP,
  SMB_SENSOR_TMP75_RF_TEMP,
  SMB_SENSOR_TMP75_UPPER_MAC_TEMP,
  SMB_SENSOR_TMP75_LOWER_MAC_TEMP,
  /* Sensors on FCM */
  SMB_SENSOR_FCM_LM75B_U1_TEMP,
  SMB_SENSOR_FCM_LM75B_U2_TEMP,
  SMB_SENSOR_FCM_HSC_VOLT,
  SMB_SENSOR_FCM_HSC_CURR,
  /* Sensors Fan Speed */
  SMB_SENSOR_FAN1_FRONT_TACH,
  SMB_SENSOR_FAN1_REAR_TACH,
  SMB_SENSOR_FAN2_FRONT_TACH,
  SMB_SENSOR_FAN2_REAR_TACH,
  SMB_SENSOR_FAN3_FRONT_TACH,
  SMB_SENSOR_FAN3_REAR_TACH,
  SMB_SENSOR_FAN4_FRONT_TACH,
  SMB_SENSOR_FAN4_REAR_TACH,
  SMB_SENSOR_FAN5_FRONT_TACH,
  SMB_SENSOR_FAN5_REAR_TACH,
  SMB_SENSOR_FAN6_FRONT_TACH,
  SMB_SENSOR_FAN6_REAR_TACH,
  /* Sensors on BMC*/
  SMB_BMC_ADC0_VSEN,
  SMB_BMC_ADC1_VSEN,
  SMB_BMC_ADC2_VSEN,
  SMB_BMC_ADC3_VSEN,
  SMB_BMC_ADC5_VSEN,
  SMB_BMC_ADC6_VSEN,
  SMB_BMC_ADC7_VSEN,
  SMB_BMC_ADC8_VSEN,
  SMB_BMC_ADC9_VSEN,
  SMB_BMC_ADC13_VSEN,
  SMB_BMC_ADC14_VSEN,
  SMB_BMC_ADC15_VSEN,
  /* PXE1211C only on Wedge400C EVT2 or later */
  SMB_SENSOR_HBM_IN_VOLT,
  SMB_SENSOR_HBM_OUT_VOLT,
  SMB_SENSOR_HBM_IN_CURR,
  SMB_SENSOR_HBM_OUT_CURR,
  SMB_SENSOR_HBM_IN_POWER,
  SMB_SENSOR_HBM_OUT_POWER,
  SMB_SENSOR_HBM_TEMP,
  SMB_SENSOR_VDDCK_0_IN_VOLT,
  SMB_SENSOR_VDDCK_0_OUT_VOLT,
  SMB_SENSOR_VDDCK_0_IN_CURR,
  SMB_SENSOR_VDDCK_0_OUT_CURR,
  SMB_SENSOR_VDDCK_0_IN_POWER,
  SMB_SENSOR_VDDCK_0_OUT_POWER,
  SMB_SENSOR_VDDCK_0_TEMP,
  SMB_SENSOR_VDDCK_1_IN_VOLT,
  SMB_SENSOR_VDDCK_1_OUT_VOLT,
  SMB_SENSOR_VDDCK_1_IN_CURR,
  SMB_SENSOR_VDDCK_1_OUT_CURR,
  SMB_SENSOR_VDDCK_1_IN_POWER,
  SMB_SENSOR_VDDCK_1_OUT_POWER,
  SMB_SENSOR_VDDCK_1_TEMP,
};

/* Sensors on PSU */
enum {
  PSU1_SENSOR_IN_VOLT = 0x01,
  PSU1_SENSOR_OUT_VOLT = 0x02,
  PSU1_SENSOR_IN_CURR = 0x03,
  PSU1_SENSOR_OUT_CURR = 0x04,
  PSU1_SENSOR_IN_POWER = 0x05,
  PSU1_SENSOR_OUT_POWER = 0x06,
  PSU1_SENSOR_FAN_TACH = 0x07,
  PSU1_SENSOR_TEMP1 = 0x08,
  PSU1_SENSOR_TEMP2 = 0x09,
  PSU1_SENSOR_CNT = PSU1_SENSOR_TEMP2,

  PSU2_SENSOR_IN_VOLT = 0x0a,
  PSU2_SENSOR_OUT_VOLT = 0x0b,
  PSU2_SENSOR_IN_CURR = 0x0c,
  PSU2_SENSOR_OUT_CURR = 0x0d,
  PSU2_SENSOR_IN_POWER = 0x0e,
  PSU2_SENSOR_OUT_POWER = 0x0f,
  PSU2_SENSOR_FAN_TACH = 0x10,
  PSU2_SENSOR_TEMP1 = 0x11,
  PSU2_SENSOR_TEMP2 = 0x12,
};

enum
{
  SLED_CLR_BLUE = 0x3,
  SLED_CLR_YELLOW = 0x4,
  SLED_CLR_GREEN = 0x5,
  SLED_CLR_RED = 0x6,
  SLED_CLR_OFF = 0x7,
};

enum
{
  SLED_SYS = 1,
  SLED_FAN = 2,
  SLED_PSU = 3,
  SLED_SMB = 4,
};

enum
{
  SCM_LED_BLUE = 0x01,
  SCM_LED_AMBER = 0x05,
};
/* Add button function for Debug Card */
enum {
  HAND_SW_SERVER = 0,
  HAND_SW_BMC = 1
};

int pal_handle_oem_1s_intr(uint8_t slot, uint8_t *data);
int pal_get_plat_sku_id(void);
int pal_get_poss_pcie_config(uint8_t slot, uint8_t *req_data, uint8_t req_len, uint8_t *res_data, uint8_t *res_len);
int pal_get_key_value(char *key, char *value);
int pal_set_key_value(char *key, char *value);
int pal_set_sysfw_ver(uint8_t slot, uint8_t *ver);
int pal_get_sysfw_ver(uint8_t slot, uint8_t *ver);
int pal_get_platform_name(char *name);
int pal_is_fru_prsnt(uint8_t fru, uint8_t *status);
int pal_is_fru_ready(uint8_t fru, uint8_t *status);
int pal_get_fru_id(char *str, uint8_t *fru);
int pal_get_fru_name(uint8_t fru, char *name);
int pal_get_fru_list(char *list);
int pal_get_fru_sensor_list(uint8_t fru, uint8_t **sensor_list, int *cnt);
int pal_get_fru_discrete_list(uint8_t fru, uint8_t **sensor_list, int *cnt);
int pal_is_debug_card_prsnt(uint8_t *status);
int pal_post_enable(uint8_t slot);
int pal_post_get_last(uint8_t slot, uint8_t *status);
int pal_post_get_last_and_len(uint8_t slot, uint8_t *data, uint8_t *len);
int pal_post_handle(uint8_t slot, uint8_t status);
int pal_set_last_pwr_state(uint8_t fru, char *state);
int pal_get_last_pwr_state(uint8_t fru, char *state);
int pal_get_dev_guid(uint8_t slot, char *guid);
int pal_set_dev_guid(uint8_t slot, char *str);
int pal_get_sys_guid(uint8_t slot, char *guid);
int pal_set_sys_guid(uint8_t slot, char *str);
int pal_set_com_pwr_btn_n(char *status);
int pal_set_server_power(uint8_t slot_id, uint8_t cmd);
int pal_get_server_power(uint8_t slot_id, uint8_t *status);
int pal_set_th3_power(int option);
int pal_get_fan_speed(uint8_t fan, int *rpm);
int pal_get_cpld_fpga_fw_ver(uint8_t fru, const char *device, uint8_t* ver);
int pal_get_board_rev(int *rev);
int pal_get_board_type(uint8_t *brd_type);
int pal_get_board_type_rev(uint8_t *brd_type_rev);
int pal_get_board_id(uint8_t slot, uint8_t *req_data, uint8_t req_len, uint8_t *res_data, uint8_t *res_len);
int pal_sensor_read_raw(uint8_t fru, uint8_t sensor_num, void *value);
int pal_sensor_discrete_read_raw(uint8_t fru, uint8_t sensor_num, void *value);
int pal_get_fw_info(uint8_t fru, unsigned char target, unsigned char* res, unsigned char* res_len);
void pal_update_ts_sled();
int pal_get_sensor_name(uint8_t fru, uint8_t sensor_num, char *name);
int pal_get_sensor_units(uint8_t fru, uint8_t sensor_num, char *units);
int pal_get_sensor_threshold(uint8_t fru, uint8_t sensor_num, uint8_t thresh, void *value);
void pal_sensor_assert_handle(uint8_t fru, uint8_t snr_num, float val, uint8_t thresh);
void pal_sensor_deassert_handle(uint8_t fru, uint8_t snr_num, float val, uint8_t thresh);
int pal_sensor_threshold_flag(uint8_t fru, uint8_t snr_num, uint16_t *flag);
int pal_detect_i2c_device(uint8_t bus_num, uint8_t addr);
int pal_add_i2c_device(uint8_t bus, uint8_t addr, char *device_name);
int pal_del_i2c_device(uint8_t bus, uint8_t addr);
void pal_get_chassis_status(uint8_t slot, uint8_t *req_data, uint8_t *res_data, uint8_t *res_len);
uint8_t pal_set_power_restore_policy(uint8_t slot, uint8_t *pwr_policy, uint8_t *res_data);
int pal_sel_handler(uint8_t fru, uint8_t snr_num, uint8_t *event_data);
void *generate_dump(void *arg);
int pal_mon_fw_upgrade(int brd_rev, uint8_t *sys_ug, uint8_t *fan_ug, uint8_t *psu_ug, uint8_t *smb_ug);
void set_sys_led(int brd_rev);
void set_fan_led(int brd_rev);
void set_psu_led(int brd_rev);
void set_scm_led(int brd_rev);
int set_sled(int brd_rev, uint8_t color, int led_name);
void init_led(void);
int pal_light_scm_led(uint8_t led_color);
int pal_get_fru_health(uint8_t fru, uint8_t *value);
int pal_set_def_key_value(void);
int pal_init_sensor_check(uint8_t fru, uint8_t snr_num, void *snr);
int agc032a_sensor_name(uint8_t fru, uint8_t sensor_num, char *name);
int pal_get_num_slots(uint8_t *num);
int pal_get_boot_order(uint8_t slot, uint8_t *req_data, uint8_t *boot, uint8_t *res_len);
int pal_set_boot_order(uint8_t slot, uint8_t *boot, uint8_t *res_data, uint8_t *res_len);
int pal_get_bmc_ipmb_slave_addr(uint16_t *slave_addr, uint8_t bus_id);
int pal_ipmb_processing(int bus, void *buf, uint16_t size);
bool pal_is_mcu_working(void);
int pal_get_hand_sw_physically(uint8_t *pos);
int pal_get_hand_sw(uint8_t *pos);
int pal_get_dbg_pwr_btn(uint8_t *status);
int pal_get_dbg_rst_btn(uint8_t *status);
int pal_set_rst_btn(uint8_t slot, uint8_t status);
int pal_get_dbg_uart_btn(uint8_t *status);
int pal_clr_dbg_uart_btn();
int pal_switch_uart_mux();
#ifdef __cplusplus
} // extern "C"
#endif

#endif /* __PAL_H__ */
