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
#include <openbmc/kv.h>

#ifdef __cplusplus
extern "C" {
#endif

#include <openbmc/ipmi.h>
#include <openbmc/obmc-pal.h>
#include <stdbool.h>
#include <math.h>
#include <facebook/bic.h>

#define PLATFORM_NAME "wedge400"
#define READ_UNIT_SENSOR_TIMEOUT 5

#define SYS_PLATFROM_DIR(dev) "/sys/devices/platform/"#dev"/"
#define I2C_DRIVER_DIR(name, bus, addr) "/sys/bus/i2c/drivers/"#name"/"#bus"-00"#addr"/"
#define I2C_DEV_DIR(bus, addr) "/sys/bus/i2c/devices/i2c-"#bus"/"#bus"-00"#addr"/"
#define HW_MON_DIR "hwmon/hwmon*"

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
#define PWR_SYSFS        I2C_DEV_DIR(31, 3e)"%s"
#define FCM_SYSFS        I2C_DEV_DIR(32, 3e)"%s"
#define DOMFPGA1_SYSFS   I2C_DEV_DIR(13, 60)"%s"
#define DOMFPGA2_SYSFS   I2C_DEV_DIR(5, 60)"%s"
#define SENSORD_FILE_SMB "/tmp/cache_store/smb_sensor%d"
#define SENSORD_FILE_PSU "/tmp/cache_store/psu%d_sensor%d"
#define SENSORD_FILE_PEM "/tmp/cache_store/pem%d_sensor%d"
#define KV_PATH "/mnt/data/kv_store/%s"

#define WEDGE400_SDR_PATH "/tmp/sdr_%s.bin"

#define COM_PWR_BTN_N "come_pwr_btn_n"
#define SYS_LED_COLOR "sys_led_color"
#define SCM_COM_RST_BTN "iso_com_rst_n"

#define GB_POWER  "gb_turn_on"
#define TH3_POWER "th3_turn_on"
#define TH3_RESET_BTN "mac_reset_n"

#define SMB_CPLD_TH3_ROV "th3_rov%d"
#define SMB_CPLD_TH3_ROV_NUM 8
#define SMB_CPLD_GB_ROV "gb_rov%d"
#define SMB_CPLD_GB_ROV_NUM SMB_CPLD_TH3_ROV_NUM

#define SCM_PRSNT_STATUS "scm_present_int_status"
#define SCM_SUS_S3_STATUS "iso_com_sus_s3_n"
#define SCM_SUS_S4_STATUS "iso_com_sus_s4_n"
#define SCM_SUS_S5_STATUS "iso_com_sus_s5_n"
#define SCM_SUS_STAT_STATUS "iso_com_sus_stat_n"
#define FAN_PRSNT_STATUS "fan%d_present"
#define PSU_PRSNT_STATUS "psu_present_%d_N_int_status"
#define PEM_PRSNT_STATUS PSU_PRSNT_STATUS

#define CRASHDUMP_BIN    "/usr/local/bin/autodump.sh"

#define LAST_KEY "last_key"

// SCM Sensor Devices
#define SCM_HSC_ADM1278_DIR           I2C_DRIVER_DIR(adm1275, 16, 10)
#define SCM_HSC_LM25066_DIR           I2C_DRIVER_DIR(lm25066, 16, 44)

#define SCM_HSC_ADM1278_DEVICE        I2C_DEV_DIR(16, 10)HW_MON_DIR
#define SCM_HSC_LM25066_DEVICE        I2C_DEV_DIR(16, 44)HW_MON_DIR
#define SCM_OUTLET_TEMP_DEVICE I2C_DEV_DIR(17, 4d)HW_MON_DIR
#define SCM_INLET_TEMP_DEVICE  I2C_DEV_DIR(17, 4c)HW_MON_DIR

// SMB Sensor Devices
#define SMB_FCM_HSC_ADM1278_DIR       I2C_DRIVER_DIR(adm1275, 35, 10)
#define SMB_FCM_HSC_LM25066_DIR       I2C_DRIVER_DIR(lm25066, 35, 44)

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
#define SMB_FCM_TACH_DEVICE           I2C_DEV_DIR(32, 3e)
#define SMB_FCM_LM75B_U1_DEVICE       I2C_DEV_DIR(34, 48)HW_MON_DIR
#define SMB_FCM_LM75B_U2_DEVICE       I2C_DEV_DIR(34, 49)HW_MON_DIR
#define SMB_FCM_HSC_ADM1278_DEVICE    I2C_DEV_DIR(35, 10)HW_MON_DIR
#define SMB_FCM_HSC_LM25066_DEVICE    I2C_DEV_DIR(35, 44)HW_MON_DIR

//BMC Sensor Devices
#define AST_ADC_DEVICE         SYS_PLATFROM_DIR("ast-adc-hwmon")HW_MON_DIR

// PSU Sensor Devices
#define PSU_DRIVER             "psu_driver"
#define PSU1_DEVICE            I2C_DEV_DIR(24, 58)
#define PSU2_DEVICE            I2C_DEV_DIR(25, 58)

#define PEM1_LTC4282_DIR       I2C_DRIVER_DIR(ltc4282, 24, 58)
#define PEM2_LTC4282_DIR       I2C_DRIVER_DIR(ltc4282, 25, 58)
#define PEM1_MAX6615_DIR       I2C_DRIVER_DIR(max6615, 24, 18)
#define PEM2_MAX6615_DIR       I2C_DRIVER_DIR(max6615, 25, 18)
#define PEM_LTC4282_DRIVER     "ltc4282"
#define PEM_MAX6615_DRIVER     "max6615"
#define PEM1_DEVICE            I2C_DEV_DIR(24, 58)HW_MON_DIR
#define PEM2_DEVICE            I2C_DEV_DIR(25, 58)HW_MON_DIR
#define PEM1_DEVICE_EXT        I2C_DEV_DIR(24, 18)HW_MON_DIR
#define PEM2_DEVICE_EXT        I2C_DEV_DIR(25, 18)HW_MON_DIR

#define SMB_TMP421_1_DEVICE    I2C_DEV_DIR(30, 4d)HW_MON_DIR
#define SMB_TMP421_2_DEVICE    I2C_DEV_DIR(28, 4d)HW_MON_DIR
#define SMB_TMP421_3_DEVICE    I2C_DEV_DIR(27, 4d)HW_MON_DIR
#define SMB_TMP421_4_DEVICE    I2C_DEV_DIR(26, 4d)HW_MON_DIR
#define SMB_TMP421_1_DIR       I2C_DRIVER_DIR(tmp421, 30, 4d)
#define SMB_TMP421_2_DIR       I2C_DRIVER_DIR(tmp421, 28, 4d)
#define SMB_TMP421_3_DIR       I2C_DRIVER_DIR(tmp421, 27, 4d)
#define SMB_TMP421_4_DIR       I2C_DRIVER_DIR(tmp421, 26, 4d)
#define SMB_ADM1032_1_DEVICE   I2C_DEV_DIR(30, 4c)HW_MON_DIR
#define SMB_ADM1032_2_DEVICE   I2C_DEV_DIR(28, 4c)HW_MON_DIR
#define SMB_ADM1032_3_DEVICE   I2C_DEV_DIR(27, 4c)HW_MON_DIR
#define SMB_ADM1032_4_DEVICE   I2C_DEV_DIR(26, 4c)HW_MON_DIR
#define SMB_ADM1032_1_DIR      I2C_DRIVER_DIR(lm90, 30, 4c)
#define SMB_ADM1032_2_DIR      I2C_DRIVER_DIR(lm90, 28, 4c)
#define SMB_ADM1032_3_DIR      I2C_DRIVER_DIR(lm90, 27, 4c)
#define SMB_ADM1032_4_DIR      I2C_DRIVER_DIR(lm90, 26, 4c)

#define TEMP(x)                "temp"#x"_input"
#define VOLT(x)                "in"#x"_input"
#define VOLT_SET(x)            "vo"#x"_input"
#define CURR(x)                "curr"#x"_input"
#define POWER(x)               "power"#x"_input"

#define GPIO_SMB_REV_ID_0   "/tmp/gpionames/BMC_CPLD_BOARD_REV_ID0/%s"
#define GPIO_SMB_REV_ID_1   "/tmp/gpionames/BMC_CPLD_BOARD_REV_ID1/%s"
#define GPIO_SMB_REV_ID_2   "/tmp/gpionames/BMC_CPLD_BOARD_REV_ID2/%s"
#define GPIO_BMC_BRD_TYPE_0   "/tmp/gpionames/BMC_CPLD_BOARD_TYPE_0/%s"
#define GPIO_BMC_BRD_TYPE_1   "/tmp/gpionames/BMC_CPLD_BOARD_TYPE_1/%s"
#define GPIO_BMC_BRD_TYPE_2   "/tmp/gpionames/BMC_CPLD_BOARD_TYPE_2/%s"
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
#define WEDGE400_MAX_NUM_SLOTS 1

#define UNIT_DIV 1000

#define READING_SKIP 1
#define READING_NA -2

#define SCM_RSENSE 1

#define IPMB_BUS 0

#define BMC_READY_N   28
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
  BRD_TYPE_WEDGE400 = 0x00,
  BRD_TYPE_WEDGE400C = 0x01,
};

