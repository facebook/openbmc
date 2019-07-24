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
#include <stdbool.h>
#include <math.h>
#include <facebook/bic.h>

#define PLATFORM_NAME "wedge400"

#define GPIO_VAL "/sys/class/gpio/gpio%d/value"
#define GPIO_DIR "/sys/class/gpio/gpio%d/direction"
#define SCM_SYSFS "/sys/bus/i2c/devices/i2c-2/2-003e/%s"
#define SMB_SYSFS "/sys/bus/i2c/devices/i2c-12/12-003e/%s"
#define FCM_SYSFS "/sys/bus/i2c/devices/i2c-30/30-003e/%s"
#define SENSORD_FILE_SMB "/tmp/cache_store/smb_sensor%d"
#define SENSORD_FILE_PSU "/tmp/cache_store/psu%d_sensor%d"
#define KV_PATH "/mnt/data/kv_store/%s"

#define WEDGE400_SDR_PATH "/tmp/sdr_%s.bin"

#define COM_PWR_BTN_N "com_pwr_btn_n"
#define SYS_LED_COLOR "sys_led_color"

#define SMB_MAC_CPLD_ROV "mac_cpld_rov%d"
#define SMB_MAC_CPLD_ROV_NUM 8
#define SCM_PRSNT_STATUS "scm_present_int_status"
#define FAN_PRSNT_STATUS "fan%d_present"
#define PSU_PRSNT_STATUS "psu_present_%d_N_int_status"
#define CRASHDUMP_BIN    "/usr/local/bin/autodump.sh"

#define LAST_KEY "last_key"

#define I2C_DEV_DIR(bus, addr) "/sys/bus/i2c/devices/i2c-"#bus"/"#bus"-00"#addr"/"
#define HW_MON_DIR "hwmon/hwmon*"

#define SCM_HSC_DEVICE         I2C_DEV_DIR(14, 10)HW_MON_DIR
#define SCM_OUTLET_TEMP_DEVICE I2C_DEV_DIR(15, 4c)HW_MON_DIR
#define SCM_INLET_TEMP_DEVICE  I2C_DEV_DIR(15, 4d)HW_MON_DIR

#define SMB_IR_L_DEVICE        I2C_DEV_DIR(1, 47)
#define SMB_IR_R_DEVICE        I2C_DEV_DIR(1, 4d)
#define SMB_ISL_DEVICE         I2C_DEV_DIR(1, 60)
#define SMB_1220_DEVICE        I2C_DEV_DIR(1, 3a)HW_MON_DIR
#define SMB_TEMP1_DEVICE       I2C_DEV_DIR(3, 48)HW_MON_DIR
#define SMB_TEMP2_DEVICE       I2C_DEV_DIR(3, 49)HW_MON_DIR
#define SMB_TEMP3_DEVICE       I2C_DEV_DIR(3, 4a)HW_MON_DIR
#define SMB_TEMP4_DEVICE       I2C_DEV_DIR(3, 4b)HW_MON_DIR
#define SMB_TEMP5_DEVICE       I2C_DEV_DIR(3, 4c)HW_MON_DIR
#define SMB_TEMP6_DEVICE       I2C_DEV_DIR(3, 4e)HW_MON_DIR
#define SMB_TH3_TEMP_DEVICE    I2C_DEV_DIR(3, 4f)HW_MON_DIR
#define SMB_DOM_DEVICE         I2C_DEV_DIR(13, 60)
#define SMB_FCM_TACH_DEVICE    I2C_DEV_DIR(30, 3e)
#define SMB_FCM_TEMP1_DEVICE   I2C_DEV_DIR(32, 48)HW_MON_DIR
#define SMB_FCM_TEMP2_DEVICE   I2C_DEV_DIR(32, 49)HW_MON_DIR
#define SMB_FCM_HSC_DEVICE     I2C_DEV_DIR(33, 10)HW_MON_DIR

#define PSU1_DEVICE            I2C_DEV_DIR(22, 58)
#define PSU2_DEVICE            I2C_DEV_DIR(23, 58)

#define TEMP(x)  "temp"#x"_input"
#define VOLT(x)  "in"#x"_input"
#define VOLT_SET(x)  "vo"#x"_input"
#define CURR(x)  "curr"#x"_input"
#define POWER(x) "power"#x"_input"

