/* Copyright 2020-present Facebook. All Rights Reserved.
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

#ifndef __FBY35_GPIO_H__
#define __FBY35_GPIO_H__

#include <stdint.h>
#include <openbmc/libgpio.h>
#include <facebook/bic.h>

#ifdef __cplusplus
extern "C" {
#endif

#include <facebook/bic.h>

#define SYS_CFG_CLASS1 (1 << 0)
#define SYS_CFG_CLASS2 (1 << 1)

typedef struct gpio_info {
  char *shadow_name;
  char *pin_name;
  gpio_direction_t direction;
  gpio_value_t value;
  uint8_t sys_cfg;
} gpio_cfg;

// GPIO Expander GPIO pins
enum {
  MAX_GPIO_EXPANDER_GPIO_PINS = 0x0,
};

// BMC GPIO pins
enum {
  GPIO_PWRGD_NIC_BMC = MAX_GPIO_EXPANDER_GPIO_PINS,
  GPIO_OCP_DEBUG_BMC_PRSNT_N,
  GPIO_PWROK_STBY_BMC_SLOT1_R,
  GPIO_PWROK_STBY_BMC_SLOT2,
  GPIO_PWROK_STBY_BMC_SLOT3_R,
  GPIO_PWROK_STBY_BMC_SLOT4,
  GPIO_OCP_NIC_PRSNT_BMC_N,
  GPIO_FM_NIC_WAKE_BMC_N,
  GPIO_FM_PWRBRK_PRIMARY_R,
  GPIO_SMB_RST_PRIMARY_BMC_N_R,
  GPIO_SMB_RST_SECONDARY_BMC_N_R,
  GPIO_USB_MUX_CB_R,
  GPIO_FM_RESBTN_SLOT1_BMC_N,
  GPIO_FM_RESBTN_SLOT2_N,
  GPIO_FM_RESBTN_SLOT3_BMC_N,
  GPIO_FM_RESBTN_SLOT4_N,
  GPIO_SMB_TEMP_ALERT_BMC_N_R,
  GPIO_FM_DEBUG_UART_MUX_BMC_R,
  GPIO_SMB_HOTSWAP_BMC_ALERT_N_R,
  GPIO_EMMC_PRESENT_N,
  GPIO_SMB_BMC_SLOT1_ALT_R_N,
  GPIO_SMB_BMC_SLOT2_ALT_R_N,
  GPIO_SMB_BMC_SLOT3_ALT_R_N,
  GPIO_SMB_BMC_SLOT4_ALT_R_N,
  GPIO_PRSNT_MB_BMC_SLOT1_BB_N,
  GPIO_PRSNT_MB_SLOT2_BB_N,
  GPIO_PRSNT_MB_BMC_SLOT3_BB_N,
  GPIO_PRSNT_MB_SLOT4_BB_N,
  GPIO_FM_SPI_WP_DISABLE_STATUS_R_N,
  GPIO_BB_BUTTON_BMC_CO_N_R,
  GPIO_FM_PWRBRK_SECONDARY_R,
  GPIO_P3V3_NIC_FAULT_N,
  GPIO_NIC_POWER_BMC_EN_R,
  GPIO_P12V_NIC_FAULT_N,
  GPIO_USB_BMC_EN_R,
  GPIO_FAST_PROCHOT_BMC_N_R,
  GPIO_RST_BMC_USB_HUB_N_R,
  GPIO_HSC_FAULT_BMC_SLOT1_N_R,
  GPIO_HSC_FAULT_SLOT2_N,
  GPIO_HSC_FAULT_BMC_SLOT3_N_R,
  GPIO_HSC_FAULT_SLOT4_N,
  GPIO_FM_HSC_BMC_FAULT_N_R,
  GPIO_DUAL_FAN0_DETECT_BMC_N_R,
  GPIO_DUAL_FAN1_DETECT_BMC_N_R,
  GPIO_FAN0_BMC_CPLD_EN_R,
  GPIO_FAN1_BMC_CPLD_EN_R,
  GPIO_FAN2_BMC_CPLD_EN_R,
  GPIO_FAN3_BMC_CPLD_EN_R,
  GPIO_RST_PCIE_RESET_SLOT1_N,
  GPIO_RST_PCIE_RESET_SLOT2_N,
  GPIO_RST_PCIE_RESET_SLOT3_N,
  GPIO_RST_PCIE_RESET_SLOT4_N,
  GPIO_USB_MUX_EN_BMC_N_R,
  GPIO_SLOT1_ID1_DETECT_BMC_N,
  GPIO_SLOT1_ID0_DETECT_BMC_N,
  GPIO_SLOT2_ID1_DETECT_BMC_N,
  GPIO_SLOT2_ID0_DETECT_BMC_N,
  GPIO_SLOT3_ID1_DETECT_BMC_N,
  GPIO_SLOT3_ID0_DETECT_BMC_N,
  GPIO_SLOT4_ID1_DETECT_BMC_N,
  GPIO_SLOT4_ID0_DETECT_BMC_N,
  GPIO_P5V_USB_PG_BMC,
  GPIO_FM_BMC_TPM_PRSNT_N,
  GPIO_FM_BMC_SLOT1_ISOLATED_EN_R,
  GPIO_FM_BMC_SLOT2_ISOLATED_EN_R,
  GPIO_FM_BMC_SLOT3_ISOLATED_EN_R,
  GPIO_FM_BMC_SLOT4_ISOLATED_EN_R,
  GPIO_FM_SMB_ISOLATED_EN_R,
  GPIO_P12V_EFUSE_DETECT_N,
  GPIO_AC_ON_OFF_BTN_BMC_SLOT1_N_R,
  GPIO_AC_ON_OFF_BTN_SLOT2_N,
  GPIO_AC_ON_OFF_BTN_BMC_SLOT3_N_R,
  GPIO_AC_ON_OFF_BTN_SLOT4_N,
  GPIO_BOARD_ID0,
  GPIO_BOARD_ID1,
  GPIO_BOARD_ID2,
  GPIO_BOARD_ID3,
  GPIO_HSC_BB_BMC_DETECT0,
  //GPIO_RST_BMC_WDRST1_R,
  GPIO_RST_BMC_WDRST2_R,
  GPIO_SPI_LOCK_REQ_BMC_N,
  GPIO_EMMC_RST_N_R,
  GPIO_BMC_READY_R,
  GPIO_HSC_BB_BMC_DETECT1,
  MAX_BMC_GPIO_PINS,
};

extern gpio_cfg gpio_expander_gpio_table[];
extern gpio_cfg bmc_gpio_table[]; 

uint8_t y35_get_gpio_list_size(void);
int y35_get_gpio_name(uint8_t fru, uint8_t gpio, char *name);

uint8_t fby35_get_gpio_list_size(void);
int fby35_get_gpio_name(uint8_t fru, uint8_t gpio, char *name);

#ifdef __cplusplus
} // extern "C"
#endif

#endif /* __FBY35_GPIO_H__ */