enum {
  BOARD_WEDGE400_EVT_EVT3       = 0x00,
  BOARD_WEDGE400_DVT            = 0x02,
  BOARD_WEDGE400_DVT2_PVT_PVT2  = 0x03,
  BOARD_WEDGE400_PVT3           = 0x04,
  BOARD_WEDGE400_MP             = 0x05,
  BOARD_WEDGE400_MP_RESPIN      = 0x06,
  BOARD_WEDGE400C_EVT           = 0x10,
  BOARD_WEDGE400C_EVT2          = 0x11,
  BOARD_WEDGE400C_DVT           = 0x12,
  BOARD_WEDGE400C_DVT2          = 0x13,
  BOARD_WEDGE400C_MP_RESPIN     = 0x14,
  BOARD_UNDEFINED               = 0xFF,
};

enum {
  BIC_MODE_NORMAL = 0x01,
  BIC_MODE_UPDATE = 0x0F,
};

enum {
  TH3_POWER_ON,
  TH3_POWER_OFF,
  TH3_RESET,
};

enum {
  PEM_ON,
  PEM_REBOOT,
  PEM_OFF,
};

enum {
  FRU_ALL  = 0,
  FRU_SCM,
  FRU_SMB,
  FRU_PEM1,
  FRU_PEM2,
  FRU_PSU1,
  FRU_PSU2,
  FRU_FAN1,
  FRU_FAN2,
  FRU_FAN3,
  FRU_FAN4,
  MAX_NUM_FRUS = FRU_FAN4,
  // virtual FRU ID for fw-util 0.2, make sure they are bigger than MAX_NUM_FRUS
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
  SCM_SENSOR_HSC_CURR = 0x0b,
  /* Threshold Sensors on COM-e */
  BIC_SENSOR_MB_OUTLET_TEMP = 0x01,
  BIC_SENSOR_MB_INLET_TEMP = 0x07,
  BIC_SENSOR_PCH_TEMP = 0x08,
  BIC_SENSOR_VCCIN_VR_TEMP = 0x80,
  BIC_SENSOR_1V05COMB_VR_TEMP = 0x81,
  BIC_SENSOR_SOC_TEMP = 0x05,
  BIC_SENSOR_SOC_THERM_MARGIN = 0x09,
  BIC_SENSOR_VDDR_VR_TEMP = 0x82,
  BIC_SENSOR_SOC_DIMMA_TEMP = 0xB4,
  BIC_SENSOR_SOC_DIMMB_TEMP = 0xB6,
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
  BIC_SENSOR_P1V05_COMB = 0x8E,
  BIC_SENSOR_1V05COMB_VR_CURR = 0x84,
  BIC_SENSOR_VDDR_VR_CURR = 0x85,
  BIC_SENSOR_VCCIN_VR_CURR = 0x83,
  BIC_SENSOR_VCCIN_VR_VOL = 0x88,
  BIC_SENSOR_VDDR_VR_VOL = 0x8A,
  BIC_SENSOR_P1V05COMB_VR_VOL = 0x89,
  BIC_SENSOR_P1V05COMB_VR_POUT = 0x8C,
  BIC_SENSOR_INA230_POWER = 0x29,
  BIC_SENSOR_INA230_VOL = 0x2A,
  /* Discrete sensors on COM-e*/
  BIC_SENSOR_SYSTEM_STATUS = 0x10, //Discrete
  BIC_SENSOR_PROC_FAIL = 0x65, //Discrete
  BIC_SENSOR_SYS_BOOT_STAT = 0x7E, //Discrete
  BIC_SENSOR_VR_HOT = 0xB2, //Discrete
  BIC_SENSOR_CPU_DIMM_HOT = 0xB3, //Discrete None in Spec
  /* Event-only sensors on COM-e */
  BIC_SENSOR_SPS_FW_HLTH = 0x17, //Event-only
  BIC_SENSOR_POST_ERR = 0x2B, //Event-only
  BIC_SENSOR_POWER_THRESH_EVENT = 0x3B, //Event-only
  BIC_SENSOR_MACHINE_CHK_ERR = 0x40, //Event-only
  BIC_SENSOR_PCIE_ERR = 0x41, //Event-only
  BIC_SENSOR_OTHER_IIO_ERR = 0x43, //Event-only
  BIC_SENSOR_PROC_HOT_EXT = 0x51, //Event-only None in Spec
  BIC_SENSOR_POWER_ERR = 0x56, //Event-only
  BIC_SENSOR_MEM_ECC_ERR = 0x63, //Event-only
  BIC_SENSOR_CAT_ERR = 0xEB, //Event-only
};