#define GPIO_SMB_REV_ID_0   "/tmp/gpionames/BMC_CPLD_BOARD_REV_ID0/%s"
#define GPIO_SMB_REV_ID_1   "/tmp/gpionames/BMC_CPLD_BOARD_REV_ID1/%s"
#define GPIO_SMB_REV_ID_2   "/tmp/gpionames/BMC_CPLD_BOARD_REV_ID2/%s"
#define GPIO_POSTCODE_0     "/tmp/gpionames/GPIOH0/%s"
#define GPIO_POSTCODE_1     "/tmp/gpionames/GPIOH1/%s"
#define GPIO_POSTCODE_2     "/tmp/gpionames/GPIOH2/%s"
#define GPIO_POSTCODE_3     "/tmp/gpionames/GPIOH3/%s"
#define GPIO_POSTCODE_4     "/tmp/gpionames/GPIOH4/%s"
#define GPIO_POSTCODE_5     "/tmp/gpionames/GPIOH5/%s"
#define GPIO_POSTCODE_6     "/tmp/gpionames/GPIOH6/%s"
#define GPIO_POSTCODE_7     "/tmp/gpionames/GPIOH7/%s"
#define GPIO_SCM_USB_PRSNT  "/tmp/gpionames/SCM_USB_PRSNT/%s"

#define MAX_NUM_FAN 4
#define MAX_RETRIES_SDR_INIT  30
#define MAX_READ_RETRY 10
#define DELAY_POWER_OFF 5
#define DELAY_POWER_CYCLE 10

#define MAX_POS_READING_MARGIN 127
#define LARGEST_DEVICE_NAME 128
#define WEDGE400_MAX_NUM_SLOTS 1

#define UNIT_DIV 1000

#define READING_SKIP 1
#define READING_NA -2

#define SCM_RSENSE 1.16

#define IPMB_BUS 0

#define BMC_READY_N   28
#define BOARD_REV_EVTA 4
#define BOARD_REV_EVTB 0
#define BIC_SENSOR_READ_NA 0x20
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

extern const char pal_fru_list[];

enum {
  BIC_MODE_NORMAL = 0x01,
  BIC_MODE_UPDATE = 0x0F,
};

enum {
  FRU_ALL  = 0,
  FRU_SCM  = 1,
  FRU_SMB  = 2,
  FRU_PSU1 = 3,
  FRU_PSU2 = 4,
  FRU_FAN1 = 5,
  FRU_FAN2 = 6,
  FRU_FAN3 = 7,
  FRU_FAN4 = 8,
  MAX_NUM_FRUS = 8,
};

enum {
  HSC_FCM = 0,
};

/* Sensors on SCM */
enum {
  SCM_SENSOR_OUTLET_TEMP = 0x02,
  SCM_SENSOR_INLET_TEMP = 0x04,
  SCM_SENSOR_HSC_VOLT = 0x0a,
  SCM_SENSOR_HSC_CURR = 0x0b,
  SCM_SENSOR_HSC_POWER = 0x0c,
  /* Sensors on COM-e */
  BIC_SENSOR_MB_OUTLET_TEMP = 0x01,
  BIC_SENSOR_MB_INLET_TEMP = 0x07,
  BIC_SENSOR_PCH_TEMP = 0x08,
  BIC_SENSOR_VCCIN_VR_TEMP = 0x80,
  BIC_SENSOR_1V05MIX_VR_TEMP = 0x81,
  BIC_SENSOR_SOC_TEMP = 0x05,
  BIC_SENSOR_SOC_THERM_MARGIN = 0x09,
  BIC_SENSOR_VDDR_VR_TEMP = 0x82,
  BIC_SENSOR_SOC_DIMMA0_TEMP = 0xB4,
  BIC_SENSOR_SOC_DIMMB0_TEMP = 0xB6,
  BIC_SENSOR_SOC_PACKAGE_PWR = 0x2C,
  BIC_SENSOR_VCCIN_VR_POUT = 0x8B,
  BIC_SENSOR_VDDR_VR_POUT = 0x8D,
  BIC_SENSOR_SOC_TJMAX = 0x30,
  BIC_SENSOR_P3V3_MB = 0xD0,
  BIC_SENSOR_P12V_MB = 0xD2,
  BIC_SENSOR_P1V05_PCH = 0xD3,
  BIC_SENSOR_P3V3_STBY_MB = 0xD5,
  BIC_SENSOR_P5V_STBY_MB = 0xD6,
  BIC_SENSOR_PV_BAT = 0xD7,
  BIC_SENSOR_PVDDR = 0xD8,
  BIC_SENSOR_P1V05_MIX = 0x8E,
  BIC_SENSOR_1V05MIX_VR_CURR = 0x84,
  BIC_SENSOR_VDDR_VR_CURR = 0x85,
  BIC_SENSOR_VCCIN_VR_CURR = 0x83,
  BIC_SENSOR_VCCIN_VR_VOL = 0x88,
  BIC_SENSOR_VDDR_VR_VOL = 0x8A,
  BIC_SENSOR_P1V05MIX_VR_VOL = 0x89,
  BIC_SENSOR_P1V05MIX_VR_POUT = 0x8C,
  BIC_SENSOR_INA230_POWER = 0x29,
  BIC_SENSOR_INA230_VOL = 0x2A,
  BIC_SENSOR_SYSTEM_STATUS = 0x10, //Discrete
  BIC_SENSOR_PROC_FAIL = 0x65, //Discrete
  BIC_SENSOR_SYS_BOOT_STAT = 0x7E, //Discrete
  BIC_SENSOR_VR_HOT = 0xB2, //Discrete
  BIC_SENSOR_CPU_DIMM_HOT = 0xB3, //Discrete
  BIC_SENSOR_SPS_FW_HLTH = 0x17, //Event-only
  BIC_SENSOR_POST_ERR = 0x2B, //Event-only
  BIC_SENSOR_POWER_THRESH_EVENT = 0x3B, //Event-only
  BIC_SENSOR_MACHINE_CHK_ERR = 0x40, //Event-only
  BIC_SENSOR_PCIE_ERR = 0x41, //Event-only
  BIC_SENSOR_OTHER_IIO_ERR = 0x43, //Event-only
  BIC_SENSOR_PROC_HOT_EXT = 0x51, //Event-only
  BIC_SENSOR_POWER_ERR = 0x56, //Event-only
  BIC_SENSOR_MEM_ECC_ERR = 0x63, //Event-only
  BIC_SENSOR_CAT_ERR = 0xEB, //Event-only
};

