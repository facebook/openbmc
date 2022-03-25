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
#include <openbmc/log.h>

#ifdef __cplusplus
extern "C" {
#endif

#include <openbmc/ipmi.h>
#include <openbmc/obmc-pal.h>
#include <openbmc/obmc-i2c.h>
#include <stdbool.h>
#include <math.h>
#include <facebook/bic.h>

#include "pal-led.h"
#include "pal-power.h"
#include "pal-sensors.h"
#include "pal-debugcard.h"

#ifdef DEBUG
#define PAL_DEBUG(fmt, args...) OBMC_INFO(fmt, ##args)
#else
#define PAL_DEBUG(fmt, args...)
#endif

#define PLATFORM_NAME "cloudripper"

#define OFFSET_DEV_GUID 0x1800

#define SCM_BRD_ID "6-0021"

#define SYS_PLATFROM_DIR(dev) "/sys/devices/platform/"#dev"/"
#define I2C_DEV_DIR(bus, addr) I2C_SYSFS_DEV_DIR(bus-00##addr/)
#define HW_MON_DIR "hwmon/hwmon*"

#define SCM_CPLD       "scmcpld"
#define SMB_CPLD       "smbcpld"
#define FCM_CPLD       "fcmcpld"
#define PWR_CPLD       "pwrcpld"
#define DOM_FPGA1      "domfpga1"
#define DOM_FPGA2      "domfpga2"

#define SCM_SYSFS        I2C_DEV_DIR(2, 3e)"%s"
#define SMB_SYSFS        I2C_DEV_DIR(12, 3e)"%s"
#define PWR_SYSFS        I2C_DEV_DIR(47, 3e)"%s"
#define FCM_SYSFS        I2C_DEV_DIR(48, 3e)"%s"
#define DOMFPGA1_SYSFS   I2C_DEV_DIR(13, 60)"%s"
#define DOMFPGA2_SYSFS   I2C_DEV_DIR(5, 60)"%s"
#define KV_PATH "/mnt/data/kv_store/%s"

#define SCM_BIC_SDR_PATH "/tmp/sdr_%s.bin"

#define COM_PWR_BTN_N "come_pwr_btn_n"
#define SCM_COM_RST_BTN "iso_com_rst_n"

#define GB_POWER  "gb_turn_on"
#define GB_RESET_BTN "mac_reset_n"

#define SCM_PRSNT_STATUS "scm_present_int_status"
#define SCM_SUS_S3_STATUS "iso_com_sus_s3_n"
#define SCM_SUS_S4_STATUS "iso_com_sus_s4_n"
#define SCM_SUS_S5_STATUS "iso_com_sus_s5_n"
#define SCM_SUS_STAT_STATUS "iso_com_sus_stat_n"
#define FAN_PRSNT_STATUS "fan%d_present"
#define PSU_PRSNT_STATUS "psu_present_%d_N_int_status"

#define CRASHDUMP_BIN    "/usr/local/bin/autodump.sh"

#define LAST_KEY "last_key"

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
#define BMC_UART_SEL        I2C_DEV_DIR(12, 3e)"uart_selection"

#define MAX_NUM_SCM 1
#define MAX_NUM_FAN 4
#define MAX_RETRIES_SDR_INIT  30
#define MAX_READ_RETRY 3
#define DELAY_POWER_BTN 2
#define DELAY_POWER_CYCLE 5

#define MAX_POS_READING_MARGIN 127
#define LARGEST_DEVICE_NAME 128
#define MAX_NUM_SLOTS 1

#define IPMB_BUS 0

#define BMC_READY_N   28
#define BIC_SENSOR_READ_NA 0x20
#define THERMAL_CONSTANT   256
#define ERR_NOT_READY   -2
#define MAX_NODES     1
// #define MAX_NUM_FRUS    6

extern const char pal_fru_list[];

enum {
  BOARD_TYPE_CLOUDRIPPER,
};

enum {
  BOARD_CLOUDRIPPER,
  BOARD_UNDEFINED = 0xFF,
};

enum {
  BIC_MODE_NORMAL = 0x01,
  BIC_MODE_UPDATE = 0x0F,
};

enum {
  GB_POWER_ON,
  GB_POWER_OFF,
  GB_RESET,
};

enum {
  FRU_ALL,
  FRU_SCM,
  FRU_SMB,
  FRU_PSU1,
  FRU_PSU2,
  FRU_FAN1,
  FRU_FAN2,
  FRU_FAN3,
  FRU_FAN4,
  // MAX_NUM_FRUS should be equal to last physical FRU ID
  MAX_NUM_FRUS = FRU_FAN4,
  // virtual FRU ID for fw-util 0.2, make sure they are bigger than MAX_NUM_FRUS
  FRU_BMC,
  FRU_CPLD,
  FRU_FPGA,
};

enum {
  HSC_FCM = 0,
};

/* Add button function for Debug Card */
enum {
  HAND_SW_SERVER = 0,
  HAND_SW_BMC = 1
};

int pal_is_debug_card_prsnt(uint8_t *status);
int pal_get_cpld_fpga_fw_ver(uint8_t fru, const char *device, uint8_t* ver);
int pal_get_board_rev(int *rev);
int pal_get_board_type(uint8_t *brd_type);
int pal_get_board_type_rev(uint8_t *brd_type_rev);
int pal_get_cpld_board_rev(int *rev, const char *device);
void *generate_dump(void *arg);
int pal_mon_fw_upgrade(uint8_t *status_ug);
bool pal_is_mcu_working(void);

#ifdef __cplusplus
} // extern "C"
#endif

#endif /* __PAL_H__ */