/* Sensors on FAN */
enum {
  FAN_SENSOR_FAN1_FRONT_TACH = 0x40,
  FAN_SENSOR_FAN1_REAR_TACH,
  FAN_SENSOR_FAN2_FRONT_TACH,
  FAN_SENSOR_FAN2_REAR_TACH,
  FAN_SENSOR_FAN3_FRONT_TACH,
  FAN_SENSOR_FAN3_REAR_TACH,
  FAN_SENSOR_FAN4_FRONT_TACH,
  FAN_SENSOR_FAN4_REAR_TACH,
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
  /* Sensors on FCM */
  SMB_SENSOR_FCM_LM75B_U1_TEMP,
  SMB_SENSOR_FCM_LM75B_U2_TEMP,
  SMB_SENSOR_FCM_HSC_VOLT,
  SMB_SENSOR_FCM_HSC_CURR,
  /* Sensors on BMC*/
  SMB_BMC_ADC0_VSEN = 0x48,
  SMB_BMC_ADC1_VSEN,
  SMB_BMC_ADC2_VSEN,
  SMB_BMC_ADC3_VSEN,
  SMB_BMC_ADC4_VSEN,
  SMB_BMC_ADC5_VSEN,
  SMB_BMC_ADC6_VSEN,
  SMB_BMC_ADC7_VSEN,
  SMB_BMC_ADC8_VSEN,
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
  SMB_SENSOR_VDDCK_1_OUT_CURR,
  SMB_SENSOR_VDDCK_1_OUT_POWER,
  SMB_SENSOR_VDDCK_1_TEMP,
  /* Wedge400C only: 10 internal temp sensors within GB switch */
  SMB_SENSOR_GB_TEMP1,
  SMB_SENSOR_GB_TEMP2,
  SMB_SENSOR_GB_TEMP3,
  SMB_SENSOR_GB_TEMP4,
  SMB_SENSOR_GB_TEMP5,
  SMB_SENSOR_GB_TEMP6,
  SMB_SENSOR_GB_TEMP7,
  SMB_SENSOR_GB_TEMP8,
  SMB_SENSOR_GB_TEMP9,
  SMB_SENSOR_GB_TEMP10,
};