/* Sensors on SMB */
enum {
  SMB_SENSOR_1220_VMON1 = 0x01,
  SMB_SENSOR_1220_VMON2 = 0x02,
  SMB_SENSOR_1220_VMON3 = 0x03,
  SMB_SENSOR_1220_VMON4 = 0x04,
  SMB_SENSOR_1220_VMON5 = 0x05,
  SMB_SENSOR_1220_VMON6 = 0x06,
  SMB_SENSOR_1220_VMON7 = 0x07,
  SMB_SENSOR_1220_VMON8 = 0x08,
  SMB_SENSOR_1220_VMON9 = 0x09,
  SMB_SENSOR_1220_VMON10 = 0x0a,
  SMB_SENSOR_1220_VMON11 = 0x0b,
  SMB_SENSOR_1220_VMON12 = 0x0c,
  SMB_SENSOR_1220_VCCA = 0x0d,
  SMB_SENSOR_1220_VCCINP = 0x0e,
  SMB_SENSOR_TH3_SERDES_L_VOLT = 0x0f,
  SMB_SENSOR_TH3_SERDES_L_CURR = 0x10,
  SMB_SENSOR_TH3_SERDES_R_VOLT = 0x11,
  SMB_SENSOR_TH3_SERDES_R_CURR = 0x12,
  SMB_SENSOR_TH3_CORE_VOLT = 0x13,
  SMB_SENSOR_TH3_CORE_CURR = 0x14,
  SMB_SENSOR_TEMP1 = 0x15,
  SMB_SENSOR_TEMP2 = 0x16,
  SMB_SENSOR_TEMP3 = 0x17,
  SMB_SENSOR_TEMP4 = 0x18,
  SMB_SENSOR_TEMP5 = 0x19,
  SMB_SENSOR_TEMP6 = 0x1a,
  SMB_SENSOR_TH3_DIE_TEMP1 = 0x1b,
  SMB_SENSOR_TH3_DIE_TEMP2 = 0x1c,
  /* Sensors on FCM */
  SMB_SENSOR_FCM_TEMP1 = 0x20,
  SMB_SENSOR_FCM_TEMP2 = 0x21,
  SMB_SENSOR_FCM_HSC_VOLT = 0x24,
  SMB_SENSOR_FCM_HSC_CURR = 0x25,
  SMB_SENSOR_FCM_HSC_POWER = 0x26,
  /* Sensors FAN Speed */
  SMB_SENSOR_FAN1_FRONT_TACH = 0x2a,
  SMB_SENSOR_FAN1_REAR_TACH = 0x2b,
  SMB_SENSOR_FAN2_FRONT_TACH = 0x2c,
  SMB_SENSOR_FAN2_REAR_TACH = 0x2d,
  SMB_SENSOR_FAN3_FRONT_TACH = 0x2e,
  SMB_SENSOR_FAN3_REAR_TACH = 0x2f,
  SMB_SENSOR_FAN4_FRONT_TACH = 0x30,
  SMB_SENSOR_FAN4_REAR_TACH = 0x31,
};

