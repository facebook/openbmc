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

#ifndef __FBGC_GPIO_H__
#define __FBGC_GPIO_H__

#ifdef __cplusplus
extern "C" {
#endif

enum {
  GPIO_FPGA_CRCERROR = 0,
  GPIO_FPGA_NCONFIG,
  GPIO_BMC_NIC_PWRBRK_R,
  GPIO_BMC_NIC_SMRST_N_R,
  GPIO_NIC_WAKE_BMC_N,
  GPIO_BMC_NIC_FULL_PWR_EN_R,
  GPIO_NIC_FULL_PWR_PG,
  GPIO_BOARD_REV_ID0,
  GPIO_BOARD_REV_ID1,
  GPIO_BOARD_REV_ID2,
  GPIO_EN_ASD_DEBUG,
  GPIO_DEBUG_RST_BTN_N,
  GPIO_E1S_1_P3V3_PG_R,
  GPIO_E1S_2_P3V3_PG_R,
  GPIO_BMC_FPGA_UART_SEL0_R,
  GPIO_BMC_FPGA_UART_SEL1_R,
  GPIO_BMC_FPGA_UART_SEL2_R,
  GPIO_BMC_FPGA_UART_SEL3_R,
  GPIO_DEBUG_BMC_UART_SEL_R,
  GPIO_DEBUG_GPIO_BMC_1,
  GPIO_DEBUG_GPIO_BMC_2,
  GPIO_DEBUG_GPIO_BMC_3,
  GPIO_DEBUG_GPIO_BMC_4,
  GPIO_DEBUG_GPIO_BMC_5,
  GPIO_DEBUG_GPIO_BMC_6,
  GPIO_USB_OC_N1,
  GPIO_SCC_I2C_EN_R,
  GPIO_BMC_READY_R,
  GPIO_LED_POSTCODE_0,
  GPIO_LED_POSTCODE_1,
  GPIO_LED_POSTCODE_2,
  GPIO_LED_POSTCODE_3,
  GPIO_LED_POSTCODE_4,
  GPIO_LED_POSTCODE_5,
  GPIO_LED_POSTCODE_6,
  GPIO_LED_POSTCODE_7,
  GPIO_BMC_LOC_HEARTBEAT_R,
  GPIO_BIC_READY_IN,
  GPIO_COMP_STBY_PG_IN,
  GPIO_UIC_LOC_ID_IN,
  GPIO_UIC_RMT_ID_IN,
  GPIO_BMC_COMP_PWR_EN_R,
  GPIO_EXT_MINISAS_INS_OR_N_IN,
  GPIO_NIC_PRSNTB3_N,
  GPIO_FM_BMC_TPM_PRSNT_N,
  GPIO_DEBUG_CARD_PRSNT_N,
  GPIO_BMC_RST_BTN_N_R,
  GPIO_PCIE_COMP_UIC_RST_N,
  GPIO_BMC_COMP_SYS_RST_N_R,
  GPIO_BMC_LED_STATUS_BLUE_EN_R,
  GPIO_BMC_LED_STATUS_YELLOW_EN_R,
  GPIO_BMC_LED_PWR_BTN_EN_R,
  GPIO_UIC_STBY_PG,
  GPIO_PWRGD_NIC_BMC,
  GPIO_EMMC_PRESENT_N,
  GPIO_HSC_P12V_DPB_FAULT_N_IN,
  GPIO_HSC_COMP_FAULT_N_IN,
  GPIO_SCC_FAULT_N_IN,
  GPIO_EMMC_RST_N,
  MAX_GPIO_PINS,
};

uint8_t fbgc_get_gpio_list_size(void);
const char * fbgc_get_gpio_name(uint8_t gpio);

#ifdef __cplusplus
} // extern "C"
#endif

#endif /* __FBGC_GPIO_H__ */