/* Sensors on PEM */
enum {
  /* Threshold Sensors on PEM1 */
  PEM1_SENSOR_IN_VOLT = 0x01,
  PEM1_SENSOR_OUT_VOLT,
  PEM1_SENSOR_FET_BAD,
  PEM1_SENSOR_FET_SHORT,
  PEM1_SENSOR_CURR,
  PEM1_SENSOR_POWER,
  PEM1_SENSOR_FAN1_TACH,
  PEM1_SENSOR_FAN2_TACH,
  PEM1_SENSOR_TEMP1,
  PEM1_SENSOR_TEMP2,
  PEM1_SENSOR_TEMP3,
  /* Discrete fault sensors on PEM1 */
  PEM1_SENSOR_FAULT_OV,
  PEM1_SENSOR_FAULT_UV,
  PEM1_SENSOR_FAULT_OC,
  PEM1_SENSOR_FAULT_POWER,
  PEM1_SENSOR_ON_FAULT,
  PEM1_SENSOR_FAULT_FET_SHORT,
  PEM1_SENSOR_FAULT_FET_BAD,
  PEM1_SENSOR_EEPROM_DONE,
  /* Discrete ADC alert sensors on PEM1 */
  PEM1_SENSOR_POWER_ALARM_HIGH,
  PEM1_SENSOR_POWER_ALARM_LOW,
  PEM1_SENSOR_VSENSE_ALARM_HIGH,
  PEM1_SENSOR_VSENSE_ALARM_LOW,
  PEM1_SENSOR_VSOURCE_ALARM_HIGH,
  PEM1_SENSOR_VSOURCE_ALARM_LOW,
  PEM1_SENSOR_GPIO_ALARM_HIGH,
  PEM1_SENSOR_GPIO_ALARM_LOW,
  /* Discrete status sensors on PEM1 */
  PEM1_SENSOR_ON_STATUS,
  PEM1_SENSOR_STATUS_FET_BAD,
  PEM1_SENSOR_STATUS_FET_SHORT,
  PEM1_SENSOR_STATUS_ON_PIN,
  PEM1_SENSOR_STATUS_POWER_GOOD,
  PEM1_SENSOR_STATUS_OC,
  PEM1_SENSOR_STATUS_UV,
  PEM1_SENSOR_STATUS_OV,
  PEM1_SENSOR_STATUS_GPIO3,
  PEM1_SENSOR_STATUS_GPIO2,
  PEM1_SENSOR_STATUS_GPIO1,
  PEM1_SENSOR_STATUS_ALERT,
  PEM1_SENSOR_STATUS_EEPROM_BUSY,
  PEM1_SENSOR_STATUS_ADC_IDLE,
  PEM1_SENSOR_STATUS_TICKER_OVERFLOW,
  PEM1_SENSOR_STATUS_METER_OVERFLOW,
  PEM1_SENSOR_CNT = PEM1_SENSOR_STATUS_METER_OVERFLOW,