/* Sensors on PSU */
enum {
  PSU1_SENSOR_IN_VOLT = 0x01,
  PSU1_SENSOR_12V_VOLT = 0x02,
  PSU1_SENSOR_STBY_VOLT = 0x03,
  PSU1_SENSOR_IN_CURR = 0x04,
  PSU1_SENSOR_12V_CURR = 0x05,
  PSU1_SENSOR_STBY_CURR = 0x06,
  PSU1_SENSOR_IN_POWER = 0x07,
  PSU1_SENSOR_12V_POWER = 0x08,
  PSU1_SENSOR_STBY_POWER = 0x09,
  PSU1_SENSOR_FAN_TACH = 0x0a,
  PSU1_SENSOR_TEMP1 = 0x0b,
  PSU1_SENSOR_TEMP2 = 0x0c,
  PSU1_SENSOR_TEMP3 = 0x0d,
  PSU2_SENSOR_IN_VOLT = 0x0e,
  PSU2_SENSOR_12V_VOLT = 0x0f,
  PSU2_SENSOR_STBY_VOLT = 0x10,
  PSU2_SENSOR_IN_CURR = 0x11,
  PSU2_SENSOR_12V_CURR = 0x12,
  PSU2_SENSOR_STBY_CURR = 0x13,
  PSU2_SENSOR_IN_POWER = 0x14,
  PSU2_SENSOR_12V_POWER = 0x15,
  PSU2_SENSOR_STBY_POWER = 0x16,
  PSU2_SENSOR_FAN_TACH = 0x17,
  PSU2_SENSOR_TEMP1 = 0x18,
  PSU2_SENSOR_TEMP2 = 0x19,
  PSU2_SENSOR_TEMP3 = 0x1a,
};

enum
{
  SLED_CLR_BLUE = 0x3,
  SLED_CLR_YELLOW = 0x4,
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


int pal_handle_oem_1s_intr(uint8_t slot, uint8_t *data);
void pal_inform_bic_mode(uint8_t fru, uint8_t mode);
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
int pal_post_disable(uint8_t slot);
int pal_post_get_last(uint8_t slot, uint8_t *status);
int pal_post_handle(uint8_t slot, uint8_t status);
int pal_set_last_pwr_state(uint8_t fru, char *state);
int pal_get_last_pwr_state(uint8_t fru, char *state);
int pal_set_com_pwr_btn_n(char *status);
int pal_set_server_power(uint8_t slot_id, uint8_t cmd);
int pal_get_server_power(uint8_t slot_id, uint8_t *status);
int pal_get_fan_speed(uint8_t fan, int *rpm);
int pal_get_board_rev(int *rev);
int pal_sensor_read_raw(uint8_t fru, uint8_t sensor_num, void *value);
bool pal_is_fw_update_ongoing(uint8_t fru);
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
int pal_clear_thresh_value(uint8_t fru);
void pal_get_chassis_status(uint8_t slot, uint8_t *req_data, uint8_t *res_data, uint8_t *res_len);
uint8_t pal_set_power_restore_policy(uint8_t slot, uint8_t *pwr_policy, uint8_t *res_data);
int pal_sel_handler(uint8_t fru, uint8_t snr_num, uint8_t *event_data);
void *generate_dump(void *arg);
int pal_mon_fw_upgrade(int brd_rev, uint8_t *sys_ug, uint8_t *fan_ug, uint8_t *psu_ug, uint8_t *smb_ug);
void set_sys_led(int brd_rev);
void set_fan_led(int brd_rev);
void set_psu_led(int brd_rev);
void set_smb_led(int brd_rev);
int set_sled(int brd_rev, uint8_t color, int led_name);
void init_led(void);
int pal_light_scm_led(uint8_t led_color);
int pal_get_fru_health(uint8_t fru, uint8_t *value);
int pal_set_def_key_value(void);
int pal_init_sensor_check(uint8_t fru, uint8_t snr_num, void *snr);
int wedge400_sensor_name(uint8_t fru, uint8_t sensor_num, char *name);
#ifdef __cplusplus
} // extern "C"
#endif

#endif /* __PAL_H__ */
