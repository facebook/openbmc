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
  BYPASS_BIC     = 0,
  BYPASS_ME      = 1,
  BYPASS_IMC     = 2,
  BYPASS_NCSI    = 3,
  BYPASS_NETWORK = 4,
};

enum {
  TYPE_SPB_YV2     = 0,
  TYPE_SPB_YV250   = 1,
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

#define GPIOA0_BMC_READY_N                  0
#define GPIOA1_SMB_HOTSWAP_ALERT_N          1
#define GPIOB0_SMB_SLOT1_CPLD_BIC_ALERT_N   8
#define GPIOB1_SMB_SLOT2_CPLD_BIC_ALERT_N   9
#define GPIOB2_SMB_SLOT3_CPLD_BIC_ALERT_N  10
#define GPIOB3_SMB_SLOT4_CPLD_BIC_ALERT_N  11
#define GPIOB4_PRSNT_MB_SLOT1_BB_N         12
#define GPIOB5_PRSNT_MB_SLOT2_BB_N         13
#define GPIOB6_PRSNT_MB_SLOT3_BB_N         14
#define GPIOB7_PRSNT_MB_SLOT4_BB_N         15
#define GPIOE0_ADAPTER_CARD_TYPE_1         32
#define GPIOE1_ADAPTER_CARD_TYPE_2         33
#define GPIOE2_ADAPTER_CARD_TYPE_3         34
#define GPIOE3_ADAPTER_BUTTON_BMC_N        35
#define GPIOF0_HSC_CPLD_SLOT1_EN_R         40
#define GPIOF1_HSC_CPLD_SLOT2_EN_R         41
#define GPIOF2_HSC_CPLD_SLOT3_EN_R         42
#define GPIOF3_HSC_CPLD_SLOT4_EN_R         43
#define GPIOF4_SMB_RST_PRIMARY_CPLD_N_R    44
#define GPIOG0_BOARD_REV_ID0               48
#define GPIOG1_BOARD_REV_ID1               49
#define GPIOG2_BOARD_REV_ID2               50
#define GPIOG3_SMB_RST_SECONDARY_CPLD_N_R  51
#define GPIOG4_SLOT1_HSC_SET_CPLD_EN_R     52
#define GPIOG5_SLOT2_HSC_SET_CPLD_EN_R     53
#define GPIOG6_SLOT3_HSC_SET_CPLD_EN_R     54
#define GPIOG7_SLOT4_HSC_SET_CPLD_EN_R     55
#define GPIOJ0_PWROK_STBY_BMC_SLOT1        72
#define GPIOJ1_PWROK_STBY_BMC_SLOT2        73
#define GPIOJ2_PWROK_STBY_BMC_SLOT3        74
#define GPIOJ3_PWROK_STBY_BMC_SLOT4        75
#define GPIOJ4_RST_PCIE_RESET_SLOT1_N      76
#define GPIOJ5_RST_PCIE_RESET_SLOT2_N      77
#define GPIOJ6_RST_PCIE_RESET_SLOT3_N      78
#define GPIOJ7_RST_PCIE_RESET_SLOT4_N      79
#define GPIOL0_AC_ON_OFF_BTN_SLOT1_N       88
#define GPIOL1_AC_ON_OFF_BTN_SLOT2_N       89
#define GPIOL2_AC_ON_OFF_BTN_SLOT3_N       90
#define GPIOL3_AC_ON_OFF_BTN_SLOT4_N       91
#define GPIOL4_FAST_PROCHOT_N              92
#define GPIOL5_USB_CPLD_EN_N_R             93
#define GPIOM0_HS0_FAULT_SLOT1_N           96
#define GPIOM1_HS0_FAULT_SLOT2_N           97
#define GPIOM2_HS0_FAULT_SLOT3_N           98
#define GPIOM3_HS0_FAULT_SLOT4_N           99
#define GPIOM4_FM_BB_CPLD_SLOT_ID0_R      100
#define GPIOM5_FM_BB_CPLD_SLOT_ID1_R      101
#define GPIOAB0_PWRGD_NIC_BMC             216
#define GPIOAB1_OCP_DEBUG_PRSNT_N         217
#define GPIOAB2_RST_BMC_WDRST1_R          218
#define GPIOAB3_RST_BMC_WDRST2_R          219
#define GPIOAC0_CMOS_CLEAR_SLOT1_R        220
#define GPIOAC1_CMOS_CLEAR_SLOT2_R        221
#define GPIOAC2_CMOS_CLEAR_SLOT3_R        222
#define GPIOAC3_CMOS_CLEAR_SLOT4_R        223

#define GPIO_BOARD_ID GPIOG0_BOARD_REV_ID0

const static uint8_t gpio_server_resistor_en[] =
{ 0,
  GPIOG4_SLOT1_HSC_SET_CPLD_EN_R,
  GPIOG5_SLOT2_HSC_SET_CPLD_EN_R,
  GPIOG6_SLOT3_HSC_SET_CPLD_EN_R,
  GPIOG7_SLOT4_HSC_SET_CPLD_EN_R
};

const static uint8_t gpio_server_prsnt[] =
{ 0,
  GPIOB4_PRSNT_MB_SLOT1_BB_N,
  GPIOB5_PRSNT_MB_SLOT2_BB_N,
  GPIOB6_PRSNT_MB_SLOT3_BB_N,
  GPIOB7_PRSNT_MB_SLOT4_BB_N
};

const static uint8_t gpio_bic_ready[] =
{ 0,
  GPIOB0_SMB_SLOT1_CPLD_BIC_ALERT_N,
  GPIOB1_SMB_SLOT2_CPLD_BIC_ALERT_N,
  GPIOB2_SMB_SLOT3_CPLD_BIC_ALERT_N,
  GPIOB3_SMB_SLOT4_CPLD_BIC_ALERT_N
};

const static uint8_t gpio_server_stby_power_en[] =
{ 0,
  GPIOF0_HSC_CPLD_SLOT1_EN_R,
  GPIOF1_HSC_CPLD_SLOT2_EN_R,
  GPIOF2_HSC_CPLD_SLOT3_EN_R,
  GPIOF3_HSC_CPLD_SLOT4_EN_R
};

const static uint8_t gpio_server_power_sts[] =
{ 0,
  GPIOJ0_PWROK_STBY_BMC_SLOT1,
  GPIOJ1_PWROK_STBY_BMC_SLOT2,
  GPIOJ2_PWROK_STBY_BMC_SLOT3,
  GPIOJ3_PWROK_STBY_BMC_SLOT4
};

#define MAX_KEY_LEN       64
#define MAX_VALUE_LEN     64

#define BIC_CACHED_PID "/var/run/bic-cached_%d.lock"

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
#ifdef __cplusplus
} // extern "C"
#endif

#endif /* __FBY2_COMMON_H__ */