    /* Threshold Sensors on PEM2 */
  PEM2_SENSOR_IN_VOLT,
  PEM2_SENSOR_OUT_VOLT,
  PEM2_SENSOR_FET_BAD,
  PEM2_SENSOR_FET_SHORT,
  PEM2_SENSOR_CURR,
  PEM2_SENSOR_POWER,
  PEM2_SENSOR_FAN1_TACH,
  PEM2_SENSOR_FAN2_TACH,
  PEM2_SENSOR_TEMP1,
  PEM2_SENSOR_TEMP2,
  PEM2_SENSOR_TEMP3,
  /* Discrete fault sensors on PEM2 */
  PEM2_SENSOR_FAULT_OV,
  PEM2_SENSOR_FAULT_UV,
  PEM2_SENSOR_FAULT_OC,
  PEM2_SENSOR_FAULT_POWER,
  PEM2_SENSOR_ON_FAULT,
  PEM2_SENSOR_FAULT_FET_SHORT,
  PEM2_SENSOR_FAULT_FET_BAD,
  PEM2_SENSOR_EEPROM_DONE,
  /* Discrete ADC alert sensors on PEM2 */
  PEM2_SENSOR_POWER_ALARM_HIGH,
  PEM2_SENSOR_POWER_ALARM_LOW,
  PEM2_SENSOR_VSENSE_ALARM_HIGH,
  PEM2_SENSOR_VSENSE_ALARM_LOW,
  PEM2_SENSOR_VSOURCE_ALARM_HIGH,
  PEM2_SENSOR_VSOURCE_ALARM_LOW,
  PEM2_SENSOR_GPIO_ALARM_HIGH,
  PEM2_SENSOR_GPIO_ALARM_LOW,
  /* Discrete status sensors on PEM2 */
  PEM2_SENSOR_ON_STATUS,
  PEM2_SENSOR_STATUS_FET_BAD,
  PEM2_SENSOR_STATUS_FET_SHORT,
  PEM2_SENSOR_STATUS_ON_PIN,
  PEM2_SENSOR_STATUS_POWER_GOOD,
  PEM2_SENSOR_STATUS_OC,
  PEM2_SENSOR_STATUS_UV,
  PEM2_SENSOR_STATUS_OV,
  PEM2_SENSOR_STATUS_GPIO3,
  PEM2_SENSOR_STATUS_GPIO2,
  PEM2_SENSOR_STATUS_GPIO1,
  PEM2_SENSOR_STATUS_ALERT,
  PEM2_SENSOR_STATUS_EEPROM_BUSY,
  PEM2_SENSOR_STATUS_ADC_IDLE,
  PEM2_SENSOR_STATUS_TICKER_OVERFLOW,
  PEM2_SENSOR_STATUS_METER_OVERFLOW,
};

