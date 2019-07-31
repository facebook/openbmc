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

//The definition of the type are used in fruid
//which is the common code
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
#define MAX_SPB_GPIO_NUM 256
#define GPIO_RSVD_VAL 255

#define GPIOA0_BMC_READY_R                       0
#define GPIOA1_SMB_HOTSWAP_BMC_ALERT_N_R         1
#define GPIOA2_FM_HSC_BMC_FAULT_N_R              2
#define GPIOA3_FM_NIC_WAKE_BMC_N                 3
#define GPIOB0_SMB_BMC_SLOT1_ALT_N               8
#define GPIOB1_SMB_BMC_SLOT2_ALT_N               9
#define GPIOB2_SMB_BMC_SLOT3_ALT_N              10
#define GPIOB3_SMB_BMC_SLOT4_ALT_N              11
#define GPIOB4_PRSNT_MB_BMC_SLOT1_BB_N          12
#define GPIOB5_PRSNT_MB_BMC_SLOT2_BB_N          13
#define GPIOB6_PRSNT_MB_BMC_SLOT3_BB_N          14
#define GPIOB7_PRSNT_MB_BMC_SLOT4_BB_N          15
#define GPIOE0_ADAPTER_CARD_TYPE_BMC_1          32
#define GPIOE1_ADAPTER_CARD_TYPE_BMC_2          33
#define GPIOE2_ADAPTER_CARD_TYPE_BMC_3          34
#define GPIOE3_ADAPTER_BUTTON_BMC_CO_N_R        35
#define GPIOE4_RST_BMC_USB_HUB_N_R              36
#define GPIOE5_EMMC_RST_N_R                     37
#define GPIOF0_HSC_BMC_SLOT1_EN_R               40
#define GPIOF1_HSC_BMC_SLOT2_EN_R               41
#define GPIOF2_HSC_BMC_SLOT3_EN_R               42
#define GPIOF3_HSC_BMC_SLOT4_EN_R               43
#define GPIOF4_SMB_RST_PRIMARY_BMC_N_R          44
#define GPIOG0_DISABLE_BMC_FAN_2_N_R            48
#define GPIOG1_DISABLE_BMC_FAN_3_N_R            49
#define GPIOG2_NIC_ADAPTER_CARD_PRSNT_BMC_1_N_R 50
#define GPIOG3_SMB_RST_SECONDARY_BMC_N_R        51
#define GPIOG4_SLOT1_HSC_SET_CPLD_EN_RSVD       GPIO_RSVD_VAL
#define GPIOG5_SLOT2_HSC_SET_CPLD_EN_RSVD       GPIO_RSVD_VAL
#define GPIOG6_SLOT3_HSC_SET_CPLD_EN_RSVD       GPIO_RSVD_VAL
#define GPIOG7_SLOT4_HSC_SET_CPLD_EN_RSVD       GPIO_RSVD_VAL
#define GPIOJ4_RST_PCIE_RESET_SLOT1_N           76
#define GPIOJ5_RST_PCIE_RESET_SLOT2_N           77
#define GPIOJ6_RST_PCIE_RESET_SLOT3_N           78
#define GPIOJ7_RST_PCIE_RESET_SLOT4_N           79
#define GPIOL0_AC_ON_OFF_BTN_SLOT1_N            88
#define GPIOL1_AC_ON_OFF_BTN_BMC_SLOT2_N_R      89
#define GPIOL2_AC_ON_OFF_BTN_SLOT3_N            90
#define GPIOL3_AC_ON_OFF_BTN_BMC_SLOT4_N_R      91
#define GPIOL4_FAST_PROCHOT_BMC_N_R             92
#define GPIOL5_USB_BMC_EN_N_R                   93
#define GPIOM0_HSC_FAULT_SLOT1_N                96
#define GPIOM1_HSC_FAULT_BMC_SLOT2_N_R          97
#define GPIOM2_HSC_FAULT_SLOT3_N                98
#define GPIOM3_HSC_FAULT_BMC_SLOT4_N_R          99
#define GPIOM4_FM_PWRBRK_PRIMARY_R              100
#define GPIOM5_FM_PWRBRK_SECONDARY_R            101
#define GPION4_DUAL_FAN0_DETECT_BMC_N_R         108
#define GPION5_DUAL_FAN1_DETECT_BMC_N_R         109
#define GPION6_DISABLE_BMC_FAN_0_N_R            110
#define GPION7_DISABLE_BMC_FAN_1_N_R            111
#define GPIOQ6_USB_OC_N                         134
#define GPIOQ7_FM_BMC_TPM_PRSNT_N               135
#define GPIOS0_PWROK_STBY_BMC_SLOT1             144
#define GPIOS1_PWROK_STBY_BMC_SLOT2             145
#define GPIOS2_PWROK_STBY_BMC_SLOT3             146
#define GPIOS3_PWROK_STBY_BMC_SLOT4             147
#define GPIOZ0_FM_BMC_SLOT1_ISOLATED_EN_R       200
#define GPIOZ1_FM_BMC_SLOT2_ISOLATED_EN_R       201
#define GPIOZ2_FM_BMC_SLOT3_ISOLATED_EN_R       202
#define GPIOZ3_FM_BMC_SLOT4_ISOLATED_EN_R       203
#define GPIOAB0_PWRGD_NIC_BMC                   216
#define GPIOAB1_OCP_DEBUG_BMC_PRSNT_N           217
#define GPIOAB2_RST_BMC_WDRST1_R                218
#define GPIOAB3_RST_BMC_WDRST2_R                219
#define GPIOAC4_SMB_FAN_INA_BMC_ALERT_N_R       224
#define GPIOAC5_SMB_TEMP_INLET_ALERT_BMC_N_R    225
#define GPIOAC6_SMB_TEMP_OUTLET_ALERT_BMC_N_R   226
#define GPIOAC7_FM_BMC_BIC_DETECT_N             227

