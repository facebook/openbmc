/*
 *
 * Copyright 2020-present Facebook. All Rights Reserved.
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
#include <openbmc/kv.h>
#include "pal_sensors.h"
#include "pal_power.h"
#include "pal-pim.h"
#include "pal_debugcard.h"

#ifdef __cplusplus
extern "C" {
#endif

#include <openbmc/ipmi.h>
#include <stdbool.h>
#include <math.h>
#include <facebook/bic.h>

#define PLATFORM_NAME "fuji"
#define READ_UNIT_SENSOR_TIMEOUT 5

#define SYS_PLATFROM_DIR(dev) "/sys/devices/platform/"#dev"/"
#define I2C_DRIVER_DIR(name, bus, addr) "/sys/bus/i2c/drivers/"#name"/"#bus"-00"#addr"/"
#define I2C_DEV_DIR(bus, addr) "/sys/bus/i2c/devices/"#bus"-00"#addr"/"
#define HW_MON_DIR "hwmon/hwmon*"

#define SCM_CPLD       "scmcpld"
#define SMB_CPLD       "smbcpld"
#define FCM_CPLD_T     "fcmcpld_t"
#define FCM_CPLD_B     "fcmcpld_b"
#define PWR_CPLD_L     "pwrcpld_l"
#define PWR_CPLD_R     "pwrcpld_r"
#define PIM1_DOM_FPGA  "pim1_domfpga"
#define PIM2_DOM_FPGA  "pim2_domfpga"
#define PIM3_DOM_FPGA  "pim3_domfpga"
#define PIM4_DOM_FPGA  "pim4_domfpga"
#define PIM5_DOM_FPGA  "pim5_domfpga"
#define PIM6_DOM_FPGA  "pim6_domfpga"
#define PIM7_DOM_FPGA  "pim7_domfpga"
#define PIM8_DOM_FPGA  "pim8_domfpga"
#define IOB_FPGA  "iob_fpga"

#define GPIO_VAL "/sys/class/gpio/gpio%d/value"
#define GPIO_DIR "/sys/class/gpio/gpio%d/direction"
#define SCM_SYSFS        I2C_DEV_DIR(2, 35)
#define SMBCPLD_PATH_FMT I2C_SYSFS_DEV_DIR(12-003e)"/%s"
#define TOP_FCMCPLD_PATH_FMT I2C_SYSFS_DEV_DIR(64-0033)"/%s"
#define BOTTOM_FCMCPLD_PATH_FMT I2C_SYSFS_DEV_DIR(72-0033)"/%s"
#define LEFT_PDBCPLD_PATH_FMT I2C_SYSFS_DEV_DIR(55-0060)"/%s"
#define RIGHT_PDBCPLD_PATH_FMT I2C_SYSFS_DEV_DIR(63-0060)"/%s"
#define IOBFPGA_PATH_FMT I2C_SYSFS_DEV_DIR(13-0035)"/%s"
#define SMB_SYSFS        I2C_DEV_DIR(12, 3e)
#define PWR_L_SYSFS_DVT  I2C_DEV_DIR(55, 60)
#define PWR_R_SYSFS_DVT  I2C_DEV_DIR(63, 60)
#define PWR_L_SYSFS_EVT  I2C_DEV_DIR(53, 60)
#define PWR_R_SYSFS_EVT  I2C_DEV_DIR(61, 60)
#define FCM_T_SYSFS      I2C_DEV_DIR(64, 33)
#define FCM_B_SYSFS      I2C_DEV_DIR(72, 33)
#define PIM1_DOMFPGA_SYSFS   I2C_DEV_DIR(80, 60)
#define PIM2_DOMFPGA_SYSFS   I2C_DEV_DIR(88, 60)
#define PIM3_DOMFPGA_SYSFS   I2C_DEV_DIR(96, 60)
#define PIM4_DOMFPGA_SYSFS   I2C_DEV_DIR(104, 60)
#define PIM5_DOMFPGA_SYSFS   I2C_DEV_DIR(112, 60)
#define PIM6_DOMFPGA_SYSFS   I2C_DEV_DIR(120, 60)
#define PIM7_DOMFPGA_SYSFS   I2C_DEV_DIR(128, 60)
#define PIM8_DOMFPGA_SYSFS   I2C_DEV_DIR(136, 60)
#define IOBFPGA_SYSFS   I2C_DEV_DIR(13, 35)
#define SENSORD_FILE_SMB "/tmp/cache_store/smb_sensor%d"
#define SENSORD_FILE_PSU "/tmp/cache_store/psu%d_sensor%d"
#define KV_PATH "/mnt/data/kv_store/%s"

enum {
  KEY_PWRSEQ,
  KEY_PWRSEQ1,
  KEY_PWRSEQ2,
  KEY_HSC,
  KEY_FCMT_HSC,
  KEY_FCMB_HSC,
};

#define KEY_PWRSEQ1_ADDR  "pwrseq_1_addr"
#define KEY_PWRSEQ2_ADDR  "pwrseq_2_addr"
#define KEY_PWRSEQ_ADDR   "pwrseq_addr"
#define KEY_HSC_ADDR      "hsc_addr"
#define KEY_FCMT_HSC_ADDR "fcm_t_hsc_addr"
#define KEY_FCMB_HSC_ADDR "fcm_b_hsc_addr"

#define FUJI_FRU_PATH "/tmp/fruid_%s.bin"

#define SCM_SYS_LED_COLOR    I2C_SYSFS_DEV_ENTRY(2-0035, sys_led_color)
#define BMC_UART_SEL         I2C_SYSFS_DEV_ENTRY(12-003e, uart_selection)

#define DEBUGCARD_PRSNT_STATUS "debugcard_present"
#define SCM_PRSNT_STATUS "scm_present"
#define PIM_PRSNT_STATUS "pim%d_present_L"
#define FAN_PRSNT_STATUS "fan%d_present"
#define PSU_PRSNT_STATUS "psu_prnst_%d_N_status"
#define PEM_PRSNT_STATUS PSU_PRSNT_STATUS
#define KV_PIM_HEALTH "pim%d_sensor_health"
#define CRASHDUMP_BIN       "/usr/local/bin/autodump.sh"

#define LAST_KEY "last_key"

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
#define GPIO_BMC_UART_SEL5  "/tmp/gpionames/BMC_UART_SEL5/%s"

#define MAX_READ_RETRY 10

#define MAX_POS_READING_MARGIN 127
#define LARGEST_DEVICE_NAME 128
#define FUJI_MAX_NUM_SLOTS 1

#define READING_SKIP 1

#define BMC_READY_N   28
#define BOARD_REV_EVTA 4
#define BOARD_REV_EVTB 0
#define BIC_SENSOR_READ_NA 0x20
#define MAX_NODES     1

#define MAX_SDR_LEN     64
#define MAX_PIM 8

#define I2C_ADDR_SIM_LED 0x20
#define I2C_ADDR_PIM16Q 0x60
#define I2C_ADDR_PIM4DD 0x61

#define PFR_MAILBOX_BUS (10)
#define PFR_MAILBOX_ADDR (0x70)
#define PFR_UPDATE_ADDR (0x74)

#define FPGA_STS_CLR_BLUE 0x01
#define FPGA_STS_CLR_YELLOW 0x05
#define IPMB_BUS 0
#define ERR_NOT_READY   -2
#define MAX_SENSOR_NUM  0xFF
#define MAX_SDR_THRESH_RETRY 30
#define THERMAL_CONSTANT   256
#define BIC_SENSOR_READ_NA 0x20
#define BIC_SDR_PATH "/tmp/sdr_%s.bin"
#define SCM_INIT_THRESH_STATUS "scm_init_thresh-status"

extern const char pal_fru_list[];

enum {
  FRU_ALL = 0,
  FRU_SCM = 1,
  FRU_SMB = 2,
  FRU_PIM1 = 3,
  FRU_PIM2 = 4,
  FRU_PIM3 = 5,
  FRU_PIM4 = 6,
  FRU_PIM5 = 7,
  FRU_PIM6 = 8,
  FRU_PIM7 = 9,
  FRU_PIM8 = 10,
  FRU_PSU1 = 11,
  FRU_PSU2 = 12,
  FRU_PSU3 = 13,
  FRU_PSU4 = 14,
  MAX_NUM_FRUS = 14,
  FRU_PEM1 = 15,
  FRU_PEM2 = 16,
  FRU_PEM3 = 17,
  FRU_PEM4 = 18,
  FRU_FAN1 = 19,
  FRU_FAN2 = 20,
  FRU_FAN3 = 21,
  FRU_FAN4 = 22,
  FRU_FAN5 = 23,
  FRU_FAN6 = 24,
  FRU_FAN7 = 25,
  FRU_FAN8 = 26,
    // virtual FRU ID for fw-util 0.2, make sure they are bigger than MAX_NUM_FRUS
  FRU_BMC,
  FRU_CPLD,
  FRU_FPGA,
};

enum {
  HSC_FCM_T = 0,
  HSC_FCM_B = 1
};

enum
{
  PSU_ACOK_DOWN = 0,
  PSU_ACOK_UP = 1
};

enum {
  SIM_LED_OFF = 0,
  SIM_LED_BLUE,
  SIM_LED_AMBER,
  SIM_LED_ALT_BLINK,
  SIM_LED_AMBER_BLINK,
};

enum
{
  SLED_COLOR_OFF = 0,
  SLED_COLOR_BLUE,
  SLED_COLOR_GREEN,
  SLED_COLOR_AMBER,
  SLED_COLOR_RED,
  SLED_COLOR_MAX,
};

enum
{
  SLED_NAME_SYS = 0,
  SLED_NAME_FAN,
  SLED_NAME_PSU,
  SLED_NAME_SMB,
  SLED_NAME_MAX,
};

enum
{
  SCM_LED_BLUE = 0x01,
  SCM_LED_AMBER = 0x05,
};

enum {
  PIM_TYPE_UNPLUG = 0,
  PIM_TYPE_16Q = 1,
  PIM_TYPE_16O = 2,
  PIM_TYPE_NONE = 4
};

enum {
  PIM_16O_SIMULATE = 0,
  PIM_16O_ALPHA1 = 1,
  PIM_16O_ALPHA2 = 2,  
  PIM_16O_ALPHA3 = 3,
  PIM_16O_ALPHA4 = 4,
  PIM_16O_ALPHA5 = 5,
  PIM_16O_ALPHA6 = 6,
  PIM_16O_BETA = 8,
  PIM_16O_NONE_VERSION = -1,
};

enum {
  MODE_AUTO	= 0,
  MODE_QUICK = 1,
  MODE_READ = 2
};

enum {
  I2C_BUS_ERROR	= -1,
  I2C_FUNC_ERROR	= 1,
  I2C_DEVICE_ERROR = 2,
  I2c_DRIVER_EXIST = 3
};

enum {
  BIC_MODE_NORMAL = 0x01,
  BIC_MODE_UPDATE = 0x0F,
};

enum {
  HAND_SW_SERVER = 0,
  HAND_SW_BMC = 1
};

enum {
  BOARD_FUJI_EVT1       = 0x40,
  BOARD_FUJI_EVT2       = 0x41,
  BOARD_FUJI_EVT3       = 0x42,
  BOARD_FUJI_DVT1       = 0x43,
};

#define PIM_I2C_DEVICE_NUMBER 11
#define SCM_I2C_DEVICE_NUMBER 8

struct dev_bus_addr {
  uint8_t bus_id;
  uint8_t device_address;
};

struct dev_addr_driver {
  uint8_t addr;
  const char driver[32];
  const char chip_name[32];
};

extern sensor_info_t g_sinfo[MAX_NUM_FRUS][MAX_SENSOR_NUM + 1];
extern struct dev_addr_driver PIM_UCD_addr_list[5];
extern struct dev_addr_driver PIM_HSC_addr_list[2];
extern struct dev_addr_driver SCM_HSC_addr_list[2];

int pal_post_enable(uint8_t slot);
int pal_post_disable(uint8_t slot);
int pal_post_get_last(uint8_t slot, uint8_t *status);
int pal_set_com_pwr_btn_n(char *status);
int pal_get_board_rev(int *rev);
int pal_mon_fw_upgrade(int brd_rev, uint8_t *status);
int pal_get_pim_type(uint8_t fru, int retry);
int pal_set_pim_type_to_file(uint8_t fru, uint8_t type);
int pal_get_pim_type_from_file(uint8_t fru);
int pal_get_pim_pedigree(uint8_t fru, int retry);
int pal_set_pim_pedigree_to_file(uint8_t fru, uint8_t type);
int pal_get_pim_pedigree_from_file(uint8_t fru);
int pal_get_pim_phy_type(uint8_t fru, int retry);
int pal_set_pim_phy_type_to_file(uint8_t fru, uint8_t type);
int pal_get_pim_phy_type_from_file(uint8_t fru);
int pal_set_pim_thresh(uint8_t fru);

bool is_scm_i2c_mux_binded();
int scm_i2c_mux_bind();
int scm_i2c_mux_unbind();
bool is_pim_i2c_mux_binded(int pim);
int pim_i2c_mux_bind(int pim);
int pim_i2c_mux_unbind(int pim);
int pal_add_i2c_device(uint8_t bus, uint8_t addr, char *device_name);
int pal_del_i2c_device(uint8_t bus, uint8_t addr);

struct dev_addr_driver* pal_get_pim_ucd(uint8_t fru);
struct dev_addr_driver* pal_get_pim_hsc(uint8_t fru);
struct dev_addr_driver* pal_get_scm_hsc();

int pal_set_dev_addr_to_file(uint8_t fru, uint8_t dev, uint8_t addr);
int pal_get_dev_addr_from_file(uint8_t fru, uint8_t dev);

int pal_clear_thresh_value(uint8_t fru);
void *generate_dump(void *arg);
int set_sled(int brd_rev, uint8_t color, uint8_t led_name);
void init_led(void);
int pal_light_scm_led(uint8_t led_color);
void pal_set_pim_sts_led(uint8_t fru);
int pal_get_cpld_fpga_fw_ver(uint8_t fru, const char *device, uint8_t* ver);
int pal_get_cpld_board_rev(int *rev, const char *device);
int pal_get_board_id(uint8_t slot, uint8_t *req_data, uint8_t req_len, uint8_t *res_data, uint8_t *res_len);
int pal_sensor_discrete_read_raw(uint8_t fru, uint8_t sensor_num, void *value);
#ifdef __cplusplus
} // extern "C"
#endif

#endif /* __PAL_H__ */
