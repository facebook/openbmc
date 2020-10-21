/*
 *
 * Copyright 2015-present Facebook. All Rights Reserved.
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

#ifndef __FBY2_COMMON_H__
#define __FBY2_COMMON_H__

#include<stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

#define MAX_NUM_FRUS 6
enum {
  FRU_ALL   = 0,
  FRU_SLOT1 = 1,
  FRU_SLOT2 = 2,
  FRU_SLOT3 = 3,
  FRU_SLOT4 = 4,
  FRU_SPB   = 5,
  FRU_NIC   = 6,
  FRU_BMC   = 7,
};

enum {
  SLOT_TYPE_SERVER = 0,
  SLOT_TYPE_CF     = 1,
  SLOT_TYPE_GP     = 2,
  SLOT_TYPE_NULL   = 3,
  SLOT_TYPE_GPV2   = 4,
};

enum {
  IPMB_BUS_SLOT1 = 1,
  IPMB_BUS_SLOT2 = 3,
  IPMB_BUS_SLOT3 = 5,
  IPMB_BUS_SLOT4 = 7,
};

enum {
  PCIE_CONFIG_4xTL      = 0x00,
  PCIE_CONFIG_2xCF_2xTL = 0x11,
  PCIE_CONFIG_2xGP_2xTL = 0x22,
};

enum {
  TYPE_SPB_YV2     = 0,
  TYPE_SPB_YV250   = 1,
  TYPE_SPB_YV2ND   = 2,
};

enum {
  TYPE_DUAL_R_FAN     = 0,
  TYPE_SINGLE_FAN   = 1,
};

enum {
  TYPE_10K_FAN   = 0,
  TYPE_15K_FAN   = 1,
};

enum {
  TYPE_SV_A_SV     = 0,
  TYPE_CF_A_SV     = 1,
  TYPE_GP_A_SV     = 2,
  TYPE_NULL_A_SV   = 3,
  TYPE_GPV2_A_SV   = 4,
  TYPE_SV_A_CF     = 16,
  TYPE_CF_A_CF     = 17,
  TYPE_GP_A_CF     = 18,
  TYPE_NULL_A_CF   = 19,
  TYPE_GPV2_A_CF   = 20,
  TYPE_SV_A_GP     = 32,
  TYPE_CF_A_GP     = 33,
  TYPE_GP_A_GP     = 34,
  TYPE_NULL_A_GP   = 35,
  TYPE_GPV2_A_GP   = 36,
  TYPE_SV_A_NULL   = 48,
  TYPE_CF_A_NULL   = 49,
  TYPE_GP_A_NULL   = 50,
  TYPE_GPV2_A_NULL = 52,
  TYPE_SV_A_GPV2   = 64,
  TYPE_CF_A_GPV2   = 65,
  TYPE_GP_A_GPV2   = 66,
  TYPE_NULL_A_GPV2 = 67,
  TYPE_GPV2_A_GPV2 = 68,
};

typedef struct {
  char slot_key[32];
  char slot_def_val[32];
} slot_kv_st;


// Structure for Accuracy Sensor Reading (Bridge IC spec v0.6)
typedef struct {
  uint8_t int_value;
  uint8_t dec_value;
  uint8_t flags;
} ipmi_accuracy_sensor_reading_t;

//GPIO definition
#define MAX_SPB_GPIO_NUM                  256

#if defined(CONFIG_FBY2_KERNEL)
#define GPIO_BASE_NUM                     792
#else
#define GPIO_BASE_NUM                       0
#endif

#define GPIO_BMC_READY_N                  "BMC_READY_N"
#define GPIO_PE_BUFF_OE_0_R_N             "PE_BUFF_OE_0_R_N"
#define GPIO_PE_BUFF_OE_1_R_N             "PE_BUFF_OE_1_R_N"
#define GPIO_PE_BUFF_OE_2_R_N             "PE_BUFF_OE_2_R_N"
#define GPIO_PE_BUFF_OE_3_R_N             "PE_BUFF_OE_3_R_N"
#define GPIO_PWR_BTN                      "PWR_BTN"
#define GPIO_PWR_SLOT1_BTN_N              "PWR_SLOT1_BTN_N "
#define GPIO_PWR_SLOT2_BTN_N              "PWR_SLOT2_BTN_N "
#define GPIO_PWR_SLOT3_BTN_N              "PWR_SLOT3_BTN_N "
#define GPIO_PWR_SLOT4_BTN_N              "PWR_SLOT4_BTN_N "
#define GPIO_UART_SEL0                    "UART_SEL0"
#define GPIO_UART_SEL1                    "UART_SEL1"
#define GPIO_UART_SEL2                    "UART_SEL2"
#define GPIO_UART_RX                      "UART_RX"
#define GPIO_USB_SW0                      "USB_SW0"
#define GPIO_USB_SW1                      "USB_SW1"
#define GPIO_SYSTEM_ID1_LED_N             "SYSTEM_ID1_LED_N"
#define GPIO_SYSTEM_ID2_LED_N             "SYSTEM_ID2_LED_N"
#define GPIO_SYSTEM_ID3_LED_N             "SYSTEM_ID3_LED_N"
#define GPIO_SYSTEM_ID4_LED_N             "SYSTEM_ID4_LED_N"
#define GPIO_POSTCODE_0                   "POSTCODE_0"
#define GPIO_POSTCODE_1                   "POSTCODE_1"
#define GPIO_POSTCODE_2                   "POSTCODE_2"
#define GPIO_POSTCODE_3                   "POSTCODE_3"
#define GPIO_DUAL_FAN_DETECT              "DUAL_FAN_DETECT"
#define GPIO_YV250_USB_OCP_UART_SWITCH_N  "YV250_USB_OCP_UART_SWITCH_N" // YV2.50
#define GPIO_FAN_LATCH_DETECT             "FAN_LATCH_DETECT"
#define GPIO_SLOT1_POWER_EN               "SLOT1_POWER_EN"
#define GPIO_SLOT2_POWER_EN               "SLOT2_POWER_EN"
#define GPIO_SLOT3_POWER_EN               "SLOT3_POWER_EN"
#define GPIO_SLOT4_POWER_EN               "SLOT4_POWER_EN"
#define GPIO_CLK_BUFF1_PWR_EN_N           "CLK_BUFF1_PWR_EN_N"
#define GPIO_CLK_BUFF2_PWR_EN_N           "CLK_BUFF2_PWR_EN_N"
#define GPIO_VGA_SW0                      "VGA_SW0"
#define GPIO_VGA_SW1                      "VGA_SW1"
#define GPIO_MEZZ_PRSNTA2_N               "MEZZ_PRSNTA2_N"
#define GPIO_MEZZ_PRSNTB2_N               "MEZZ_PRSNTB2_N"
#define GPIO_PWR1_LED                     "PWR1_LED"
#define GPIO_PWR2_LED                     "PWR2_LED"
#define GPIO_PWR3_LED                     "PWR3_LED"
#define GPIO_PWR4_LED                     "PWR4_LED"
#define GPIO_I2C_SLOT1_ALERT_N            "I2C_SLOT1_ALERT_N"
#define GPIO_I2C_SLOT2_ALERT_N            "I2C_SLOT2_ALERT_N"
#define GPIO_I2C_SLOT3_ALERT_N            "I2C_SLOT3_ALERT_N"
#define GPIO_I2C_SLOT4_ALERT_N            "I2C_SLOT4_ALERT_N"
#define GPIO_UART_SEL                     "UART_SEL"                     // YV2
#define GPIO_P12V_STBY_SLOT1_EN           "P12V_STBY_SLOT1_EN"
#define GPIO_P12V_STBY_SLOT2_EN           "P12V_STBY_SLOT2_EN"
#define GPIO_P12V_STBY_SLOT3_EN           "P12V_STBY_SLOT3_EN"
#define GPIO_P12V_STBY_SLOT4_EN           "P12V_STBY_SLOT4_EN"
#define GPIO_SLOT1_EJECTOR_LATCH_DETECT_N "SLOT1_EJECTOR_LATCH_DETECT_N"
#define GPIO_SLOT2_EJECTOR_LATCH_DETECT_N "SLOT2_EJECTOR_LATCH_DETECT_N"
#define GPIO_SLOT3_EJECTOR_LATCH_DETECT_N "SLOT3_EJECTOR_LATCH_DETECT_N"
#define GPIO_SLOT4_EJECTOR_LATCH_DETECT_N "SLOT4_EJECTOR_LATCH_DETECT_N"
#define GPIO_POSTCODE_4                   "POSTCODE_4"
#define GPIO_POSTCODE_5                   "POSTCODE_5"
#define GPIO_POSTCODE_6                   "POSTCODE_6"
#define GPIO_POSTCODE_7                   "POSTCODE_7"
#define GPIO_DBG_CARD_PRSNT               "DBG_CARD_PRSNT"
#define GPIO_RST_SLOT1_SYS_RESET_N        "RST_SLOT1_SYS_RESET_N"
#define GPIO_RST_SLOT2_SYS_RESET_N        "RST_SLOT2_SYS_RESET_N"
#define GPIO_RST_SLOT3_SYS_RESET_N        "RST_SLOT3_SYS_RESET_N"
#define GPIO_RST_SLOT4_SYS_RESET_N        "RST_SLOT4_SYS_RESET_N"
#define GPIO_BOARD_REV_ID0                "BOARD_REV_ID0"
#define GPIO_BOARD_REV_ID1                "BOARD_REV_ID1"
#define GPIO_BOARD_REV_ID2                "BOARD_REV_ID2"
#define GPIO_BOARD_ID                     "BOARD_ID"
#define GPIO_SLOT1_PRSNT_B_N              "SLOT1_PRSNT_B_N"
#define GPIO_SLOT2_PRSNT_B_N              "SLOT2_PRSNT_B_N"
#define GPIO_SLOT3_PRSNT_B_N              "SLOT3_PRSNT_B_N"
#define GPIO_SLOT4_PRSNT_B_N              "SLOT4_PRSNT_B_N"
#define GPIO_SLOT1_PRSNT_N                "SLOT1_PRSNT_N"
#define GPIO_SLOT2_PRSNT_N                "SLOT2_PRSNT_N"
#define GPIO_SLOT3_PRSNT_N                "SLOT3_PRSNT_N"
#define GPIO_SLOT4_PRSNT_N                "SLOT4_PRSNT_N"
#define GPIO_HAND_SW_ID1                  "HAND_SW_ID1"
#define GPIO_HAND_SW_ID2                  "HAND_SW_ID2"
#define GPIO_HAND_SW_ID4                  "HAND_SW_ID4"
#define GPIO_HAND_SW_ID8                  "HAND_SW_ID8"
#define GPIO_RST_BTN                      "RST_BTN"
#define GPIO_BMC_SELF_HW_RST              "BMC_SELF_HW_RST"
#define GPIO_USB_MUX_EN_N                 "USB_MUX_EN_N"
#define GPIOAB4_RESERVED_PIN              //GPIOAB4 is reserved and could not be used
#define GPIOAB5_RESERVED_PIN              //GPIOAB5 is reserved and could not be used
#define GPIOAB6_RESERVED_PIN              //GPIOAB6 is reserved and could not be used
#define GPIOAB7_RESERVED_PIN              //GPIOAB7 is reserved and could not be used
#define GPIO_SLOT1_LED                    "SLOT1_LED"
#define GPIO_SLOT2_LED                    "SLOT2_LED"
#define GPIO_SLOT3_LED                    "SLOT3_LED"
#define GPIO_SLOT4_LED                    "SLOT4_LED"
#define GPIO_SLED_SEATED_N                "SLED_SEATED_N" //GPIOAC7

#define BIC_CACHED_PID "/var/run/bic-cached_%d.lock"

#define SPB_REV_FILE "/tmp/spb_rev"
#define SPB_TYPE_FILE "/tmp/spb_type"
#define FAN_TYPE_FILE "/tmp/fan_type"
#define SPB_BOARD_ID_FILE "/tmp/spb_board_id"

int fby2_common_fru_name(uint8_t fru, char *str);
int fby2_common_fru_id(char *str, uint8_t *fru);
int fby2_common_dev_id(char *str, uint8_t *dev);
int fby2_common_crashdump(uint8_t fru, bool ierr, bool platform_reset);
int fby2_common_set_ierr(uint8_t fru, bool value);
int fby2_common_get_ierr(uint8_t fru, bool *value);
int fby2_common_cpld_dump(uint8_t fru);
int fby2_common_sboot_cpld_dump(uint8_t fru);
int fby2_common_get_spb_type(void);
int fby2_common_get_fan_type(void);
int fby2_common_get_fan_config(void);
int fby2_common_set_gpio_val(char *shadow, int val);
int fby2_common_get_gpio_val(char *shadow, int *val);
int fby2_common_get_spb_rev(void);
int fby2_common_get_board_id(void);
#ifdef __cplusplus
} // extern "C"
#endif

#endif /* __FBY2_COMMON_H__ */