#define GPIO_BOARD_ID GPIO_RSVD_VAL

const static uint8_t gpio_server_i2c_en[] =
{ 0,
  GPIOZ0_FM_BMC_SLOT1_ISOLATED_EN_R,
  GPIOZ1_FM_BMC_SLOT2_ISOLATED_EN_R,
  GPIOZ2_FM_BMC_SLOT3_ISOLATED_EN_R,
  GPIOZ3_FM_BMC_SLOT4_ISOLATED_EN_R
};

const static uint8_t gpio_server_prsnt[] =
{ 0,
  GPIOB4_PRSNT_MB_BMC_SLOT1_BB_N,
  GPIOB5_PRSNT_MB_BMC_SLOT2_BB_N,
  GPIOB6_PRSNT_MB_BMC_SLOT3_BB_N,
  GPIOB7_PRSNT_MB_BMC_SLOT4_BB_N
};

const static uint8_t gpio_bic_ready[] =
{ 0,
  GPIOB0_SMB_BMC_SLOT1_ALT_N,
  GPIOB1_SMB_BMC_SLOT2_ALT_N,
  GPIOB2_SMB_BMC_SLOT3_ALT_N,
  GPIOB3_SMB_BMC_SLOT4_ALT_N
};

const static uint8_t gpio_server_hsc_en[] =
{ 0,
  GPIOF0_HSC_BMC_SLOT1_EN_R,
  GPIOF1_HSC_BMC_SLOT2_EN_R,
  GPIOF2_HSC_BMC_SLOT3_EN_R,
  GPIOF3_HSC_BMC_SLOT4_EN_R
};

const static uint8_t gpio_server_hsc_pgood_sts[] =
{ 0,
  GPIOS0_PWROK_STBY_BMC_SLOT1,
  GPIOS1_PWROK_STBY_BMC_SLOT2,
  GPIOS2_PWROK_STBY_BMC_SLOT3,
  GPIOS3_PWROK_STBY_BMC_SLOT4
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