/* Sensors on PSU */
enum {
  PSU1_SENSOR_IN_VOLT = 0x01,
  PSU1_SENSOR_12V_VOLT,
  PSU1_SENSOR_STBY_VOLT,
  PSU1_SENSOR_IN_CURR,
  PSU1_SENSOR_12V_CURR,
  PSU1_SENSOR_STBY_CURR,
  PSU1_SENSOR_IN_POWER,
  PSU1_SENSOR_12V_POWER,
  PSU1_SENSOR_STBY_POWER,
  PSU1_SENSOR_FAN_TACH,
  PSU1_SENSOR_TEMP1,
  PSU1_SENSOR_TEMP2,
  PSU1_SENSOR_TEMP3,
  PSU1_SENSOR_FAN2_TACH,
  PSU1_SENSOR_CNT = PSU1_SENSOR_FAN2_TACH,

  PSU2_SENSOR_IN_VOLT,
  PSU2_SENSOR_12V_VOLT,
  PSU2_SENSOR_STBY_VOLT,
  PSU2_SENSOR_IN_CURR,
  PSU2_SENSOR_12V_CURR,
  PSU2_SENSOR_STBY_CURR,
  PSU2_SENSOR_IN_POWER,
  PSU2_SENSOR_12V_POWER,
  PSU2_SENSOR_STBY_POWER,
  PSU2_SENSOR_FAN_TACH,
  PSU2_SENSOR_TEMP1,
  PSU2_SENSOR_TEMP2,
  PSU2_SENSOR_TEMP3,
  PSU2_SENSOR_FAN2_TACH,
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

// LED positon at front-panel
// for all version except Wedge400 MP Respin
enum {
  SLED_1 = 1,
  SLED_2 = 2,
  SLED_3 = 3,
  SLED_4 = 4,
};

enum
{
  SCM_LED_BLUE = 0x01,
  SCM_LED_AMBER = 0x05,
};

// userver state use for set_scm_led()
enum
{
  USERVER_STATE_NONE = 0,
  USERVER_STATE_NORMAL,
  USERVER_STATE_POWER_OFF,
  USERVER_STATE_PING_DOWN,
};

/* Add button function for Debug Card */
enum {
  HAND_SW_SERVER = 0,
  HAND_SW_BMC = 1
};



int pal_is_debug_card_prsnt(uint8_t *status);
int pal_post_enable(uint8_t slot);
int pal_post_get_last(uint8_t slot, uint8_t *status);
int pal_post_get_last_and_len(uint8_t slot, uint8_t *data, uint8_t *len);
int pal_set_com_pwr_btn_n(char *status);
int pal_set_th3_power(int option);
int pal_get_cpld_fpga_fw_ver(uint8_t fru, const char *device, uint8_t* ver);
int pal_get_board_rev(int *rev);
int pal_get_board_type(uint8_t *brd_type);
int pal_get_full_board_type(uint8_t *full_brd_type);
int pal_get_board_type_rev(uint8_t *brd_type_rev);
int pal_get_board_id(uint8_t slot, uint8_t *req_data, uint8_t req_len, uint8_t *res_data, uint8_t *res_len);
int pal_sensor_discrete_read_raw(uint8_t fru, uint8_t sensor_num, void *value);
void *generate_dump(void *arg);
void set_sys_led(int brd_rev);
void set_fan_led(int brd_rev);
void set_psu_led(int brd_rev);
void set_scm_led(int brd_rev);
int set_sled(int brd_rev, uint8_t color, int led_name);
void init_led(void);
int pal_light_scm_led(uint8_t led_color);
int wedge400_sensor_name(uint8_t fru, uint8_t sensor_num, char *name);
bool pal_is_mcu_working(void);
int pal_get_hand_sw_physically(uint8_t *pos);
int pal_get_hand_sw(uint8_t *pos);
int pal_get_dbg_pwr_btn(uint8_t *status);
int pal_get_dbg_rst_btn(uint8_t *status);
int pal_get_dbg_uart_btn(uint8_t *status);
int pal_clr_dbg_uart_btn();
int pal_switch_uart_mux();
#ifdef __cplusplus
} // extern "C"
#endif

#endif /* __PAL_H__ */
